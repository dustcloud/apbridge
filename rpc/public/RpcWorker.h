/*
 * Copyright (c) 2013, Linear Technology.  All rights reserved.
 */

#pragma once

#include <string>
#include <map>

#include "common/dn_typedef.h"
#include "common/common.h"
#include "zmq.hpp"
#include "Logger.h"
#include "RpcProtocol.h"
#include "zmqUtils.h"
#include "rpc/utils/public/rpc_logger.h"
#include "rpc/common.pb.h"
#include <exception>

using namespace zmqUtils;

const uint32_t DEFAULT_HEARTBEAT = 2500; // milliseconds
const uint32_t DEFAULT_RECONNECT_DELAY = 1000; // milliseconds

/** Base class for RpcWorkers
 *
 * A RpcWorker is the base class for RPC workers. Each worker provides an
 * implementation for a specific set of RPC commands, collectively called a
 * service.
 *
 * A child class should implement either processCall or processCmdMsg (preferred)
 * in order to process the call and generate a reply message. 
 * processCmdMsg is preferred because it performs some of the common message parsing.
 */
class RpcWorker 
{
public:
   typedef std::map<rpc_cmdid_t, common::UserPrivilege> cmdprivlist_t;

   /**
    * Constructor.
    *
    * \param   context       ZMQ context
    * \param   serverAddr    The server address.
    * \param   serviceName   Name of the service.
    * \param   pCmdPriv      List of commands and required privileges (can be NULL) 
    * \param   defaultPriv   The default privilege required to perform any command. 
    * \param   identity      (Optional) the identity of workers
    * \param   logname       (Optional) the log-name.
    */
   RpcWorker(zmq::context_t* context, std::string serverAddr, std::string serviceName, 
             const cmdprivlist_t * pCmdPriv, common::UserPrivilege defaultPriv,
             std::string identity = "", std::string logname=rpclogger::RPC_WORKER_NAME,
             bool allowUninitializedAuth = false);

   virtual ~RpcWorker();
   
   /**
    * Set the worker's identity
    */
   void setIdentity(std::string identity);

   std::string getService() const { return m_service; }
   
   // ----------------------------------------------------------------
   // Connect or reconnect to server
   void connectToServer(std::string serverAddr);

   // ----------------------------------------------------------------
   // Set heartbeat delay
   void set_heartbeat(uint32_t heartbeat) { m_heartbeat = heartbeat; }

   // ----------------------------------------------------------------
   // Set reconnect delay
   void set_reconnect(uint32_t reconnect) { m_reconnect = reconnect; }
   
   // ----------------------------------------------------------------
   // Worker main loop
   void run();

   // ----------------------------------------------------------------
   // Cancel the work loop
   void cancel() { m_done = true; }
   
   // ----------------------------------------------------------------
   // Set flag: uninitialized authentication is allowed or not
   void setUninitializedAuthFlag(bool isAllow) { m_allowUninitializedAuth = isAllow; }
protected:
   /**
    * Process the command
    *
    * processCmdMsg is called by the worker thread to handle a client
    * request. The worker should implement this method to dispatch based on
    * the cmdId to a handler that parses the protobuf request parameters and
    * returns a response using one of the createResponse methods.
    *
    * \param cmdId  Command ID
    * \param params Protobuf serialized byte string, parsed with msg.FromString
    * \returns a response message    
    */
   virtual zmessage* processCmdMsg(rpc_cmdid_t cmdId, const std::string& params) = 0;

   /**
    * Get the privilege required for a command.
    * Returns the minimum user privilege needed to run the specified command. 
    * If no user authorization checking is needed, the return value 
    * should be UserPrivilege::NONE.
    *
    * \param   cmdId Identifier for the command.
    *
    * \return  The minimum privilege required to perform the command.
    */
   virtual common::UserPrivilege getCommandPrivilege(rpc_cmdid_t cmdId);


   // ----------------------------------------------------------------
   // Create a response message with the expected format

   /**
    * Helper method to create a response from a protobuf message
    */
   zmessage* createResponse(rpc_cmdid_t cmdId, 
                            const google::protobuf::Message& respMsg);
   
   /**
    * Helper method to create a response from a error code
    */
   zmessage* createResponse(rpc_cmdid_t cmdId, 
                            int32_t errorCode, std::string errorMsg);

   /**
    * Log a RPC command received message
    * 
    * \param cmdId Command Id
    */
   void logCmdReceived(rpc_cmdid_t cmdId);
   
   /**
    * Convert cmdcode from command enumeration to string. This method should
    * be overriden by each service. The default implementation just returns
    * the integer command id.
    * 
    * \param cmdId Command Id
    * \return command code as string
    */
   virtual const std::string cmdCodeToStr(rpc_cmdid_t cmdId);

protected:
   class CProtoBufParseErrorException : public std::runtime_error {
   public:
      CProtoBufParseErrorException(const std::string& msg) : std::runtime_error(msg){;}
   };
   std::string              m_logname;
   const cmdprivlist_t    * m_cmdPrivileges;
   common::UserPrivilege    m_defaultCmdPrivilege;

   template<class T> void parseFromString_p(T& request, std::string requestStr) {
      if (!request.ParseFromString(requestStr))
         throw CProtoBufParseErrorException(request.GetTypeName());
   }

private:
   // ----------------------------------------------------------------
   // Process RPC call
   zmessage* processCall(zmessage* msg);

   // ---------------------------------------------------------------------
   // Send message to server
   // If no _msg is provided, creates one internally
   void sendToServer(RpcCommands command, std::string option, zmessage *_msg);
   
   // ----------------------------------------------------------------------
   // Send reply
   void sendReply(zmessage* reply);
   
   // ----------------------------------------------------------------------
   // Check user privilege against required command privilege
   bool validatePrivilege(rpc_cmdid_t cmdId, const common::AuthReq& auth, 
                          bool allowUninitializedAuth);
   

   std::string m_server;
   std::string m_service; // Service name
   std::string m_identity;
   zmq::context_t *m_context;
   zmq::socket_t  *m_worker;     //  Socket to server

   //  Heartbeat management
   mngr_time_t m_heartbeat_at;   //  When to send HEARTBEAT
   size_t m_liveness;            //  How many attempts left
   int m_heartbeat;              //  Heartbeat delay, msecs
   int m_reconnect;              //  Reconnect delay, msecs
   bool m_allowUninitializedAuth;
   //  Internal state
   bool m_expect_reply;          //  Zero only at start

   //  Return address, if any
   std::string m_reply_to;

   bool m_done;
};
