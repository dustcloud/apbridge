#pragma once

#include "APInterface/public/IAPCClient.h"
#include "APCConnector.h"
#include "APCCache.h"
#include "APCCntrlNotifThread.h"
#include "IOSrvThread.h"

#include <boost/thread.hpp>

class CAPCClient : public IAPCClient, IAPCConnectorNotif
{
public:

   CAPCClient(uint32_t  cacheSize);
   ~CAPCClient();
   
   //[ IAPCClient interface --------------------------------------------------------
   virtual apc_error_t open(const open_param_t& param);
   virtual void        close();
   virtual apc_error_t start(const start_param_t& param);
   virtual void        stop();
   virtual apcclient_state_t getState() const { return m_state; }
   
   virtual apc_error_t sendData(const uint8_t * payload, uint32_t payloadLength);
   virtual apc_error_t sendTxDone(const ap_intf_txdone_t& p);
   virtual apc_error_t sendTimeMap(const ap_time_map_t& p);
   virtual apc_error_t sendApLost();
   virtual apc_error_t sendResume();
   virtual apc_error_t sendPause ();
   virtual apc_error_t sendGpsLock(const ap_intf_gpslock_t& p);
   virtual bool        isConnected();

   virtual size_t getCachedPkts() { return m_cache.getNumCachedPkts(); }
   virtual statdelays_s getAPCCStats() const { return m_pConnector->getAPCCStatistics().m_sendStat; }
   virtual uint32_t getNumRcvPkt() const {return m_pConnector->getAPCCStatistics().m_numRcvPkt; }

   virtual void clearStats() { m_pConnector->clearStats(); }

   virtual uint32_t getNetId() const { return m_netId; }
private:

   friend class      CAPCConnector;
   typedef boost::shared_ptr<boost::asio::deadline_timer> tmrptr_t;
   boost::mutex                    m_lock;            // Main lock
   apcclient_state_t               m_state;
   boost::condition_variable       m_sigDisconnect;   // Signal after processing apcDisconnect notification
   ap_intf_id_t                    m_intfId;          // Interface ID (session ID)
   CAPCConnector::ptr              m_pConnector;      // Connector to server
   CAPCCtrlNotifThread             m_notifThread;     // Thread for Connector notification
   CAPCCache                       m_cache;           // Cache of output packets
   uint32_t                        m_lastRxSeqNum;    // Last received seq.number
   std::string                     m_host;            // Server host name
   std::string                     m_port;            // Server port
   uint32_t                        m_kaTimeout;       // Connector: Keep Alive timeout
   uint32_t                        m_freeBufTimeout;  // Connector: Max time to wait for free buffer
   std::string                     m_intfName;        // Name of Client Connector
   IAPCClientNotif               * m_pInput;          // IAPCClientNotif interface
   std::string                     m_logName;         // Logger name
   boost::asio::io_service         m_IOService;       // Boost IO service
   CIOSrvThread                    m_ioSrvThread;     // Thread that runs IO service
   tmrptr_t                        m_reconnectTimer;  // Timer for reconnection
   mngr_time_t                     m_disconnectTime;  // Time when offline connection will disconnect
   // Reconnection
   uint32_t                        m_reconnectionDelayMsec; // Timeout between reconnection attempts
   uint32_t                        m_disconnectTimeoutMsec; // Max time before disconnecting when offline
   ap_int_gpslockstat_t            m_currentGpsState; // Current gps state (0 = no lock, 1 = lock)
   uint32_t                        m_netId;           // Network ID

   // Open IP connection with server
   CAPCConnector::ptr  ipConnect_p(std::string& errMsg);
   // Start / stop timer
   apc_error_t         startTimer_p();
   void                stopTimer_p(tmrptr_t& timer);
   // Send data
   apc_error_t         sendData_p(apc_msg_type_t  type, const uint8_t * payload1 = NULL, uint16_t size = 0, 
                                  const uint8_t * payload2 = NULL, uint16_t size2 = 0);

   // Callback function for timers. 
   // Depends from expire time try reconnect to server or disconnect
   void                reconnectTimerFun_p(const boost::system::error_code& error);
   // Reconnect to server
   void                reconnect_p();
   // Start disconnect process
   // Return true if disconnect notification should be send
   bool                disconnect_p();

   //[ IAPCConnectorNotif interface ------------------------------------------------------------------
   virtual void apcStarted(CAPCConnector::ptr pAPC);
   virtual void apcConnected(CAPCConnector::ptr pAPC, param_connected_s& param);
   virtual void apcDisconnected(CAPCConnector::ptr pAPC, param_disconnected_s& param);
   virtual void messageReceived(param_received_s& param, const uint8_t * pPayload, uint16_t size);

};

