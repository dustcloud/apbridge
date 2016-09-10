/*
 * Copyright (c) 2016, Linear Technology. All rights reserved.
 */

#pragma once
#include "common.h"
#include "StatDelaysCalc.h"
#include "APCProto.h"
#include "public/APCError.h"
#include "APCSerializer.h"
#include "logging/Logger.h"
#include <string>
#include <boost/asio.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/chrono.hpp>
#include <boost/thread.hpp>
#include <boost/array.hpp>
#include "public/IAPCCommon.h"

class IAPCConnectorNotif;
const uint32_t APC_MAX_MSG_SIZE = MAX_NET_PKT_SIZE + MAX_APC_HDR_SIZE;       ///< Max size of APC message
const uint32_t APC_NUM_OUT_BUFS = 2;          ///< Number of output buffers !!! must be power 2 (or 1) !!!
const uint32_t APC_NUM_OUT_BUFS_MASK = APC_NUM_OUT_BUFS - 1;   // Mask .for calculating index of free buffer
const uint32_t APC_CONNECTOR_NAME_LENGTH = 31;                   // Max length of connector name (see 'apcConnected')

// Keep Alive timers for Transmit / Received operation
class CAPCKATimer  : public boost::enable_shared_from_this<CAPCKATimer>
{
public:
   typedef boost::shared_ptr<CAPCKATimer> ptr;

   
   //  Defines an alias representing the timer expire callback function
   //
   //  bool F(bool isSuccessful, time_point& timeLastOp, milliseconds& timeout) 
   //    param isSuccessful true if timer run without errors otherwise false
   //    param timeLastOp   time of last operation (Transmit or Receive)
   //    return             true if timer should be reschedule, otherwise false
   typedef std::function<bool(const boost::chrono::steady_clock::time_point&)> cbfun_t;

   // Create timer
   static ptr createKATimer(boost::asio::io_service * pIOService, uint32_t timeoutMsec, const char * logName);
    // Destructor
   ~CAPCKATimer();
   // Start timer
   apc_error_t startTimer(cbfun_t fun);
   // Stop timer
   void        stopTimer();
   // Set current time as time of last operation (Transmit or Receive)
   void        recordActivity();
private:
   CAPCKATimer(boost::asio::io_service * pIOService, uint32_t timeoutMsec, const char * logName);

   boost::mutex                             m_lockWrkFlag;
   boost::mutex                             m_lock;
   boost::asio::deadline_timer              m_kaTimer;
   cbfun_t                                  m_cbFun;
   uint32_t                                 m_kaTimeoutMsec;
   boost::chrono::steady_clock::time_point  m_timeLastActivity;
   bool                                     m_isWorking;
   std::string                              m_log;

   void handle_timer_p(const boost::system::error_code& error);
   apc_error_t startTimer_p();

   boost::chrono::steady_clock::time_point getTimeLastActivity_p();
};

/**
 * APC connection.
 * 
 * Class implements send / receive operations between Client (AP) and Server (Manager).
 * After start(), the object begins rx processing and sends keep alive packets. 
 * Object automatically generates 'APC_CONNECT' message.
 */
class CAPCConnector : public boost::enable_shared_from_this<CAPCConnector>, public ISerRxHandler 
{

public:
  typedef boost::shared_ptr<CAPCConnector> ptr;
  typedef boost::array<uint8_t, APC_MAX_MSG_SIZE> iobuf_t;

   /**
    * Initialization parameters of APC connector
    */
   struct init_param_t
   {
      std::string                  apcConnect;     ///< APC name
      boost::asio::io_service    * pIOService;     ///< pointer to boost::asio::io_service object
      IAPCConnectorNotif           * pApcNotif;      ///< Interface to APC notification system
      uint32_t                     kaTimeout;      ///< Max timeout between packet (milliseconds)
      uint32_t                     freeBufTimeout; ///< Max timeout waiting free packet (milliseconds)
      uint32_t                     unconfirmedInpPkt; ///< Max number of unreported input numbers
      std::string                  logName;        ///< Name of logger
      std::string                  swVersion;      ///< string with software version
      void clear() {
         pIOService = NULL; pApcNotif = NULL; 
         kaTimeout = 0;
         apcConnect.clear(); logName.clear(); swVersion.clear();
      }
   };

   /**
    * Connector property
    */
   struct property_t
   {
      bool                     isConnected;
      ap_intf_id_t             intfId;             ///< APC Interface ID
      std::string              localName;          ///< Name of owner connection
      boost::asio::ip::address localAddress;       ///< IP address of owner connection
      uint16_t                 localPort;          ///< TCP/IP port of owner connection
      std::string              peerName;           ///< Name of other side of connection
      boost::asio::ip::address peerAddress;        ///< IP address of other side of connection
      uint16_t                 peerPort;           ///< IP port of other side of connection
      uint16_t                 curAllocOutBuf;
      uint16_t                 maxAllocOutBuf;     ///< Min number of free output buffers
      uint32_t                 numReceivedPkt;     ///< Number received packets
      statdelays_s             sendStat;
   };
  
   /* 
    * APC Connector Statistics
    */
   struct stats_s {
      statdelays_s  m_sendStat;
      uint32_t      m_faultAllocTime;              // Time unsuccessful buffer allocation
      uint32_t      m_numRcvPkt;                   // Number or received packets
   };

   /**
    * Get APC Connector Statistics
    */
   stats_s getAPCCStatistics();


   enum stopflags_t {  //"#IGNORE"
      STOP_FL_OFFLINE,
      STOP_FL_DISCONNECT, // Send apcDisconnect notification with flag isImmediately
   };

  /**
   * Creates a connection.
   *
   * \param param    initialize parameters of connector
   *
   * \return   The shared pointer to new connection.
   */
  static ptr     createConnection(const init_param_t& param);

  ~CAPCConnector();

  /**
   * Starts processing receive data and sending KA 
   */
  apc_error_t start();

  /**
   * Send Connect message.
   *
   * \param sesId    Session ID
   * \param mySeq    Sender's sequence number.
   * \param yourSeq  Sequence number of last received peer packet
   *
   * \return   result of operation
   */
  apc_error_t connect(ap_intf_id_t sesId, uint32_t mySeq, uint32_t yourSeq, 
                     const apc_msg_net_gpslock_s& gpsState, uint32_t netId, uint32_t flags = 0); 
  apc_error_t connect(ap_intf_id_t sesId, uint32_t mySeq, uint32_t yourSeq, uint32_t netId, uint32_t flags = 0) {
     apc_msg_net_gpslock_s gpsState = {APINTF_GPS_NOLOCK};
     return connect(sesId, mySeq, yourSeq, gpsState, netId, flags);
  }

  /**
   * Terminate connection.
   */
  void       disconnect();

  /**
   * Gets interface ID.
   *
   * \return   The interface ID.
   */
  ap_intf_id_t  getIntfId() const { return m_intfId; }

  /**
   * Clears the interface ID.
   */
  void       clearIntfId() { m_intfId = APINTFID_EMPTY; }

  /**
   * Stop all processes (receiving, KA) and close socket.
   *
   * \param reason      Reason for stopping the APC Connector.
   * \param err         The error.
   * \param stopFlags   The stop flags.
   */

  void      stop(apc_stop_reason_t reason, apc_error_t err, stopflags_t stopFlags, 
                 bool isFinishWriting = true, bool isSendNotif = true);

  /**
   * Send a message to the other side of the APC Connection
   *
   * \param type        Message type.
   * \param flags       Message flags, see \ref apc_hdr_flags_t.
   * \param seqNum      Sequence number of this packet.
   * \param payload1    Packet data
   * \param size1       Size of packet data.
   * \param payload2    Packet data, part 2
   * \param size2       Size of packet data part 2.
   *
   * \return   result of operation
   */
  apc_error_t sendData(apc_msg_type_t  type, uint8_t flags, uint32_t seqNum, 
                       const uint8_t * payload1, uint16_t size1, 
                       const uint8_t * payload2, uint16_t size2);
  /**
   * Gets the socket.
   *
   * \return   The socket.
   */
  boost::asio::ip::tcp::socket&  getSocket() { return m_socket; }

  /**
   * Gets interface property.
   *
   * \param [out] pProperty   The current property structure.
   */
  void   getProperty(property_t * pProperty);

  bool   isWorking();

  void   clearStats() { m_stats.reset(); }

  const std::string& getPeerName() const { return m_peerIntfName; }

private:
   struct APCCStats {
      CStatDelaysCalc  m_sendStat;
      uint32_t         m_faultAllocTime;              // Time unsuccessful buffer allocation
      uint32_t         m_numRcvPkt;                   // Number or received packets

      APCCStats() {
         reset();
      };

      // Reset all statistics
      void reset() {
         m_sendStat.clear();
         m_faultAllocTime   = 0;
         m_numRcvPkt        = 0;
      }
   };

   std::string      m_apcName;                     // Name of connector owner
   std::string      m_swVersion;                   // Software version
   IAPCConnectorNotif * m_pApcNotif;               // Interface to APC notification system
   CAPCSerializer * m_pSerializer;                 // Serialize / de-serialize one message
   ap_intf_id_t     m_intfId;                      // APC Interface ID
   bool             m_isWorking;                   // Flag - Connecter is working
   bool             m_isConnected;                 // Flag - connection is established
   bool             m_forceDisconnect;             // Disconnect by 'disconnect' method
   CAPCKATimer::ptr m_kaTxTimer;                   // TX Keep Alive timer
   CAPCKATimer::ptr m_kaRxTimer;                   // RX Keep Alive timer
   uint32_t         m_unconfirmedInpPkt;           // Max number of unreported input numbers
   boost::chrono::milliseconds m_txTimeout;        // Max time between transition (KA timeout * 0.25)
   boost::chrono::milliseconds m_rxTimeout;        // Max time between receiving  (KA timeout * 2)
   iobuf_t          m_inpbuf;                      // Input buffer
   iobuf_t          m_outbufs[APC_NUM_OUT_BUFS];   // Output buffers
   uint32_t         m_numFreeOutBuf;               // Number of free output buffers
   uint32_t         m_minNumFreeOutBuf;            // Min number of free buffers
   uint32_t         m_lastReceivedSeqNum;           // Seq. number of received packet (input 'mySeq')
   uint32_t         m_lastReportedSeqNum;          // Last reported reported seq.number (out 'yourSeq')
   uint32_t         m_curFreeBufIdx;               // Index of current free buffer

   APCCStats                    m_stats;           // APC Connector Statistics

   std::string                   m_log;            // Logger
   boost::asio::ip::tcp::socket m_socket;          // TCP socket
   boost::mutex                 m_lock;            // Lock for object
   std::string                  m_peerIntfName;    // Name of other side of connection

   boost::condition_variable    m_freeBufSig;      // Signal of buffer free
   usec_t                       m_freeBufWait;     // max time of waiting buffer

   CAPCConnector(const init_param_t& param);
   
   // Get free buffer.
   // If no free buffers then wait 'Free Buffer' notification.
   uint8_t *          getFreeBuf_p(boost::unique_lock<boost::mutex>& lock, bool allowIfNotWorking = false);        
   // Free first allocated buffer (send 'Free Buffer' notification)
   void               freeBuf_p();           
   // Free last allocated buffer  
   void               returnBuf_p();         
   // Increase number of free buffers
   void               incrNumFreeBuf_p();    

   // RX Keep Alive timer callback function
   bool        ka_timeout_rx_p(const boost::chrono::steady_clock::time_point& lastAction);
   // TX Keep Alive timer callback function
   bool        ka_timeout_tx_p(const boost::chrono::steady_clock::time_point& lastAction);
   // Async write callback function
   void        handle_write_p(const boost::system::error_code& error, size_t len);
   // Async read callback function
   void        handle_read_p (const boost::system::error_code& error, size_t len);
   // Start async read
   apc_error_t async_read_p();
   // Send 'Keep Alive' message.
   apc_error_t send_ka_p();

   // IAPCRxHandler interface -------------------------------------------------
   // 'Message Received' callback function
   virtual apc_error_t messageReceived(ap_intf_id_t apcId, apc_msg_type_t type, uint8_t flags,
                                       uint32_t myseq, uint32_t yourSeq,
                                       const uint8_t * pPayload, uint16_t size);
   friend class CAPCSerializer;
};

/**
 * Interface to APC notification system
 */
class IAPCConnectorNotif 
{
public:
   struct param_connected_s {
      uint8_t        ver; 
      uint32_t       netId;
      ap_intf_id_t   apcId; 
      char           name[APC_CONNECTOR_NAME_LENGTH+1];
      uint32_t       cmdFlags;
      uint8_t        hdrFlags;
      uint32_t       mySeq;
      uint32_t       yourSeq; 
      char           version[SIZE_STR_VER];
   };

   struct param_disconnected_s {
      CAPCConnector::stopflags_t flags;
      apc_stop_reason_t          reason;
      uint32_t                   maxAllocOutPkt;
   };

   struct param_received_s {
      ap_intf_id_t   apcId;
      uint8_t        flags;
      uint32_t       mySeq;
      uint32_t       yourSeq;
      apc_msg_type_t type;
   };


   virtual ~IAPCConnectorNotif () {;};

   /**
    * AP connector started.
    *
    * \param   pAPC    pointer to connector object
    */
   virtual void apcStarted(CAPCConnector::ptr pAPC) = 0;

   /**
    * APC establish connection.
    *
    * \param   name  Name of other side of connection
    */
   virtual void apcConnected(CAPCConnector::ptr pAPC, const param_connected_s& param) = 0;

   /**
    * AP disconnect notification.
    *
    * \param   pAPC           Disconnected interface
    * \param   isImmediately  Don't wait for reconnection
    * \param   reason         Reason for disconnection       
    * \param   maxAllocOutPkt Number of used output buffers
    */
   virtual void apcDisconnected(CAPCConnector::ptr pAPC, const param_disconnected_s& param) = 0;

   /**
    * Data receive notification
    *
    * \param   apcId    Interface ID
    * \param   type     The type of receiving data
    * \param   pPayload The payload.
    * \param   size     The size.
    */
   virtual void messageReceived(const param_received_s& param, const uint8_t * pPayload, uint16_t size) = 0;

};

