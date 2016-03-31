/*
 * Copyright (c) 2013, Linear Technology. All rights reserved.
 */

#include "public/RpcWorker.h"

#include <iostream>
#include <sstream>

#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/thread/thread.hpp>
#include "public/RpcCommon.h"
#include "public/SerializerUtility.h"


// Reliability parameters
#define HEARTBEAT_LIVENESS  10       //  3-5 is reasonable

using namespace zmqUtils;
using namespace std;

// ---------------------------------------------------------------------
// Constructor(s) and destructor
RpcWorker::RpcWorker(zmq::context_t* context,
                     string serverAddr, string serviceName,
                     const cmdprivlist_t * pCmdPriv,
                     common::UserPrivilege defaultPriv,
                     string identity, string logname,
                     bool allowUninitializedAuth)
   : m_logname(logname),
     m_cmdPrivileges(pCmdPriv),
     m_defaultCmdPrivilege(defaultPriv),
     m_server(serverAddr),
     m_service(serviceName),
     m_identity(identity),
     m_context(context),
     m_worker(NULL),    // really init'ed on connect
     m_heartbeat_at(),  // really init'ed on connect
     m_liveness(0),     // really init'ed on connect
     m_heartbeat(DEFAULT_HEARTBEAT),
     m_reconnect(DEFAULT_RECONNECT_DELAY),
     m_allowUninitializedAuth(allowUninitializedAuth),
     m_expect_reply(false),
     m_reply_to(),
     m_done(false)
{

}

RpcWorker::~RpcWorker() {
   if (m_worker) {
      delete m_worker;
   }
}

// ---------------------------------------------------------------------
// Send message to server
// If no _msg is provided, creates one internally
void RpcWorker::sendToServer(RpcCommands command, string option, zmessage *_msg)
{
   zmessage *msg = _msg ? _msg : new zmessage();

   // Stack protocol envelope to start of message
   if (option.length() != 0) {
      msg->pushstr(option);
   }
   zframe_t* frame = zframe_new(&command, 1);
   msg->push(frame);
   msg->pushstr(RPC_WORKER);
   msg->pushstr("");

   if (command != RPC_HEARTBEAT) {
      DUSTLOG_MSGTRACE(m_logname, "sending msg to server: ", msg);
   }
   int rc = msg->send(m_worker);
   if (rc != 0) {
      DUSTLOG_ERROR(m_logname, "error sending msg (" << toString(command) << ") to server: err=" << errno);
   }
   delete msg; // send deletes the msg buffer, but not the zmessage container
}

// ---------------------------------------------------------------------
// Connect or reconnect to server

void RpcWorker::connectToServer(string serverAddr)
{
   m_server = serverAddr;
   if (m_worker) {
      delete m_worker;
   }
   m_worker = new zmq::socket_t(*m_context, ZMQ_DEALER);
   int linger = 0;
   m_worker->setsockopt(ZMQ_LINGER, &linger, sizeof (linger));
   m_worker->setsockopt(ZMQ_IDENTITY, m_identity.c_str(), m_identity.length());

   // Register service with server
   string ep = m_server.c_str();
   if (ep.substr(0, 3) == "tcp") {
      ep = string("tcp://127.0.0.1") + ep.substr(ep.find_last_of(":"));
   }
   DUSTLOG_INFO(m_logname, m_identity << " connecting to server at " << m_server);
   m_worker->connect(ep.c_str());

   sendToServer(RPC_REGISTER, m_service, NULL);
   m_done = false; // reset the completion flag
   // If liveness hits zero, queue is considered disconnected
   m_liveness = HEARTBEAT_LIVENESS;
   m_heartbeat_at = TIME_NOW() + msec_t(m_heartbeat);
}

void RpcWorker::setIdentity(string identity)
{
   m_identity = identity;
   m_worker->setsockopt(ZMQ_IDENTITY, m_identity.c_str(), m_identity.length());
}

// ----------------------------------------------------------------------
// Send reply
// TODO: no need to keep caller id as a member variable

void RpcWorker::sendReply(zmessage* reply) 
{
   if (reply) {
      assert (m_reply_to.size()!=0);
      reply->wrap(m_reply_to, "");
      m_reply_to = "";
      sendToServer(RPC_REPLY, "", reply);
      // TODO ??? delete reply;
   }
}


zmessage* RpcWorker::processCall(zmessage* msg)
{
   // Get command id and request parameters
   zframe_t* cmdFrame = msg->pop();
   rpc_cmdid_t cmdId = zframe_data(cmdFrame)[0];
   string requestParams = msg->popstr();
   zframe_destroy(&cmdFrame);

   // Parse the authorization structure
   common::AuthReq auth;
   if (msg->size() > 0) {
      string authParams = msg->popstr();
      auth.ParseFromString(authParams);
   }
   
   if (!validatePrivilege(cmdId, auth, m_allowUninitializedAuth)) {
      return createResponse(cmdId, RPC_NOT_AUTHORIZED, "");
   }

   logCmdReceived(cmdId);
   try {
      return processCmdMsg(cmdId, requestParams);
   } catch ( const CProtoBufParseErrorException& ex) {
      DUSTLOG_ERROR(m_logname.c_str(), "RPC parse error for command " << cmdCodeToStr(cmdId) << " " << ex.what());
      return createResponse(cmdId, RPC_PARSE, "");
   }
}

//zmessage* RpcWorker::processCmdMsg(rpc_cmdid_t cmdId, const string& params)
//{
//   // this virtual function should be implemented by the worker sub-class
//   return createResponse(cmdId, RPC_INVALID_COMMAND, "");
//}

zmessage* RpcWorker::createResponse(rpc_cmdid_t cmdId, const google::protobuf::Message& respMsg)
{
   zmessage* responseMsg = new zmessage();
   zframe_t* cmdFrame = zframe_new(&cmdId, 1);
   responseMsg->push(cmdFrame);
   responseMsg->addint(RPC_OK);
   serialize2zmsg(respMsg, *responseMsg);
   return responseMsg;   
}

zmessage* RpcWorker::createResponse(rpc_cmdid_t cmdId, int32_t errorCode, string errorMsg)
{
   zmessage* responseMsg = new zmessage();
   zframe_t* cmdFrame = zframe_new(&cmdId, 1);
   responseMsg->push(cmdFrame);
   responseMsg->addint(errorCode);
   responseMsg->addstr(errorMsg);
   return responseMsg;
}

bool RpcWorker::validatePrivilege(rpc_cmdid_t cmdId, const common::AuthReq& auth,
                                  bool allowUninitializedAuth)
{
   common::UserPrivilege privilege = getCommandPrivilege(cmdId);

   if (privilege == common::NONE)   // Don't check privilege
      return true;   

   if (!auth.IsInitialized()) {
      if (allowUninitializedAuth)
         return true;
      DUSTLOG_ERROR(m_logname, "No authorization for cmd: " << cmdCodeToStr(cmdId));
      return false;
   }
   BOOST_ASSERT(auth.IsInitialized());

   switch (privilege) {
   case common::VIEWER:    // VIEWER or USER is OK
      if (auth.privilege() == common::USER || auth.privilege() == common::VIEWER)
         return true;
      break;
   case common::USER:     // privilege must be USER
      if (auth.privilege() == common::USER) 
         return true;
      break;

   default:
      DUSTLOG_ERROR(m_logname, "Unknown privilege requested: " << privilege);
      break;
   }

   DUSTLOG_ERROR(m_logname, "User '" << auth.userid() << "' is not authorized for cmd: " << cmdCodeToStr(cmdId));
   return false;
}


// ---------------------------------------------------------------------
// Main worker loop

void RpcWorker::run()
{
   DUSTLOG_INFO(m_logname, "Worker starting: " << m_identity);
   while (!m_done) {
      try {
         zmq::pollitem_t items [] = {
            { *m_worker,  0, ZMQ_POLLIN, 0 } };
         zmq::poll (items, 1, m_heartbeat);

         if (items [0].revents & ZMQ_POLLIN) {
            zmessage *msg = new zmessage(m_worker);
            m_liveness = HEARTBEAT_LIVENESS;

            // The RPC Worker receives:
            // - [empty]
            // - RPC_WORKER
            // - procotol command id
            // - parameters
            // - command signature
            
            // don't try to handle errors, just assert noisily
            // TODO: handle errors
            assert(msg->size() >= 3);

            string empty = msg->popstr();
            assert(empty.empty());
            //assert (strcmp (empty, "") == 0);
            //free (empty);
            string header = msg->popstr();
            assert(header.compare(RPC_WORKER) == 0);
            //free (header);

            zframe_t* frame = msg->pop();
            assert(zframe_size(frame) == 1);
            int command = zframe_data(frame)[0];
            zframe_destroy(&frame);
                
            if (command == RPC_REQUEST) {
               // An RPC Request contains:
               // - caller id
               // - [empty]
               // - RPC command id
               // - command parameters
               
               DUSTLOG_MSGTRACE(rpclogger::RPC_WORKER_NAME, "received message from server: ", msg);
               // We should pop and save as many addresses as there are
               // up to a null part, but for now, just save one...
               zframe_t* frame = msg->unwrap();
               m_reply_to = string((char*)zframe_data(frame), zframe_size(frame));
               zmessage* reply = processCall(msg);

               // the caller must format the response
               sendReply(reply);
               zframe_destroy(&frame);
            }
            else if (command == RPC_HEARTBEAT) {
               // Do nothing for heartbeats
            }
            else if (command == RPC_DISCONNECT) {
               connectToServer(m_server);
            }
            else {
               ostringstream os;
               os << *msg;
               DUSTLOG_ERROR(m_logname, "invalid input message " << command << ": " << os.str());
            }
            delete msg;
         }
         else {
            if (HEARTBEAT_LIVENESS - m_liveness > 2) { 
               // Lost of one ping is not problem
               DUSTLOG_WARN(m_logname, "no activity in last " << m_heartbeat << "ms");
            }
            m_liveness--;
            if (m_liveness == 0) {
               DUSTLOG_ERROR(m_logname, "Missed " << HEARTBEAT_LIVENESS << " heartbeats. Disconnected from server - retrying...");
               boost::this_thread::sleep(boost::posix_time::milliseconds(m_reconnect));
               connectToServer(m_server);
            }
         }
         // Send HEARTBEAT if it's time
         if (TIME_NOW() > m_heartbeat_at) {
            sendToServer(RPC_HEARTBEAT, "", NULL);
            m_heartbeat_at = TIME_NOW() + msec_t(m_heartbeat);
         }
      }
      catch (const zmq::error_t& ex) {
         DUSTLOG_ERROR(m_logname, "poll error [" << ex.num() << "]: " << ex.what());
         switch (ex.num()) {
         case ETERM:
            // if the context was terminated, we can't do anything, so exit 
            m_done = true;
            break;
         case EINTR:
            // under gdb we see this for an interrupted system call
            break;
         default:
            // retry
            break;
         }
      }
   }
   m_worker->close();
   DUSTLOG_INFO(m_logname, "Worker finished: " << m_identity);
}

common::UserPrivilege RpcWorker::getCommandPrivilege(rpc_cmdid_t cmdId)
{
   if (m_cmdPrivileges != NULL) {
      cmdprivlist_t::const_iterator ii = m_cmdPrivileges->find(cmdId);
      if (ii != m_cmdPrivileges->end())
         return ii->second;
   }
   return m_defaultCmdPrivilege;
}

/**
 * Log the received command with its human-readable name
 */
void RpcWorker::logCmdReceived(rpc_cmdid_t cmdId) {
   DUSTLOG_TRACE(m_logname, "Received command " << cmdCodeToStr(cmdId)
                 << ": cmdid=" << (int)cmdId);
}

/**
 * Convert cmdId to string
 * \param cmdId Command id
 * \return command id as string
 */
const string RpcWorker::cmdCodeToStr(rpc_cmdid_t cmdId) {
   return to_string(cmdId);
}
