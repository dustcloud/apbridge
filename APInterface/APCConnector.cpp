#include "APCConnector.h"
#include <boost/bind.hpp>
#include <boost/thread.hpp>
#include <string>
using namespace std;

#ifdef WIN32
   #pragma warning( disable : 4996 )
#endif

/////////////////////////////////////////////////
//    CAPCKATimer
/////////////////////////////////////////////////
CAPCKATimer::ptr CAPCKATimer::createKATimer(boost::asio::io_service * pIOService, uint32_t timeoutMsec, 
                                            const char * logName)
{
   return ptr(new CAPCKATimer(pIOService, timeoutMsec, logName));
}

CAPCKATimer::CAPCKATimer(boost::asio::io_service * pIOService, uint32_t timeoutMsec, const char * logName) :
   m_kaTimer(*pIOService), m_cbFun(nullptr), m_kaTimeoutMsec(timeoutMsec), m_isWorking(false)
{
   m_log = logName;
}

CAPCKATimer::~CAPCKATimer()
{
}

apc_error_t CAPCKATimer::startTimer(cbfun_t fun)
{
   m_isWorking = true;
   m_cbFun  = fun;
   m_timeLastActivity = boost::chrono::steady_clock::now();
   return startTimer_p();
}

void  CAPCKATimer::stopTimer()
{
   boost::unique_lock<boost::mutex> lock(m_lockWrkFlag);
   m_isWorking = false;
   m_cbFun  = nullptr;
   try {
      m_kaTimer.cancel();
   } catch(exception& e) {
      DUSTLOG_WARN(m_log, "Stop Timer error: " << e.what());
   }
}

void  CAPCKATimer::recordActivity()
{
   boost::unique_lock<boost::mutex> lock(m_lock);
   m_timeLastActivity = boost::chrono::steady_clock::now();
}

void CAPCKATimer::handle_timer_p(const boost::system::error_code& error)
{
   if (error) 
      return;              // Timer is canceled
   cbfun_t cbFun = nullptr;
   {
      boost::unique_lock<boost::mutex> lock(m_lockWrkFlag);
      // Restart working timer
      if (m_isWorking) {
         cbFun = m_cbFun;
         startTimer_p();      
      }
   }
   if (cbFun != nullptr)
      (cbFun)(getTimeLastActivity_p());
}

apc_error_t CAPCKATimer::startTimer_p()
{
   ptr p = shared_from_this();
   try {
      m_kaTimer.expires_from_now(boost::posix_time::milliseconds(m_kaTimeoutMsec));
      m_kaTimer.async_wait(boost::bind(&CAPCKATimer::handle_timer_p, p, boost::asio::placeholders::error));
   } catch(exception& e) {
      DUSTLOG_ERROR(m_log, "Start Timer error: " << e.what());
      return APC_ERR_ASYNC_OPERATION;
   }
   return APC_OK;
}

boost::chrono::steady_clock::time_point CAPCKATimer::getTimeLastActivity_p() 
{
   boost::unique_lock<boost::mutex> lock(m_lock);
   return m_timeLastActivity;
}

/////////////////////////////////////////////////
//    CAPCConnector
/////////////////////////////////////////////////
CAPCConnector::ptr CAPCConnector::createConnection(const CAPCConnector::init_param_t& param)
{
   return ptr(new CAPCConnector(param));
}

CAPCConnector::CAPCConnector(const init_param_t& param) :
   m_apcName(param.apcConnect),
   m_pApcNotif(param.pApcNotif),  
   m_pSerializer(nullptr),
   m_intfId(APINTFID_EMPTY), 
   m_isWorking(false),
   m_isConnected(false),
   m_forceDisconnect(false),
   m_kaTxTimer(nullptr),
   m_kaRxTimer(nullptr),
   m_unconfirmedInpPkt(param.unconfirmedInpPkt),
   m_txTimeout(boost::chrono::milliseconds(param.kaTimeout / 4)),
   m_rxTimeout(boost::chrono::milliseconds(param.kaTimeout * 2)),
   m_numFreeOutBuf(APC_NUM_OUT_BUFS),
   m_minNumFreeOutBuf(APC_NUM_OUT_BUFS),
   m_lastReceivedSeqNum(0),
   m_lastReportedSeqNum(0),
   m_curFreeBufIdx(0),
   m_stats(),
   m_socket(*param.pIOService),
   m_freeBufWait(boost::chrono::milliseconds(param.freeBufTimeout))
{
   boost::unique_lock<boost::mutex> lock(m_lock);
   BOOST_ASSERT(param.pIOService != NULL);
   m_log = param.logName;
   m_pSerializer = new CAPCSerializer(APC_MAX_MSG_SIZE, this, m_log.c_str());
   // Create Keep Alive Timers
   if (param.kaTimeout > 0) {
      m_kaTxTimer = CAPCKATimer::createKATimer(param.pIOService, param.kaTimeout, param.logName.c_str());
      m_kaRxTimer = CAPCKATimer::createKATimer(param.pIOService, param.kaTimeout, param.logName.c_str());
   }
   //DUSTLOG_DEBUG(m_log, "CAPCConnector (" << (uint32_t)this << ") created");
}

CAPCConnector::~CAPCConnector() 
{
   // Close socket
   if (m_socket.is_open()) {
      try {
         m_socket.close();
      } catch(...) {;}
   }

   delete m_pSerializer;
   //DUSTLOG_DEBUG(m_log, "CAPCConnector #" << m_intfId << " (" << (uint32_t)this << ") deleted " );
}

CAPCConnector::stats_s CAPCConnector::getAPCCStatistics()
{
   stats_s res;
   res.m_faultAllocTime  = m_stats.m_faultAllocTime;
   res.m_numRcvPkt       = m_stats.m_numRcvPkt;
   m_stats.m_sendStat.getStat(&res.m_sendStat);
   return res;
}

bool CAPCConnector::isWorking() 
{ 
   boost::unique_lock<boost::mutex> lock(m_lock);
   return m_isWorking; 
}

//Starts processing receive data and sending KA 
apc_error_t CAPCConnector::start()
{
   boost::unique_lock<boost::mutex> lock(m_lock);
   apc_error_t res = APC_OK;
   ptr p = shared_from_this();

   if (m_pApcNotif)  
      m_pApcNotif->apcStarted(p);

   m_isWorking = true;

   try {
      // TRY block prevent from error if socket is not initialize yet
      DUSTLOG_INFO(m_log, "CAPCConnector #" << m_intfId  << " Start. Port:" << m_socket.local_endpoint().port() 
                  << " from " << m_socket.remote_endpoint().address() << ":" << m_socket.remote_endpoint().port());
   } catch (...) {;}

   //[ ---- Start Keep alive timers
   if (m_kaRxTimer != nullptr)
      res = m_kaRxTimer->startTimer(boost::bind(&CAPCConnector::ka_timeout_rx_p, p, _1));
   if (res != APC_OK)
      return res;
   //]
   
   // Set request for socket read
   res = async_read_p();   
   return res;
}

// Send Connect message.
apc_error_t CAPCConnector::connect(ap_intf_id_t intfId, uint32_t mySeq, uint32_t yourSeq, 
                                   const apc_msg_net_gpslock_s& gpsState, uint32_t netId)
{
   apc_msg_connect_s conMsg;
   m_intfId = intfId;
   m_lastReceivedSeqNum = m_lastReportedSeqNum = yourSeq;
   // Set connection parameters
   conMsg.ver   = APC_PROTO_VER;
   conMsg.flags = 0;
   conMsg.sesId = intfId;
   strncpy(conMsg.identity, m_apcName.c_str(), sizeof(conMsg.identity));
   conMsg.identity[sizeof(conMsg.identity) - 1] = 0;
   conMsg.gpsstate = gpsState;
   conMsg.netId = netId;

   return sendData(APC_CONNECT, 0, mySeq, (const uint8_t *)&conMsg, sizeof(conMsg), NULL, 0);
}

// Terminate connection.
void CAPCConnector::disconnect()
{
   m_forceDisconnect = true;
   sendData(APC_DISCONNECT, APC_HDR_FLAGS_NOTRACK, 0, NULL, 0, NULL, 0);
   stop(APC_STOP_CLOSE, APC_OK, STOP_FL_DISCONNECT, true);
}

// Closes all processes (receiving, KA) and socket
void CAPCConnector::stop(apc_stop_reason_t reason, apc_error_t err, stopflags_t stopFlags, bool isFinishWriting)
{
   boost::unique_lock<boost::mutex> lock(m_lock); //lock after non working 
   ptr p = shared_from_this();

   if (m_isWorking == false) 
      return;
   
   m_isWorking = false;

   if (isFinishWriting)
      // Wait release of all output buffers
      for(uint32_t ii = 0; ii < APC_NUM_OUT_BUFS && getFreeBuf_p(lock, true) != NULL; ii++);

   // Print statistics
   DUSTLOG_INFO(m_log, "CAPCConnector #" << m_intfId << " Stop. Reason: " << toString(reason) << " " << (err != APC_OK ? toString(err) : "")) ;
   DUSTLOG_INFO(m_log, "         Stat #" << m_intfId << " numRX: " << m_stats.m_numRcvPkt << " TX-stat: " << m_stats.m_sendStat);
   if (m_stats.m_faultAllocTime > 0)
      DUSTLOG_INFO(m_log, "         Stat #" << m_intfId << " Unsuccessful allocation time: " << m_stats.m_faultAllocTime);

   // Send signal for unlock 'getFreeBuf_p' function (if it lock)
   m_freeBufSig.notify_all();

   //[ ----- Stop timers
   if (m_kaTxTimer != nullptr) {
      m_kaTxTimer->stopTimer();
      m_kaTxTimer = nullptr;
   }

   if (m_kaRxTimer != nullptr) {
      m_kaRxTimer->stopTimer();
      m_kaRxTimer = nullptr;
   }
   //]
   
   // Close IP socket and any async read/write operations
   if (m_socket.is_open()) {
      try {
         m_socket.close();
      } catch(exception& e) {
         DUSTLOG_ERROR(m_log, "CAPCConnector #" << m_intfId << " Close socket error: " << e.what());
      }
   }

   m_isConnected = false;

   // Send 'apcDisconnect' notification
   if (m_pApcNotif) {
      if (m_forceDisconnect) {
         stopFlags = (stopflags_t)((uint32_t)stopFlags & ~STOP_FL_OFFLINE);
         stopFlags = (stopflags_t)((uint32_t)stopFlags | STOP_FL_DISCONNECT);
      }
      m_pApcNotif->apcDisconnected(p, stopFlags, reason, APC_NUM_OUT_BUFS - m_minNumFreeOutBuf);
   }
}

// Sends a data.
apc_error_t CAPCConnector::sendData(apc_msg_type_t type, uint8_t flags, uint32_t mySeq, 
                                    const uint8_t * payload1, uint16_t size1, 
                                    const uint8_t * payload2, uint16_t size2)
{
   boost::unique_lock<boost::mutex> lock(m_lock);
   size_t    msgSize;
   ptr       p = shared_from_this();
   uint8_t * pBuf = NULL;

   if (m_isWorking == false || (!m_isConnected && (type != APC_CONNECT && type != APC_KA)))
      return APC_ERR_NOTCONNECT;

   mngr_time_t startTime = TIME_NOW();
   //[ ---- Get free buffer
   pBuf = getFreeBuf_p(lock);
   if (pBuf == NULL) {
      m_stats.m_faultAllocTime = (uint32_t)(TO_USEC(TIME_NOW() - startTime).count());
      return APC_ERR_OUTBUFOVERFLOW;
   }
   //]
   
   //iobuf_t outbufs;
   //pBuf = outbufs.data();

   // Prepare packet
   apc_error_t res = m_pSerializer->prepMsg(pBuf, APC_MAX_MSG_SIZE, &msgSize, m_intfId, type, flags,
                                            mySeq, m_lastReceivedSeqNum,
                                            payload1, size1, payload2, size2);
   if (res != APC_OK) {
      // Serialization error. Free buffer
      returnBuf_p();
      return APC_ERR_PKTSERIALIZATION;
   }

   DUSTLOG_TRACEDATA(m_log, string("TX #") + to_string(m_intfId), pBuf, msgSize);
   try {
      // Send data. 
      boost::asio::async_write(m_socket, boost::asio::buffer(pBuf, msgSize), 
                  boost::bind(&CAPCConnector::handle_write_p, p,
                              boost::asio::placeholders::error,
                              boost::asio::placeholders::bytes_transferred));
      //boost::asio::write(m_socket, boost::asio::buffer(pBuf, msgSize));

      //[ ----- Statistics calculation
      m_stats.m_sendStat.addEvent(startTime);
	   DUSTLOG_TRACE(m_log, "CAPCConnector #" << m_intfId << " sendData counter #" << m_stats.m_sendStat.getNumEvents() << ", type " << type);
      //]
   } catch (exception& e) {
      DUSTLOG_ERROR(m_log, "CAPCConnector #" << m_intfId << " async_write error: " << e.what());
      return APC_ERR_ASYNC_OPERATION;
   }
   m_lastReportedSeqNum = m_lastReceivedSeqNum;
   return APC_OK;
}

// Get connector property
void CAPCConnector::getProperty(property_t * pProperty)
{
   if (pProperty == NULL)
      return;
   pProperty->isConnected  = true;
   pProperty->intfId       = m_intfId;
   pProperty->localName    = m_apcName;
   pProperty->peerName     = m_peerIntfName;
   pProperty->curAllocOutBuf  = APC_NUM_OUT_BUFS - m_numFreeOutBuf;
   pProperty->maxAllocOutBuf  = APC_NUM_OUT_BUFS - m_minNumFreeOutBuf;
   m_stats.m_sendStat.getStat(&pProperty->sendStat);
   pProperty->numReceivedPkt  = m_stats.m_numRcvPkt;
   try {
      auto l = m_socket.local_endpoint(), r = m_socket.remote_endpoint();
      pProperty->localAddress = l.address();
      pProperty->localPort    = l.port();
      pProperty->peerAddress  = r.address();
      pProperty->peerPort     = r.port();
   }  catch(...) {;}
}

// Callback for Keep Alive timer of INPUT packets
bool CAPCConnector::ka_timeout_rx_p(const boost::chrono::steady_clock::time_point& lastAction)
{
   // If after last input packet > max - stop connection and go to state 'Offline'
   if (boost::chrono::steady_clock::now() - lastAction >= m_rxTimeout) {
      stop(APC_STOP_TIMEOUT, APC_OK, STOP_FL_OFFLINE, false);
      return false;
   } 
   return true;
}

// Callback for Keep Alive timer of OUTPUT packets
bool CAPCConnector::ka_timeout_tx_p(const boost::chrono::steady_clock::time_point& lastAction)
{
   if (boost::chrono::steady_clock::now() - lastAction >= m_txTimeout) {
      apc_error_t res = send_ka_p();
      if (res != APC_OK) {
         stop(APC_STOP_WRITE, res, STOP_FL_OFFLINE, false);
         return false;
      }
   }
   return true;
}

// Callback for finish of write operation
void CAPCConnector::handle_write_p(const boost::system::error_code& error, size_t len)
{
   if (error) {
      freeBuf_p();   
   } else {
      boost::unique_lock<boost::mutex> lock(m_lock);
      // Refresh activity of Output Keep Alive timer
      if (m_kaTxTimer != nullptr)
         m_kaTxTimer->recordActivity();
      // Free output buffer
      freeBuf_p();   
   }
}

// Callback for read operation
void CAPCConnector::handle_read_p (const boost::system::error_code& error, size_t len)
{
   apc_error_t res;

   if (error) {
      DUSTLOG_ERROR(m_log, "CAPCConnector #" << m_intfId << " Read error: " << error.message().c_str());
      stop(APC_STOP_READ, APC_OK, STOP_FL_OFFLINE, false);
      return;
   } 

   if (len > 0) {
      DUSTLOG_TRACEDATA(m_log, string("RX #") + to_string(m_intfId), m_inpbuf.data(), len);

      // Read data
      res = m_pSerializer->dataReceived(m_intfId, m_inpbuf.data(), len);
      if (res != APC_OK) {
         if (res == APC_STOP_CONNECTOR) {
            stop(APC_DISCONNECT_MSG, res, STOP_FL_DISCONNECT, false); 
         } else {
            stop(APC_STOP_PKTPARSE, res, STOP_FL_OFFLINE, false);  // TODO STOP_FL_DISCONNECT ???
         }
         return;
      }

      {  
         boost::unique_lock<boost::mutex> lock(m_lock); // Lock to schedule next read operation
         if(!m_isWorking)  {
            return;
         }
         // Refresh activity of Input Keep Alive timer
         if (m_kaRxTimer != nullptr) 
            m_kaRxTimer->recordActivity();
      }     
   }

   res = async_read_p();   
   // If number of last received packet is not reported long time then generate Keep Alive
   if (res == APC_OK && m_lastReceivedSeqNum - m_lastReportedSeqNum > m_unconfirmedInpPkt)
      res = send_ka_p();

   if (res != APC_OK) 
      stop(APC_STOP_READ, res, STOP_FL_OFFLINE, false);

}

// Start asynch reading
apc_error_t CAPCConnector::async_read_p() {
   ptr p = shared_from_this();
   try {
      m_socket.async_read_some(boost::asio::buffer(m_inpbuf),  
         boost::bind(&CAPCConnector::handle_read_p, p,
                     boost::asio::placeholders::error,
                     boost::asio::placeholders::bytes_transferred));
   } catch(std::exception e) {
      DUSTLOG_ERROR(m_log, "CAPCConnector #" << m_intfId << " async_read error: " << e.what());
      return APC_ERR_ASYNC_OPERATION;
   }
   return APC_OK;
}

// Send Keep alive (by timer or number of received packets)
apc_error_t CAPCConnector::send_ka_p()
{
   return sendData(APC_KA, APC_HDR_FLAGS_NOTRACK, 0, NULL, 0, NULL, 0);
}

void CAPCConnector::incrNumFreeBuf_p() 
{
   if (m_numFreeOutBuf < APC_NUM_OUT_BUFS)
      m_numFreeOutBuf++;
   else
      DUSTLOG_WARN(m_log, "CAPCConnector #" << m_intfId << " Unexpected number free buffers");
}

// Get free buffer
uint8_t *  CAPCConnector::getFreeBuf_p(boost::unique_lock<boost::mutex>& lock, bool allowIfNotWorking)
{
   mngr_time_t startTime = TIME_NOW();
   while (m_numFreeOutBuf == 0 && (m_isWorking || allowIfNotWorking)) {
      // Wait free buffer up to m_freeBufWait time
      m_freeBufSig.wait_for(lock, m_freeBufWait);
      if (TO_USEC(TIME_NOW() - startTime) > m_freeBufWait) 
         break;
   }

   // Error if time expire or APCConnector closing
   if (m_numFreeOutBuf == 0 || (!m_isWorking && !allowIfNotWorking)) {
      return NULL;
   }

   // Get free buffer and change index and number of free buffers
   uint8_t * pBuf = m_outbufs[m_curFreeBufIdx].data();
   if (--m_numFreeOutBuf < m_minNumFreeOutBuf)
      m_minNumFreeOutBuf = m_numFreeOutBuf;
   ++m_curFreeBufIdx &= APC_NUM_OUT_BUFS_MASK;
   return pBuf;
}

// Free buffer
void CAPCConnector::freeBuf_p()
{
   incrNumFreeBuf_p();
   m_freeBufSig.notify_all();    // Send free buffer signal
}

// Return buffer to buffer pull
void CAPCConnector::returnBuf_p()
{
   incrNumFreeBuf_p();
   if (m_curFreeBufIdx == 0) m_curFreeBufIdx = APC_NUM_OUT_BUFS;
   --m_curFreeBufIdx;
}

// Process parsed input packet (called by m_pSerializer->dataReceived)
apc_error_t CAPCConnector::messageReceived(ap_intf_id_t apcId, apc_msg_type_t type, uint8_t flags,
                                           uint32_t mySeq, uint32_t yourSeq,
                                           const uint8_t * pPayload, uint16_t size)
{
   apc_error_t res = APC_OK;
   ptr p = shared_from_this();

   // If not connected then allow only 'connect' 
   if (!m_isConnected && type != APC_CONNECT && type != APC_DISCONNECT && type != APC_KA) {
      return APC_ERR_PROTOCOL;
   }

   m_stats.m_numRcvPkt++; 
   DUSTLOG_TRACE(m_log, "CAPCConnector#" << m_intfId  << " messageReceived counter #" << m_stats.m_numRcvPkt << ", type " << type);

   if (type == APC_CONNECT) {
      // Generate apcConnect notification
      m_isConnected = true;
      if (m_pApcNotif) {
         apc_msg_connect_s * pConnect = (apc_msg_connect_s *)pPayload;
         m_peerIntfName = pConnect->identity;
         DUSTLOG_INFO(m_log, "CAPCConnector #" << m_intfId  << " Peer name: '" << m_peerIntfName << "'");
         m_pApcNotif->apcConnected(p, pConnect->ver, pConnect->netId,
            pConnect->sesId,  pConnect->identity, flags, mySeq, yourSeq);
      }
      //[ ---- Start Keep alive timers after receiving CONNECT back from Manager
      if (m_kaTxTimer != nullptr) {
         res = m_kaTxTimer->startTimer(boost::bind(&CAPCConnector::ka_timeout_tx_p, p, _1));
         if (res != APC_OK)
            return res;
      }
   } else if (type == APC_DISCONNECT) {
      // Disconnect
      res = APC_STOP_CONNECTOR;
   } else if (m_pApcNotif) {
      // Generate messageReceive notification
      m_pApcNotif->messageReceived(apcId, flags, mySeq, yourSeq, type, pPayload, size);
   }

   if (mySeq > m_lastReceivedSeqNum)
      m_lastReceivedSeqNum = mySeq;
   else if (mySeq != 0)
      DUSTLOG_WARN(m_log, "CAPCConnector #" << m_intfId << " received packet with wrong sequence number: " << 
                           mySeq << " expected: > " << m_lastReceivedSeqNum);
   return res;
}
