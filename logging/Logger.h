/*
 * Copyright (c) 2013, Linear Technology. All rights reserved.
 */

#ifndef Logger_H_
#define Logger_H_

#include <string>
#include <zmq.hpp>
#include <log4cxx/logger.h>

/** \file Logger.h
 * Interface to the logging system
 */

// The DUSTLOG_* macros mirror the underlying log4cxx message generators.
// We use macros here so that the message parameter isn't evaluated unless
// the log message will be output.

/**
 * Log a message using the specified logger at the specified level
 */
#define DUSTLOG_MSG(loggerName, level, message) LOG4CXX_LOG(Logger::getLog(loggerName), level, message)
/**
 * Log a TRACE-level message
 */
#define DUSTLOG_TRACE(loggerName, message) LOG4CXX_TRACE(Logger::getLog(loggerName), message)
/**
 * Log a DEBUG-level message
 */
#define DUSTLOG_DEBUG(loggerName, message) LOG4CXX_DEBUG(Logger::getLog(loggerName), message)
/**
 * Log an INFO-level message
 */
#define DUSTLOG_INFO(loggerName, message)  LOG4CXX_INFO(Logger::getLog(loggerName), message)
/**
 * Log a WARN-level message
 */
#define DUSTLOG_WARN(loggerName, message)  LOG4CXX_WARN(Logger::getLog(loggerName), message)
/**
 * Log an ERROR message
 */
#define DUSTLOG_ERROR(loggerName, message) LOG4CXX_ERROR(Logger::getLog(loggerName), message)   
/**
 * Log a FATAL message
 */
#define DUSTLOG_FATAL(loggerName, message) LOG4CXX_FATAL(Logger::getLog(loggerName), message)
/**
 * Log a TRACE-level message with the hex dump of a buffer
 */
#define DUSTLOG_TRACEDATA(loggerName, prefix, buffer, length) \
   Logger::logDumpData(Logger::getLog(loggerName), log4cxx::Level::getTrace(), prefix, buffer, length)

/**
 * Set the log level for a logger
 */
#define DUSTLOG_SETLEVEL(loggerName, loglevel) Logger::setLevel(Logger::getLog(loggerName), loglevel)
/**
 * Check whether logging is enabled at a certain level
 */
#define DUSTLOG_ISENABLED(loggerName, loglevel) Logger::isEnabled(Logger::getLog(loggerName), loglevel)

/** 
 * Functions for managing the logging system
 */
namespace Logger {

   /** Shortcuts for common log levels
    *
    * \note Originally, these constants were in a Level namespace and named
    * without the LEVEL suffix, but Windows doesn't like anything named DEBUG.
    */
   const log4cxx::LevelPtr INFO_LEVEL = log4cxx::Level::getInfo();
   const log4cxx::LevelPtr DEBUG_LEVEL = log4cxx::Level::getDebug();
   const log4cxx::LevelPtr TRACE_LEVEL = log4cxx::Level::getTrace();
   
   const char DEFAULT_LOG_FILE[] = "manager.log";

   typedef log4cxx::LoggerPtr ILogger;

   /**
    * Initialize the logging system for this process. This function must be
    * called before any log messages are generated. This function sets up
    * the default logging configuration.
    *
    * In the default configuration, log messages will be printed to the
    * console and written to the specified log file.
    *
    * \param logFile path to a file for log messages
    */
   void openLogging(const std::string& logFile = DEFAULT_LOG_FILE);

   /**
    * Closes the logging system. Free resources.
    */
   void closeLogging();

   /** Initialize the logging system.
    *
    * \attention This function is deprecated. Use openLogging and readConfigFile.
    * 
    * This function must be called before any log messages are generated. If a
    * configuration file is provided, the configuration will be applied,
    * otherwise the default configuration will be used.
    *
    * \param confFile path to a log configuration file
    */
   void initLogging();

   /**
    * Initialize the remote log publisher. This enables log and trace
    * notifications to be published to other processes using the zeromq
    * notification transport.
    *
    * \param ctx ZeroMQ context pointer
    * \param remoteLog Endpoint address for subscribers to connect to
    * \param threshold Log level threshold for publishing messages
    */
   void initPublisher(zmq::context_t* ctx, std::string remoteLog,
                      log4cxx::LevelPtr threshold = log4cxx::Level::getInfo());

   /**
    * Shut down the remote log publisher
    */
   void closePublisher();

   /**
    * Guard class for automatically initializing and closing the log publisher
    */
   class CScopedPublisher {
   public:
      /**
       * Initialize the log publisher
       * \param ctx ZeroMQ context pointer
       * \param remoteLog Endpoint address for subscribers to connect to
       * \param threshold Log level threshold for publishing messages
       */
      CScopedPublisher(zmq::context_t* ctx, std::string remoteLog,
                       log4cxx::LevelPtr threshold = log4cxx::Level::getInfo()) {
         initPublisher(ctx, remoteLog, threshold);
      }
      
      ~CScopedPublisher() {
         closePublisher();
      }
   };
   
   /**
    * Create a logger with the specified name
    * May be called any number of times to get the same logger. 
    */
   inline ILogger getLog(const std::string& logname)
   {
      return log4cxx::Logger::getLogger(logname);
   }
   
   /**
    * Read the logging configuration for this process from a file
    */
   void readConfigFile(const std::string& confFile);   

   /**
    * Sets the logging level for the specified logger
    */
   inline void setLevel(ILogger logger, log4cxx::LevelPtr level)
   {
      logger->setLevel(level);
   }

   /**
    * \return: whether a log level is enabled for output on the specified logger
    */
   inline bool isEnabled(ILogger logger, log4cxx::LevelPtr level)
   {
      return logger->isEnabledFor(level);
   }
   
   /**
    * Log a binary data (hex dump of buffer)
    */
   void logDumpData(ILogger logger, log4cxx::LevelPtr level, 
                    const std::string& prefix, const unsigned char* buffer, size_t length);
   
   /**
    * Set the same log level to all loggers
    */   
   void              setLevelForAll(log4cxx::LevelPtr level);

   /**
    * Print all loggers
    */      
   std::ostream&     printLoggers(std::ostream&);

   bool isLogger(const std::string& name);
}

/**
 * Print to 'cout' and log messages about start / stop function
 */
class CLoggerUnittestLabel
{
public:
   CLoggerUnittestLabel(const char * logName, const char * label);
   ~CLoggerUnittestLabel();
private:
   std::string m_logName;
   std::string m_label;
};


#endif /* ! Logger_H_ */
