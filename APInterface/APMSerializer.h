/*
 * Copyright (c) 2014, Linear Technology. All rights reserved.
 */

#pragma once

#include "6lowpan/public/dn_api_common.h"
#include "6lowpan/public/dn_api_local.h"

#include "Logger.h"
#include "public/APCError.h"

#include "IAPCoupler.h" // for IAPMNotifHandler

class IAPMCmdHandler
{
public:
   virtual ~IAPMCmdHandler() {
      ;
   }
   virtual apc_error_t handleCmd(uint8_t cmdId, const uint8_t* data, size_t size) = 0;
   virtual void        handleError(uint8_t cmdId, uint8_t rc) = 0;
};

/**
 *
 * The APM Serializer is initialized with a IAPMNotifHandler. The Serializer
 * provides callbacks to the APMNotifHandler with parsed notifications.
 * 
 * The APM Serializer implements the CmdHandler interface to receive input
 * from the Transport layer.
 *
 */
class CAPMSerializer : public IAPMCmdHandler
{
public:
   enum converttype_t // "# CAPMSerializer"
   {
      HOST_TO_NET,
      NET_TO_HOST,
   };
   
   static apc_error_t convert(converttype_t convertType, uint8_t cmdId,
                              uint8_t * payload, size_t size);
   static apc_error_t prepMsg(uint8_t cmdId, const uint8_t * payload, size_t size, 
                              uint8_t * msg, size_t maxSize, size_t * pMsgSize);

   CAPMSerializer(size_t maxMsgSize, IAPMNotifHandler* notifHandler);
   virtual ~CAPMSerializer();

   // Parse the input command payload and notify the IAPMNotifHander
   virtual apc_error_t handleCmd(uint8_t cmdId, const uint8_t* data, size_t length);
   virtual void        handleError(uint8_t cmdId, uint8_t rc);
   
   // TODO: convert to network byte order
   // TODO: convert to host byte order

   int sendJoin();
   int sendAPSend(const dn_api_loc_apsend_ctrl_t& ctrl, const uint8_t* data, size_t length);
   int sendReset();

private:
   IAPMNotifHandler* m_notifHandler;

   std::vector<uint8_t>  m_inputBuffer;
   size_t                m_bytesReceived;
   size_t                m_bytesExpected;
   uint16_t              m_bootCounter;

private:
  void handleGetParam(const uint8_t* data, size_t length);
 

};
