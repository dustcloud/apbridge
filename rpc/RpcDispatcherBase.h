#pragma once
#include "rpc/public/IRpcDispatcher.h"
#include "rpc/public/RpcProtocol.h"
#include "rpc/public/zmqUtils.h"
#include <string>

class worker;

#define HEARTBEAT_LIVENESS  3       // 3-5 is reasonable
#define HEARTBEAT_INTERVAL  2500    // msecs
#define HEARTBEAT_EXPIRY    HEARTBEAT_INTERVAL * HEARTBEAT_LIVENESS

class i_service {
public:
   virtual ~i_service(){;}
   virtual const std::string& getName() const = 0;
};

class RpcDispatcherBase : public  IRpcDispatcher {
   typedef std::map<std::string, service*> ServiceMap;
   typedef std::map<std::string, worker*>  WorkerMap;
   
public:
   virtual ~RpcDispatcherBase();

   /**
    * Bind the RpcDispatcherBase to a ZMQ endpoint. This endpoint is used for
    * communication with both clients and workers, identified by the
    * protocol. The bind method can be called multiple times to listen on
    * different endpoints.
    *
    * \param endpoint ZMQ endpoint
    */
   virtual void bind(std::string endpoint);

   /** Get the number of registered workers
    * \return the number of registered workers
    */
   virtual size_t getNumWorkers() const { return m_workers.size(); }

   /**
    * Main loop to receive and process messages
    */
   virtual void run();

   /**
    * Stop the main loop
    */
   virtual void cancel() { m_done = true; }

   //  ---------------------------------------------------------------------
   //  Locate or create new service entry
   virtual service* getService(std::string name);

   virtual service* findService(std::string name);

   virtual bool isAuthorizationCheck() const { return false; }

protected:
   friend IRpcDispatcher;

   /**
    * The RpcDispatcherBase is initialized with the ZMQ context. 
    * 
    * \param context ZMQ context
    */
   RpcDispatcherBase(zmq::context_t* context, const std::string secretKey="");

   //  ---------------------------------------------------------------------
   //  Delete any idle workers that haven't pinged us in a while.
   void purge_workers();

   //  ---------------------------------------------------------------------
   //  Dispatch requests to waiting workers as possible
   virtual void service_dispatch(i_service *srv, zmqUtils::zmessage *msg);

   //  ---------------------------------------------------------------------
   //  Handle internal service according to 8/MMI specification
   void service_internal(std::string service_name, zmqUtils::zmessage *msg);

   //  ---------------------------------------------------------------------
   //  Creates worker if necessary
   worker * worker_require(std::string identity);

   //  ---------------------------------------------------------------------
   //  Deletes worker from all data structures, and destroys worker
   void worker_delete(worker *&wrk, int disconnect);

   //  ---------------------------------------------------------------------
   //  Process message sent to us by a worker
   void worker_process(std::string sender, zmqUtils::zmessage *msg);

   //  ---------------------------------------------------------------------
   //  Send message to worker
   //  If pointer to message is provided, sends that message
   void worker_send(worker *worker, RpcCommands command, zmqUtils::zmessage *msg);

   //  ---------------------------------------------------------------------
   //  This worker is now waiting for work
   void worker_waiting(worker *worker);

   //  ---------------------------------------------------------------------
   //  Process a request coming from a client
   void client_process(std::string sender, zmqUtils::zmessage *msg);

   void setExpired(int32_t offset) 
   {
      mngr_time_t::duration dur = msec_t(offset);
      m_heartbeat_at = TIME_NOW() + dur;
   }
   
   /**
   * Create and send error response to the client. 
   * This function is used when the dispatcher detects an error, like
   * a service that doesn't exist or an invalid command signature.
   *
   * \note reqMsg is destroyed as in the other methods that send responses. 
   * 
   * \param reqMsg    Request message
   * \param srvId     Service Id
   * \param errorCode RPC Error code
   * \param errorMsg  Error message
   */
   void sendErrorResponse(zmqUtils::zmessage*& reqMsg, const std::string srvId,
                          int32_t errorCode, std::string errorMsg);
   
protected:
   zmq::context_t * m_context;                // ZMQ context
   zmq::socket_t * m_socket;                  // Socket for clients & workers
   std::string m_endpoint;                    // Dispatcher binds to this endpoint
   ServiceMap m_services;                     // Map of known services
   WorkerMap m_workers;                       // Map of known workers
   std::vector<worker*> m_waiting;            // List of waiting workers
   mngr_time_t m_heartbeat_at;                // When to send HEARTBEAT

   bool m_done;
   timestampNonceLog_t  m_timestampNonceLog;  // Timestamp and nonce pair log
};



