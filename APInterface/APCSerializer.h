#pragma once
#include "common.h"
#include "APCProto.h"
#include "public/APCError.h"
#include <string>
#include <vector>

class ISerRxHandler 
{
public:
   virtual ~ISerRxHandler() {;}
   virtual apc_error_t messageReceived(ap_intf_id_t apcId, apc_msg_type_t type, uint8_t flags, 
                                       uint32_t mySeq, uint32_t yourSeq,
                                       const uint8_t * pPayload, uint16_t size) = 0;
};

class CAPCSerializer 
{
public:
   CAPCSerializer(size_t maxMsgSize, ISerRxHandler * pRxHndlr, const char * logName);
   ~CAPCSerializer();
   apc_error_t convert(converttype_t convertType, apc_msg_type_t msgType, uint8_t * payload, size_t size);
   apc_error_t prepMsg(uint8_t * msg, size_t maxSize, size_t * pMsgSize, ap_intf_id_t intId,
                       apc_msg_type_t type, uint8_t flags, uint32_t mySeq, uint32_t yourSeq,
                       const uint8_t * payload1, size_t size1, 
                       const uint8_t * payload2, size_t size2);

   apc_error_t dataReceived(ap_intf_id_t apcId, const uint8_t * data, size_t size);
private:
   ISerRxHandler    * m_pRxHandler;
   std::vector<uint8_t>  m_inpBuf;
   size_t                m_inpNumReceived;
   size_t                m_inpNumExpected;
   std::string           m_logName;

   void                  trace_p(const char * title, ap_intf_id_t intfId, const apc_hdr_s * pHdr, uint8_t * payload, size_t size);
   size_t                fillInpBuf_p(const uint8_t * data, size_t size);
};