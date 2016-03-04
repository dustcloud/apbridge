/*
 * Copyright (c) 2013, Linear Technology. All rights reserved.
 */

#include "APCRpcWorker.h"

#include <iostream>
#include <sstream>

#include "rpc/public/RpcCommon.h"
#include "rpc/public/ConvertDelayStat.h"
#include "rpc/apc.pb.h"


//#include "public/IAPCClient.h" // TODO: really need pointer to APCoupler
#include "APCoupler.h"

#include "common/Version.h"

using namespace zmqUtils;

std::string Id;

const char APC_LOGGER[] = "apc";
const unsigned int  AP_RESPONSE_TIMEOUT = 20000; //milliseconds wait time. TODO: From config or CLI ?

CAPCRpcWorker::CAPCRpcWorker(zmq::context_t* ctx,
                             std::string svrAddr, 
                             std::string identity,
                             CAPCoupler* apCoupler,
                             IAPCClient  * pApcClient,
                             CSerialPort * pSerPort)
   : RpcWorker(ctx, svrAddr, toString(APC_RPC_SERVICE, true),
               NULL, common::NONE,
               identity, APC_LOGGER, true),
     m_apcApi(apCoupler),
     m_apcClient(pApcClient),
     m_serPort(pSerPort),
     m_isRespReceived(false)
{
	DUSTLOG_INFO(m_logname, "CAPCRpcWorker ID: " << identity);
	Id = identity;
   ;
}

/*
 * Process commands from the client 
 */
zmessage* CAPCRpcWorker::processCmdMsg(uint8_t cmdId, const std::string& params)
{
   zmessage* responseMsg;
   DUSTLOG_DEBUG(m_logname, "processCmdMsg is processing command..." << int(cmdId));
   // process the command
   logCmdReceived(cmdId);
   switch (cmdId) {
      case apc::GET_APC_INFO:
         responseMsg = handleAPCInfo(params);
         break;
      case apc::GET_AP_INFO:
         responseMsg = handleAPInfo(params);
         break;
      case apc::GET_APC_STATS:
         responseMsg = handleGetStats(params);
         break;
      case apc::CLEAR_APC_STATS:
         responseMsg = handleClearStats(params);
         break;
      case apc::AP_RESET:
         responseMsg = handleResetAP(params);
         break;
      case apc::AP_API:
         responseMsg = handleAP_API(params);
         break;
      default:
         DUSTLOG_ERROR(m_logname.c_str(), 
                       "Invalid RPC command: " << (int)cmdId);
         responseMsg = createResponse(cmdId, RPC_INVALID_COMMAND, "");
         break;
   }

   return responseMsg;
}

zmessage* CAPCRpcWorker::handleAPCInfo(std::string requestStr)
{
   apc::APCInfoReq request;
   parseFromString_p(request, requestStr);

   SAPCInfo apcInfo = m_apcApi->getAPCInfo();
   gps_info_t gpsInfo = m_apcApi->getGpsInfo();

   apc::APCInfoResp response;
   // TODO: get info
   response.set_clientid(Id);
   response.set_managerstate(m_apcApi->getClientState());
   response.set_managerhost(apcInfo.m_host);
   response.set_managerport(apcInfo.m_port);
   response.set_gpsstate(m_apcApi->getGpsStatus());
   response.set_gpssatsused(gpsInfo.satellites_used);
   response.set_gpssatsvisible(gpsInfo.satellites_visible);
   response.set_apcversion(getVersionLabel());

   return createResponse(apc::GET_APC_INFO, response);
}

zmessage* CAPCRpcWorker::handleAPInfo(std::string requestStr)
{
   apc::APInfoReq request;
   parseFromString_p(request, requestStr);

   apc::APInfoResp response;

   if (m_apcApi->isAPConnected()) {
      SAPMInfo apInfo = m_apcApi->getAPMInfo();
      response.set_macaddr(apInfo.mac, sizeof(apInfo.mac));
      response.set_clksource(apInfo.clksource);
      response.set_networkid(apInfo.networkId);
   
      // version is major (int8u) + minor (int8u) + patch (int8u) + build (int16U)
      // the build is in network order (Big Endian).
      uint16_t build = (apInfo.appVersion[3] << 8) + apInfo.appVersion[4];
      std::ostringstream os;
      os << int(apInfo.appVersion[0]) << "." <<
      	     int(apInfo.appVersion[1]) << "." <<
      	     int(apInfo.appVersion[2]) << "." << build;
      response.set_appver(os.str());
      response.set_apstate(apInfo.apState);
      return createResponse(apc::GET_AP_INFO, response);
   }
   
   return createResponse(apc::GET_AP_INFO, RPC_OK, "");
}

zmessage* CAPCRpcWorker::handleGetStats(std::string requestStr)
{
   apc::APCStatsReq request;
   parseFromString_p(request, requestStr);

   apc::APCStatsResp response;  
   bool managerConnected = FALSE;
   bool apConnected = FALSE;
   
   managerConnected = (m_apcApi->getClientState() == APCCLIENT_STATE_ONLINE);
   apConnected = m_apcApi->isAPConnected();

   if (managerConnected) {
      statdelays_s  apcc_stats = m_apcApi->getAPCCStats();
      response.set_mgrqueuelen(m_apcApi->getMgrCachedPkts());
      response.set_mgrpktsent((uint32_t)apcc_stats.m_numEvents);
      response.set_mgrpktrecv(m_apcApi->getNumRcvPkt());
      common::DelayStat * pStat = response.mutable_tomngr();
      statdelays_s apcStat = m_apcClient->getAPCCStats();
      convertDelayStat(apcStat, pStat);
   }

   if (apConnected) {
      CAPMTransport::APMTStats apm_stats = m_apcApi->getAPMStats();
      response.set_apqueuelen(m_apcApi->getAPMQueueSize());
      response.set_apminresp((uint32_t)apm_stats.m_minRspTime);
      response.set_apmaxresp((uint32_t)apm_stats.m_maxRspTime);
      if (apm_stats.m_numRespRecv > 0) { 
         response.set_apavgresp((uint32_t)(apm_stats.m_totalRspTime / apm_stats.m_numRespRecv));
      } else {
         response.set_apavgresp(0);
      }
      response.set_appktssent(apm_stats.m_numPktsSent);
      response.set_appktsrecv(apm_stats.m_numPktsRecv);
      response.set_apretriessent(apm_stats.m_numRetriesSent);
      response.set_apresprecv(apm_stats.m_numRespRecv);
      response.set_apnackrecv(apm_stats.m_numNacks);
      response.set_apmaxnackcount(apm_stats.m_maxNackCount);
      response.set_apretriesrecv(apm_stats.m_numRetriesRecv);
      response.set_apmintimeinqueue((uint32_t)apm_stats.m_minTimeInQueue);
      response.set_apmaxtimeinqueue((uint32_t)apm_stats.m_maxTimeInQueue);
      if (apm_stats.m_numPacketsQueued > 0) {
         response.set_apavgtimeinqueue((uint32_t)(apm_stats.m_totalTimeInQueue / apm_stats.m_numPacketsQueued));
      } else {
         response.set_apavgtimeinqueue(0);
      }
      common::DelayStat * pStatTo   = response.mutable_toap();
      convertDelayStat(m_serPort->getStatToAP(), pStatTo);
      common::DelayStat * pStatFrom = response.mutable_fromap();
      convertDelayStat(m_serPort->getStatFromAP(), pStatFrom);
      response.set_numoutbuffers(m_serPort->getNumOutBuffers());
	  response.set_apcurrpktrate(apm_stats.m_packetRate);
	  response.set_ap30secpktrate((double)apm_stats.m_30secPacketRate);
	  response.set_ap5minpktrate((double)apm_stats.m_5minPacketRate);
	  response.set_apavgpktrate((double)(apm_stats.m_numPktsSent * 1.0 / apm_stats.m_totalSecs));
   }
   
   return createResponse(apc::GET_APC_STATS, response);
}


zmessage* CAPCRpcWorker::handleClearStats(std::string requestStr)
{
   m_apcApi->clearAPMStats();
   m_apcApi->clearMgrStats();
   	
   return createResponse(apc::CLEAR_APC_STATS, RPC_OK, "");
}


zmessage* CAPCRpcWorker::handleResetAP(std::string requestStr)
{
   m_apcApi->resetAP();   
   return createResponse(apc::AP_RESET, RPC_OK, "");
}
void CAPCRpcWorker::handleAP_APIResponse(uint8_t cmdId, const uint8_t* resBuffer,size_t size)
{
   DUSTLOG_DEBUG(m_logname.c_str(), "handleAP_APIResponse Payload Length : " << size << "Response :" << resBuffer );
   //store the response of the last command
   m_apAPIResponse = std::vector<uint8_t>(resBuffer, resBuffer+size);

   //signal that response is ready
   boost::mutex::scoped_lock lock(m_mutex);
	m_isRespReceived = true;
	m_condv.notify_all();

 }
zmessage* CAPCRpcWorker::handleAP_API(std::string requestStr)
{
   apc::AP_APIReq request;
   parseFromString_p(request, requestStr);

   if(request.payload().size() > 0 && m_apcApi->isAPConnected())
   {
      //Register callback for the response
      ResponseCallback resCallback = boost::bind(&CAPCRpcWorker::handleAP_APIResponse, this, _1, _2, _3);

      m_isRespReceived = false;
      m_apAPIResponse.clear(); //clear buffer to store the response

      m_apcApi->sendCmd(request.cmdid(),
                        (uint8_t*)request.payload().data(),
                        request.payload().size(),
                        resCallback); 

      //wait for response 
      boost::system_time const timeout = boost::get_system_time()+boost::posix_time::milliseconds(AP_RESPONSE_TIMEOUT);
      boost::mutex::scoped_lock lock(m_mutex);
     
      while (!m_isRespReceived) {
		   if(!m_condv.timed_wait(lock, timeout))
            break;
	   }

      if(!m_apAPIResponse.empty())
      {
         //copy response
         apc::AP_APIResp response;
         response.set_cmdid(request.cmdid());
         response.set_response(m_apAPIResponse.data(),  m_apAPIResponse.size());
         
         DUSTLOG_DEBUG(m_logname.c_str(), "APCRpcWorker - Response =" << m_apAPIResponse.data() << "  m_apAPIResponse Len : " << m_apAPIResponse.size());
         
         return createResponse(apc::AP_API,response);
      }
 
   }

   return createResponse(apc::AP_API, RPC_OK, "");
}
const std::string CAPCRpcWorker::cmdCodeToStr(uint8_t cmdcode)
{
   return apc::APCCommandType_Name((apc::APCCommandType)cmdcode);
}

