#include "common.h"
#include "APCSerializer.h"
#include "logging/Logger.h"
#include "common/FmtOutput.h"
#include <map>
#include <boost/assert.hpp>
#include <boost/assign/list_of.hpp> 

CAPCSerializer::CAPCSerializer(size_t maxMsgSize, ISerRxHandler * pRxHndlr, const char * logName) : 
   m_pRxHandler(pRxHndlr),
   m_inpBuf(maxMsgSize),
   m_inpNumReceived(0),
   m_inpNumExpected(sizeof(apc_hdr_s)),
   m_logName(logName)
{;}

CAPCSerializer::~CAPCSerializer()
{;}

apc_error_t CAPCSerializer::prepMsg(uint8_t * msg, size_t maxSize, size_t * pMsgSize, ap_intf_id_t intfId,
                                    apc_msg_type_t type, uint8_t flags, uint32_t mySeq, uint32_t yourSeq,
                                    const uint8_t * payload1, size_t size1, 
                                    const uint8_t * payload2, size_t size2)
{
   BOOST_ASSERT((size1 == 0 || payload1 != NULL) && (size2 == 0 || payload2 != NULL));

   size_t fullSize = size1 + size2;

   *pMsgSize = fullSize + sizeof(apc_hdr_s);
   if (*pMsgSize > maxSize)
      return APC_ERR_SIZE;

   apc_hdr_s * pHdr = (apc_hdr_s *)msg;
   pHdr->cookie = APC_COOKIE;
   pHdr->flags = flags;
   pHdr->type = type;
   pHdr->mySeq  = htonl(mySeq);
   pHdr->yourSeq = htonl(yourSeq);
   pHdr->length = htons((uint16_t) fullSize);

   if (size1 > 0) 
      memcpy((uint8_t *)(pHdr+1), payload1, size1);
   if (size2 > 0) 
      memcpy((uint8_t *)(pHdr+1) + size1, payload2, size2);

   apc_error_t res = convert(HOST_TO_NET, type, (uint8_t *)(pHdr+1), fullSize);
   trace_p("TX", intfId, pHdr, (uint8_t *)(pHdr+1), fullSize);
   return res;
}

size_t CAPCSerializer::fillInpBuf_p(const uint8_t * data, size_t size)
{
   size_t len = m_inpNumExpected - m_inpNumReceived;
   if (size < len)
      len = size;
   memcpy(m_inpBuf.data()+m_inpNumReceived, data, len);
   m_inpNumReceived += len;
   return len;
}

apc_error_t CAPCSerializer::dataReceived(ap_intf_id_t apcId, const uint8_t * data, size_t receivedLen)
{
   apc_hdr_s * pHdr = (apc_hdr_s *)m_inpBuf.data();
   size_t      processedLen;
   apc_error_t res = APC_OK;
   while(receivedLen > 0 && res == APC_OK) {
      processedLen = fillInpBuf_p(data, receivedLen);
      data += processedLen; receivedLen -= processedLen;
      if (m_inpNumReceived == sizeof(apc_hdr_s)) {
         m_inpNumExpected += ntohs(pHdr->length);
         if (pHdr->cookie != APC_COOKIE)
            return APC_ERR_PROTOCOL;
         if (m_inpNumExpected > m_inpBuf.size())
            return APC_ERR_SIZE;
      }
      if (m_inpNumExpected != m_inpNumReceived && receivedLen > 0) {
         processedLen = fillInpBuf_p(data, receivedLen);
         data += processedLen; receivedLen -= processedLen;
      }
      if (m_inpNumExpected == m_inpNumReceived) {
         trace_p("RX", apcId, pHdr, (uint8_t *)(pHdr+1), m_inpNumReceived-sizeof(apc_hdr_s));
         res = convert(NET_TO_HOST, pHdr->type, (uint8_t *)(pHdr+1), m_inpNumReceived-sizeof(apc_hdr_s));
         if (res == APC_OK)
            res = m_pRxHandler->messageReceived(apcId, pHdr->type, pHdr->flags, ntohl(pHdr->mySeq), ntohl(pHdr->yourSeq), 
                                                (uint8_t *)(pHdr+1), m_inpNumReceived-sizeof(apc_hdr_s));
         m_inpNumReceived = 0;
         m_inpNumExpected = sizeof(apc_hdr_s);
      }
   }
   return res;
}

// Function for cast binary buffer to message type. 
// Return NULLPTR if buffer size < size of type 
template <class T> 
static T * bufferCast_p(uint8_t * payload, size_t size) {
   if (size < sizeof(T))
      return nullptr;
   return (T *) payload;
}

apc_error_t CAPCSerializer::convert(converttype_t convertType, apc_msg_type_t msgType, uint8_t * payload, size_t size)
{
   apc_error_t res = APC_OK;
   switch(msgType) {
   case APC_NET_TX: 
      {
         apc_msg_net_tx_s * pMsg = bufferCast_p<apc_msg_net_tx_s>(payload, size);
         if (pMsg == nullptr) {
            res = APC_ERR_SIZE;
            break;
         }
         pMsg->txDoneId = CONVERT_S(convertType, pMsg->txDoneId);
         pMsg->slot     = CONVERT_L(convertType, pMsg->slot);
         pMsg->dst      = CONVERT_S(convertType, pMsg->dst);
         break;
      }
   case APC_NET_TXDONE:
      {
         apc_msg_net_txdone_s * pMsg = bufferCast_p<apc_msg_net_txdone_s>(payload, size);
         if (pMsg == nullptr) {
            res = APC_ERR_SIZE;
            break;
         }
         pMsg->txDoneId = CONVERT_S(convertType, pMsg->txDoneId);
         break;
      }
   case APC_TIME_MAP:
      {
         apc_msg_timemap_s * pMsg = bufferCast_p<apc_msg_timemap_s>(payload, size);
         if (pMsg == nullptr) {
            res = APC_ERR_SIZE;
            break;
         }
         pMsg->utcSeconds      = CONVERT_LL(convertType, pMsg->utcSeconds     );
         pMsg->utcMicroseconds = CONVERT_L(convertType,  pMsg->utcMicroseconds);
         pMsg->asn             = CONVERT_LL(convertType, pMsg->asn            );
         pMsg->asnOffset       = CONVERT_S(convertType,  pMsg->asnOffset      );
         break;
      }
   case APC_CONNECT:
      {
         apc_msg_connect_s * pMsg = bufferCast_p<apc_msg_connect_s>(payload, size);
         if (pMsg == nullptr) {
            res = APC_ERR_SIZE;
            break;
         }
         pMsg->flags = CONVERT_L(convertType,  pMsg->flags);
         pMsg->sesId = CONVERT_L(convertType,  pMsg->sesId);
         pMsg->netId = CONVERT_L(convertType,  pMsg->netId);
         break;
      }
   default:
      break;
   }
   return res;
}

void  CAPCSerializer::trace_p(const char * title, ap_intf_id_t intfId, const apc_hdr_s * pHdr, uint8_t * payload, size_t size)
{
   if (!DUSTLOG_ISENABLED(m_logName, Logger::TRACE_LEVEL))
      return;

   CFmtBuffer<256> fb;
   fb.printf("%s #%d %s flags=0x%x mySeq=%d yourSeq=%d ", title, intfId, toString(pHdr->type),
      pHdr->flags, ntohl(pHdr->mySeq), ntohl(pHdr->yourSeq));

   switch(pHdr->type) {
   case APC_NET_TX: 
      {
         apc_msg_net_tx_s * pMsg = bufferCast_p<apc_msg_net_tx_s>(payload, size);
         if (pMsg != nullptr) {
            fb.printf("txdoneid=%d dst=%d", ntohs(pMsg->txDoneId), ntohs(pMsg->dst));
            if (pMsg->flags & APC_NETTX_FL_USETXINFO)
               fb.printf(" frame=%d slot=%u offset=%d", pMsg->frame,
                         ntohl(pMsg->slot), pMsg->offset);
         }
         break;
      }
   case APC_NET_TXDONE:
      {
         apc_msg_net_txdone_s * pMsg = bufferCast_p<apc_msg_net_txdone_s>(payload, size);
         if (pMsg != nullptr) 
            fb.printf("txdoneid=%d", ntohs(pMsg->txDoneId));
         break;
      }
   case APC_TIME_MAP:
      {
         apc_msg_timemap_s * pMsg = bufferCast_p<apc_msg_timemap_s>(payload, size);
         if (pMsg != nullptr) 
            fb.printf("sec=%u, usec=%d asn=%d offset=%d", (uint32_t)ntohll(pMsg->utcSeconds),
               ntohl(pMsg->utcMicroseconds), (uint32_t)ntohll(pMsg->asn), ntohs(pMsg->asnOffset));
         break;
      }
   case APC_CONNECT:
      {
         apc_msg_connect_s * pMsg = bufferCast_p<apc_msg_connect_s>(payload, size);
         if (pMsg != nullptr) 
            fb.printf("flags=0x%x, sesId=%d", ntohl(pMsg->flags), ntohl(pMsg->sesId));
         break;
      }
   default:
      if (size > 0 && payload != NULL)   
         fb.printDump(payload, size, ":", "data=");
      break;
   }
   DUSTLOG_TRACE(m_logName, (const char *)fb);
}
