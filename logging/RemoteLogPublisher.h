/*
 * Copyright (c) 2013, Linear Technology. All rights reserved.
 */

#pragma once

#include <log4cxx/level.h>
#include <log4cxx/appenderskeleton.h>
#include <zmq.hpp>
#include <boost/thread.hpp>
#include <fstream>
#include "rpc/public/zmqUtils.h"
#include "logevent.pb.h"


// Conversion functions for log levels
logevent::LogLevel convertLogLevel(const log4cxx::LevelPtr& aLevel);
log4cxx::LevelPtr convertLogLevel(const logevent::LogLevel& aLevel);


class RemoteLogger : public log4cxx::AppenderSkeleton
{
   static RemoteLogger* remoteLogger;
   
public:
   static RemoteLogger* create(zmq::context_t* ctx, std::string remoteLogAddr);
   
   static void shutdown();

   RemoteLogger(zmq::context_t* ctx, std::string remoteLogAddr);
   virtual ~RemoteLogger();

   // TODO: declare log4cxx object wrappers
   // used for objectptr ref counting and configuration
   // DECLARE_LOG4CXX_OBJECT(RemoteLogPublisher)
   // BEGIN_LOG4CXX_CAST_MAP()
   //   LOG4CXX_CAST_ENTRY(RemoteLogPublisher)
   //   LOG4CXX_CAST_ENTRY_CHAIN(AppenderSkeleton)
   // END_LOG4CXX_CAST_MAP()


public:
   // AppenderSkeleton methods
   
   virtual void close();

   virtual bool requiresLayout() const { return false; }
   
protected:
   // AppenderSkeleton methods

   virtual void append(const log4cxx::spi::LoggingEventPtr& event,
                       log4cxx::helpers::Pool& p);

private:
   zmq::context_t*   m_ctx;
   std::string       m_remoteLogAddr;

   std::list<zmqUtils::zmessage*>   m_logMsgQueue;
   boost::thread                  * m_queueThread;
   boost::mutex                     m_queueLock;
   boost::condition_variable        m_queueSignal;
   bool                             m_queueRunning;

   void queueThreadFun_p();
   void stopThread_p();
};
