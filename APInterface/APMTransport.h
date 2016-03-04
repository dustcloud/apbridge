/*
 * Copyright (c) 2014, Linear Technology. All rights reserved.
 */

#pragma once
#include "Logger.h"

#include "6lowpan/public/dn_api_common.h"
#include "6lowpan/public/dn_api_local.h"

#include "public/APCError.h"
#include "APMSerializer.h"
#include "SerialPort.h" // TODO: for IInputHandler, IAPMMsgHandler
#include "boost/circular_buffer.hpp"
#include "boost/asio.hpp"
#include "boost/bind.hpp"
#include "boost/thread/mutex.hpp"
#include "boost/date_time/posix_time/posix_time.hpp"

#include <algorithm>
#include <sstream>
#include <boost/atomic.hpp>

#include "IAPCoupler.h"


typedef boost::chrono::steady_clock::time_point mngr_time_t;
typedef boost::chrono::steady_clock::duration mngr_duration_t;

// TODO: is the AP serial header already defined somewhere common?
struct apt_hdr_s {
   uint8_t  cmdId;
   uint8_t  length; // TODO: uint16_t
   uint8_t  flags;
   static const size_t LENGTH = 3;

   apt_hdr_s(uint8_t id, uint16_t l, uint8_t f)
      : cmdId(id), length((uint8_t)l), flags(f) {
      ;
   }

   int serialize(uint8_t* output, size_t len);
   int parse(const uint8_t* input, size_t len);
};

enum apm_flow_control_t
{
   APM_FLOW_NORMAL,
   APM_FLOW_PAUSE,
};

/**
 * Manage reliable transport protocol with AP
 *
 * The transport layer requires:
 * - a send interface to the lower layer,
 * - a callback for the upper layer, and
 * - a timer mechanism for retries.
 *
 * Acks are dependent on upper layer state, so
 */
class CAPMTransport : public IAPMMsgHandler, public IAPMNotifHandler 
{
   struct APMCommand {
      uint8_t cmdId;
      std::vector<uint8_t>  payload;
      bool                  isSynch;
      ResponseCallback      resCallback;
      ErrorResponseCallback errResCallback;
      boost::chrono::steady_clock::time_point timestamp;

      APMCommand(uint8_t aCmdId, const uint8_t* aData, size_t aSize, bool aIsSynch,
                 ResponseCallback aResCallback, ErrorResponseCallback aErrResCallback,
                 boost::chrono::steady_clock::time_point aTimestamp)
         : cmdId(aCmdId),
           payload(aData, aData+aSize),
           isSynch(aIsSynch),
           resCallback(aResCallback),
           errResCallback(aErrResCallback),
           timestamp(aTimestamp)
      { ; }
   };

public:
   /// APMTransport initialization parameters
   struct init_param_t
   {
      init_param_t();
      init_param_t(uint16_t aQueueSize,
                   uint16_t aHighWatermark,
                   uint16_t aLowWatermark,
                   uint32_t aPingTimeout,
                   uint32_t aNackDelay,
                   uint32_t aRetryTimeout,
                   uint16_t aMaxRetries,
                   uint16_t aMsgSize,
                   uint32_t aMaxPacketAge);
      
      uint16_t maxQueueSize;       ///< Maximum queue size for messages to AP
      uint16_t highQueueWatermark; ///< Flow control: stop accepting messages to the AP
      uint16_t lowQueueWatermark;  ///< Flow control: resume sending messages to the AP
      uint32_t pingTimeout;	       ///< Keep-alive interval to the AP
      uint32_t retryDelay;         ///< Retry delay after receiving a NACK (milliseconds)
      uint32_t retryTimeout;       ///< Retry timeout (milliseconds)
      uint16_t maxRetries;         ///< Maximum number of retries
      uint16_t maxMsgSize;         ///< Maximum message size (bytes)
      uint16_t maxPacketAge;       ///< Maximum packet age to trigger TX_PAUSE to manager
   };

   CAPMTransport(size_t maxMsgSize, boost::asio::io_service& io_service,
                 IInputHandler* outputHandler, IAPMNotifHandler* notifHandler,
                 bool bReconnectSerial);

   virtual ~CAPMTransport() {  delete m_cmdHandler; }

   void        open(const init_param_t& param);
   apc_error_t stop();

   // handle data from serial port

   virtual apc_error_t handleMsg(apt_hdr_s* pHdr, const uint8_t* data, size_t size);

   // add outgoing (to AP) message to queue
   virtual apc_error_t insertMsg(uint8_t cmdId, const uint8_t* data, size_t size,
                                 bool isHighPriority = false,
                                 ResponseCallback resCallback = NULL,
                                 ErrorResponseCallback errRespCallback = NULL,
                                 bool isSynch = false);

   virtual apc_error_t dataReceived(const uint8_t* data, size_t size);

   // IAPMNotifHandler interface
   virtual void handleAPReceive(const uint8_t* data, size_t length)                  { m_notifHandler->handleAPReceive(data, length)   ;}
   virtual void handleEvent(const dn_api_loc_notif_events_t& event)                  { m_notifHandler->handleEvent(event)              ;}
   virtual void handleTimeIndication(const dn_api_loc_notif_time_t& timeMap)         { m_notifHandler->handleTimeIndication(timeMap)   ;}
   virtual void handleReadyForTime(const dn_api_loc_notif_ready_for_time_t& ready)   { m_notifHandler->handleReadyForTime(ready)       ;}
   virtual void handleTXDone(const dn_api_loc_notif_txdone_t& txDone)                { m_notifHandler->handleTXDone(txDone)            ;}
   virtual void handleParamMacAddress(const dn_api_rsp_get_macaddr_t& getParam)      { m_notifHandler->handleParamMacAddress(getParam) ;}
   virtual void handleParamMoteInfo(const dn_api_rsp_get_moteinfo_t& getMoteInfo)    { m_notifHandler->handleParamMoteInfo(getMoteInfo);}
   virtual void handleParamAppInfo(const dn_api_rsp_get_appinfo_t& getAppInfo)       { m_notifHandler->handleParamAppInfo(getAppInfo);}
   virtual void handleParamClkSrc(const dn_api_rsp_get_ap_clksrc_t& getClkSrc)       { m_notifHandler->handleParamClkSrc(getClkSrc)    ;}
   virtual void handleParamNetId(const dn_api_rsp_get_netid_t& getNetId)             { m_notifHandler->handleParamNetId(getNetId)      ;}
   virtual void handleParamGetTime(const dn_api_rsp_get_time_t& getTime)             { m_notifHandler->handleParamGetTime(getTime)     ;}
   virtual void handleParamApStatus(const dn_api_rsp_get_apstatus_t& getApStatus)    { m_notifHandler->handleParamApStatus(getApStatus);}
   virtual void handleAPLost();
   virtual void handleAPPause()                                                      { m_notifHandler->handleAPPause()                 ;}
   virtual void handleAPResume()                                                     { m_notifHandler->handleAPResume()                ;}
   virtual void handleAPBoot();
   virtual void handleAPReboot();
   virtual void handleError(uint8_t cmdId, uint8_t rc)                               { m_notifHandler->handleError(cmdId, rc); }

   // Output timer management
   void startRetryTimer();
   void stopRetryTimer();
   void handleRetryTimeout(const boost::system::error_code& error);

   // Packet age timer management
   void sendMgrResume();
   void startQueueCheckTimer();
   void stopQueueCheckTimer();
   void handleQueueCheckTimeout(const boost::system::error_code& error);

   // Output timer for NACK from AP
   void startNackRetryTimer();
   void handleNackRetryTimeout(const boost::system::error_code& error);
   
   // Ping timer management
   void startPingTimer();
   void stopPingTimer();
   void handlePingTimeout(const boost::system::error_code& error);

   // Packet Per Second timer management
   void startPPSTimer();
   void handlePPSTimeout(const boost::system::error_code& error);

   size_t queueSize();

   // send the command in the front of the output queue
   void send(bool isNew);

   bool isPortReady();
   void hwResetAP();

   // Enable / disable Join process
   void disableJoin();
   void enableJoin(uint32_t netId);

   // Send Disconnect command to AP
   void disconnectAP(bool isSinchBit = false);

   // Transport statistics
   struct APMTStats {
      uint64_t         m_minRspTime;      // Minimum Response Time of AP ( usec )
      uint64_t         m_maxRspTime;      // Maximum Response Time of AP ( usec )
      mngr_duration_t  m_lastRspTime;     // Last Command Response Time (APMTransport->AP)
      uint32_t         m_numRespRecv;     // Number of Responses Received
      uint64_t         m_totalRspTime;    // Sum of Response Time for all packets ( usec )
      uint32_t         m_numPktsSent;     // Number of packets sent (APMTransport->AP)
      uint32_t         m_numBytesSent;    // Number of bytes sent (APMTransport->AP)
      uint32_t         m_numPktsRecv;     // Number of packets received (AP->APMTransport)
      uint32_t         m_numBytesRecv;    // Number of bytes received (AP->APMTransport)
      uint32_t         m_numNacks;        // Number of NACKs received from AP
      uint32_t         m_numRetriesSent;  // Number of Retries/TimeOuts sent to AP
      uint32_t         m_numRetriesRecv;  // Number of Retries received from AP
      uint32_t         m_maxNackCount;    // Maximum number of NACKs received from AP in a row
      uint64_t         m_minTimeInQueue;  // Minimum time packet spent in queue
      uint64_t         m_maxTimeInQueue;  // Maximum time packet spent in queue
      uint64_t         m_totalTimeInQueue;// Sum of time in queue for all packets (usec)
      uint32_t         m_numPacketsQueued;// Number of Packets ever pushed into queue
      mngr_duration_t  m_timeInQueue;     // time the current packet spent in queue
      uint32_t         m_packetRate;      // record the current packet per second, TX and RX have same rate
      double           m_30secPacketRate; // record the last 30 seconds packet rate
      double           m_5minPacketRate;  // record the last 5 minutes packet rate
      uint32_t         m_totalSecs;       // record the total seconds, needed to calculate avg pkt rate
      APMTStats() {
         reset();
      };

      // Reset all statistics
      void reset() {
         m_minRspTime     = 10000000; // Initialized with big number so first response time will be always smaller
         m_maxRspTime     = 0;
         m_lastRspTime    = mngr_duration_t::zero();
         m_numRespRecv    = 0;
         m_totalRspTime   = 0;
         m_numPktsSent    = 0;
         m_numBytesSent   = 0;
         m_numPktsRecv    = 0;
         m_numBytesRecv   = 0;
         m_numNacks       = 0;
         m_numRetriesSent = 0;
         m_numRetriesRecv = 0;
         m_maxNackCount   = 0;
         m_minTimeInQueue = 10000000; // Initialized with big number so first response time will be always smaller
         m_maxTimeInQueue = 0;
         m_totalTimeInQueue = 0;
         m_numPacketsQueued = 0;
         m_timeInQueue    = mngr_duration_t::zero();
         m_packetRate = 0;
         m_30secPacketRate = 0;
         m_5minPacketRate = 0;
         m_totalSecs = 0;
      }
   };

   /**
    * Get Transport Statistics
    */
   APMTStats getAPMStatistics() const { return m_stats; }

   /**
    * Clear Transport Statistics
    */
   void clearAPMStatistics() { m_stats.reset(); }

   apc_error_t sendPing();

   void setAPMState(apm_flow_control_t newstate) { m_apInputState = newstate; }

   void clearOutputQueue();

private:
   // generate data to serial port
   apc_error_t sendCommand(uint8_t cmdId, const uint8_t* data, size_t length, bool isSynch,
                           ResponseCallback resCallback, ErrorResponseCallback errRespCallback);
   void sendRetry();
   void sendAck(uint8_t notifId, uint8_t pktId, uint8_t rc = DN_API_RC_OK);
   apc_error_t handleResponse(const apt_hdr_s& hdr, const uint8_t* data,
                              size_t size);
   apc_error_t handleNotification(const apt_hdr_s& hdr, const uint8_t* data,
                                  size_t size);
private:
   boost::mutex m_lock;

   IAPMCmdHandler* m_cmdHandler;     ///< interface for receiving input
   IInputHandler* m_outputHandler;   ///< interface for sending data
   IAPMNotifHandler* m_notifHandler; ///< interface for handling notifications

   init_param_t m_init_params; ///< initialization parameters

   boost::asio::deadline_timer m_outputTimer;
   boost::asio::deadline_timer m_pingTimer;
   boost::asio::deadline_timer m_ppsTimer;

   uint32_t m_curRetryCount;  ///< current send retry count
   uint32_t m_curNackCount;   ///< current Nack count
   uint8_t m_respPacketId;    ///< outgoing (Response) packet id
   uint8_t m_notifPacketId;   ///< incoming (Notification) packet id
   
   // Queue of messages to AP
   boost::atomic<bool> m_pending;
   boost::circular_buffer<APMCommand> m_outputQueue;

   std::string      m_log;

   // Track command sent time from APMTransport->AP to calculate response time
   mngr_time_t m_sendTime;

   APMTStats m_stats; ///< AP transport statistics

   boost::atomic<apm_flow_control_t> m_apInputState;   ///< flow control to AP
   boost::atomic<apm_flow_control_t> m_mngrInputState; ///< flow control to Manager

   boost::mutex     m_stateLock;
   bool             m_isMngrConnected;
   bool             m_isApConnected;
   bool             m_isRunning;
   uint32_t         m_netId;
   bool             m_reconnectSerial;
   boost::asio::deadline_timer m_queueCheckTimer; ///< periodically check queu

   bool            setJoinProcessState_p(bool isConnected);
   bool            setAPConnectionState_p(bool isConnected);
   void            startJoin_p();
   void            sendAPLostNotif_p();
   void            handleDisconnectErrorResponse_p(uint8_t cmdId, uint8_t rc);

};

