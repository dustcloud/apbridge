/*
 * Copyright (c) 2013, Linear Technology. All rights reserved.
 */

#include "Logger.h"

#include <iostream>
#include <iomanip>
#include <sstream>
#include <google/protobuf/stubs/common.h>

#include "log4cxx/basicconfigurator.h"
#include "log4cxx/propertyconfigurator.h"
#include "log4cxx/helpers/exception.h"
#include "log4cxx/helpers/properties.h"
#include <log4cxx/logmanager.h>

#include "RemoteLogPublisher.h"


struct ConfigProperty 
{
   const char* key;
   const char* value;
};

ConfigProperty DEFAULT_CONFIGURATION[] = {
   { "log4j.rootLogger", "INFO, FileLog" },
   { "log4j.appender.ConsoleLog", "org.apache.log4j.ConsoleAppender" },
   { "log4j.appender.ConsoleLog.layout", "org.apache.log4j.PatternLayout" },
   { "log4j.appender.ConsoleLog.layout.ConversionPattern", "%d %-5p %c - %m%n" },
   // Log to file
   { "log4j.appender.FileLog", "org.apache.log4j.RollingFileAppender" },
   { "log4j.appender.FileLog.MaxFileSize", "20MB" },
   { "log4j.appender.FileLog.MaxBackupIndex", "5" },
   { "log4j.appender.FileLog.layout", "org.apache.log4j.PatternLayout" },
   { "log4j.appender.FileLog.layout.ConversionPattern", "%d %-5p %c - %m%n" },
   { NULL, NULL },
};

static void defaultConfiguration(const char* logFile)
{
   log4cxx::helpers::Properties props;
   for (size_t i = 0; DEFAULT_CONFIGURATION[i].key != NULL; i++) {
      const ConfigProperty& cp = DEFAULT_CONFIGURATION[i];
      props.setProperty(cp.key, cp.value);
   }
   // special case to set log file
   props.setProperty("log4j.appender.FileLog.File", logFile);
   
   log4cxx::PropertyConfigurator::configure(props);
}

// Initialize the log system for this process
void Logger::openLogging(const std::string& logFile)
{
   defaultConfiguration(logFile.c_str());
}

// Free logger resources
void Logger::closeLogging()
{
   google::protobuf::ShutdownProtobufLibrary();
}

// Initialize the log system for this process
void Logger::initLogging()
{
   defaultConfiguration(DEFAULT_LOG_FILE);
}

// Initialize the remote log publishing for this process
void Logger::initPublisher(zmq::context_t* ctx,
                           std::string remoteLog,
                           log4cxx::LevelPtr threshold)
{
   RemoteLogger* rl = RemoteLogger::create(ctx, remoteLog);
   rl->setThreshold(threshold);
}

// Initialize the log system for this process
void Logger::closePublisher()
{
   RemoteLogger::shutdown();
}


// Read global logging configuration from a file
void Logger::readConfigFile(const std::string& confFile)
{
   log4cxx::PropertyConfigurator::configure(confFile.c_str());
}

// Detailed Trace-level log of buffer contents
void Logger::logDumpData(Logger::ILogger logger, log4cxx::LevelPtr level, 
                         const std::string& prefix, const unsigned char* buffer, size_t length)
{
   // short circuit generating the message if the Trace level is not enabled
   if (!isEnabled(logger, level))
      return;

   std::ostringstream os;
   const char byteSeparator = ' ';
   const char lineSeparator[] = "\n  ";
   const int lineLen = 20;

   // print the prefix string
   os << prefix << ":" << lineSeparator; // TODO: add length

   // dump the buffer bytes as hex
   os << std::setfill('0') << std::hex;
   for (size_t i = 0; i < length; i++) {
      if (i > 0 && i % lineLen == 0) {
         os << lineSeparator;
      }
      os << std::setw(2) << (int)buffer[i];
      if (byteSeparator != '\0') os << byteSeparator;
   }
      
   LOG4CXX_TRACE(logger, os.str());
}

void Logger::setLevelForAll(log4cxx::LevelPtr level)
{
   log4cxx::LoggerList loggers = log4cxx::LogManager::getCurrentLoggers();
   for (auto logger : loggers) {
       logger->setLevel(level);
   }
}

std::ostream& Logger::printLoggers(std::ostream& os)
{
   log4cxx::LoggerList loggers = log4cxx::LogManager::getCurrentLoggers();
   for (auto logger : loggers) {
       os << logger->getName() << std::endl;
   }

   return os;
}

bool Logger::isLogger(const std::string& name)
{
   log4cxx::LoggerList loggers = log4cxx::LogManager::getCurrentLoggers();
   for (auto logger : loggers) {
       if(name == logger->getName())
          return true;
   }
   return false;
}


CLoggerUnittestLabel::CLoggerUnittestLabel(const char * logName, const char * label) {
   if (label) {
      m_label = label;
   }
   if (logName) {
      m_logName = logName;
      DUSTLOG_INFO(m_logName, "------------ START:  " << m_label.c_str() << "------------------");
   }
   std::cout << "------------ START:  " << m_label.c_str() << "------------------" << std::endl;
}
CLoggerUnittestLabel::~CLoggerUnittestLabel() {
   if (!m_logName.empty()) {
      DUSTLOG_INFO(m_logName, "------------ FINISH: " << m_label.c_str() << "------------------");
   }
   std::cout << "------------ FINISH: " << m_label.c_str() << "------------------" << std::endl;
}

