#pragma once
#include "common.h"

/**
 * \file APCError.h
 */

/**
 * Values that represent APC Errors.
 */
enum apc_error_t
{
   APC_OK,
   APC_STOP_CONNECTOR,        ///< Stop APC connector
   APC_ERR_INIT,              ///< Initialization
   APC_ERR_SIZE,              ///< Wrong Size of data
   APC_ERR_OUTBUFOVERFLOW,    ///< Output buffers overflow
   APC_ERR_UNCONFIRMED_PKT,   ///< Number of unconfirmed output packets > max 
   APC_ERR_IO,                ///< Error in low-level IO synch operation... (read, write)
   APC_ERR_ASYNC_OPERATION,   ///< Error in low-level asynchronous operation... (read, write, timer)
   APC_ERR_STATE,             ///< Wrong state of object
   APC_ERR_PROTOCOL,          ///< APC protocol error
   APC_ERR_NOTFOUND,          ///< Object is not found
   APC_ERR_PKTSERIALIZATION,  ///< Error in packet serialization
   APC_ERR_NOTCONNECT,        ///< Connection is not establish 
   APC_ERR_DISCONNECT,        ///< Session is disconnect           
   APC_ERR_OFFLINE,           ///< Can not send data. Connection is offline
   APC_ERR_CONNECT,           ///< Handshake error 
};
ENUM2STR(apc_error_t);

/**
 * Values that represent APC stop reasons
 */
enum apc_stop_reason_t
{
   APC_STOP_NA,            ///< Keep Alive timeout expire
   APC_DISCONNECT_MSG,     ///< Disconnect message is received
   APC_STOP_TIMEOUT,       ///< Keep Alive timeout expire
   APC_STOP_CREATE,        ///< Create connector error
   APC_STOP_READ,          ///< Read error
   APC_STOP_WRITE,         ///< Write error
   APC_STOP_CLOSE,         ///< Close Connector process
   APC_STOP_DELAYED,       ///< Close after delay
   APC_STOP_CONNECT_NOT_FOUND, ///< Can not find requested session
   APC_STOP_CONNECT_NEW_PORT,  ///< APC reconnected for new port (old connection is closed)
   APC_STOP_CONNECT_SAME_PORT, ///< APC reconnect to same port (current connection is closed)
   APC_STOP_PKTPARSE,      ///< Error in packet parsing
   APC_STOP_VER,           ///< Protocol version
   APC_STOP_MAXAPC,        ///< Max number of APC
   APC_STOP_RECONNECTION,  ///< APC client reconnection error
};
ENUM2STR(apc_stop_reason_t);

