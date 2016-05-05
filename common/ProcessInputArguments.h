/*
 * Copyright (c) 2015, Linear Technology. All rights reserved.
 */

#pragma once

/**
 * \file ProcessInputArguments.h
 * Common utility functions for processing common command line arguments
 */

#include "common.h"
#include "logging/Logger.h"
#include <exception>
#include <boost/program_options.hpp>
#include <string>

const apiproto_t        DEFAULT_API_PROTO         = APIPROTO_IPC;
const uint16_t          DEFAULT_API_BASE_PORT     = 9000;
const char              DEFAULT_API_HOST[]        = "127.0.0.1";
const char              DEFAULT_API_IPCPATH_DIR[] = "var/run";
const char              DEFAULT_LOG_DIR[]         = "logs";
const char              DEFAULT_CONF_DIR[]        = "conf";
const log4cxx::LevelPtr DEFAULT_LOG_LEVEL         = Logger::INFO_LEVEL;   

/**
 * Handle common command line options
 */
class CProcessInputArguments 
{
public:
   struct values_s {
      apiproto_t        apiProto;
      std::string       apiIpcpath;
      std::string       apiHost;
      uint16_t          apiPort;
      std::string       wdName;
      std::string       logName;
      std::string       confName;
      log4cxx::LevelPtr logLevel;
   };

   CProcessInputArguments(const std::string& baseName , const char * hdr = nullptr);
   virtual ~CProcessInputArguments() {;}
   values_s& getDefVal() { return m_defVal; }
   bool parse(int argc, char* argv[], bool isWdClnt = true, bool isHost = false);
   const values_s& getVal() const { return m_val; }
   std::string toString() const;

protected:
   virtual void add_options(boost::program_options::options_description& options) {;}
   virtual void process(boost::program_options::variables_map &vm) { ; }

   values_s    m_defVal;
   values_s    m_val;
   std::string m_hdr;

   apiproto_t  str2proto_p(std::string s) const;
};
