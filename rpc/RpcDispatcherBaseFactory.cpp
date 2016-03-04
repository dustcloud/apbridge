#include "RpcDispatcherBase.h"

IRpcDispatcher * IRpcDispatcher::createRpcDispatcher(zmq::context_t* context, const std::string secretKey)
{
   return new RpcDispatcherBase(context, secretKey);
}