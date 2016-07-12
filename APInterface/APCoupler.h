/*
 * Copyright (c) 2014, Linear Technology. All rights reserved.
 */

#pragma once

#include <iostream>
#include <iomanip>
#include <iterator>
#include <algorithm>
#include <vector>
#include <string>
#include <sstream>

#include "common.h"
#include "Logger.h"

#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/thread.hpp>

#include "SerialPort.h"
#include "APMTransport.h"


#include "IAPCoupler.h"
#include "NTPLeapSec.h"
#include "public/IAPCClient.h"
#include "public/IGPS.h"

#include "6lowpan/public/dn_api_common.h"
#include "6lowpan/public/dn_api_local.h"


// TODO: APCoupler
// Manage the protocol with the AP, integrate with AP Connector

struct SAPMInfo {
   macaddr_t mac;
   uint8_t   swVersion[5];    ///< value from MoteInfo
   uint8_t   appVersion[5];   ///< value from appInfo
   uint8_t   clksource;
   uint16_t  networkId;
   uint8_t   apState;
   std::string serialPort;
};

struct SAPCInfo {
   std::string m_host;
   int m_port;
};

struct gps_info_t {
   uint16_t satellites_visible; ///< number of satellites visible
   uint16_t satellites_used;    ///< number of satellites used
};

/**
 * AP Coupler
 *
 * Manage the protocol with the AP, integrate with AP Connector
 */
class CAPCoupler : public IAPMNotifHandler,  // input from Serializer
                   public IAPCClientNotif,     // input from Manager
                   public IGPSNotifHandler   // input from GPS
{
public:

   /// APCoupler initialization parameters
   struct init_param_t
   {
      CAPMTransport*              transport;
      IAPCClient*                 mngrClient;
      IAPCClient::start_param_t   mngrClientParam;
      IGPS*                       gps;
      IGPS::start_param_t         gpsCfg;
      uint32_t                    resetBootTimeoutMsec;
      uint32_t                    disconnectShortBootTimeoutMsec;
      uint32_t                    disconnectLongBootTimeoutMsec;
      EAPClockSource              apClkSource;
   };

   CAPCoupler(boost::asio::io_service& io_service);

   virtual ~CAPCoupler();

   SAPMInfo getAPMInfo() { return m_apInfo; }

   SAPCInfo getAPCInfo() { return m_apcInfo; }

   gps_info_t getGpsInfo() { return m_gps_info; }

   CAPMTransport::APMTStats getAPMStats() { return m_transport->getAPMStatistics(); }

   void clearAPMStats() { m_transport->clearAPMStatistics();}

   size_t getAPMQueueSize() { return  m_transport->queueSize(); }

   size_t getMgrCachedPkts() { return  m_mngrClient->getCachedPkts(); }

   statdelays_s getAPCCStats() const { return m_mngrClient->getAPCCStats(); }
   uint32_t     getNumRcvPkt() const { return m_mngrClient->getNumRcvPkt(); }

   void clearMgrStats() { m_mngrClient->clearStats(); }

   gps_status_t getGpsStatus() { return m_cur_gps_status; }

   apc_error_t open(init_param_t& init_params);
   void start();
   void stop();

   apcclient_state_t getClientState() const { return m_mngrClient->getState(); }
   bool isAPConnected() const { return m_apConnected;}
   
   // IAPCClientNotif

   virtual void connected(const std::string server, uint32_t flags);
   virtual void disconnected(apc_stop_reason_t  reason);
   virtual void dataRx(const ap_intf_sendhdr_t& hdr, const uint8_t * pPayload, uint32_t size);
   virtual void resume();
   virtual void pause();
   virtual void resetAP();
   virtual void disconnectAP();
   virtual void getTime();

   virtual void offline(apc_stop_reason_t  reason);
   virtual void online();
   
   // IAPNotifHandler - input from APM Serializer
   
   virtual void handleAPReceive(const uint8_t* data, size_t length);
   virtual void handleEvent(const dn_api_loc_notif_events_t& event);
   virtual void handleTimeIndication(const dn_api_loc_notif_time_t& timeMap);
   virtual void handleReadyForTime(const dn_api_loc_notif_ready_for_time_t& ready);
   virtual void handleTXDone(const dn_api_loc_notif_txdone_t& txDone);
   virtual void handleParamMacAddress(const dn_api_rsp_get_macaddr_t& getParam);
   virtual void handleParamMoteInfo(const dn_api_rsp_get_moteinfo_t& getMoteInfo);
   virtual void handleParamAppInfo(const dn_api_rsp_get_appinfo_t& getAppInfo);
   virtual void handleParamClkSrc(const dn_api_rsp_get_ap_clksrc_t& getClkSrc);
   virtual void handleParamNetId(const dn_api_rsp_get_netid_t& getNetId);
   virtual void handleParamGetTime(const dn_api_rsp_get_time_t& getTime);
   virtual void handleParamApStatus(const dn_api_rsp_get_apstatus_t&);
   virtual void handleAPPause();
   virtual void handleAPResume();
   virtual void handleAPLost();
   virtual void handleAPBoot();
   virtual void handleAPReboot();
   virtual void handleError(uint8_t cmdId, uint8_t rc);
   
   apc_error_t sendCmd(uint8_t cmdId, const uint8_t* data, size_t length,ResponseCallback resCallback = NULL,
                      ErrorResponseCallback errRespCallback = NULL);

   // IAPCGPSNotifHandler - GPS status change feed from GPS
   virtual void handleGPSStatusChanged(const gps_status_t& newstatus);
   virtual void handleSatellitesVisibleChanged(uint16_t);
   virtual void handleSatellitesUsedChanged(uint16_t);
   
private:
   class exeption_stop : public std::exception {
   public:
      exeption_stop(){;}
   };

   apc_error_t sendApSend(dn_api_loc_apsend_ctrl_t& hdr,
                          const uint8_t* data, size_t length);
   apc_error_t sendSetApClkSource(uint8_t clkSource);
   apc_error_t sendGetApNetId();
   apc_error_t sendGetApMoteInfo();
   apc_error_t sendGetApAppInfo();
   apc_error_t sendGetApStatus();
   apc_error_t sendGetNetId();
   apc_error_t sendGetApClkSource();
   apc_error_t sendGetParam(uint8_t paramId);
   apc_error_t sendSetParam(uint8_t paramId, const uint8_t* data, size_t length);
   // Send current Manager time to AP
   void sendSetSystime(uint8_t seqNum);

   // Implementation of Coupler State machine
   void     smThreadFun_p();
   void     prepareAP_p(uint32_t event);
   uint32_t runStateMachine_p();
   void     synchStop_p();

   void     sendEvent_p(uint32_t e);
   uint32_t waitEvents_p(uint32_t mask = ~0u, uint32_t timeoutMsec = 0);

   // Check day for leap second changing
   void     leapCheckingTimer_p(const boost::system::error_code& error);


   // Get the interval (in seconds) between the given time and now, 
   // where the given time is expressed as a number of seconds since 
   // the previous midnight. If borrowDay is TRUE, then calculate 
   // the interval to the next occurrence of the specified time. 
   int secSinceMidnight_p(int utcSec, bool borrowDay = true);
   

   boost::asio::deadline_timer m_leapCheckTimer;
   
   SAPMInfo m_apInfo;  ///< AP information
   SAPCInfo m_apcInfo; ///< APC information
   
   CAPMTransport* m_transport;   ///< pointer to the APM Transport
   IAPCClient*    m_mngrClient;  ///< interface to the APCClient
   IGPS*          m_gps;         ///< interface to the GPS
   bool           m_isIntClk;    ///< Set Internal Clock Source to AP
   bool           m_apConnected;
   // Timeout for wait Boot / Re-boot event
   uint32_t       m_hwResetTimeoutMsec;         // Timeout after hardware reset
   uint32_t       m_disconnectTimeoutShortMsec; // Short timeout after disconnect command
   uint32_t       m_disconnectTimeoutLongMsec;  // Long timeout after disconnect command
   EAPClockSource m_apClkSource;                // AP clock source
   bool           m_resetAp;                    // need to reset AP because of clock source change?

   std::string m_logname;
   IAPCClient::open_param_t   m_client_open_param;  ///< Client open parameters 
   IAPCClient::start_param_t  m_client_start_param; ///< Client start parameters
   IGPS::start_param_t        m_gps_start_param;    ///< GPS start parameters
   gps_status_t               m_cur_gps_status;     ///< Current GPS Status   
   gps_info_t                 m_gps_info;           ///< Current GPS info
   
   uint32_t                  m_events;
   boost::mutex              m_eventLock;
   boost::condition_variable m_eventSignal;
   boost::thread*            m_smThread;           

   sys_time_t           m_blackoutStart;      ///< Start time of blackout interval
};
