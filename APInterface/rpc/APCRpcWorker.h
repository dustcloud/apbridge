/*
 * Copyright (c) 2014, Linear Technology.  All rights reserved.
 */

#pragma once

#include <zmq.hpp>
#include <string>
#include "rpc/public/RpcWorker.h"

//#include "manager/common/mngr_error.h"
//#include "rpc/public/RpcCommon.h"
#include "APInterface/public/GPSError.h"
#include "6lowpan/public/dn_api_net.h"

#include "public/IAPCClient.h"
#include "APInterface/SerialPort.h"

class CAPCoupler;

/**
 * \class CAPCRpcWorker
 * 
 * \brief RPC worker for APC
 * 
 */
class CAPCRpcWorker : public RpcWorker
{
public:
   CAPCRpcWorker(zmq::context_t* ctx, std::string svrAddr, 
                 std::string identity, std::string confName, 
                 CAPCoupler* apCoupler, 
                 IAPCClient  * pApcClient,
                 CSerialPort * pSerPort);
    
   virtual ~CAPCRpcWorker() {

   }
   
   /**
    * Process commands from the client 
    */
   virtual zmessage* processCmdMsg(uint8_t cmdId, const std::string& params);

protected:   
   /**
    * Convert cmdcode to string from enumeration commands
    * 
    * \param cmdcode Command Code
    * \return command code as string
    */
   virtual const std::string cmdCodeToStr(uint8_t cmdcode);

   /**
   * Callback AP_API command response
   */
   void handleAP_APIResponse(uint8_t cmdId, const uint8_t* resBuffer,size_t size);

private:
   
   /**
    * Process APC Info
    */ 
   zmessage* handleAPCInfo(std::string requestStr);

   /**
    * Process AP Info
    */
   zmessage* handleAPInfo(std::string requestStr);

   /**
    * Process APC Get Stats
    */
   zmessage* handleGetStats(std::string requestStr);

   /**
    * Process APC Clear Stats
    */
   zmessage* handleClearStats(std::string requestStr);

   /**
    * Process Reset AP
    */
   zmessage* handleResetAP(std::string requestStr);

   /**
    * Process AP_API
    */
   zmessage* handleAP_API(std::string requestStr);

   /**
    * Process Set_AP_ClkSrc
    */
   zmessage* handleSetAPClkSrc(std::string requestStr);

   /**
    * Process Get_AP_ClkSrc
    */
   zmessage* handleGetAPClkSrc(std::string requestStr);

   /**
    * Convert from Enum to APM definition
    */
   const uint8_t ClkSourceToInt(EAPClockSource apClkSrc);
   
   CAPCoupler  * m_apcApi;
   IAPCClient  * m_apcClient;
   CSerialPort * m_serPort;
   // Response of AP_API command
   std::vector<uint8_t> m_apAPIResponse;
   
   //signal
   bool m_isRespReceived;
   boost::mutex  m_mutex;
   boost::condition_variable  m_condv;

};
