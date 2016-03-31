#pragma once

#pragma once
#include "IAPCCommon.h"
#include "APCError.h"
#include <string>
#include "common/StatDelays.h"

namespace boost { namespace asio { class io_service; } }

enum apcclient_state_t { 
   APCCLIENT_STATE_INIT,   
   APCCLIENT_STATE_ONLINE,
   APCCLIENT_STATE_OFFLINE,
   APCCLIENT_STATE_DISCONNECT,
};

/**
 * \file IAPCClient.h
 */

/**
 * Class supports processing input from Manager.
 */
class IAPCClientNotif 
{
public:
   virtual ~IAPCClientNotif() {;}

   /**
    * Connection established
    *
    * \param   name name of server
    */
   virtual void connected (const std::string name) = 0;

   /**
    * The client has been disconnected from the Manager.
    *
    * \param   reason   The disconnection reason.
    */
   virtual void disconnected (apc_stop_reason_t reason) = 0; 

   /**
    * Data received.
    *
    * \param   hdr            The header of NET_TX message.
    * \param   payload        The payload.
    * \param   payloadLength  Length of the payload.
    */
   virtual void dataRx (const ap_intf_sendhdr_t& hdr, const uint8_t * payload, uint32_t payloadLength) = 0;

   /**
    * Resume sending data to server.
    */
   virtual void resume() = 0;

   /**
    * Pause sending data to server.
    */
   virtual void pause () = 0;

   /**
    * Connection goes to offline.
    */
   virtual void offline(apc_stop_reason_t reason) = 0;

   /**
    * Connection returns to online.
    */
   virtual void online() = 0;

   /**
    * Request to Reset AP.
    */
   virtual void resetAP() = 0;

   /**
    * Request to Disconnect AP.
    */
   virtual void disconnectAP() = 0;

   /**
    * Request for time map.
    */
   virtual void getTime() = 0;
};

/**
 * APC client interface
 */
class IAPCClient
{
public:
   struct open_param_t
   {
      IAPCClientNotif  * pInput;       ///< Call-back object for processing manager messages
      std::string      intfName;     ///< Name of Client Connector
      std::string      logName;      ///< Name of client logger
   };

   struct start_param_t
   {
      std::string      host;         ///< Hostname or IP address of manager
      uint16_t         port;         ///< TCP port of manager
      uint32_t         kaTimeout;    ///< AP Keep Alive Timeouts (milliseconds)
      uint32_t         freeBufTimeout; ///< Max timeout waiting free packet (milliseconds)
      uint32_t         reconnectionDelayMsec;   ///< Delay between reconnection attempts
      uint32_t         disconnectTimeoutMsec;   ///< Max time before disconnecting when offline
   };

   virtual ~IAPCClient() {;}

   virtual apc_error_t open(const open_param_t& param) = 0;
   virtual void        close() = 0;

   /**
    * Starts the client event loop. 
    * This method connects to the  APC server and 
    * starts the io_service event loop in a separate thread.
    *
    * \param param The initialize parameter of client.
    *
    * \return result code.
    */
   virtual apc_error_t start(const start_param_t& param) = 0;

   /**
    * Stops the client event loop thread. 
    * This method returns when the event loop thread completes.
    */
   virtual void        stop() = 0;

   /**
    * Data transmit.
    *
    * \param   payload        The payload.
    * \param   payloadLength  Length of the payload.
    *
    * \return  result code.
    */
   virtual apc_error_t sendData(const uint8_t * payload, uint32_t payloadLength) = 0;

   /**
    * Send message "Transmit done".
    *
    * \param  p  txDone parameter
    *
    * \return result code.
    */
   virtual apc_error_t sendTxDone(const ap_intf_txdone_t& p) = 0;

   /**
    * Send current time mapping.
    *
    * \param  p  time map
    *
    * \return result code.
    */
   virtual apc_error_t sendTimeMap(const ap_time_map_t& p) = 0;

   /**
    * Send Lost AP message
    *
    * \return result code.
    */
   virtual apc_error_t sendApLost() = 0;

   /**
    * Send Resume message
    *
    * \return result code.
    */
   virtual apc_error_t sendResume() = 0;

   /**
    * Send Pause message
    *
    * \return result code.
    */
   virtual apc_error_t sendPause () = 0;

   /**
    * Gets the state.
    *
    * \return  The state.
    */
   virtual apcclient_state_t getState() const = 0;

   /**
    * Send message "GPS Lock State".
    *
    * \param  p  gpslock parameter
    *
    * \return result code.
    */
   virtual apc_error_t sendGpsLock(const ap_intf_gpslock_t& p) = 0;

   virtual bool        isConnected() = 0;

   /**
    * Gets net identifier received from manager.
    *
    * \return  The net identifier.
    */
   virtual uint32_t    getNetId() const = 0;
   
   
   /**
    * Gets the server available queue size.
    *
    * \return	The server available queue size.
    */
   virtual size_t getCachedPkts() = 0;

   /**
    * Gets the APC Connector Statistics structure
    *
    * \return	The server total sent packets
    */
   virtual statdelays_s getAPCCStats() const = 0;
   virtual uint32_t getNumRcvPkt() const = 0;

   /**
    * Clear the statistics
    *
    * \return	None
    */
   virtual void clearStats() = 0;

};
