#include "RpcDispatcherBase.h"
#include "Logger.h"
#include "rpc/utils/public/rpc_logger.h"

class service;
class RpcDispatcherBase;

using namespace zmqUtils;

// Internal class for a worker connection
class worker {
   friend class RpcDispatcherBase;
public:
   // Destroy worker object, called when worker is removed from
   // server->workers.
   virtual ~worker() { ; }

   // Return true if worker has expired and must be deleted
   bool isExpired() {
      mngr_time_t now = TIME_NOW();
      if (m_expiration < now) {
         m_logExp = 20; // log expiration messages for a short time
      }
      if (m_logExp > 0) {
         DUSTLOG_DEBUG(rpclogger::RPC_SERVER_NAME, "isExpired: detected expiration now=" << now << ", exp=" << m_expiration);
         m_logExp--;
      }
      return m_expiration < now;
   }

   // advance the current expiration time
   void setExpired(int64_t offset) {
      mngr_time_t now = TIME_NOW();
      m_expiration = now + msec_t(offset);
      if (m_logExp > 0) {
         DUSTLOG_DEBUG(rpclogger::RPC_SERVER_NAME, "setExpired: updating expiration now=" << now << ", offset=" << offset);
         m_logExp--;
      }
   }
   
private:
   std::string m_identity;   // Name of worker
   service * m_service;      // Owning service, if known

   mngr_time_t m_expiration; // Expiration time unless a heartbeat is received
   int m_logExp;   // flag to log expiration messages
   
   // Constructor is private, only used from server
   worker(std::string identity, service * service = 0) {
      m_identity = identity;
      m_service = service;
      m_expiration = TIME_EMPTY;
      m_logExp = 0;
   }
};

typedef std::vector<worker*> Workers;
typedef Workers::iterator WorkerIter;

// Internal class for a service definition
class service : public i_service {
public:
   friend class RpcDispatcherBase;

   virtual const std::string& getName() const {
      return m_name;
   }

   // Destroy service object, called when service is removed from
   // server->services.
   virtual ~service() {
      for (size_t i = 0; i < m_requests.size(); i++) {
         delete m_requests[i];
      }
      m_requests.clear();
      for (size_t i = 0; i < m_waiting.size(); i++) {
         delete m_waiting[i];
      }
      m_waiting.clear();
   }
   
private:
   std::string m_name;             // Service name
   std::vector<zmessage*> m_requests;  // List of client requests
   Workers m_waiting;              // List of waiting workers
   size_t m_numWorkers;            // How many workers we have
   
   service(std::string name) : m_name(name), m_numWorkers(0) {}
};


// ---------------------------------------------------------------------
// RpcDispatcherBase implementation

void RpcDispatcherBase::bind(std::string endpoint)
{
   m_endpoint = endpoint;
   m_socket->bind(m_endpoint.c_str());
   DUSTLOG_INFO(rpclogger::RPC_SERVER_NAME, "RpcDispatcherBase is active at " << endpoint);
}

RpcDispatcherBase::RpcDispatcherBase(zmq::context_t* context,
                             const std::string secretKey)
   : m_context(context),
     m_done(false)
{
   //  Initialize RpcDispatcherBase state
   m_socket = new zmq::socket_t(*m_context, ZMQ_ROUTER);
   setExpired(HEARTBEAT_INTERVAL);
}

RpcDispatcherBase::~RpcDispatcherBase()   
{
   m_waiting.clear();
   
   // delete worker and service instances -- each service deletes its list of workers
   for (auto& serviceEntry : m_services) {
      delete serviceEntry.second;
   }
   
   m_services.clear();
   m_workers.clear();
   
   m_socket->close();
   delete m_socket;
}


// ---------------------------------------------------------------------
// Detect / delete any idle workers that haven't pinged us in a while.

void RpcDispatcherBase::purge_workers()
{
#ifdef DELETE_EXPIRED_WORKERS
   worker * wrk = m_waiting.size()>0 ? m_waiting.front() : 0;
   while (wrk) {
      if (!wrk->isExpired()) {
         break;              // Worker is alive, we're done here
      }
      
      DUSTLOG_INFO(rpclogger::RPC_SERVER_NAME, "Deleting expired worker: " << wrk->m_identity);
      worker_delete (wrk, 0);
      wrk = m_waiting.size()>0 ? m_waiting.front() : 0;
   }
#else
   // only check for expiration, don't delete
   for (worker* wrk : m_waiting) {
      if (!wrk->isExpired()) {
         break;              // Worker is alive, we're done here
      }
   }
#endif
}

// ---------------------------------------------------------------------
// Locate or create new service entry

// Find or create a service entry
service* RpcDispatcherBase::getService(std::string name)
{
   assert(name.size() > 0);
   service* srv = findService(name);
   if (!srv) {
      service * srv = new service(name);
      m_services.insert(std::make_pair(name, srv));
      DUSTLOG_DEBUG(rpclogger::RPC_SERVER_NAME, "Created service: " << name);
      return srv;      
   } else {
      return srv;
   }
}

// Find a service entry
// Returns NULL if not found
service* RpcDispatcherBase::findService(std::string name)
{
   assert(name.size() > 0);   
   if (m_services.count(name)) {
      return m_services.at(name);
   } else {
      return NULL;
   }
}

// ---------------------------------------------------------------------
// Dispatch requests to waiting workers as possible
void RpcDispatcherBase::service_dispatch(i_service *i_srv, zmessage *msg)
{
   service * srv = static_cast<service *>(i_srv);
   assert (srv);

   // Queue message if any and the command signature is valid
   if (msg) {
      // The RPC msg structure:
      // Frame 1 Client identifier
      // Frame 2 [Empty]
      // Frame 3 protocol command id
      // Frame 4 parameters
      srv->m_requests.push_back(msg);
   }

   purge_workers();
   DUSTLOG_DEBUG(rpclogger::RPC_SERVER_NAME, "Checking request queue for " << srv->m_name
                 << ", num workers=" << srv->m_waiting.size()
                 << ", num requests=" << srv->m_requests.size());
   while (srv->m_waiting.size()>0 && srv->m_requests.size()>0)
   {
      worker *wrk = srv->m_waiting.size() ? srv->m_waiting.front() : 0;
      assert (wrk);
      srv->m_waiting.erase(srv->m_waiting.begin());
      zmessage *msg = srv->m_requests.size() ? srv->m_requests.front() : 0;
      srv->m_requests.erase(srv->m_requests.begin());
      worker_send(wrk, RPC_REQUEST, msg);
      for (WorkerIter it = m_waiting.begin(); it != m_waiting.end(); ) {
          (*it == wrk) ? ( it = m_waiting.erase(it) ) : (it++);
      }
   }
}

// ---------------------------------------------------------------------
// Handle internal service

void RpcDispatcherBase::service_internal(std::string service_name, zmessage *msg)
{
#if 0  // TODO: support internal services
   if (service_name.compare("mmi.service") == 0) {
      service * srv = m_services.at(msg->body());
      if (srv && srv->m_numWorkers) {
         msg->body_set("200");
      } else {
         msg->body_set("404");
      }
   } else {
      msg->body_set("501");
   }
#endif
   //  Remove & save client return envelope and insert the
   //  protocol header and service name, then rewrap envelope.
   zframe_t* frame = msg->unwrap();
   char *client = (char*)zframe_data(frame);
   msg->wrap(RPC_CLIENT, service_name.c_str()); // TODO
   msg->wrap(client, "");
   zframe_destroy(&frame);
       
   msg->send(m_socket);
}


// ---------------------------------------------------------------------
// Find or create a worker connection

worker * RpcDispatcherBase::worker_require(std::string identity)
{
   assert (identity.length()!=0);

   // self->workers is keyed off worker identity
   if (m_workers.count(identity)) {
      return m_workers.at(identity);
   } else {
      worker *wrk = new worker(identity);
      m_workers.insert(std::make_pair(identity, wrk));
      DUSTLOG_DEBUG(rpclogger::RPC_SERVER_NAME, "Creating new worker: " << identity);
      return wrk;
   }
}

// ---------------------------------------------------------------------
// Deletes worker from all data structures, and destroys worker

void RpcDispatcherBase::worker_delete(worker *&wrk, int disconnect)
{
   assert (wrk);
   if (disconnect) {
      worker_send(wrk, RPC_DISCONNECT, NULL);
   }

   if (wrk->m_service) {
      for (std::vector<worker*>::iterator it = wrk->m_service->m_waiting.begin();
          it != wrk->m_service->m_waiting.end(); ) {
            (*it == wrk) ? (it = wrk->m_service->m_waiting.erase(it)) : (it++);
      }
      wrk->m_service->m_numWorkers--;
   }
   for (std::vector<worker*>::iterator it = m_waiting.begin(); it != m_waiting.end(); ) {
      (*it == wrk) ? (it = m_waiting.erase(it)) : (++it);
   }
   DUSTLOG_DEBUG(rpclogger::RPC_SERVER_NAME, "Deleting worker: " << wrk->m_identity);
   m_workers.erase(wrk->m_identity);
   delete wrk;
}

// ---------------------------------------------------------------------
// Process message sent to us by a worker

void RpcDispatcherBase::worker_process(std::string sender, zmessage *msg)
{
   assert (msg && msg->size() >= 1);     //  At least, command

   zframe_t* frame = msg->pop();
   assert (zframe_size(frame) == 1);
   int command = zframe_data(frame)[0];
   zframe_destroy(&frame);

   bool worker_ready = m_workers.count(sender)>0;
   worker *wrk = worker_require(sender);

   switch (command) {
   case RPC_REGISTER:
      DUSTLOG_MSGDEBUG(rpclogger::RPC_SERVER_NAME, "received worker registration: ", msg);
      
      if (worker_ready)  {              // Not first command in session
         worker_delete(wrk, 1);
      }
      else {
         // Attach worker to service and mark as idle
         std::string service_name = msg->popstr();
         DUSTLOG_INFO(rpclogger::RPC_SERVER_NAME, "Registering worker " << sender << " for service "
                      << service_name);
         wrk->m_service = getService(service_name);
         wrk->m_service->m_numWorkers++;
         worker_waiting(wrk);
      }
      break;

   case RPC_REPLY:
      if (worker_ready) {
         // Remove & save client return envelope and insert the
         // protocol header and service name, then rewrap envelope.

         zframe_t* frame = msg->unwrap();
         std::string client((char*)zframe_data(frame), zframe_size(frame));
         msg->pushstr(wrk->m_service->m_name); // TODO
         msg->pushstr(RPC_CLIENT);
         msg->wrap(client, "");

         DUSTLOG_MSGTRACE(rpclogger::RPC_SERVER_NAME, "sending reply to " << client << ": ", msg);
         
         msg->send(m_socket);
         worker_waiting(wrk);
         zframe_destroy(&frame);
      }
      else {
         worker_delete(wrk, 1);
      }    
      break;

   case RPC_HEARTBEAT:
      if (worker_ready) {
         wrk->setExpired(HEARTBEAT_EXPIRY);
      } else {
         worker_delete(wrk, 1);
      }
      break;

   case RPC_DISCONNECT:
      worker_delete(wrk, 0);
      break;

      // TODO case RPC_NOTIFY:
      // break
          
   default:
      DUSTLOG_ERROR(rpclogger::RPC_SERVER_NAME, "unknown command in input: " << (int)command);
      break;
   }
       
   delete msg;
}

// ---------------------------------------------------------------------
// Send message to worker
// If pointer to message is provided, sends that message

void RpcDispatcherBase::worker_send(worker *worker, RpcCommands command, zmessage *msg)
{
   if (msg == NULL) {
      msg = new zmessage();
   }
   
   // Push protocol envelope to start of message
   // Frame 1: "RPCWxy" (6 bytes, RPC/Worker x.y)
   // Frame 2: Protocol Command ID (1 byte)
   
   zframe_t* frame = zframe_new(&command, 1);
   msg->push(frame);
   msg->pushstr(RPC_WORKER);
   // Push worker ID to start of message for zeromq routing
   msg->wrap(worker->m_identity, "");
   //DUSTLOG_MSGTRACE(rpclogger::RPC_SERVER_NAME, "sending msg to worker: ", msg);
   int result = msg->send(m_socket);
   if (result < 0) {
      DUSTLOG_ERROR(rpclogger::RPC_SERVER_NAME, "error sending msg (" << toString(command) << ") to worker: err=" << errno);
   }
   delete msg;
}

// ---------------------------------------------------------------------
// This worker is now waiting for work

void RpcDispatcherBase::worker_waiting(worker *worker)
{
   assert (worker);
   // Queue to server and service waiting lists
   m_waiting.push_back(worker);
   worker->m_service->m_waiting.push_back(worker);
   worker->setExpired(HEARTBEAT_EXPIRY);
   service_dispatch(worker->m_service, 0);
}

/**
 * Create and send error response
 */
void RpcDispatcherBase::sendErrorResponse(zmessage*& reqMsg, 
                                      const std::string srvId,
                                      int32_t errorCode,
                                      std::string errorMsg)
{
   // The RPC response msg structure:
   // 1 Client identifier
   // 2 protocol string
   // 3 service identifier
   // 4 procotol command id
   // 5 parameters
   // 6 command signature

   zmessage* responseMsg = new zmessage();
   
   // Get client identifier
   zframe_t* frame = reqMsg->unwrap();
   std::string client((char*)zframe_data(frame), zframe_size(frame));
   zframe_destroy(&frame);
   
   // Get cmdId
   // TODO: should we just re-use the cmdid frame?
   frame = reqMsg->next();
   rpc_cmdid_t cmdId = zframe_data(frame)[0];
   zframe_t* cmdFrame = zframe_new(&cmdId, 1);
   responseMsg->push(cmdFrame);
   
   // Add RPC error code and string
   responseMsg->addint(errorCode);
   responseMsg->addstr(errorMsg);
 
   // Response must contain the requested service name and RPC Client id
   // because the client checks the service before processing the response
   responseMsg->pushstr(srvId);
   
   // Add RPC Client protocol string and client identifier
   responseMsg->pushstr(RPC_CLIENT);
   responseMsg->wrap(client, "");  

   DUSTLOG_MSGDEBUG(rpclogger::RPC_SERVER_NAME, "sending error reply to " << 
                    client, responseMsg);

   responseMsg->send(m_socket);

   delete reqMsg;
   reqMsg = nullptr;
   delete responseMsg;
}

// ---------------------------------------------------------------------
// Process a request coming from a client

void RpcDispatcherBase::client_process(std::string sender, zmessage *msg)
{
   assert (msg && msg->size() >= 2); // Service name + body
   DUSTLOG_MSGTRACE(rpclogger::RPC_SERVER_NAME, "received client request: ", msg);
   
   std::string service_name = msg->popstr();
   service *srv = findService(service_name);
   // Set reply return address to client sender
   msg->wrap(sender, "");

   // If there is no known service, return an error
   if (srv == NULL) {
      DUSTLOG_ERROR(rpclogger::RPC_SERVER_NAME,
                    service_name << " service is not available");
      // Send an error response to the client
      sendErrorResponse(msg, service_name, RPC_INVALID_SERVICE_ID, "");
      return;
   } else {
      // Otherwise, dispatch the message -- there may not be any workers ready
      // yet if this is an early message, but there will be
      service_dispatch(srv, msg);
   }
}

// Get and process messages forever or until interrupted
void RpcDispatcherBase::run() 
{
   DUSTLOG_INFO(rpclogger::RPC_SERVER_NAME, "Dispatcher starting");
   while (!m_done) {
      try {
         zmq::pollitem_t items [] = {
            { *m_socket,  0, ZMQ_POLLIN, 0 } };
         zmq::poll (items, 1, HEARTBEAT_INTERVAL);

         // Process next input message, if any
         if (items[0].revents & ZMQ_POLLIN) {
            zmessage *msg = new zmessage(m_socket);
            // we don't log here to avoid heartbeat noise
            //DUSTLOG_MSGTRACE(rpclogger::RPC_SERVER_NAME, "received request: ", msg);
            std::string sender = msg->popstr();
            msg->popstr(); // empty message
            std::string header = msg->popstr();

            if (header.compare(RPC_CLIENT) == 0) {
               client_process(sender, msg);
            }
            else if (header.compare(RPC_WORKER) == 0) {
               worker_process(sender, msg);
            }
            else {
               DUSTLOG_ERROR(rpclogger::RPC_SERVER_NAME, "invalid message: unknown header: " << header);
               delete msg;
            }
         }
         // Disconnect and delete any expired workers
         // Send heartbeats to idle workers if needed
         if (TIME_NOW() > m_heartbeat_at) {
            purge_workers();
            for (WorkerIter it = m_waiting.begin(); it != m_waiting.end() && (*it)!=0; it++) {
               worker_send(*it, RPC_HEARTBEAT, NULL);
            }
            setExpired(HEARTBEAT_INTERVAL);
         }

      }
      catch (const zmq::error_t& ex) {
         DUSTLOG_ERROR(rpclogger::RPC_SERVER_NAME, "poll error [" << ex.num() << "]: " << ex.what());
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
   DUSTLOG_INFO(rpclogger::RPC_SERVER_NAME, "Dispatcher finished");
}



