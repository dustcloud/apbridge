/*
 * Copyright (c) 2014, Linear Technology. All rights reserved.
 */

#include "APCoupler.h"
#include "apc_common.h"

#include <iostream>
#include <iomanip>
#include <iterator>
#include <algorithm>
#include <vector>
#include <string>
#include <sstream>

#include <boost/asio.hpp>
#include <boost/thread.hpp>
#include <boost/bind.hpp>
#include <boost/format.hpp>
#include "6lowpan/public/dn_api_common.h"
#include "6lowpan/public/dn_api_local.h"
#include "6lowpan/public/dn_api_net.h" //For Clock Source Constants


// Manage the protocol with the AP, integrate with AP Connector

const char CLIENT_LOG[] = "apc.io";

const int SEND_TIME_TIMEOUT = 60000; // milliseconds

static const int32_t UTC_MIDNIGHT = (23*60 + 59) * 60 + 59;       // 23:59:59
static const int32_t AP_BLACKOUT          = UTC_MIDNIGHT - 30;    // Blackout time 30 sec before midnight 
static const int32_t LEAP_CHECK_TIME      = UTC_MIDNIGHT - 180;   // Time to check if the leap second occurs today (3 min before midnight)
static const int32_t AP_BLACKOUT_DURATION = 60;                   // Duration of blackout period 60 sec

// Coupler events
static const uint32_t E_APM_BOOT        = 0x1;
static const uint32_t E_APM_REBOOT      = 0x2;
static const uint32_t E_APM_LOST        = 0x4;
static const uint32_t E_MNGR_CONNECT    = 0x8;
static const uint32_t E_MNGR_DISCONNECT = 0x10;
static const uint32_t E_TIMEOUT         = 0x20;
static const uint32_t E_AP_RESET        = 0x40;
static const uint32_t E_AP_DISCONNECT   = 0x80;
static const uint32_t E_AP_END_BLACKOUT = 0x100;
static const uint32_t E_STOP            = 0x200;

CAPCoupler::CAPCoupler(boost::asio::io_service& io_service) :
     m_leapCheckTimer(io_service),
     m_apInfo(),
     m_transport(nullptr),
     m_mngrClient(nullptr),
     m_gps(nullptr),
     m_apConnected(false),
     m_hwResetTimeoutMsec(0),
     m_disconnectTimeoutShortMsec(0),
     m_disconnectTimeoutLongMsec(0),
     m_logname(CLIENT_LOG),
     m_cur_gps_status(GPS_ST_NO_DEVICE),
     m_events(0),
     m_smThread(nullptr),
     m_blackoutStart(SYSTIME_EMPTY)
{ 
   m_gps_info.satellites_used = m_gps_info.satellites_visible = 0;
}

CAPCoupler::~CAPCoupler()
{
   stop();
}


apc_error_t CAPCoupler::open(init_param_t& init_params)
{
   BOOST_ASSERT(init_params.mngrClient != nullptr && init_params.transport != nullptr);
   
   if (m_smThread != nullptr)
      return APC_ERR_STATE;

   // start AP transport
   m_transport = init_params.transport;

   // Initialization Manager Client
   m_mngrClient = init_params.mngrClient;
   m_client_start_param = init_params.mngrClientParam;

   m_apcInfo.m_host = init_params.mngrClientParam.host;
   m_apcInfo.m_port = init_params.mngrClientParam.port;
   
   // Save GPS configuration
   m_gps = init_params.gps;
   m_gps_start_param = init_params.gpsCfg;

   m_gps_info.satellites_used = 0;
   m_gps_info.satellites_visible = 0;

   // Boot timeouts
   m_hwResetTimeoutMsec         = init_params.resetBootTimeoutMsec;
   m_disconnectTimeoutShortMsec = init_params.disconnectShortBootTimeoutMsec;
   m_disconnectTimeoutLongMsec  = init_params.disconnectLongBootTimeoutMsec;
   
   //m_smThread = new boost::thread(boost::bind(&CAPCoupler::smThreadFun_p, this));
   return APC_OK;
}

void CAPCoupler::start()
{
   m_smThread = new boost::thread(boost::bind(&CAPCoupler::smThreadFun_p, this));
}

void CAPCoupler::stop()
{
   if (m_smThread != nullptr) {
      sendEvent_p(E_STOP);
      m_smThread->join();
      delete m_smThread;
      m_smThread = nullptr;
   }

   if (m_gps != nullptr) 
      m_gps->stop();
   if (m_mngrClient != nullptr) 
      m_mngrClient->stop();
   if (m_transport != nullptr) 
      m_transport->stop();
   m_gps = nullptr;
   m_transport = nullptr;
   m_mngrClient = nullptr;
}

// IAPCClientNotif -- input from APC

void CAPCoupler::connected(std::string server)
{
   DUSTLOG_INFO(m_logname, "Connected to "<< server);
   sendEvent_p(E_MNGR_CONNECT);
}

void CAPCoupler::disconnected(apc_stop_reason_t reason)
{
   DUSTLOG_INFO(m_logname, "Disconnect: "<< toString(reason));
   sendEvent_p(E_MNGR_DISCONNECT);
}

void CAPCoupler::dataRx(const ap_intf_sendhdr_t& hdr,
                        const uint8_t * pPayload, uint32_t size)
{
   DUSTLOG_DEBUG(m_logname, "Manager RX dst=" << hdr.dst << " len=" << size
                 << " txDoneId=" << hdr.txDoneId);
   
   // construct the AP Send header
   // note: byte ordering is handled in the send method
   dn_api_loc_apsend_ctrl_t apHdr = {0};
   apHdr.priority = hdr.priority;
   apHdr.linkCtrl = hdr.isLinkInfoSpecified;
   apHdr.packetId = hdr.txDoneId;
   apHdr.frameId  = hdr.frId;
   apHdr.timeslot = hdr.slot;
   apHdr.channel  = hdr.offset;
   apHdr.dest     = hdr.dst;
   // send data to AP Queue
   sendApSend(apHdr, pPayload, size);
}

void CAPCoupler::resume()
{
   DUSTLOG_INFO(m_logname, "Manager Resume");
   if (m_transport)
      m_transport->setAPMState(APM_FLOW_NORMAL);
}

void CAPCoupler::pause()
{
   DUSTLOG_INFO(m_logname, "Manager Pause");
   if (m_transport)
      m_transport->setAPMState(APM_FLOW_PAUSE);
}

void CAPCoupler::offline(apc_stop_reason_t reason)
{
   DUSTLOG_INFO(m_logname, "Manager Offline: " << toString(reason));
   if (m_transport)
      m_transport->setAPMState(APM_FLOW_PAUSE);
}

void CAPCoupler::online()
{
   DUSTLOG_INFO(m_logname, "Manager Online");
   if (m_transport)
      m_transport->setAPMState(APM_FLOW_NORMAL);
}

void CAPCoupler::resetAP()
{
   DUSTLOG_INFO(m_logname, "Manager ResetAP");
   sendEvent_p(E_AP_RESET);
}

void CAPCoupler::disconnectAP()
{
   DUSTLOG_INFO(m_logname, "Manager Disconnect AP from network");
   sendEvent_p(E_AP_DISCONNECT);
}

void CAPCoupler::getTime()
{
   DUSTLOG_DEBUG(m_logname, "Manager GetTime");
   if (m_transport) {
      uint8_t paramId = DN_API_PARAM_TIME;
      m_transport->insertMsg(DN_API_LOC_CMD_GETPARAM, &paramId, sizeof(paramId));      
   }
}

// IAPNotifHandler - input from APM Serializer

void CAPCoupler::handleAPLost()
{
   DUSTLOG_INFO(m_logname, "AP Lost");
   m_apConnected = false;
   if (m_mngrClient)
      m_mngrClient->sendApLost();
   sendEvent_p(E_APM_LOST);
}

void CAPCoupler::handleAPBoot()
{
   sendGetNetId();
   m_apConnected = true;
   sendGetApAppInfo();
   sendGetApStatus();
   sendEvent_p(E_APM_BOOT);
   m_apInfo.apState = DN_API_ST_IDLE;
}

void CAPCoupler::handleAPReboot()
{
   DUSTLOG_INFO(m_logname, "AP Reboot");
   sendGetNetId();
   m_apConnected = true;
   sendEvent_p(E_APM_REBOOT);
   m_apInfo.apState = DN_API_ST_IDLE;
}
   
void CAPCoupler::handleError(uint8_t cmdId, uint8_t rc)
{
   DUSTLOG_ERROR(m_logname, "Command " << (int)cmdId << " returns error: " << (int)rc);
   //TODO add processing of errors
}

void CAPCoupler::handleAPReceive(const uint8_t* data, size_t length)
{
   DUSTLOG_DEBUG(m_logname, "AP RX Data [" << length << "]");
   DUSTLOG_TRACEDATA(m_logname, "AP data", data, length);
   if (m_mngrClient == nullptr)
      return;
   // send data to Manager
   apc_error_t res = m_mngrClient->sendData(data, length);
   DUSTLOG_DEBUG(m_logname, "AP RX Data: rc=" << (int)res);
}

void CAPCoupler::handleEvent(const dn_api_loc_notif_events_t& event)
{
   DUSTLOG_DEBUG(m_logname, "AP RX Event: events=" << std::hex << event.events);
   // TODO: handle other events      
   sendGetApStatus();  // may not do this so frequently
}

void CAPCoupler::handleTimeIndication(const dn_api_loc_notif_time_t& timeMap)
{
   DUSTLOG_DEBUG(m_logname, "AP RX Time (not implemented)");
   // TODO
   //ap_time_map_t apcTimeMap;
}

void CAPCoupler::handleReadyForTime(const dn_api_loc_notif_ready_for_time_t& ready)
{

   DUSTLOG_DEBUG(m_logname, "AP RX Ready for time: seq=" << (int)ready.seqNum);

   if (m_blackoutStart != SYSTIME_EMPTY && 
      SYSTIME_NOW() > m_blackoutStart && SYSTIME_NOW() < m_blackoutStart + sec_t(AP_BLACKOUT_DURATION)) 
   {
      DUSTLOG_DEBUG(m_logname, "Blackout Period. Doesn't set time to AP");
      return;  // Doesn't set time during blackout period
   }

   if (m_apInfo.clksource == DN_API_AP_CLK_SOURCE_INTERNAL)
   {
      DUSTLOG_DEBUG(m_logname, "AP RX Ready for time: internal sync");
	   sendSetSystime(0);
   }
   else if (m_apInfo.clksource == DN_API_AP_CLK_SOURCE_PPS)
   {
      #ifdef WIN32
            m_cur_gps_status = GPS_ST_SYNC;  // fake GPS state
      #endif

      if (!m_gps_start_param.m_gpsd_conn) {
         // if gpsd is not required, we fake GPS state
         m_cur_gps_status = GPS_ST_SYNC;
      }

      switch(m_cur_gps_status)
      {
         case GPS_ST_SYNC: 
         {
            DUSTLOG_DEBUG(m_logname, "AP RX Ready for time: GPS in sync");
            sendSetSystime(ready.seqNum);
         }
         break;
         
         case GPS_ST_CONN_LOST:
            DUSTLOG_DEBUG(m_logname, "AP RX Ready for time: Established GPS connection lost.");
            break;
         case GPS_ST_NO_SYNC :
            DUSTLOG_DEBUG(m_logname, "AP RX Ready for time: No Satellite lock available.");
            break;
         case GPS_ST_NO_DEVICE :
            DUSTLOG_DEBUG(m_logname, "AP RX Ready for time: No GPS Device Available.");
            break;
      }
   }
}

void CAPCoupler::handleTXDone(const dn_api_loc_notif_txdone_t& txDone)
{
   DUSTLOG_DEBUG(m_logname, "AP RX TXDone: pkt=" << txDone.packetId);
   // send TXDone to Manager
   ap_intf_txdone_t apcTxDone = {0};
   apcTxDone.txDoneId = txDone.packetId;
   apcTxDone.status   = (txDone.status == 0) ? APINTF_TXDONE_OK : APINTF_TXDONE_ERR;
   apc_error_t res = APC_ERR_INIT; 
   if (m_mngrClient)
      res = m_mngrClient->sendTxDone(apcTxDone);
   DUSTLOG_DEBUG(m_logname, "AP RX TXDone: rc=" << (int)res);
}

void CAPCoupler::handleParamMacAddress(const dn_api_rsp_get_macaddr_t& getMacAddr)
{
   memcpy(m_apInfo.mac, &getMacAddr.macAddr, sizeof(macaddr_t));
   DUSTLOG_DEBUG(m_logname, "AP Response MAC: mac="
                 << show_hex(m_apInfo.mac, MAC_LENGTH, '-'));
}

void CAPCoupler::handleParamMoteInfo(const dn_api_rsp_get_moteinfo_t& getMoteInfo)
{
   //TODO: 17 bytes should be further divided in to new fields other than version.
   memcpy(&m_apInfo.swVersion, &getMoteInfo.swVer, 5);
   DUSTLOG_DEBUG(m_logname, "AP Response MoteInfo: ver="
                 << show_hex(m_apInfo.swVersion, 5));
}

void CAPCoupler::handleParamAppInfo(const dn_api_rsp_get_appinfo_t& getAppInfo)
{
   memcpy(&m_apInfo.appVersion, &getAppInfo.appVer, 5);
}

void CAPCoupler::handleParamApStatus(const dn_api_rsp_get_apstatus_t& getAppStatus)
{
   m_apInfo.apState = getAppStatus.state;
}

void CAPCoupler::handleParamClkSrc(const dn_api_rsp_get_ap_clksrc_t& getClkSrc)
{
   memcpy(&m_apInfo.clksource, &getClkSrc.apClkSrc, sizeof(m_apInfo.clksource));

   //Clock Source is external then start reading from GPS daemon
   if (m_apInfo.clksource == DN_API_AP_CLK_SOURCE_PPS)
   {
      //connect to GPS only if the command line option gpsd-conn is set to true
      if (m_gps_start_param.m_gpsd_conn) {
         m_gps->start(m_gps_start_param,this);
      }
   } 
   else //Clock Source is other than external then stop the gps if gpsd-conn is true
   {
      if (m_gps_start_param.m_gpsd_conn) {
         m_gps->stop(); //Safe to call more than once.
      }
   }
}

void CAPCoupler::handleParamNetId(const dn_api_rsp_get_netid_t& getNetId)
{
   memcpy(&m_apInfo.networkId, &getNetId.netId, sizeof(m_apInfo.networkId));
}

void CAPCoupler::handleParamGetTime(const dn_api_rsp_get_time_t& getTime)
{
   if (m_transport == nullptr || m_mngrClient == nullptr)
      return;

   // note: this breaks the abstraction, but it's even more awkward to convert
   // byte order on the ASN as a byte array
   asn_t         apAsn = ntoh_asn(getTime.asn.byte, sizeof(getTime.asn));
   ap_time_map_t timemap = {0};
   timemap.asn = apAsn;
   timemap.asnOffset = getTime.offset;
   // Estimate System Time of response as Manager System time - 1/2 round trip time
   CAPMTransport::APMTStats apmStats = m_transport->getAPMStatistics();
   sys_time_t systime = SYSTIME_NOW() - TO_USEC(apmStats.m_lastRspTime/2);
   boost::chrono::system_clock::duration systimeDuration = systime.time_since_epoch();
   timemap.timeSec = TO_SEC(systimeDuration).count(); // System time Seconds since begining
   timemap.timeUsec = (uint32_t)TO_USEC(systimeDuration - TO_SEC(systimeDuration)).count(); // Microseconds since last second

   // Send timeMap to Mngr
   DUSTLOG_DEBUG(m_logname, "AP Time map: asn=" << timemap.asn << " systime=" << timemap.timeSec << " " << timemap.timeUsec);
   m_mngrClient->sendTimeMap(timemap);
}

void CAPCoupler::handleGPSStatusChanged(const gps_status_t& newstatus)
{

   // if gpsd is not required, we don't handle gps events
   if (!m_gps_start_param.m_gpsd_conn) {
      return;
   }

   m_cur_gps_status = newstatus;

   if (m_transport == nullptr || m_mngrClient == nullptr)
      return;

   if (m_apInfo.clksource == DN_API_AP_CLK_SOURCE_PPS)
   {
      /*switch(m_cur_gps_status)
      {
         case GPS_ST_CONN_LOST:
         case GPS_ST_NO_DEVICE:
            m_transport->insertMsg(DN_API_LOC_CMD_DISCONNECT, NULL, 0,true); //send disconnect to AP
            break;
         case GPS_ST_NO_SYNC: //GPS is available but signal is not strong/sync.
         case GPS_ST_SYNC: //GPS is available and in sync.
            break;
      }*/
      if(m_transport->isPortReady()) //Check if AP connected / ready
      {

         // send GPS status to AP
         ap_intf_gpslock_t apcGpsLock = {APINTF_GPS_NOLOCK};
         apcGpsLock.gpsstate = (m_cur_gps_status == GPS_ST_SYNC) ? APINTF_GPS_LOCK : APINTF_GPS_NOLOCK;
         m_mngrClient->sendGpsLock(apcGpsLock);
         m_transport->insertMsg(DN_API_LOC_CMD_WRITE_GPS_STATUS, (uint8_t*)&apcGpsLock.gpsstate, sizeof(apcGpsLock.gpsstate));

         DUSTLOG_DEBUG(m_logname, "AP TX Write GPS Status status=" << (int)apcGpsLock.gpsstate);
      }
   }

}

void CAPCoupler::handleSatellitesUsedChanged(uint16_t used)
{
	m_gps_info.satellites_used = used;
}

void CAPCoupler::handleSatellitesVisibleChanged(uint16_t visible)
{
	m_gps_info.satellites_visible = visible;
}

void CAPCoupler::handleAPPause()
{
   if (m_mngrClient)
      m_mngrClient->sendPause();
}
void CAPCoupler::handleAPResume()
{
   if (m_mngrClient)
      m_mngrClient->sendResume();
}

apc_error_t CAPCoupler::sendCmd(uint8_t cmdId, const uint8_t* data, size_t length,
                                ResponseCallback resCallback, ErrorResponseCallback errRespCallback)
{
   apc_error_t res = APC_ERR_INIT;
   if (m_transport)
      res = m_transport->insertMsg(cmdId, data, length, false, resCallback, errRespCallback);
   return res;
}

apc_error_t CAPCoupler::sendApSend(dn_api_loc_apsend_ctrl_t& hdr,
                                   const uint8_t* data, size_t length)
{
   // convert the header fields to network byte order
   hdr.packetId = htons(hdr.packetId);
   hdr.timeslot = htonl(hdr.timeslot);
   hdr.dest     = htons(hdr.dest);
   
   std::vector<uint8_t> output(sizeof(hdr) + length);
   memcpy(output.data(), (uint8_t*)&hdr, sizeof(hdr));
   memcpy(output.data() + sizeof(hdr), data, length);
   
   apc_error_t res = APC_ERR_INIT;
   if (m_transport)
      res = m_transport->insertMsg(DN_API_LOC_CMD_AP_SEND, 
                                            output.data(), output.size());
   return res;
}

apc_error_t CAPCoupler::sendSetApClkSource(uint8_t clkSource)
{
   return sendSetParam(DN_API_PARAM_AP_CLKSRC, &clkSource, sizeof(clkSource));
}

apc_error_t CAPCoupler::sendSetParam(uint8_t paramId, const uint8_t* data, size_t length)
{
   std::vector<uint8_t> output;
   //output.reserve(length+1);
   output.push_back(paramId);
   output.insert(output.begin()+1, data, data+length);

   apc_error_t res = APC_ERR_INIT;
   if (m_transport)
      res = m_transport->insertMsg(DN_API_LOC_CMD_SETPARAM,
                                            output.data(), output.size());      
   return res;
}

apc_error_t CAPCoupler::sendGetApNetId()
{
   return sendGetParam(DN_API_PARAM_NETID);
}

apc_error_t CAPCoupler::sendGetApMoteInfo()
{
   return sendGetParam(DN_API_PARAM_MOTEINFO);
}

apc_error_t CAPCoupler::sendGetApAppInfo()
{
   return sendGetParam(DN_API_PARAM_APPINFO);
}

apc_error_t CAPCoupler::sendGetApStatus()
{
   return sendGetParam(DN_API_PARAM_AP_STATUS);
}

apc_error_t CAPCoupler::sendGetNetId()
{
   return sendGetParam(DN_API_PARAM_NETID);
}

apc_error_t CAPCoupler::sendGetParam(uint8_t paramId)
{
   apc_error_t res = APC_OK;

   if (m_transport) {
      res = m_transport->insertMsg(DN_API_LOC_CMD_GETPARAM, &paramId, sizeof(paramId));
   }

   return res;
}

void CAPCoupler::sendSetSystime(uint8_t seqNum)
{
   dn_api_set_time_t sendTime;

   sendTime.paramId = DN_API_PARAM_TIME;
   sendTime.seqNum = seqNum;


	//Send SetParameter Command with System time to AP
	boost::chrono::system_clock::duration mngrUTCDuration = SYSTIME_NOW().time_since_epoch();
   // Seconds since beginning
	sendTime.utcTime.seconds = TO_SEC(mngrUTCDuration).count() - ntpGetLeapSecs(); 
	// Microseconds since last second
   sendTime.utcTime.useconds = (uint32_t)TO_USEC((mngrUTCDuration - TO_SEC(mngrUTCDuration))).count(); 
	DUSTLOG_TRACE(m_logname, "AP Set Time: utc="<< sendTime.utcTime.seconds <<"." 
                << std::setfill ('0') << std::setw(6) << sendTime.utcTime.useconds);

   // Convert host to net
   sendTime.utcTime.seconds = CONVERT_LL(HOST_TO_NET, sendTime.utcTime.seconds);
   sendTime.utcTime.useconds = CONVERT_L(HOST_TO_NET, sendTime.utcTime.useconds);

   if (m_transport)
      m_transport->insertMsg(DN_API_LOC_CMD_SETPARAM, (uint8_t *)&sendTime, sizeof(sendTime), true);
}

// [ Implementation of Coupler State Machine --------------------------------------------------
void CAPCoupler::smThreadFun_p()
{

   try {
      int sec = secSinceMidnight_p(AP_BLACKOUT, false);   // Number of seconds before blackout stat time 
      if (sec < 0  && ntpIsLeapDay()) {
         // Wait blackout period if APC start after blackout start time and today is leap second day
         DUSTLOG_INFO(m_logname, "Wait for starting: " << (AP_BLACKOUT_DURATION + sec) << " sec");
         waitEvents_p(0, (AP_BLACKOUT_DURATION + sec) * 1000);
      }
      boost::system::error_code dummy;
      leapCheckingTimer_p(dummy);      // Start checking of day for leap second changing
      m_transport->disableJoin();      // Disable joining

      uint32_t events = E_AP_RESET;    // Coupler events (see E_...)
      for(;;) {
         prepareAP_p(events);
         events = runStateMachine_p();
      }
   } catch (exeption_stop&) {;}

   try {
      prepareAP_p(E_STOP);          // Reset AP for stop it 
   } catch (exeption_stop&) {;}
}

// Check day for leap second changing
void CAPCoupler::leapCheckingTimer_p(const boost::system::error_code& error)
{
   if (error)
      return;
   if (ntpIsLeapDay()) {
      // Set blackout time (start and finish)
      m_blackoutStart = SYSTIME_NOW() + sec_t(secSinceMidnight_p(AP_BLACKOUT));
   } else {
      m_blackoutStart = SYSTIME_EMPTY;
   }
   // Schedule next check
   m_leapCheckTimer.expires_from_now(boost::posix_time::seconds(secSinceMidnight_p(LEAP_CHECK_TIME)));
   m_leapCheckTimer.async_wait(boost::bind(&CAPCoupler::leapCheckingTimer_p, 
                                           this, boost::asio::placeholders::error));
}

void CAPCoupler::prepareAP_p(uint32_t e)
{
   DUSTLOG_INFO(m_logname, "Disconnect / Reset AP. Event map: 0x" << std::hex << e);
   uint32_t timoutMsec = m_disconnectTimeoutLongMsec;

   if (e & E_STOP) {
      // Stopping process
      m_transport->disableJoin();   // Disable joining
      m_transport->disconnectAP(true);
      waitEvents_p(E_APM_BOOT | E_APM_REBOOT, timoutMsec);
      return;
   }

   if (e & (E_APM_BOOT | E_APM_REBOOT)) {
      return;  // Don't touch AP 
   }

   // Disconnect / reset AP
   if (e & E_APM_LOST) {
      // AP Lost - reset it
      m_transport->hwResetAP();
      timoutMsec = m_hwResetTimeoutMsec;
   } else {
      // AP is not lost - send disconnect command
      m_transport->disconnectAP(true);
      if (e & E_AP_RESET) 
         timoutMsec = m_disconnectTimeoutShortMsec;
   }

   while (waitEvents_p(E_APM_BOOT | E_APM_REBOOT, timoutMsec) == E_TIMEOUT) {
      m_transport->hwResetAP();
      timoutMsec = m_hwResetTimeoutMsec;
   }
}

void CAPCoupler::synchStop_p()
{
   bool isWaitDisconnectNotif = m_mngrClient->isConnected();
   m_mngrClient->stop();
   if (isWaitDisconnectNotif)
      waitEvents_p(E_MNGR_DISCONNECT, 1000);
}

uint32_t CAPCoupler::runStateMachine_p()
{
   // Start Manager Client
   apc_error_t res;
   uint32_t    events;
   // Start Manager Client
   while ((res = m_mngrClient->start(m_client_start_param)) != APC_OK) {
      synchStop_p();
      DUSTLOG_WARN(m_logname, "APC Client start failed, " << toString(res) << ". Retrying...");
      boost::this_thread::sleep(boost::posix_time::milliseconds(RETRY_INTERVAL));
   }
   // Wait connection (or error)
   events = waitEvents_p(E_MNGR_CONNECT | E_MNGR_DISCONNECT | E_APM_LOST | E_AP_RESET);

   if ((events ^ E_MNGR_CONNECT) == 0) {
      // Only one Connect event received
      m_transport->enableJoin(m_mngrClient->getNetId());
      // Wait Reboot event, Request to AP SW reset (AP-disconnect) or AP, Manager error
      events = waitEvents_p(E_MNGR_DISCONNECT | E_APM_LOST | E_APM_REBOOT | E_APM_BOOT | E_AP_DISCONNECT | E_AP_RESET);
   }

   // 'Disable Join ' is set by sendEvent_p function when event generated
   // Stop manager Client
   synchStop_p();
   return events;
}
// ] -----------------------------------------------------------------------

// Send events to coupler state machine
void CAPCoupler::sendEvent_p(uint32_t e)
{
   if ((e & (E_MNGR_DISCONNECT | E_APM_LOST | E_APM_REBOOT | E_AP_DISCONNECT | E_AP_RESET)) != 0) {
      m_transport->disableJoin();
   }

   {
      boost::unique_lock<boost::mutex>  lock(m_eventLock);
      m_events |= e;
      m_eventSignal.notify_all();
   }
}

// Wait events (set by mask) or timeout (if timeoutMsec == 0 only mask events)
uint32_t CAPCoupler::waitEvents_p(uint32_t mask, uint32_t timeoutMsec)
{
   boost::unique_lock<boost::mutex>  lock(m_eventLock);
   if (timeoutMsec == 0) {
      for(;;) {
         uint32_t e = 0;
         while(m_events == 0)          // Wait Event 
            m_eventSignal.wait(lock);
         std::swap(e, m_events);
         if (e & E_STOP)        // STOP
            throw exeption_stop();
         if ((e & mask) != 0)          // Event received
            return e;
      }
   } else {
      mngr_time_t finishTime = TIME_NOW() + msec_t(timeoutMsec);
      for(;;) {
         uint32_t e = 0;
         while(m_events == 0 && TIME_NOW() < finishTime) // Wait Event or Timeout
            m_eventSignal.wait_until(lock, finishTime);
         std::swap(e, m_events);
         if (e & E_STOP)        // STOP
            throw exeption_stop();     
         if ((e & mask) != 0)          // Event received
            return e;
         if (e == 0 &&  TIME_NOW() >= finishTime)  // No events. Timeout
            return E_TIMEOUT;
      }
   }
}

// Get the interval (in seconds) between the given time and now, 
// where the given time is expressed as a number of seconds since 
// the previous midnight. If borrowDay is TRUE, then calculate 
// the interval to the next occurrence of the specified time. 
int CAPCoupler::secSinceMidnight_p(int utcSec, bool borrowDay)
{
   tm     UTC = ntpGetCurUTC();
   int res = utcSec - (((UTC.tm_hour * 60 + UTC.tm_min) * 60) + UTC.tm_sec);
   if (res <= 0 && borrowDay)
      res += (24*60*60);
   return res;
}
