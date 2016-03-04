/*
 * Copyright (c) 2013, Linear Technology. All rights reserved.
 */

#pragma once

#include "rpc/public/RpcWorker.h"
#include "rpc/public/zmqUtils.h"

const char LOGWORKER_IDENTITY[] = "LOGWRK";

/**
 * The LogWorker class implements basic logging control functions for the RPC 'logger' service
 *
 * A LogWorker instance is attached to all RpcServers, to manage remote logging. 
 */
class LogWorker : public RpcWorker
{
public:
   LogWorker(zmq::context_t* ctx, std::string svrAddr, bool allowUninitializedAuth = false);

   /**
    * Handle an RPC request for basic logging functions
    * 
    * \return the RPC response
    */
   virtual zmessage* processCmdMsg(rpc_cmdid_t cmdId, const std::string& params);
   
private:

};
