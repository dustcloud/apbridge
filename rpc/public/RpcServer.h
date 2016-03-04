/*
 * Copyright (c) 2013 Linear Technology. All rights reserved.
 */

#pragma once

#include <string>
#include <map>
#include <vector>
#include <stdarg.h>

#include "zmq.hpp"
#include "zmqUtils.h"

#include <boost/thread.hpp>
#include "rpc/public/IRpcDispatcher.h"

class RpcWorker;

/**
 * RpcServer combines the Dispatcher and LogWorker instances into a single
 * object that can manage one or more RpcWorker services.
 *
 * Workers are added with \ref addWorker. After all workers are registered,
 * the RpcServer is started with \ref start to allow it to start processing
 * client input messages.
 *
 * \attention The current implementation does not support adding workers after
 * the server is started. 
 * 
 */
class RpcServer
{
public:
   /**
    * RpcServer is initialized with a ZMQ context and the server's address
    * 
    * \param context    ZMQ context
    * \param serverAddr ZMQ server endpoint
    * \param secretKey  Secret key
    */
   RpcServer(zmq::context_t* context,
             std::string serverAddr,
             const std::string secretKey="");
   
   ~RpcServer();

   /**
    * Start the dispatcher and worker threads
    */
   void start();

   /**
    * Stop the dispatcher and worker threads
    */
   void stop();

   /**
    * Add new worker
    * \attention workers must be added before the RpcServer is started.
    * \param worker pointer to the RpcWorker child instance
    */
   void addWorker(RpcWorker* worker, bool isManaged = false);
   
private:
   // Structure for managing worker objects
   struct WorkerData {
      RpcWorker* m_worker;
      boost::thread* m_workerThread;
      bool m_isManaged;

      WorkerData(RpcWorker* aWorker, bool aIsManaged)
         : m_worker(aWorker),
           m_workerThread(nullptr),
           m_isManaged(aIsManaged)
      { ; }
   };
   
   std::string m_serverAddr;
   IRpcDispatcher* m_dispatch;   
   boost::thread m_dispatchThread;
   std::vector<WorkerData> m_workers;
};
