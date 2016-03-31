/*
 * Copyright (c) 2014, Linear Technology.  All rights reserved.
 */
#include "public/RpcCommon.h"

#include "common/common.h"
#include <boost/algorithm/string.hpp>

using namespace std;

// Convert the mngr_time_t to milliseconds since the epoch
int64_t mngr_time2msec(mngr_time_t t)
{
   sys_time_t sTime = time_mngr2sys(t);
   int64_t msecs = TIME_D2MSEC(sTime.time_since_epoch());
   return msecs;
}

// Convert System time to milliseconds since the epoch
int64_t systime2msec(const sys_time_t& utc)
{
   return TIME_D2MSEC(utc.time_since_epoch());
}

// Return the current time in milliseconds since the epoch
int64_t getCurrentTime()
{
   return systime2msec(SYSTIME_NOW());
}

// Return the current time in seconds since the epoch
int64_t getCurrentTimeSecs()
{
   return TIME_D2SEC(SYSTIME_NOW().time_since_epoch());
}

CApiEndpointBuilder::CApiEndpointBuilder() : m_proto(APIPROTO_NA), m_basePort(0) {;}

CApiEndpointBuilder::CApiEndpointBuilder(const CProcessInputArguments& cmdArgs)
{
   init(cmdArgs);
}

void CApiEndpointBuilder::init(apiproto_t proto, const std::string& path,
                               const std::string& host, uint16_t basePort)
{
   m_proto    = proto;
   m_path     = path;
   m_basePort = basePort;
   m_host     = host;
   if (m_host.empty())
      m_host = "127.0.0.1";

   #ifdef WIN32
      m_proto = APIPROTO_TCP;
   #endif
}

void CApiEndpointBuilder::init(const CProcessInputArguments& cmdArgs)
{
   const CProcessInputArguments::values_s& argValues = cmdArgs.getVal();
   init(argValues.apiProto, argValues.apiIpcpath,
        argValues.apiHost, argValues.apiPort);
}

std::string CApiEndpointBuilder::getClient(RpcEndpoints endpointId, const string endpointExt) const
{
   string   ep, ipcName;
   uint16_t portOffset = 0;
   parseEndpointExt_p(endpointExt, &ipcName, &portOffset);

   if (m_proto == APIPROTO_IPC) {
      ep = getIPCname_p(endpointId, ipcName);
   } else if (m_proto == APIPROTO_TCP) {
      ostringstream os;
      os << "tcp://" << m_host << ":" << (m_basePort + (uint16_t)endpointId + portOffset);
      ep = os.str();
   }
   return ep;
}

std::string CApiEndpointBuilder::getServer(RpcEndpoints endpointId, const string endpointExt) const
{
   string   ep, ipcName;
   uint16_t portOffset = 0;
   parseEndpointExt_p(endpointExt, &ipcName, &portOffset);

   if (m_proto == APIPROTO_IPC) {
      ep = getIPCname_p(endpointId, ipcName);
   } else if (m_proto == APIPROTO_TCP) {
      ep = "tcp://*:"; 
      ep.append(to_string(m_basePort + (uint16_t)endpointId + portOffset));
   }
   return ep;
}

string CApiEndpointBuilder::getIPCname_p(RpcEndpoints endpointId, const string& ipcName) const
{
   static const string POSTFIX("://");
   
   string ipcStr = toString(endpointId, true);
   size_t pos = ipcStr.find(POSTFIX);
   if (pos == string::npos)
      pos = 0;
   else
      pos += POSTFIX.length();
   string fileName = ipcStr.substr(pos);
   if (!ipcName.empty()) {
      size_t pos1 = fileName.rfind(".");
      if (pos1 == string::npos) {
         fileName.append("_").append(ipcName);
      } else {
         fileName = fileName.substr(0, pos1) + "_" + ipcName + fileName.substr(pos1);
      }

   }
   return ipcStr.substr(0, pos) + getFullFileName(m_path, fileName);
}

// MaxResults value for ListReq
static const uint32_t DEFAULT_MAX_SIZE = 1000;

static uint32_t max_results_v = DEFAULT_MAX_SIZE;
uint32_t getMaxSizeListResp() {
   return max_results_v;
}
void  setMaxSizeListResp(uint32_t val) {
   if (val == 0)
      val = DEFAULT_MAX_SIZE;
   max_results_v = val;
}

void CApiEndpointBuilder::parseEndpointExt_p(const string& endpointExt, string * pIpcName, uint16_t * pPortOffset) const
{
   *pPortOffset = 0;
   if (endpointExt.empty())
      return;
   string::size_type pos = endpointExt.find(":");
   *pIpcName = endpointExt.substr(0, pos);
   if (pos != string::npos) {
      try {
         *pPortOffset = (uint16_t)stoi(endpointExt.substr(pos+1));
      } catch(...){;}
   }
}

