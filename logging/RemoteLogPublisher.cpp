/*
 * Copyright (c) 2013, Linear Technology. All rights reserved.
 */

#include "common/common.h"
#include "RemoteLogPublisher.h"

#include <boost/assert.hpp>
#include "logevent.pb.h"
#include "rpc/public/SerializerUtility.h"

using namespace zmqUtils;

extern bool changeIPCFilePrivelege(const std::string& serverAddr, std::string * pError);

logevent::LogLevel convertLogLevel(const log4cxx::LevelPtr& aLevel)
{
   logevent::LogLevel result;
   
   if (aLevel != log4cxx::LevelPtr()) {
      switch (aLevel->toInt()) {
         case log4cxx::Level::FATAL_INT:
            result = logevent::L_FATAL;
            break;
         case log4cxx::Level::ERROR_INT:
            result = logevent::L_ERROR;
            break;
         case log4cxx::Level::WARN_INT:
            result = logevent::L_WARN;
            break;
         case log4cxx::Level::INFO_INT:
            result = logevent::L_INFO;
            break;
         case log4cxx::Level::DEBUG_INT:
            result = logevent::L_DEBUG;
            break;
         case log4cxx::Level::TRACE_INT:
            result = logevent::L_TRACE;
            break;
         default:
            BOOST_ASSERT_MSG(0, "Incorrect log level");
            break;
      }
   }else{
      result = logevent::L_DEBUG;
   }
   
   return result;
}

log4cxx::LevelPtr convertLogLevel(const logevent::LogLevel& aLevel)
{
   log4cxx::LevelPtr result = log4cxx::Level::getDebug();
   
   switch (aLevel) {
      case logevent::L_FATAL:
         result = log4cxx::Level::getFatal();
         break;
      case logevent::L_ERROR:
         result = log4cxx::Level::getError();
         break;
      case logevent::L_WARN:
         result = log4cxx::Level::getWarn();
         break;
      case logevent::L_INFO:
         result = log4cxx::Level::getInfo();
         break;
      case logevent::L_DEBUG:
         result = log4cxx::Level::getDebug();
         break;
      case logevent::L_TRACE:
         result = log4cxx::Level::getTrace();
         break;
      default:
         BOOST_ASSERT_MSG(0, "Incorrect log level");
         break;
   }
   return result;
}

static
void serializeLogEvent(const log4cxx::spi::LoggingEventPtr& event,
                       zmessage& zevent)
{
   logevent::LogEvent pb_le;
   pb_le.set_logger(event->getLoggerName());
   pb_le.set_msg(event->getMessage());
   pb_le.set_timestamp(event->getTimeStamp());
   pb_le.set_loglevel(convertLogLevel(event->getLevel()));

   std::ostringstream location;
   const char* filename = event->getLocationInformation().getFileName();
   int linenum = event->getLocationInformation().getLineNumber();
   if (filename != NULL) {
      location << filename << ":" << linenum
               << " (" << event->getLocationInformation().getMethodName() << ")";
   }
   pb_le.set_location(location.str());
   
   // serialize the log event protobuf message into the notification
   serialize2zmsg(pb_le, zevent);
   
   // allow subscriber filtering based on logger name
   zevent.pushstr(event->getLoggerName().c_str());
}


RemoteLogger* RemoteLogger::remoteLogger = NULL;

RemoteLogger* RemoteLogger::create(zmq::context_t* ctx, std::string remoteLogAddr)
{
   // there should only be one RemoteLogger per process
   if (remoteLogger == NULL) {
      remoteLogger = new RemoteLogger(ctx, remoteLogAddr);
      
      log4cxx::AppenderPtr appender(remoteLogger);
      log4cxx::Logger::getRootLogger()->addAppender(appender);
   }
   return remoteLogger;
}

void RemoteLogger::shutdown() 
{
   if (remoteLogger != NULL) {
      log4cxx::Logger::getRootLogger()->removeAppender(remoteLogger);
      // note: remoteLogger is released (destroyed) on removal
      // so it *should* automatically close the publisher socket
   }
}
   

RemoteLogger::RemoteLogger(zmq::context_t* ctx, std::string remoteLogAddr)
   : log4cxx::AppenderSkeleton(),
     m_ctx(ctx),   
     m_remoteLogAddr(remoteLogAddr),
     m_queueThread(NULL),
     m_queueRunning(true)
{
   m_queueThread = new boost::thread(boost::bind(&RemoteLogger::queueThreadFun_p, this));
}
   
RemoteLogger::~RemoteLogger()
{
   stopThread_p();
}

void RemoteLogger::append(const log4cxx::spi::LoggingEventPtr& event,
                          log4cxx::helpers::Pool& p)
{
   // construct a message from the event
   zmessage * pLognotif = new zmessage();

   serializeLogEvent(event, *pLognotif);
   
   {  // Put message to Notification Queue
      boost::unique_lock<boost::mutex> lock(m_queueLock);
      if (!m_queueRunning) {  // Drop message. Output thread is closed
         delete pLognotif;
         return;
      }

      bool isEmpty = m_logMsgQueue.empty();
      m_logMsgQueue.push_front(pLognotif);
      if (isEmpty)
         m_queueSignal.notify_all();
   }
}

void RemoteLogger::close() 
{
   stopThread_p();
}

void RemoteLogger::stopThread_p()
{
   {  // STOP Notification Queue
      boost::unique_lock<boost::mutex> lock(m_queueLock);
      m_queueRunning = false;
      m_queueSignal.notify_all();
   }

   // Stop Queue Thread
   if (m_queueThread) {
      m_queueThread->join();
      delete m_queueThread;
      m_queueThread = NULL;
   }
}

void RemoteLogger::queueThreadFun_p()
{
   zmq::socket_t*    socket = new zmq::socket_t(*m_ctx, ZMQ_PUB);
   socket->bind(m_remoteLogAddr.c_str());
   std::string errMsg;
   changeIPCFilePrivelege(m_remoteLogAddr, &errMsg);

   zmessage * pMsg;
   for(;;) {
      {
         boost::unique_lock<boost::mutex> lock(m_queueLock);
         while(m_queueRunning && m_logMsgQueue.empty())
            m_queueSignal.wait(lock);
         if (!m_queueRunning)    // Stop process of queue
            break;
         pMsg = m_logMsgQueue.back();
         m_logMsgQueue.pop_back();
      }
      if (pMsg == NULL)
         continue;
      pMsg->send(socket);

      delete pMsg;
      pMsg = NULL;
   }

   delete socket;

   {  // Clean queue
      boost::unique_lock<boost::mutex> lock(m_queueLock);
      for_each(m_logMsgQueue.begin(), m_logMsgQueue.end(), [](zmqUtils::zmessage * pMsg) -> void { if (pMsg) delete pMsg; });
      m_logMsgQueue.clear();
   }

}

