#pragma once
#include "common/common.h"
#include <string>
#include <zmq.hpp>

class service;

class IRpcDispatcher {
public:
   /**
    * The RpcDispatcherBase is initialized with the ZMQ context. 
    * 
    * \param context ZMQ context
    */
   static IRpcDispatcher * createRpcDispatcher(zmq::context_t* context, const std::string secretKey="");
   
   virtual ~IRpcDispatcher() {;}

   /**
    * Bind the RpcDispatcherBase to a ZMQ endpoint. This endpoint is used for
    * communication with both clients and workers, identified by the
    * protocol. The bind method can be called multiple times to listen on
    * different endpoints.
    *
    * \param endpoint ZMQ endpoint
    */
   virtual void bind(std::string endpoint) = 0;

   /** Get the number of registered workers
    * \return the number of registered workers
    */
   virtual size_t getNumWorkers() const = 0;

   /**
    * Main loop to receive and process messages
    */
   virtual void run() = 0;

   /**
    * Stop the main loop
    */
   virtual void cancel() = 0;

   //  ---------------------------------------------------------------------
   //  Locate or create new service entry
   virtual service* getService(std::string name) = 0;

   virtual service* findService(std::string name) = 0;

   virtual bool isAuthorizationCheck() const = 0;
};
