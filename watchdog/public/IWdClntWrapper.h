#pragma once
#include "common/common.h"
#include "IWdClient.h"
#include "rpc/public/RpcCommon.h"
#include <string>
#include <zmq.hpp>

class IWdClntWrapper {
public:
   static IWdClntWrapper * createWdClntWrapper(std::string wdName, std::string logName, 
                                               CApiEndpointBuilder& ebBuildef, zmq::context_t * pCtx);
   virtual ~IWdClntWrapper() {;}
   virtual void wait() = 0;
   virtual void finish() = 0;
   virtual IWdClient * getWdClient() = 0;
};