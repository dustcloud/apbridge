/*
 * Copyright (c) 2013, Linear Technology. All rights reserved.
 */

#pragma once

/**
 * \file RpcCommon.h
 * Common RPC declarations
 */
#include "common/common.h"
#include "common/ProcessInputArguments.h"

/** Base port for RPC endpoints when using TCP
 */
//const uint16_t DEFAULT_RPC_BASE_PORT = 9000; // TODO: ini parameter

/**
 * Result code for RPC responses
 */
enum RpcResult {
   RPC_OK                     = 0,  // "OK"
   RPC_INVALID_COMMAND        = 1,  // "Invalid command"
   RPC_INVALID_PARAMETERS     = 2,  // "Invalid parameters"
   RPC_INVALID_NUM_PARAMETERS = 3,  // "Invalid number of parameters"
   RPC_OBJECT_NOT_FOUND       = 4,  // "Object not found"
   RPC_CREATE_FAILED          = 5,  // "Create object failed"
   RPC_MOTE_NOT_FOUND         = 6,  // "Mote or AP not found"
   RPC_PATH_NOT_FOUND         = 7,  // "Path not found"
   RPC_LINK_NOT_FOUND         = 8,  // "Link not found"
   RPC_DELETE_FAILED          = 9,  // "Delete object failed"
   RPC_JOIN_FAILED            = 10, // "Join event failed"
   RPC_SEND_FAILED            = 11, // "Send command failed"
   RPC_MOTE_CONNECT_ERROR     = 12, // "Mote or AP connection error"
   RPC_GET_ROUTE_ERROR        = 13, // "Get route error"
   RPC_AP_NOT_FOUND           = 14, // "Access point not found"
   RPC_USER_NOT_FOUND         = 15, // "User not found"
   RPC_USER_EXISTS            = 16, // "User exists"
   RPC_ENTRY_NOT_FOUND        = 17, // "Entry not found"
   RPC_MOTE_NOT_AP            = 18, // "Mote is not an AP"
   RPC_TIMEOUT                = 19, // "Timeout" 
   RPC_INVALID_JOINKEY        = 20, // "Invalid join key value"
   RPC_AP_EXISTS              = 21, // "Access point exists"
   RPC_SERVICE_NOT_AVAILABLE  = 22, // "RPC service not available"
   RPC_MOTE_STATE             = 23, // "Mote or AP is in the wrong state" 
   RPC_CMD_OUTPUT_QUEUE_FULL  = 24, // "Command output queue is full"
   ///< An enum constant representing the RPC invalid authentication option
   RPC_INVALID_AUTHENTICATION = 25, // "Invalid authentication"
   RPC_CLIENT_NOT_FOUND       = 26, // "Client not found"
   RPC_CLIENT_EXISTS          = 27, // "Client exists"
   RPC_ACCESSTOKEN_NOT_FOUND  = 28, // "Access token not found"
   RPC_ACCESSTOKEN_EXISTS     = 29, // "Access token exists"
   RPC_REQUESTTOKEN_NOT_FOUND = 30, // "Request token not found"
   RPC_CMD_IN_PROGRESS        = 31, // "Command in progress"
   RPC_INVALID_CMD_SIGNATURE  = 32, // "Invalid command signature"
   RPC_INVALID_SERVICE_ID     = 33, // "Invalid service id"
   RPC_DB_ERROR               = 34, // "Database Error"
   RPC_NOT_AUTHORIZED         = 35, // "User is not authorized"
   
   RPC_WD_STATE               = 36, // "Wrong watchdog state"
   RPC_WD_CLIENT_STATE        = 37, // "Wrong WD-client state"
   RPC_WD_NOTFOUND            = 38, // "WD-client not found"
   RPC_WD_CLIENT_TYPE         = 39, // "Type of WD-client incompatible with requested command"

   RPC_PARSE                  = 40, // RPC parse error
};
ENUM2STR(RpcResult);

/**
 * RPC endpoint definition to be used in C++ and Python
 *
 * This enum provides a common lookup for both endpoint paths and the
 * assignment of TCP ports relative to a base port. The convention is for the
 * path to be relative to the base Voyager directory and for the enumerator
 * value to be the offset from the base port.
 */
enum RpcEndpoints {
   TPLGDB_ENDPOINT_PATH            = 0,  // "ipc://manager_rpc.ipc"
   MNGR_NOTIFY_ENDPOINT_PATH       = 1,  // "ipc://manager_notif.ipc"
   MNGR_LOG_ENDPOINT_PATH          = 2,  // "ipc://manager_log.ipc"
   TPLGCMD_NOTIFY_ENDPOINT_PATH    = 3,  // "ipc://tplgcmd_notif.ipc"
   AUTH_MNGR_ENDPOINT_PATH         = 4,  // "ipc://authmngr_rpc.ipc"
   AUTH_MNGR_LOG_ENDPOINT_PATH     = 5,  // "ipc://authmngr_log.ipc"
   AUTH_MNGR_NOTIFY_ENDPOINT_PATH  = 6,  // "ipc://authmngr_notif.ipc"
   APC_ENDPOINT_PATH               = 7,  // "ipc://apc_rpc.ipc"
   APC_LOG_ENDPOINT_PATH           = 8,  // "ipc://apc_log.ipc"
   CONFIGDB_ENDPOINT_PATH          = 9,  // "ipc://confdb_rpc.ipc"
   CONFIGDB_NOTIFY_ENDPOINT_PATH   = 10, // "ipc://confdb_notif.ipc"
   WATCHDOG_ENDPOINT_PATH          = 11, // "ipc://watchdog_rpc.ipc"
   WATCHDOG_NOTIFY_ENDPOINT_PATH   = 12, // "ipc://watchdog_notif.ipc"
   CONFIGDB_LOG_ENDPOINT_PATH      = 13, // "ipc://confdb_log.ipc"
   WATCHDOG_LOG_ENDPOINT_PATH      = 14, // "ipc://watchdog_log.ipc"
};
ENUM2STR(RpcEndpoints);

/**
 * RPC services definition to be used in C++ and Python
 */
enum RpcServices {
   TOPOLOGY_DB_SERVICE         = 0,  // "tplgdb"
   MANAGER_RPC_SERVICE         = 1,  // "managerrpc"
   TOPOLOGY_INPUTEVENT_SERVICE = 2,  // "tplginputevent"
   CONFIG_RPC_SERVICE          = 3,  // "configrpc"
   STATISTICS_RPC_SERVICE      = 4,  // "statisticsrpc"
   LOG_RPC_SERVICE             = 5,  // "logger"
   SECURITY_RPC_SERVICE        = 6,  // "securityrpc"
   NOTIF_GENERATOR_RPC_SERVICE = 7,  // "notifgeneratorrpc"
   INTERNALCMD_RPC_SERVICE     = 8,  // "internalcmdrpc"
   AUTHENTICATION_RPC_SERVICE  = 9,  // "authenticationrpc"
   APC_RPC_SERVICE             = 10, // "apc"
   WATCHDOG_RPC_SERVICE        = 11, // "watchdog"
   WD_USER_SERVICE             = 12, // "wd_user"
};
ENUM2STR(RpcServices);

// ===================== Common functions ===========================

/**
 * Convert mngr_time_t to milliseconds since the epoch
 */
int64_t mngr_time2msec(mngr_time_t t);

/**
 * Convert System time to milliseconds since the epoch
 */
int64_t systime2msec(const sys_time_t& utc);

/**
 * Get current time in milliseconds since the epoch
 */
int64_t getCurrentTime();

/**
 * Get current time in seconds since the epoch
 */
int64_t getCurrentTimeSecs();

/**
 * API Endpoint Builder
 * 
 * Construct API endpoints from command line arguments
 */
class CApiEndpointBuilder
{
public:
   CApiEndpointBuilder();
   CApiEndpointBuilder(const CProcessInputArguments& cmdArgs);
   void init(const CProcessInputArguments& cmdArgs);
   void init(apiproto_t proto, const std::string& path,
             const std::string& host, uint16_t basePort);
   std::string getClient(RpcEndpoints endpointId, const std::string endpointExt = "") const;
   std::string getServer(RpcEndpoints endpointId, const std::string endpointExt = "") const;
   apiproto_t  getProto() const { return m_proto; }
   uint16_t    getBasePort() const { return m_basePort; }
   const std::string& getPath() const { return m_path; }
   const std::string& getHost() const { return m_host; }
private:
   apiproto_t  m_proto;
   std::string m_path;
   std::string m_host;
   uint16_t    m_basePort;

   void parseEndpointExt_p(const std::string& endpointExt, std::string * pIpcName, uint16_t * pPortOffset) const;
   std::string getIPCname_p(RpcEndpoints endpointId, const std::string& ipcName) const;
};

// Get max number of items list in RPC list-response
uint32_t getMaxSizeListResp();
// Set max number of items list in RPC list-response
// 0 - restore default value
void     setMaxSizeListResp(uint32_t val = 0);

template<class T> 
void getListReqLimit(T& req, uint32_t * pIdx, uint32_t * pMaxNumb) {
   *pIdx = 0;
   *pMaxNumb = getMaxSizeListResp();
   if(req.has_listreq())  {
      if(req.listreq().has_startindex())
         *pIdx = req.listreq().startindex();

      if(req.listreq().has_maxlength() && 
         req.listreq().maxlength() > 0 &&
         req.listreq().maxlength() < getMaxSizeListResp())
         *pMaxNumb = req.listreq().maxlength();
   }
}
