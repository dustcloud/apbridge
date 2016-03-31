/*
 * Copyright (c) 2014, Linear Technology. All rights reserved.
 */

#include "APMTransport.h"
#include <sstream>
#include <algorithm>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/thread/thread.hpp>
#include "common/IChangeNodeState.h"

using namespace std;

// Default initialization values

// TODO: don't repeat these values in apc_main
const uint16_t DEFAULT_MAX_QUEUE_SIZE = 16;
const uint16_t DEFAULT_HIGH_QUEUE_WATERMARK = (uint16_t)(16 * .8);
const uint16_t DEFAULT_LOW_QUEUE_WATERMARK = (uint16_t)(16 * .4);
const uint32_t DEFAULT_PING_TIMEOUT = 1500; // milliseconds
const uint32_t DEFAULT_RETRY_DELAY = 100;   // milliseconds, retry timer when receving NACK from AP
const uint32_t DEFAULT_RETRY_TIMEOUT = 1200; // milliseconds
const uint16_t DEFAULT_MAX_RETRIES = 3;
const uint16_t DEFAULT_MAX_MSG_SIZE = 256; // bytes -- TODO: sync with APC
const uint32_t DEFAULT_MAX_PACKET_AGE = 1500; // milliseconds, max packet age before triggering 
                                              // TX_PAUSE to manager
uint32_t last_numPktsSent = 0;                // total packets sent one second ago
uint32_t numPktSent_30sec_ago = 0;            // total packet sent 30 seconds ago
uint32_t numPktSent_5min_ago = 0;             // total packet sent 5 minutes ago


CAPMTransport::init_param_t::init_param_t()
   : maxQueueSize(DEFAULT_MAX_QUEUE_SIZE),
     highQueueWatermark(DEFAULT_HIGH_QUEUE_WATERMARK),
     lowQueueWatermark(DEFAULT_LOW_QUEUE_WATERMARK),
     pingTimeout(DEFAULT_PING_TIMEOUT),
     retryDelay(DEFAULT_RETRY_DELAY),
     retryTimeout(DEFAULT_RETRY_TIMEOUT),
     maxRetries(DEFAULT_MAX_RETRIES),
     maxMsgSize(DEFAULT_MAX_MSG_SIZE),
     maxPacketAge(DEFAULT_MAX_PACKET_AGE)
{
   // Intentionally blank
}

CAPMTransport::init_param_t::init_param_t(uint16_t aQueueSize,
                                          uint16_t aHighWatermark,
                                          uint16_t aLowWatermark,
                                          uint32_t aPingTimeout,
                                          uint32_t aNackDelay,
                                          uint32_t aRetryTimeout,
                                          uint16_t aMaxRetries,
                                          uint16_t aMsgSize,
                                          uint32_t aPacketAge)
   : maxQueueSize(aQueueSize),
     highQueueWatermark(aHighWatermark),
     lowQueueWatermark(aLowWatermark),
     pingTimeout(aPingTimeout),
     retryDelay(aNackDelay),
     retryTimeout(aRetryTimeout),
     maxRetries(aMaxRetries),
     maxMsgSize(aMsgSize),
     maxPacketAge(aPacketAge)
{
   // Intentionally blank
}


int apt_hdr_s::serialize(uint8_t* output, size_t len)
{
   BOOST_ASSERT(len >= LENGTH);
   output[0] = cmdId;
   output[1] = length & 0xFF; // TODO: htons(length);
   output[2] = flags;
   return LENGTH;
}
int apt_hdr_s::parse(const uint8_t* input, size_t len)
{
   if (len >= 3) {
      cmdId = input[0];
      length = input[1]; // TODO: ntohs(input[1] ++ input[2]);
      flags = input[2];
      return LENGTH;
   } else {
      return 0;
   }
}

const char APM_IO_LOGGER[] = "apm.io";
const char APM_RAWIO_LOGGER[] = "apm.io.raw";

CAPMTransport::CAPMTransport(size_t maxMsgSize, boost::asio::io_service& io_service,
                             IInputHandler* outputHandler, IAPMNotifHandler* notifHandler,
                             bool bReconnectSerial)
   : m_cmdHandler(new CAPMSerializer(maxMsgSize, this)),
     m_outputHandler(outputHandler),
     m_notifHandler(notifHandler),
     m_init_params(), // initialize to defaults
     m_outputTimer(io_service),
     m_pingTimer(io_service),     
     m_ppsTimer(io_service),     
     m_curRetryCount(0),
     m_curNackCount(0),
     m_respPacketId(0),
     m_notifPacketId(0),
     m_pending(false),
     m_outputQueue(),     
     m_log(), // TODO: replace static APM log strings
     m_sendTime(),
     m_stats(),
     m_apInputState(APM_FLOW_NORMAL),
     m_mngrInputState(APM_FLOW_NORMAL),
     m_isMngrConnected(false),
     m_isApConnected(false),
     m_isRunning(false),
     m_netId(0),
     m_reconnectSerial(bReconnectSerial),
     m_queueCheckTimer(io_service)
{
   ;
}

void CAPMTransport::open(const init_param_t& param)
{
   m_init_params = param;

   m_outputQueue.set_capacity(m_init_params.maxQueueSize); //Maximum number of commands which can be queued in circular buffer

   startPPSTimer();  // start calculate packet per second from/to AP
}

apc_error_t CAPMTransport::stop()
{
   stopRetryTimer();
   stopPingTimer();
   stopQueueCheckTimer();

   uint32_t avgRspTime = 0;
   uint32_t avgTimeInQueue = 0;
   if (m_stats.m_numRespRecv > 0) {
      avgRspTime = (uint32_t)(m_stats.m_totalRspTime / m_stats.m_numRespRecv);
   }

   if (m_stats.m_numPacketsQueued > 0) {
      avgTimeInQueue = (uint32_t)(m_stats.m_totalTimeInQueue / m_stats.m_numPacketsQueued);
   }
   
   //Log the statistics
   DUSTLOG_INFO(APM_IO_LOGGER, "APM Transport statistics:" << 
                "\nPACKETS SENT: " << m_stats.m_numPktsSent <<
                "\nPACKETS RECEIVED: " << m_stats.m_numPktsRecv << 
                "\nBYTES SENT: " << m_stats.m_numBytesSent << 
                "\nBYTES RECEIVED: " << m_stats.m_numBytesRecv <<
                "\nRESPS RECEIVED : " << m_stats.m_numRespRecv <<
                "\nMIN RESP TIME (usec): " << m_stats.m_minRspTime << 
                "\nMAX RESP TIME (usec): " << m_stats.m_maxRspTime <<
                "\nAVG RESP TIME (usec): " << avgRspTime << 
                " (" << m_stats.m_totalRspTime << "/" << m_stats.m_numRespRecv << ")" <<
                "\nMIN TIME IN QUEUE (usec): " << m_stats.m_minTimeInQueue << 
                "\nMAX TIME IN QUEUE (usec): " << m_stats.m_maxTimeInQueue <<
                "\nAVG TIME IN QUEUE (usec): " << avgTimeInQueue << 
                " (" << m_stats.m_numPacketsQueued << "/" << m_stats.m_numPacketsQueued << ")" <<
                "\nNACKS RECEIVED:" << m_stats.m_numNacks <<
                "\nMAX NACKS IN A ROW:" << m_stats.m_maxNackCount <<
                "\nRETRIES SENT: " << m_stats.m_numRetriesSent <<
                "\nNOTIF RETRIES : " << m_stats.m_numRetriesRecv << "\n");
   return APC_OK;
}

// handle data from serial port

apc_error_t CAPMTransport::handleMsg(apt_hdr_s* pHdr,
                                     const uint8_t* data, size_t size)
{
   if (pHdr == nullptr) {
      DUSTLOG_ERROR(APM_IO_LOGGER, "CAPMTransport::handleMsg pHdr==NULL");
      return APC_ERR_PKTSERIALIZATION;
   }

   bool isResp = pHdr->flags & 1;
   uint8_t pktId = pHdr->flags & 2;

   ostringstream cmdstr;
   cmdstr << "cmd: 0x" << hex << (int)pHdr->cmdId;
   const char* msgType = (isResp)?" RSP:":" REQ:";
   DUSTLOG_DEBUG(APM_IO_LOGGER, "INP " << cmdstr.str() << msgType << (int)pktId);

   apc_error_t res = APC_OK;
   if (isResp) {
      uint8_t lastCmd = 0;
      {
         boost::unique_lock<boost::mutex> lock(m_lock);
         if (!m_outputQueue.empty()) {
            lastCmd = m_outputQueue.front().cmdId;
         } else {
            // we are not expecting any response from AP, so discard silently
            return APC_OK;
         }
      }
      // ensure this response is for most recent command
      if (pHdr->cmdId != lastCmd) {
         DUSTLOG_WARN(APM_IO_LOGGER, "INP " << cmdstr.str()
                      << " does not match last cmd (0x" << std::hex << (int)lastCmd << ")");
         return APC_OK;
      }
      if (pktId != m_respPacketId) {
         DUSTLOG_WARN(APM_IO_LOGGER, "INP pktId " << (int)pktId
                      << " does not match last packetId (" << (int)m_respPacketId << ")");
         return APC_OK;
      }

	  // handle the response
	  res = handleResponse(*pHdr, data, size);
      
   } else {
      {
         ostringstream prefix;
         prefix << "INP " << cmdstr.str() << " data";
         DUSTLOG_TRACEDATA(APM_RAWIO_LOGGER, prefix.str(), data, size);
      }
	  // handle the notification
	  res = handleNotification(*pHdr, data, size);
   }
   return res;
}

apc_error_t CAPMTransport::handleResponse(const apt_hdr_s& hdr,
                                          const uint8_t* data, size_t size)
{
   // First thing is to cancel the retry timer for the valid response
   stopRetryTimer();
   
   apc_error_t res = APC_OK;
   bool belowLowWatermark = false;
   
   // Increment response counter
   m_stats.m_numRespRecv++;
   // calculate last response time
   m_stats.m_lastRspTime = TIME_NOW() - m_sendTime;
   uint64_t curRspTime = TO_USEC(m_stats.m_lastRspTime).count();
   m_stats.m_totalRspTime += curRspTime; // update total response time

   m_stats.m_minRspTime = min(curRspTime, m_stats.m_minRspTime);
   m_stats.m_maxRspTime = max(curRspTime, m_stats.m_maxRspTime);

   // increment number of bytes received from AP
   m_stats.m_numBytesRecv += size;
   // increment number of packets received from AP
   m_stats.m_numPktsRecv++;
   
   // handle the result code
   uint8_t rc = data[0];
   DUSTLOG_DEBUG(APM_IO_LOGGER, "INP ACK "
                 << "cmd: 0x" << hex << (int)hdr.cmdId
                 << " rc: 0x" << hex << (int)rc);

   // toggle the next outgoing packet id for both ACK and NACK
   // packet id is in bit 1
   m_respPacketId ^= (1 << 1);

   ResponseCallback      respCallback = NULL;     ///< response callback for outgoing command
   ErrorResponseCallback errRespCallback = NULL;  ///< response callback for error
   {
      boost::unique_lock<boost::mutex> lock(m_lock);
      if (m_outputQueue.empty()) 
         return APC_ERR_STATE;

      respCallback = m_outputQueue.front().resCallback;
      errRespCallback = m_outputQueue.front().errResCallback;
      if (rc == DN_API_RC_NO_RESOURCES) {
         // handle NACK -- restart retry timer with NACK delay
         m_stats.m_numNacks++; // increment total NACK counter
         m_curNackCount++;
         m_stats.m_maxNackCount = max(m_curNackCount, m_stats.m_maxNackCount);
         startNackRetryTimer();

      } else {

	      m_stats.m_timeInQueue = TIME_NOW() - m_outputQueue.front().timestamp;
	      uint64_t curTimeInQueue = TO_USEC(m_stats.m_timeInQueue).count();
	      m_stats.m_totalTimeInQueue += curTimeInQueue;

         m_stats.m_minTimeInQueue = min(curTimeInQueue, m_stats.m_minTimeInQueue);
         m_stats.m_maxTimeInQueue = max(curTimeInQueue, m_stats.m_maxTimeInQueue);

	      m_outputQueue.pop_front();
         belowLowWatermark = m_outputQueue.size() < m_init_params.lowQueueWatermark;
         m_curNackCount = 0;

         //Increment Number of packets sent from Mgr(APMTransport)->AP
         m_stats.m_numPktsSent++;

         // no lock: m_last* members are only used on the io_service thread
         // clear last output buffer
         m_pending = false;
      }
   }

   // Check if queue reached low watermark, then inform manager to resume
   if (m_mngrInputState == APM_FLOW_PAUSE && belowLowWatermark) {
      // Low watermark reached, inform APC Client
      m_notifHandler->handleAPResume();
      m_mngrInputState = APM_FLOW_NORMAL;
   }
   
   if (rc == DN_API_RC_OK) {
      DUSTLOG_DEBUG(APM_RAWIO_LOGGER, "RC = DN_API_RC_OK");
      // if the ACK contains data, then handle the response
      if (size > 1) {
         DUSTLOG_TRACEDATA(APM_RAWIO_LOGGER, "INP resp", data, size);
         if (respCallback != NULL) {
            DUSTLOG_DEBUG(APM_RAWIO_LOGGER, "Calling response callback");
            respCallback(hdr.cmdId, data, size);
         } else {
            res = m_cmdHandler->handleCmd(hdr.cmdId, data, size);
         }
      }
   } else {
      DUSTLOG_DEBUG(APM_RAWIO_LOGGER, "RC = " << (int)rc);
      if (errRespCallback != NULL) {
         DUSTLOG_DEBUG(APM_RAWIO_LOGGER, "Calling error-response callback");
         errRespCallback(hdr.cmdId, rc);
      } else {
         m_cmdHandler->handleError(hdr.cmdId, rc);
      }
   }

   send(true);
   
   return res;
}

apc_error_t CAPMTransport::handleNotification(const apt_hdr_s& hdr,
                                              const uint8_t* data, size_t size)
{
   mngr_time_t startTime = TIME_NOW();
   apc_error_t res = APC_OK;
   uint8_t pktId = hdr.flags & 2;
   bool isSync = (hdr.flags & 8) > 0;
   // record the pktId so we don't process the same notif twice
   // but we need to always packets with the sync bit (e.g. boot events)
   if (pktId != m_notifPacketId || isSync) {
      // increment number of bytes, packets received from AP
      m_stats.m_numBytesRecv += size;
      m_stats.m_numPktsRecv++;
      // check if Manager is in a state to handle apReceive Command
      if (m_apInputState == APM_FLOW_PAUSE && 
          hdr.cmdId == DN_API_LOC_NOTIF_AP_RECEIVE) {
         sendAck(hdr.cmdId, pktId, DN_API_RC_NO_RESOURCES); //NACK to AP
         DUSTLOG_WARN(APM_IO_LOGGER, "APC send NAK to AP.  (processing time " 
            << (TO_USEC(TIME_NOW() - startTime).count()) << " usec)");
         return APC_ERR_STATE;
      }
      // process the notification before generating the ack
      res = m_cmdHandler->handleCmd(hdr.cmdId, data, size);
      // if we NACK, we need to process the notif again
      if (res == APC_OK) {
         m_notifPacketId = pktId;
      }
   }
   else if (pktId == m_notifPacketId) {
      m_stats.m_numRetriesRecv++;
      DUSTLOG_WARN(APM_IO_LOGGER, "********************** Retry received " <<  m_stats.m_numRetriesRecv);
      DUSTLOG_WARN(APM_IO_LOGGER, "cmd: 0x" << setfill('0') << setw(2) << hex << (int)hdr.cmdId 
         << ", flag: 0x" << setfill('0') << setw(2) << hex << (int)hdr.flags
         << ",   time (us) : " << dec  
         << boost::chrono::duration_cast<boost::chrono::microseconds>(startTime.time_since_epoch()).count());
   }
   // send an acknowledgement -- TODO: not always?
   DUSTLOG_DEBUG(APM_IO_LOGGER, "OUT ACK " << "cmd: 0x" << hex << (int)hdr.cmdId);
   sendAck(hdr.cmdId, pktId, res);
   
   int64_t t = TO_USEC(TIME_NOW() - startTime).count();
   if (t > 10000) {
      DUSTLOG_WARN(APM_IO_LOGGER, "APC processing time of command " << (int)hdr.cmdId 
                   << " is " << t << " usec");
   }

   return res;
}

apc_error_t CAPMTransport::insertMsg(uint8_t cmdId, const uint8_t* data, size_t size,
                                     bool isHighPriority,
                                     ResponseCallback resCallback,
                                     ErrorResponseCallback errRespCallback, bool isSynch)
{

   if (!m_outputHandler->isReady()) {
      DUSTLOG_ERROR(APM_IO_LOGGER, "insertMsg: output handler is not ready. "
                    "AP is lost, resetting.");
      //Send AP Lost to manager
      sendAPLostNotif_p();
      return APC_ERR_STATE;
   }

   bool atHighWatermark = false;
   {
      boost::unique_lock<boost::mutex> lock(m_lock);
	   boost::chrono::steady_clock::time_point timestamp = TIME_NOW();

      if (m_outputQueue.full()) {
         DUSTLOG_ERROR(APM_IO_LOGGER, "insertMsg: output buffer overflow, queue="
                       << m_outputQueue.size());
         return APC_ERR_OUTBUFOVERFLOW;
      }

      // VOY-1286, include timestamp for queued packet
      m_stats.m_numPacketsQueued++;
      if (isHighPriority)
         m_outputQueue.push_front(APMCommand(cmdId, data, size, isSynch, resCallback, errRespCallback, timestamp));
      else
         m_outputQueue.push_back(APMCommand(cmdId, data, size, isSynch, resCallback, errRespCallback, timestamp));
       atHighWatermark = m_outputQueue.size() >= m_init_params.highQueueWatermark;
   }
   send(true);
      
   // check if queue reached high watermark
   if (m_mngrInputState == APM_FLOW_NORMAL && atHighWatermark) {
      DUSTLOG_WARN(APM_IO_LOGGER, "insertMsg: output buffer at high watermark, "
                   "queue>=" << m_init_params.highQueueWatermark);
      // High watermark reached, inform APC Client
      m_notifHandler->handleAPPause();
      m_mngrInputState = APM_FLOW_PAUSE;
   }     

   if (m_outputQueue.size() > 0) {
      startQueueCheckTimer();
   }

   return APC_OK;
}

// send resume to Manager
void CAPMTransport::sendMgrResume()
{
   if (m_mngrInputState == APM_FLOW_PAUSE) {
         m_notifHandler->handleAPResume();
         m_mngrInputState = APM_FLOW_NORMAL;
		 DUSTLOG_WARN(APM_IO_LOGGER, "Output queue flushed, sending Resume to manager");
   }
}

// the timer was started in insertMsg and continue to run until m_outputQueue is empty,
// the timer value will be half of the maxPacketAge
void CAPMTransport::startQueueCheckTimer()
{
   m_queueCheckTimer.expires_from_now(boost::posix_time::milliseconds(m_init_params.maxPacketAge / 2));
   m_queueCheckTimer.async_wait(boost::bind(&CAPMTransport::handleQueueCheckTimeout,
                                        this, boost::asio::placeholders::error));
}

void CAPMTransport::stopQueueCheckTimer()
{
   m_queueCheckTimer.cancel();
}

void CAPMTransport::handleQueueCheckTimeout(const boost::system::error_code& error)
{
   if (error) {
      // timer cancelled
      return;
   }

   boost::unique_lock<boost::mutex> lock(m_lock);

   // if queue is empty and manager is PAUSED, then RESUME it
   if (m_outputQueue.size() == 0) {
      sendMgrResume();
   } else {
      if (m_mngrInputState == APM_FLOW_NORMAL) {
         // check the oldest packet, if too old, we pause manager
         uint32_t diff = (uint32_t)(TO_MSEC(TIME_NOW() - m_outputQueue.front().timestamp).count());
         if (diff > m_init_params.maxPacketAge) {
            DUSTLOG_WARN(APM_IO_LOGGER, "Oldest packet reached " << diff << "ms, sending Pause to manager");
            m_notifHandler->handleAPPause();
            m_mngrInputState = APM_FLOW_PAUSE;
         }
      }

      // restart the timer only if the queue is not empty
      startQueueCheckTimer();
   }

}

apc_error_t CAPMTransport::dataReceived(const uint8_t* data, size_t size)
{
   startPingTimer(); // start/extend ping timer
   if (size < sizeof(apt_hdr_s)) {
      return APC_ERR_SIZE;
   }
   apt_hdr_s* pHdr = (apt_hdr_s*)data;
   return handleMsg(pHdr, (uint8_t*)(pHdr+1), size-sizeof(apt_hdr_s));
}

// timer callbacks

void CAPMTransport::startRetryTimer()
{
   m_outputTimer.expires_from_now(boost::posix_time::milliseconds(m_init_params.retryTimeout));
   m_outputTimer.async_wait(boost::bind(&CAPMTransport::handleRetryTimeout,
                                        this, boost::asio::placeholders::error));
}

void CAPMTransport::stopRetryTimer()
{
   m_curRetryCount = 0; //reset current retry count
   m_outputTimer.cancel();
}

void CAPMTransport::handleRetryTimeout(const boost::system::error_code& error)
{
   if (error) {
      // timer cancelled
      return;
   }
 
   //check retry count, raise error
   if (m_curRetryCount < m_init_params.maxRetries) {

      // if reconnect-serial is enabled, do close and open serial open upon failure
      if (m_reconnectSerial) {
	      // retry happens quite often, let's re-establish if happened twice in a row
	      if (m_curRetryCount > 0) {
	         DUSTLOG_WARN(APM_IO_LOGGER,"Serial port timeout occured too frequent, re-establish serial connection.");
	         try {
	            m_outputHandler->closePort();
	            m_outputHandler->openPort();
	            m_outputHandler->restart();
	         } catch (const boost::exception&) {
	            DUSTLOG_ERROR(APM_IO_LOGGER, "Failed to re-establish serial connection");
	         }
	      }
      }

      //send packet again
      sendRetry();
      //Timeout Occurred, Increment statistic counter
      m_stats.m_numRetriesSent++;
      //increment retry count
      m_curRetryCount++;
      startRetryTimer();
   } else {
      DUSTLOG_ERROR(APM_IO_LOGGER,"No response after " << m_init_params.maxRetries << " retries. AP is lost, reset AP.");
      //AP is lost so no point of keep on pinging
      stopPingTimer();
      //Send AP Lost to manager
      sendAPLostNotif_p();
      //Hardware reset AP
      hwResetAP();

      // if reconnect-serial is enabled, we restart APC after maxRetries.
	  if (m_reconnectSerial) {
	      // let's restart APC as well
	      DUSTLOG_WARN(APM_IO_LOGGER,"Restarting APC.");
	      IChangeNodeState::getChangeNodeStateObj().stop("Serial Port Error");
      }
   }
}


// timer callback when receiving NACK from AP
void CAPMTransport::startNackRetryTimer()
{
   m_outputTimer.expires_from_now(boost::posix_time::milliseconds(m_init_params.retryDelay));
   m_outputTimer.async_wait(boost::bind(&CAPMTransport::handleRetryTimeout,
                                        this, boost::asio::placeholders::error));
}

size_t CAPMTransport::queueSize() 
{
   boost::unique_lock<boost::mutex> lock(m_lock);
   size_t result = 0;
   result = m_outputQueue.size();
   return result;
}
   
apc_error_t CAPMTransport::sendCommand(uint8_t cmdId, const uint8_t* data, size_t length, bool isSynch,
                                       ResponseCallback resCallback, ErrorResponseCallback errRespCallback)
{
   if (m_outputHandler == nullptr) {
      DUSTLOG_ERROR(APM_IO_LOGGER, "CAPMTransport::sendCommand. Pending command or handler == nullptr");
      return APC_ERR_STATE;
   }

   uint8_t f = m_respPacketId; // REQ (=0) | PktId | 0 | Sync (=0)
   if (isSynch)
      f |= 8;
   apt_hdr_s hdr(cmdId, length, f);
   vector<uint8_t> output(length + hdr.LENGTH);
   hdr.serialize(output.data(), output.size());
   // copy payload to output
   if (length > 0) {
      copy(data, data+length, output.begin() + hdr.LENGTH);
   }
   {
      ostringstream prefix;
      prefix << "OUT " << "cmd:" << hex << (int)cmdId << " data";
      DUSTLOG_TRACEDATA(APM_RAWIO_LOGGER, prefix.str(), output.data(), output.size());
   }
   // send command

   //Note down send time to calculate response time when we receive packets from AP
   m_sendTime = TIME_NOW();
   //Increment Number of payload bytes sent from Mgr(APMTransport)->AP
   m_stats.m_numBytesSent+=length;
   
   // save packet for retries
   m_pending = true;

   m_outputHandler->handleData(output);

   // set timer for retry
   startRetryTimer();
   startPingTimer(); // start/extend ping timer
   return APC_OK;
}

void CAPMTransport::send(bool isNew)
{

   mngr_time_t now = TIME_NOW();

   if (!m_outputHandler->isReady()) {
      DUSTLOG_WARN(APM_IO_LOGGER, "Output handler not ready in sendNext");
      return;
   }

   {
      boost::unique_lock<boost::mutex> lock(m_lock);
      // If we called sendNext, we expect there to be a packet in the queue
      if (m_outputQueue.size() == 0 || (isNew && m_pending) || (!isNew && !m_pending)) {
         return;
      }

      // Send the packet at the front of the queue
      APMCommand& cmd = m_outputQueue.front();

      if (!isNew) {
         DUSTLOG_WARN(APM_IO_LOGGER, "sending retry cmd: 0x" 
            << setfill('0') << setw(2) << hex << (int)cmd.cmdId
            << ", time (us) : " << dec
            << boost::chrono::duration_cast<boost::chrono::microseconds>(now.time_since_epoch()).count());
      } else {
         DUSTLOG_DEBUG(APM_IO_LOGGER, "Sending packet, cmdId: 0x" << hex << (int)cmd.cmdId);
      }
      sendCommand(cmd.cmdId, cmd.payload.data(), cmd.payload.size(), cmd.isSynch, cmd.resCallback, cmd.errResCallback);
   }
   
}
   
void CAPMTransport::sendRetry()
{
   DUSTLOG_WARN(APM_RAWIO_LOGGER, "Sending retry attempt #" << m_curRetryCount+1);
   send(false);
}

void CAPMTransport::sendAck(uint8_t notifId, uint8_t pktId, uint8_t rc)
{
   apt_hdr_s hdr(notifId, 0, 1 | pktId); // Resp (=1) | PktId
   const size_t hdrLen = hdr.LENGTH;
   vector<uint8_t> output(hdrLen + 1);
   hdr.serialize(output.data(), output.size());
   output[hdrLen] = rc;
   DUSTLOG_TRACEDATA(APM_RAWIO_LOGGER, "OUT ACK", output.data(), output.size());
   //Increment Number of payload bytes sent from Mgr(APMTransport)->AP
   m_stats.m_numBytesSent+=sizeof(rc);
   //Increment Number of packets sent from Mgr(APMTransport)->AP
   m_stats.m_numPktsSent++;
   // send ack
   m_outputHandler->handleData(output);
}

apc_error_t CAPMTransport::sendPing()
{
   // Ping AP with GetMacAddress command
   uint8_t cmdId = DN_API_LOC_CMD_GETPARAM;
   uint8_t paramId = DN_API_PARAM_MACADDR;
   size_t length = sizeof(paramId);

   return insertMsg(cmdId, &paramId, length); 
}

void CAPMTransport::startPingTimer()
{
   m_pingTimer.expires_from_now(boost::posix_time::milliseconds(m_init_params.pingTimeout));
   m_pingTimer.async_wait(boost::bind(&CAPMTransport::handlePingTimeout, 
                                      this, boost::asio::placeholders::error));
}

void CAPMTransport::stopPingTimer()
{
   m_pingTimer.cancel();
}

void CAPMTransport::handlePingTimeout(const boost::system::error_code& error)
{
   if (error) {
      // timer cancelled
      return;
   }

   DUSTLOG_DEBUG(APM_IO_LOGGER, "Ping interval expired");
   // no pending response and no pending commands to send (idle) then send ping to AP
   if (queueSize() == 0) {
      DUSTLOG_DEBUG(APM_IO_LOGGER, "Sending ping");
      sendPing();
   }
   startPingTimer(); // continue to ping 
}

void CAPMTransport::clearOutputQueue()
{
   boost::unique_lock<boost::mutex> lock(m_lock);
   m_outputQueue.clear();
   stopRetryTimer();
   stopPingTimer();
   m_respPacketId = 0;
   m_notifPacketId = 0;
   m_pending = false;
   m_apInputState = APM_FLOW_NORMAL;
   m_mngrInputState = APM_FLOW_NORMAL;
}

bool CAPMTransport::isPortReady()
{
   return m_outputHandler->isReady();
}

void CAPMTransport::hwResetAP()
{
   DUSTLOG_INFO(APM_IO_LOGGER, "AP Hardware reset");
   m_isRunning = false;
   m_outputHandler->sendResetAP();
}

void  CAPMTransport::handleDisconnectErrorResponse_p(uint8_t cmdId, uint8_t rc)
{
   m_outputHandler->sendResetAP();
}

void CAPMTransport::disconnectAP(bool isSinchBit) {
   DUSTLOG_INFO(APM_IO_LOGGER, "Send command 'Disconnect AP'");
   ErrorResponseCallback errResCallback = boost::bind(&CAPMTransport::handleDisconnectErrorResponse_p, this, _1, _2);

   insertMsg(DN_API_LOC_CMD_DISCONNECT, NULL, 0, true, NULL, errResCallback, isSinchBit);
}

void CAPMTransport::disableJoin()
{
   DUSTLOG_INFO(APM_IO_LOGGER, "Join disable");
   setJoinProcessState_p(false);
}

void CAPMTransport::enableJoin(uint32_t netId)
{
   DUSTLOG_INFO(APM_IO_LOGGER, "Join enable");
   m_netId = netId;
   if (setJoinProcessState_p(true)) {
      startJoin_p();
   }

}

void CAPMTransport::handleAPLost() 
{ 
   sendAPLostNotif_p();
}

void CAPMTransport::handleAPBoot()
{
   DUSTLOG_INFO(APM_IO_LOGGER, "AP " << (m_isRunning ? "Reboot" : "Boot"));
   clearOutputQueue();  // Restart APM transport
   if (!m_isRunning) {
      m_notifHandler->handleAPBoot();
      if (setAPConnectionState_p(true)) {
         startJoin_p();
      }
   } else {
      m_notifHandler->handleAPReboot();
      m_isRunning = false;
   }
}

void CAPMTransport::handleAPReboot()
{
   DUSTLOG_ERROR(APM_IO_LOGGER, "Unexpected notification 'handleAPReboot' from APM-serializer");
}

bool CAPMTransport::setJoinProcessState_p(bool isConnected)
{
   boost::unique_lock<boost::mutex> lock(m_stateLock);
   m_isMngrConnected = isConnected;
   return m_isMngrConnected && m_isApConnected;
}

bool CAPMTransport::setAPConnectionState_p(bool isConnected)
{
   boost::unique_lock<boost::mutex> lock(m_stateLock);
   m_isApConnected = isConnected;
   return m_isMngrConnected && m_isApConnected;
}

void  CAPMTransport::startJoin_p()
{
   DUSTLOG_INFO(APM_IO_LOGGER, "Start Join process");
   uint8_t paramsId[] = {DN_API_PARAM_MACADDR, DN_API_PARAM_AP_CLKSRC};
   for (uint8_t p : paramsId) {
      insertMsg(DN_API_LOC_CMD_GETPARAM, &p, sizeof(p));      
   }

   // Set Network ID
   dn_api_set_netid_t param;
   param.paramId = DN_API_PARAM_NETID;
   param.netId  = htons((uint16_t)m_netId);
   insertMsg(DN_API_LOC_CMD_SETPARAM, (const uint8_t *)&param, sizeof(param));     

   // Start Join
   insertMsg(DN_API_LOC_CMD_JOIN, NULL, 0);
   m_isRunning = true;
}

void CAPMTransport::sendAPLostNotif_p()
{
   DUSTLOG_INFO(APM_IO_LOGGER, "AP Lost");
   m_notifHandler->handleAPLost();
   setAPConnectionState_p(false);
   m_isRunning = false;
}

void CAPMTransport::startPPSTimer()
{
   m_ppsTimer.expires_from_now(boost::posix_time::milliseconds(1000));
   m_ppsTimer.async_wait(boost::bind(&CAPMTransport::handlePPSTimeout, 
                                      this, boost::asio::placeholders::error));
}

void CAPMTransport::handlePPSTimeout(const boost::system::error_code& error)
{
   m_stats.m_packetRate = m_stats.m_numPktsSent - last_numPktsSent;
   last_numPktsSent = m_stats.m_numPktsSent;

   m_stats.m_totalSecs ++;
   // calculate last 30 sec average packet rate
   if (m_stats.m_totalSecs % 30 == 0) {
      m_stats.m_30secPacketRate = (m_stats.m_numPktsSent - numPktSent_30sec_ago) / 30.0;
	  numPktSent_30sec_ago = m_stats.m_numPktsSent;
   }

   // calculate last 5 min average packet rate
   if (m_stats.m_totalSecs % 300 == 0) {
      m_stats.m_5minPacketRate = (m_stats.m_numPktsSent - numPktSent_5min_ago) / 300.0;
	  numPktSent_5min_ago = m_stats.m_numPktsSent;
   }

   startPPSTimer();
}

