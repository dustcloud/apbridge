#include "APCClient.h"
using namespace std;

CAPCClient::CAPCClient(uint32_t  cacheSize) : m_cache( cacheSize)
{
   m_state = APCCLIENT_STATE_INIT;
   m_intfId = APINTFID_EMPTY;
   m_pConnector = nullptr;    
   m_kaTimeout = m_freeBufTimeout = 0;
   m_pConnector = nullptr;
   m_pInput = nullptr;   
   m_reconnectTimer = nullptr;
   m_disconnectTime = TIME_EMPTY;   

   m_reconnectionDelayMsec = m_disconnectTimeoutMsec = 0;
   m_lastRxSeqNum = 0;
   m_currentGpsState = ap_int_gpslockstat_t::APINTF_GPS_NOLOCK;

   m_netId = 0;
}

CAPCClient::~CAPCClient()
{
   close();
}

// Open client
apc_error_t CAPCClient::open(const open_param_t& param)
{
   // Save parameters
   m_pInput = param.pInput;
   m_logName = param.logName;
   m_intfName = param.intfName;
   m_ioSrvThread.setLogName(param.logName.c_str()); 
   // Start notification thread
   m_notifThread.init(this);
   apc_error_t res = m_notifThread.start();
   
   return res;
}

// Close client
void CAPCClient::close()
{
   stop();                 // Stop all processes
   m_notifThread.stop();   // Stop notification thread
}

// Start client 
apc_error_t CAPCClient::start(const start_param_t& param)
{
   boost::unique_lock<boost::mutex> lock(m_lock);
   apc_error_t        res;
   string             msg;
   CAPCConnector::ptr pAPC = nullptr;

   if (m_pConnector != nullptr)
      return APC_ERR_STATE;

   // Restart IO service
   try {
      m_IOService.reset();  
   } catch(exception& e) {
      DUSTLOG_ERROR(m_logName, "CAPCClient. Reset IO service error: " << e.what());
      return APC_ERR_INIT;
   }

   // Start IO service thread
   res = m_ioSrvThread.startIOSrvThread(&m_IOService);
   if (res != APC_OK)
      return res;

   // Save connection parameters: Host:Port, timeouts....
   ostringstream  os;
   os << param.port; 
   m_port = os.str();
   m_host = param.host;
   m_kaTimeout = param.kaTimeout;
   m_freeBufTimeout = param.freeBufTimeout; 
   m_reconnectionDelayMsec = param.reconnectionDelayMsec;
   m_disconnectTimeoutMsec = param.disconnectTimeoutMsec;
   BOOST_ASSERT(m_disconnectTimeoutMsec == 0 || (m_disconnectTimeoutMsec != 0 && m_reconnectionDelayMsec != 0));

   // Clean internal variable: Session ID, last received packet, cache of packets
   m_state  = APCCLIENT_STATE_INIT;
   m_intfId = APINTFID_EMPTY;
   m_lastRxSeqNum = 0;
   m_cache.clear();

   // Establish IP connection
   pAPC = ipConnect_p(msg);
   if (pAPC == nullptr)
      res = APC_ERR_INIT;

   // Start APCConnector
   if (res == APC_OK) 
      res = pAPC->start();
   
   // Send 'new' (APINTFID_EMPTY) Connect message
   if (res == APC_OK) {
	  apc_msg_net_gpslock_s gpsState = {m_currentGpsState};
      res = pAPC->connect(m_intfId, 0, 0, gpsState, 0);
   }

   if (res != APC_OK) {
      if (pAPC != nullptr)
         pAPC->stop(APC_STOP_CREATE, res, CAPCConnector::STOP_FL_DISCONNECT);
      DUSTLOG_ERROR(m_logName, "CAPCClient. Connection to '" << m_host << ":" << m_port << "' failed. " 
                    << toString(res) << " " << msg);
   }
   DUSTLOG_INFO(m_logName, "CAPCClient. START: " << toString(res));
   return res;
}

// Stop client
void CAPCClient::stop()
{
   DUSTLOG_INFO(m_logName, "CAPCClient. STOP");
   {
      boost::unique_lock<boost::mutex> lock(m_lock);

      // Stop reconnect timer
      stopTimer_p(m_reconnectTimer);
      m_disconnectTime = TIME_EMPTY;
      m_state       = APCCLIENT_STATE_DISCONNECT;

      // Stop connector
      if (m_pConnector) {
         m_pConnector->disconnect();
         m_sigDisconnect.wait_for(lock, sec_t(1)); // Wait finish of stop processing (finish apcDisconnect notification)
      }

      if (m_pConnector != nullptr) {
         DUSTLOG_ERROR(m_logName, "CAPCClient #" << m_intfId <<" Stop error. Can not close connection");
         m_pConnector = nullptr;
      }
   }

   // Stop IO Service
   m_ioSrvThread.stopIOSrvThread();

   // Clean cache
   m_cache.clear();
   DUSTLOG_TRACE(m_logName, "CAPCClient. STOP finished");
}

//[ IAPCClient interface --------------------------------------------------------
// Send data to server
apc_error_t CAPCClient::sendData(const uint8_t * payload, uint32_t payloadLength)
{
   boost::unique_lock<boost::mutex> lock(m_lock);
   return sendData_p(APC_NET_RX, payload, payloadLength);
}

// Send TxDone message
apc_error_t CAPCClient::sendTxDone(const ap_intf_txdone_t& p)
{
   boost::unique_lock<boost::mutex> lock(m_lock);
   apc_msg_net_txdone_s cmdParam;
   cmdParam.txDoneId = p.txDoneId;
   cmdParam.status   = (uint8_t)p.status;
   return sendData_p(APC_NET_TXDONE, (uint8_t *)&cmdParam, sizeof(cmdParam));
}

// Send Time Map message
apc_error_t CAPCClient::sendTimeMap(const ap_time_map_t& p)
{
   boost::unique_lock<boost::mutex> lock(m_lock);
   apc_msg_timemap_s cmdParam;
   cmdParam.utcSeconds      = p.timeSec;   
   cmdParam.utcMicroseconds = p.timeUsec;  
   cmdParam.asn             = p.asn;       
   cmdParam.asnOffset       = p.asnOffset; 
   return sendData_p(APC_TIME_MAP, (uint8_t *)&cmdParam, sizeof(cmdParam));
}

// Send AP Lost message
apc_error_t CAPCClient::sendApLost()
{
   boost::unique_lock<boost::mutex> lock(m_lock);
   return sendData_p(APC_AP_LOST);
}

// Send Resume message
apc_error_t CAPCClient::sendResume()
{
   boost::unique_lock<boost::mutex> lock(m_lock);
   return sendData_p(APC_NET_TX_RESUME);
}

// Send Pause message
apc_error_t CAPCClient::sendPause ()
{
   boost::unique_lock<boost::mutex> lock(m_lock);
   return sendData_p(APC_NET_TX_PAUSE);
}

// Send GPS satellite lock status notification message
apc_error_t CAPCClient::sendGpsLock(const ap_intf_gpslock_t& p)
{
   m_currentGpsState = p.gpsstate;  
   return APC_OK;
}

//]

// Interface IAPCConnectorNotif  -------------------------------------------
// Message received
void CAPCClient::messageReceived(ap_intf_id_t apcId, uint8_t flags, uint32_t mySeq, uint32_t yourSeq, 
                                        apc_msg_type_t type, const uint8_t * pPayload, uint16_t size)
{
   m_cache.confirmedSeqNum(yourSeq);
   if ((flags & APC_HDR_FLAGS_NOTRACK) == 0) 
      m_lastRxSeqNum = mySeq;

   if (m_pInput == NULL)
      return;

   switch(type)  {
   case APC_NET_TX:
      {
         apc_msg_net_tx_s * pCmd = (apc_msg_net_tx_s *)pPayload;
         ap_intf_sendhdr_t  hdr;

         switch(pCmd->priority) {
         case 0:  hdr.priority = APINTF_LOW; break;
         case 1:  hdr.priority = APINTF_MED; break;
         case 2:  hdr.priority = APINTF_HI ; break;
         case 3:  hdr.priority = APINTF_CMD; break;
         default: hdr.priority = APINTF_LOW; break;
         }    
         hdr.isTxDoneRequested = pCmd->txDoneId != APC_NETTX_NOTXDONE;
         hdr.txDoneId = pCmd->txDoneId;            
         hdr.isLinkInfoSpecified = (pCmd->flags & APC_NETTX_FL_USETXINFO) == APC_NETTX_FL_USETXINFO; 
         hdr.frId    = pCmd->frame;
         hdr.slot    = pCmd->slot;              
         hdr.offset  = pCmd->offset;            
         hdr.dst     = pCmd->dst;    
         m_pInput->dataRx(hdr, pPayload+sizeof(apc_msg_net_tx_s), size-sizeof(apc_msg_net_tx_s));
      }
      break;
   case APC_NET_TX_PAUSE  : m_pInput->pause ();  break;
   case APC_NET_TX_RESUME : m_pInput->resume();  break;
   case APC_RESET_AP      : m_pInput->resetAP(); break;
   case APC_DISCONNECT_AP : m_pInput->disconnectAP(); break;
   case APC_GET_TIME      : m_pInput->getTime(); break;
   default: break;
   }
}

// Process Start notification from CAPCConnector
void CAPCClient::apcStarted(CAPCConnector::ptr pAPC) 
{
   boost::unique_lock<boost::mutex> lock(m_lock);
   m_pConnector = pAPC;
}

// Process Connect notification from CAPCConnector
void CAPCClient::apcConnected(CAPCConnector::ptr pAPC, uint32_t ver, uint32_t netId, ap_intf_id_t intfId, 
                           const char * name, uint8_t flags, uint32_t mySeq, uint32_t yourSeq)
{
   bool        isNewConnection = false;
   apc_error_t res = APC_OK;
   {
      boost::unique_lock<boost::mutex> lock(m_lock);

      if (intfId == APINTFID_EMPTY || (m_intfId != APINTFID_EMPTY && intfId != m_intfId)) {
         // Request for disconnection. Close current session 
         pAPC->stop(APC_STOP_RECONNECTION, APC_ERR_PROTOCOL, CAPCConnector::STOP_FL_DISCONNECT);
         DUSTLOG_ERROR(m_logName, "CAPCClient #" << m_intfId << " Manager did not accept connection with old interface id");
         return;
      }

      isNewConnection = (m_intfId == APINTFID_EMPTY);

      // Stop reconnection timers
      stopTimer_p(m_reconnectTimer);
      m_disconnectTime = TIME_EMPTY;
      // Initialize interface ID
      m_intfId = intfId;
      m_netId = netId;

      // Save (if needed) server seq. number
      if ((flags & APC_HDR_FLAGS_NOTRACK) == 0) 
         m_lastRxSeqNum = mySeq;

      // For restoring connection
      if (!isNewConnection) {   
         // Send data from cache
         CAPCCache::apc_cache_pkt_s  pkt;
         m_cache.confirmedSeqNum(yourSeq, true); 
         m_cache.prepForGet();
         while(res == APC_OK && m_cache.getNextPacket(&pkt) != APC_ERR_NOTFOUND) {
            res = m_pConnector->sendData(pkt.m_type, 0, pkt.m_seqNumb, pkt.m_payload.data(), pkt.m_size, NULL, 0);
         }
      }
      if (res == APC_OK)
         m_state  = APCCLIENT_STATE_ONLINE;
   }

   if (res == APC_OK) {
      // Send connect/online notification
      DUSTLOG_INFO(m_logName, "CAPCClient #" << m_intfId << (isNewConnection ? " Connect" : " Online"));
      if (m_pInput) {
         if (isNewConnection)
            m_pInput->connected(name);
         else
            m_pInput->online();
      }
   } else {
      if (res == APC_ERR_PKTSERIALIZATION)   // Fatal error. Close session
         pAPC->stop(APC_STOP_RECONNECTION, res, CAPCConnector::STOP_FL_DISCONNECT);
      else if (res != APC_OK)                // Error. Go to offline
         pAPC->stop(APC_STOP_RECONNECTION, res, CAPCConnector::STOP_FL_OFFLINE);
      DUSTLOG_ERROR(m_logName, "CAPCClient #" << m_intfId << " Cache output error: " << toString(res));
   }

}

// Process Disconnection notification from CAPCConnector
void CAPCClient::apcDisconnected(CAPCConnector::ptr pAPC, CAPCConnector::stopflags_t flags,
                              apc_stop_reason_t reason, uint32_t maxAllocOutPkt)
{
   bool isImmediately  = false;
   bool isSendNotif    = false;
   {
      boost::unique_lock<boost::mutex> lock(m_lock);

      if (pAPC != m_pConnector) 
         DUSTLOG_ERROR(m_logName, "CAPCClient #" << m_intfId << " 'apcDisconnect' APC pointers is not equal.");
      m_pConnector = nullptr;

      // Disconnect if ...
      if (flags == CAPCConnector::STOP_FL_DISCONNECT || // Disconnect explicitly required 
          m_intfId == APINTFID_EMPTY ||                 // Client never receive 'connect' from manager 
          m_disconnectTimeoutMsec == 0)                           // Reconnection is disable 
         isImmediately = true;


      // Send notification if ...
      isSendNotif = isImmediately ||            // Connection is closed 
                    m_state != APCCLIENT_STATE_OFFLINE;  // Goes to offline 

      m_state  = isImmediately ? APCCLIENT_STATE_DISCONNECT : APCCLIENT_STATE_OFFLINE;

      stopTimer_p(m_reconnectTimer);   // Kill old reconnection timer
      if (isImmediately) {
         m_disconnectTime = TIME_EMPTY;
      } else {
         // Client is offline. Try reconnect
         if (m_disconnectTime == TIME_EMPTY) 
            m_disconnectTime = TIME_NOW() + msec_t(m_disconnectTimeoutMsec);
         // Start reconnection timer
         startTimer_p();
      }

      // Send signal to continue 'stop' process
      m_sigDisconnect.notify_all();
   }      
   DUSTLOG_INFO(m_logName, "CAPCClient #" << m_intfId << (isImmediately ? " Disconnect" : " Offline")
               << " Reason:" << toString(reason));

   // Send disconnected/offline notification
   if (isSendNotif  && m_pInput) {
      if (isImmediately)
         m_pInput->disconnected(reason);
      else
         m_pInput->offline(reason);   
   }
}

//[ Timer Callback functions --------------------------------------------------
// Callback function
void CAPCClient::reconnectTimerFun_p(const boost::system::error_code& error) 
{
   if (error) // Timer is canceled. Nothing do
      return;
   bool isSendNotif = false;
   {
      boost::unique_lock<boost::mutex> lock(m_lock);
      if (m_state == APCCLIENT_STATE_DISCONNECT)
         return;
      if (TIME_NOW() < m_disconnectTime)
         reconnect_p();
      else
         isSendNotif = disconnect_p();
   }
   if (isSendNotif && m_pInput) 
      m_pInput->disconnected(APC_STOP_TIMEOUT);
}

// Try reconnect to server
void CAPCClient::reconnect_p() 
{
   string      msg;
   apc_error_t res;
   
   if (m_pConnector != nullptr) 
      return; // Ignore. Previous connection is not finished

   // Try to open IP connection
   CAPCConnector::ptr pAPC = ipConnect_p(msg);
   if (pAPC == nullptr) { 
      // Can not open IP connection. Restart timer
      startTimer_p();
      DUSTLOG_ERROR(m_logName, "CAPCClient. Re-connection to '" << m_host << ":" << m_port << "' failed. " << msg);
   } else {
      m_reconnectTimer = nullptr;   // Doesn't restart timer
      // Start CAPCConnector
      res = pAPC->start();
      // Send 'connect' message with current session ID
      if (res == APC_OK) {
		 apc_msg_net_gpslock_s gpsState = {m_currentGpsState};
         res = pAPC->connect(m_intfId, m_cache.getLastSent(), m_lastRxSeqNum, gpsState, 0);
	  }
      if (res != APC_OK) 
         pAPC->stop(APC_STOP_CREATE, res, CAPCConnector::STOP_FL_OFFLINE); 
   }
}

// Start disconnect process
bool CAPCClient::disconnect_p() 
{
   // Close connection
   if (m_pConnector)  {
      m_pConnector->stop(APC_STOP_RECONNECTION, APC_OK, CAPCConnector::STOP_FL_DISCONNECT);
      return false;
   }
   m_state = APCCLIENT_STATE_DISCONNECT;
   stopTimer_p(m_reconnectTimer); 
   m_disconnectTime = TIME_EMPTY;
   m_sigDisconnect.notify_all();
   DUSTLOG_INFO(m_logName,  "CAPCClient #" << m_intfId << " Disconnect by timer" );
   return true;
}
// ] 

// Send data to server
apc_error_t CAPCClient::sendData_p(apc_msg_type_t  type, const uint8_t * payload1, uint16_t size1, 
                                   const uint8_t * payload2, uint16_t size2)
{
   apc_error_t res;
   uint32_t    seqNum;
   
   if (m_state == APCCLIENT_STATE_DISCONNECT)
      return APC_ERR_DISCONNECT;
   if (m_state == APCCLIENT_STATE_OFFLINE)
      return APC_ERR_OFFLINE;
   if (m_state != APCCLIENT_STATE_ONLINE)
      return APC_ERR_NOTCONNECT;
   if (m_pConnector == nullptr)
      return APC_ERR_STATE;

   // Save data in cache
   res = m_cache.addPacket(type, payload1, size1, payload2, size2, &seqNum);
   if (res == APC_ERR_OUTBUFOVERFLOW) 
      DUSTLOG_WARN(m_logName, "CAPCClient #" << m_intfId << "Cache overflow");
   
   // Send data
   res = m_pConnector->sendData(type, 0, seqNum, payload1, size1, payload2, size2);
   if (res == APC_ERR_PKTSERIALIZATION)   // Fatal error. Close session
      m_pConnector->stop(APC_STOP_WRITE, res, CAPCConnector::STOP_FL_DISCONNECT);
   else if (res != APC_OK)                // Error. Go to offline
      m_pConnector->stop(APC_STOP_WRITE, res, CAPCConnector::STOP_FL_OFFLINE);
   return res;
}

// Open IP connection
CAPCConnector::ptr CAPCClient::ipConnect_p(string& errMsg)
{
   CAPCConnector::ptr pAPC = nullptr;

   CAPCConnector::init_param_t connectorParam = {
      m_intfName, &m_IOService, &m_notifThread, m_kaTimeout, 
      m_freeBufTimeout, (uint32_t)(m_cache.getCacheSize() * 0.75), m_logName 
   };

   pAPC = CAPCConnector::createConnection(connectorParam);

   try {
      boost::asio::ip::tcp::resolver           resolver(m_IOService);
      boost::asio::ip::tcp::resolver::query    query(m_host.c_str(), m_port.c_str());
      boost::asio::ip::tcp::resolver::iterator endpoint_iterator = resolver.resolve(query);
      boost::asio::connect(pAPC->getSocket(), endpoint_iterator);
   } catch (exception& e) {
      errMsg = e.what();
      pAPC = nullptr;
   }
   
   return pAPC;
}

// Start reconnection timer
apc_error_t CAPCClient::startTimer_p()
{
   if (m_reconnectTimer == nullptr)
      m_reconnectTimer = tmrptr_t(new boost::asio::deadline_timer(m_IOService));

   try {
      m_reconnectTimer->expires_from_now(boost::posix_time::milliseconds(m_reconnectionDelayMsec));
      m_reconnectTimer->async_wait(boost::bind(&CAPCClient::reconnectTimerFun_p, this, boost::asio::placeholders::error));
   } catch(exception& e) {
      DUSTLOG_ERROR(m_logName, "CAPCClient #" << m_intfId << "Start Reconnect Timer error: " << e.what());
      return APC_ERR_ASYNC_OPERATION;
   }
   return APC_OK;
}

// Stop and destroy timer
void CAPCClient::stopTimer_p(tmrptr_t& timer)
{
   if (timer == nullptr) 
      return;
   try {
      timer->cancel();
   } catch(...){;}
   timer= nullptr;
}

bool  CAPCClient::isConnected()
{
   if (m_pConnector)
      return m_pConnector->isWorking();
   return false;
}


