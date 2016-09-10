/*
 * Copyright (c) 2015, Linear Technology. All rights reserved.
 */

#include "ProcessInputArguments.h"
#include <boost/algorithm/string.hpp>
#include <sstream>
#include <fstream>

using namespace std;
namespace po = boost::program_options;

CProcessInputArguments::CProcessInputArguments(const string& baseName, const char * hdr) 
{
   m_defVal.apiProto    = DEFAULT_API_PROTO;
   m_defVal.apiIpcpath  = DEFAULT_API_IPCPATH_DIR;
   m_defVal.apiHost     = DEFAULT_API_HOST;
   m_defVal.apiPort     = DEFAULT_API_BASE_PORT;
   m_defVal.wdName      = "";
   m_defVal.logName     = baseName + ".log";
   m_defVal.confName    = baseName + ".conf";
   m_defVal.logLevel    = DEFAULT_LOG_LEVEL;
   if (hdr)
      m_hdr = hdr;
}

bool CProcessInputArguments::parse(int argc, char* argv[], bool isWdClnt, bool isHost)
{
   string proto,   ipcpath,
          host,    port,
          logName, logLevel,
          wdname;

   m_val = m_defVal;

   if (m_hdr.empty())
      m_hdr = "Arguments";
   po::options_description desc(m_hdr);
   desc.add_options()
   ("help", "Help message")
   ("api-proto",   po::value<string>(&proto),          "API protocol: IPC or TCP")
   ("api-ipcpath", po::value<string>(&m_val.apiIpcpath),  "Path to IPC files (only for IPC)");

   if (isHost)  {
      desc.add_options()
      ("api-host", po::value<string>(&m_val.apiHost), "RPC server address (only for TCP)");
   }

   desc.add_options()
   ("api-port",  po::value<uint16_t>(&m_val.apiPort), "Base IP port for services (only for TCP)")
   ("config-file,c", po::value<string>(&m_val.confName),  "Name of configuration file")
   ("log-file",  po::value<string>(&m_val.logName),   "Name of log file")
   ("log-level", po::value<string>(&logLevel),        "Initial default log level");

   if (isWdClnt) {
      desc.add_options()
      ("wd-name",   po::value<string>(&m_val.wdName), "Watchdog application name");
   }

   add_options(desc);

   try {
      po::variables_map vm;
      po::store(po::parse_command_line(argc, argv, desc), vm);
      po::notify(vm);

      if (vm.count("help"))
         throw po::error("");

      m_val.confName = getFullFileName(DEFAULT_CONF_DIR, m_val.confName);
      ifstream fIn(m_val.confName);
      if (fIn.good()) {
         try { // Read configuration file
            vm.clear();
            po::store(po::parse_config_file<char>(fIn, desc), vm);
            po::notify(vm);
         } catch(const std::exception& e) {
            ostringstream os;
            os << "Can not parse file: " << m_val.confName << " (" << e.what() << ")";
            throw runtime_error(os.str().c_str());
         }
         // Re-read command line for overwrite configuration-file parameters
         vm.clear();
         po::store(po::parse_command_line(argc, argv, desc), vm);
         po::notify(vm);
      }
      fIn.close();

      if (!proto.empty()) 
         m_val.apiProto = str2proto_p(proto);

      if (!logLevel.empty()) 
         m_val.logLevel = log4cxx::Level::toLevel(logLevel, m_val.logLevel);

      m_val.logName    = getFullFileName(DEFAULT_LOG_DIR, m_val.logName);

      process(vm);

   } catch (const std::exception& e) {
      if (strlen(e.what()) > 0)
         std::cerr << e.what() << std::endl;
      std::cout << desc << std::endl;
      return false;
   }

   return true;
}

string CProcessInputArguments::toString() const
{
   ostringstream os;
   if (getVal().apiProto == APIPROTO_IPC) {
      string ipcsubdir = getVal().apiIpcpath;
      if (ipcsubdir.empty())
         ipcsubdir = DEFAULT_API_IPCPATH_DIR;
      os << "API protocol: IPC. IPC directory: " << getFullFileName(ipcsubdir, " ");
   } else {
      os << "API protocol: TCP. " << getVal().apiHost << ":" << getVal().apiPort;
   }
   return os.str();
}

apiproto_t  CProcessInputArguments::str2proto_p(std::string s) const
{
   if (boost::iequals(s, "ipc"))
      return APIPROTO_IPC;
   if (boost::iequals(s, "tcp"))
      return APIPROTO_TCP;
   throw po::error(string("Unsupported protocol: ") + s);
}
