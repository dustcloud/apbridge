/*
 * Copyright (c) 2013, Linear Technology. All rights reserved.
 */

#include "public/RpcServer.h"

#include "public/RpcCommon.h"
#include "public/RpcWorker.h"
#include "logging/LogWorker.h"
#include "Logger.h"

extern bool changeIPCFilePrivilege(const std::string& serverAddr, std::string * pError);

/**
 * RpcServer
 */
RpcServer::RpcServer(zmq::context_t* context,
                     std::string serverAddr,
                     const std::string secretKey)
   : m_serverAddr(serverAddr),
     m_dispatch(IRpcDispatcher::createRpcDispatcher(context, secretKey))
{
   std::string errMsg;
   m_dispatch->bind(serverAddr);
   if (!changeIPCFilePrivilege(serverAddr, & errMsg)) {
      DUSTLOG_ERROR(rpclogger::RPC_SERVER_NAME, "Can not change privilege of IPC address '" 
                    << serverAddr << "': " << errMsg);
   }
   // create the log worker
   addWorker(new LogWorker(context, serverAddr, !m_dispatch->isAuthorizationCheck()), true);
}

// TODO: perhaps better to use boost::thread_group for m_workerThreadList
// it's made for managing a collection of threads

void RpcServer::start()
{
   // start the worker threads
   for (auto& worker : m_workers) {
      worker.m_workerThread = new boost::thread(&RpcWorker::run, worker.m_worker);
   }
   
#if 0
   // start the worker threads
   for (rpc_worker_list_i iter = m_workerList.begin();
        iter != m_workerList.end(); iter++) {
      m_workerThreadList.push_back(boost::thread(&RpcWorker::run, (*iter)));
   }

   m_logworkerThread = boost::thread(&RpcWorker::run, m_logworker);
#endif
   
   // start the main dispatcher thread
   m_dispatchThread = boost::thread(&IRpcDispatcher::run, m_dispatch);
   DUSTLOG_DEBUG(rpclogger::RPC_SERVER_NAME, "Started server and worker threads");
   
   // TODO: wait until all workers are registered?

   // The workers send registration messages to the server immediately on
   // connect, but the dispatcher thread does not process the messages until
   // it's started. The dispatcher will queue client requests for a service
   // and process them after a worker has registered.

   // TODO: does the dispatcher queue cause a state mismatch if a client
   // abandons a request by timeout ???
}

void RpcServer::stop()
{
   // cancel worker run loops
   for (auto& worker : m_workers) {
      worker.m_worker->cancel();
   }
   // join worker threads
   for (auto& worker : m_workers) {
      if (worker.m_workerThread != nullptr) {
         worker.m_workerThread->join();
         delete worker.m_workerThread;
         worker.m_workerThread = nullptr;
      }
   }

#if 0   
   // cancel worker run loops
   for (rpc_worker_list_i iter = m_workerList.begin();
       iter != m_workerList.end(); iter++) {
      (*iter)->cancel();
   }
   m_logworker->cancel();

   // make sure the worker threads are joined
   for (rpc_wThread_list_i iter = m_workerThreadList.begin();
       iter != m_workerThreadList.end(); iter++) {
      iter->join();
   }
   m_logworkerThread.join();
#endif
   
   // clean up the dispatcher last
   m_dispatch->cancel();
   m_dispatchThread.join(); 

   DUSTLOG_DEBUG(rpclogger::RPC_SERVER_NAME, "Stopped server and worker threads");
}

void RpcServer::addWorker(RpcWorker* worker, bool isManaged)
{
#if 0
   // set up connection
   m_workerList.push_back(worker);
#endif
   // register the service with the dispatcher
   // this allows clients to connect to the server before the worker has completed its registration
   m_dispatch->getService(worker->getService());
   
   worker->connectToServer(m_serverAddr);
   // TODO: handle error case
   m_workers.push_back(WorkerData(worker, isManaged));
}

RpcServer::~RpcServer()
{
   stop(); // just in case   
   delete m_dispatch;
   // delete managed RpcWorker object(s)
   for (auto& worker : m_workers) {
      if (worker.m_isManaged) {
         delete worker.m_worker;
      }
   }
   // caller is responsible for deleting the unmanaged RpcWorker object(s)
}
