/*
 * Copyright (c) 2013, Linear Technology. All rights reserved.
 */
#include "LogWorker.h"

#include "common.h"
#include "Logger.h"
#include "logevent.pb.h"
#include "rpc/public/RpcCommon.h"
#include "rpc/public/SerializerUtility.h"
#include "RemoteLogPublisher.h" // for log level conversion
#include <log4cxx/logmanager.h>

LogWorker::LogWorker(zmq::context_t* ctx, std::string svrAddr, bool allowUninitializedAuth)
   : RpcWorker(ctx, svrAddr, toString(LOG_RPC_SERVICE, true), NULL, common::VIEWER, LOGWORKER_IDENTITY, 
               rpclogger::RPC_WORKER_NAME, allowUninitializedAuth)
{
   ;
}

zmessage* LogWorker::processCmdMsg(rpc_cmdid_t cmdId, const std::string& params)
{
   zmessage* responseMsg = NULL;

   // TODO: handle authorization
   
   // process the command
   switch (cmdId) {
   case logevent::GET_LOGGERS:
   {
      logevent::GetLoggersResponse loggersResp;
      log4cxx::LoggerList loggers = log4cxx::LogManager::getCurrentLoggers();
      for (auto logger : loggers) {
         loggersResp.add_loggers(logger->getName());
      }
      responseMsg = createResponse(cmdId, loggersResp);
   }
   break;

   case logevent::GET_LOG_LEVEL:
   {
      logevent::GetLoggerRequest loggerReq;
      parseFromString_p(loggerReq, params);
      logevent::GetLogLevelResponse logLevelResp;

      // don't create a new logger, make sure the requested logger already exists
      log4cxx::LoggerPtr logger = log4cxx::LogManager::exists(loggerReq.logger());
      if (logger != log4cxx::LoggerPtr()) {
         // TODO: getLevel might return a NULL level (not set)
         logLevelResp.set_loglevel(convertLogLevel(logger->getLevel()));
         responseMsg = createResponse(cmdId, logLevelResp);
      } else {
         responseMsg = createResponse(cmdId, RPC_INVALID_PARAMETERS, "");
      }
   }
   break;

   case logevent::SET_LOG_LEVEL:
   {
      logevent::SetLogLevelRequest setLogLevelReq;
      parseFromString_p(setLogLevelReq, params);
      
      // don't create a new logger, make sure the requested logger already exists
      log4cxx::LoggerPtr logger = log4cxx::LogManager::exists(setLogLevelReq.logger());
      if (logger != log4cxx::LoggerPtr()) {
         Logger::setLevel(logger, convertLogLevel(setLogLevelReq.loglevel()));
         responseMsg = createResponse(cmdId, RPC_OK, "");
      } else {
         responseMsg = createResponse(cmdId, RPC_INVALID_PARAMETERS, "");
      }
   }
   break;

   default:
      responseMsg = createResponse(cmdId, RPC_INVALID_COMMAND, "");
      break;
   }

   return responseMsg;
}
