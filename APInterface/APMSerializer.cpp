/*
 * Copyright (c) 2014, Linear Technology. All rights reserved.
 */

#include "APMSerializer.h"
#include "APMTransport.h"
#include "logging/Logger.h"
#include "6lowpan/public/dn_api_param.h"

#include <algorithm>
#include <sstream>

const uint16_t APM_MAX_BOOT_EVENTS = 10; // continuous BOOT event means AP didn't received our BOOT ACK,
                                                // we need to reset AP to restore communication

CAPMSerializer::CAPMSerializer(size_t maxMsgSize, IAPMNotifHandler* notifHandler)
   : m_notifHandler(notifHandler),
     m_inputBuffer(maxMsgSize),
     m_bytesReceived(0),
     m_bytesExpected(sizeof(apt_hdr_s)),
     m_bootCounter(0)
{ ; }

CAPMSerializer::~CAPMSerializer() { ; }


// Function to cast binary buffer to message type. 
// Return NULLPTR if buffer size < size of type 
template <class T> 
static T * bufferCast_p(uint8_t * payload, size_t size) {
   if (size < sizeof(T))
      return nullptr;
   return (T *) payload;
}

void CAPMSerializer::handleGetParam(const uint8_t* data, size_t length)
{
   uint8_t paramId = data[1];
   uint8_t* payload = const_cast<uint8_t*>(data);  

   DUSTLOG_DEBUG("apm.io", "AP RX GetParam: paramId=" << (int)paramId);
   switch (paramId) 
   {
   case DN_API_PARAM_MACADDR:
   {
      dn_api_rsp_get_macaddr_t* pMsg = bufferCast_p<dn_api_rsp_get_macaddr_t>(payload, length);
      if (pMsg != nullptr) {
         m_notifHandler->handleParamMacAddress(*pMsg);
      } else {
         DUSTLOG_ERROR("apm.io", "getParameter<MacAddr> expected " << sizeof(dn_api_rsp_get_macaddr_t) 
		 	<< " bytes received " << length << " bytes");
      }
   }
   break;
   case DN_API_PARAM_MOTEINFO:
   {
      dn_api_rsp_get_moteinfo_t* pMsg = bufferCast_p<dn_api_rsp_get_moteinfo_t>(payload, length);
      if (pMsg != nullptr) {
         m_notifHandler->handleParamMoteInfo(*pMsg);
      } else {
         DUSTLOG_ERROR("apm.io", "getParameter<MoteInfo> expected " << sizeof(dn_api_rsp_get_moteinfo_t) 
		 	<< " bytes received " << length << " bytes");
      }
   }
   break;
   case DN_API_PARAM_APPINFO:
   {
      dn_api_rsp_get_appinfo_t* pMsg = bufferCast_p<dn_api_rsp_get_appinfo_t>(payload, length);
      if (pMsg != nullptr) {
         m_notifHandler->handleParamAppInfo(*pMsg);
      } else {
         DUSTLOG_ERROR("apm.io", "getParameter<AppInfo> expected " << sizeof(dn_api_rsp_get_appinfo_t) 
		 	<< " bytes received " << length << " bytes");
      }
   }
   break;
   case DN_API_PARAM_AP_CLKSRC:
   {
      dn_api_rsp_get_ap_clksrc_t* pMsg = bufferCast_p<dn_api_rsp_get_ap_clksrc_t>(payload, length);
      if (pMsg != nullptr) {
         m_notifHandler->handleParamClkSrc(*pMsg);
      } else {
         DUSTLOG_ERROR("apm.io", "getParameter<ClkSrc> expected " << sizeof(dn_api_rsp_get_ap_clksrc_t) 
		 	<< " bytes received " << length << " bytes");
      }
   }
   break;
   case DN_API_PARAM_NETID:
   {
      dn_api_rsp_get_netid_t* pMsg = bufferCast_p<dn_api_rsp_get_netid_t>(payload, length);
      if (pMsg != nullptr) {
         pMsg->netId = ntohs(pMsg->netId);
         m_notifHandler->handleParamNetId(*pMsg);
      } else {
         DUSTLOG_ERROR("apm.io", "getParameter<NetId> expected " << sizeof(dn_api_rsp_get_netid_t) 
		 	<< " bytes received " << length << " bytes");
      }
   }
   break;
   case DN_API_PARAM_TIME:
   {
      dn_api_rsp_get_time_t* pMsg = bufferCast_p<dn_api_rsp_get_time_t>(payload, length);
      if (pMsg != nullptr) {
         // note: asn is left as a byte array, the upper layer transforms to
         // host byte order
         pMsg->offset = ntohs(pMsg->offset);
         // TODO: upTime, utcTime are unused
         m_notifHandler->handleParamGetTime(*pMsg);
      } else {
         DUSTLOG_ERROR("apm.io", "getParameter<getTime> expected " << sizeof(dn_api_rsp_get_time_t) 
		 	<< " bytes received " << length << " bytes");
      }
   }
   break;
   case DN_API_PARAM_AP_STATUS:
   {
      dn_api_rsp_get_apstatus_t* pMsg = bufferCast_p<dn_api_rsp_get_apstatus_t>(payload, length);
      if (pMsg != nullptr) {
         m_notifHandler->handleParamApStatus(*pMsg);
      } else {
         DUSTLOG_ERROR("apm.io", "getParameter<ApStatus> expected " << sizeof(dn_api_rsp_get_apstatus_t) 
		 	<< " bytes received " << length << " bytes");
      }
   }
   break;

   //case DN_API_PARAM_TXPOWER:
   //break;
   default:
      DUSTLOG_WARN("apm.io", "Unknown paramId: " << (int)paramId);
      break;
   }
}
apc_error_t CAPMSerializer::handleCmd(uint8_t cmdId, const uint8_t* data, size_t length)
{
   uint8_t* payload = const_cast<uint8_t*>(data);
   // parse the payload

   if (cmdId != DN_API_LOC_NOTIF_EVENTS) {
       // we only count continuous BOOT event
       m_bootCounter = 0;
   }
   switch (cmdId) {
   case DN_API_LOC_NOTIF_EVENTS:
   {
      DUSTLOG_DEBUG("apm.io", "received event notif");
      dn_api_loc_notif_events_t* pMsg = bufferCast_p<dn_api_loc_notif_events_t>(payload, length);
      if (pMsg != nullptr) {
         // byte order conversion
         pMsg->events = ntohl(pMsg->events);
         pMsg->alarms = ntohl(pMsg->alarms);
         if (pMsg->events & DN_API_LOC_EV_BOOT) {
            m_bootCounter++;
            if (m_bootCounter > APM_MAX_BOOT_EVENTS) {
                m_bootCounter = 0;
                m_notifHandler->handleAPLost();
            } else {
                m_notifHandler->handleAPBoot();
            }
         } else {
            m_bootCounter = 0;
            m_notifHandler->handleEvent(*pMsg);
         }
      } else {
         DUSTLOG_ERROR("apm.io", "bad event notif");
      }
   }
   break;
      
   case DN_API_LOC_NOTIF_TXDONE:
   {
      DUSTLOG_TRACE("apm.io", "received TX done");
      dn_api_loc_notif_txdone_t* pMsg = bufferCast_p<dn_api_loc_notif_txdone_t>(payload, length);
      if (pMsg != nullptr) {
         // byte order conversion
         pMsg->packetId = ntohs(pMsg->packetId);
         m_notifHandler->handleTXDone(*pMsg);
      } else {
         DUSTLOG_ERROR("apm.io", "bad TX done notif");
      }
   }
   break;
      
   case DN_API_LOC_NOTIF_AP_RECEIVE:
   {
      DUSTLOG_TRACE("apm.io", "received AP data");
      // TODO: is there a minimum length?
      m_notifHandler->handleAPReceive(payload, length);
   }
   break;

   case DN_API_LOC_CMD_GETPARAM:
   {
      //DUSTLOG_TRACE("apm.io", "received AP get param response");
      handleGetParam(payload, length);
   }
   break;

   case DN_API_LOC_NOTIF_TIME:
   {
      DUSTLOG_TRACE("apm.io", "received AP time indication");
      dn_api_loc_notif_time_t* pMsg = bufferCast_p<dn_api_loc_notif_time_t>(payload, length);
      if (pMsg != nullptr) {
         // TODO: byte order conversion
         m_notifHandler->handleTimeIndication(*pMsg);
      } else {
         DUSTLOG_ERROR("apm.io", "bad Time indication notif");
      }
   }
   break;

   case DN_API_LOC_NOTIF_READY_FOR_TIME:
   {
      DUSTLOG_DEBUG("apm.io", "received AP ready for time");
      dn_api_loc_notif_ready_for_time_t* pMsg = bufferCast_p<dn_api_loc_notif_ready_for_time_t>(payload, length);
      if (pMsg != nullptr) {
         m_notifHandler->handleReadyForTime(*pMsg);
      } else {
         DUSTLOG_ERROR("apm.io", "bad Ready for time notif");
      }
   }
   break;

   case DN_API_LOC_CMD_WRITE_GPS_STATUS:
   {
      if (data[0] != DN_API_RC_OK) {
         DUSTLOG_ERROR("apm.io", "APC TX writeGpsStatus failed: rc=" << (int)data[0]);
      }
   }
   break;
   
   default:
      break;
   }
   return APC_OK;
}

void CAPMSerializer::handleError(uint8_t cmdId, uint8_t rc)
{
   m_notifHandler->handleError(cmdId, rc);
}

apc_error_t CAPMSerializer::prepMsg(uint8_t cmdId, const uint8_t * payload, size_t size, 
                                    uint8_t * msg, size_t maxSize, size_t * pMsgSize)
{
   *pMsgSize = size + sizeof(apt_hdr_s);
   if (*pMsgSize > maxSize)
      return APC_ERR_SIZE;

   // TODO: fill in apt_hdr_s
   
   apt_hdr_s * pHdr = (apt_hdr_s *)msg;
   pHdr->cmdId = cmdId;
   pHdr->length = size & 0xFF; // TODO: htons((uint16_t) size);
   // TODO pHdr->flags = 0;
   
   if (size > 0) {
      BOOST_ASSERT(payload != NULL);
      memcpy((uint8_t *)(pHdr+1), payload, size);
   }
   apc_error_t res = convert(HOST_TO_NET, cmdId, (uint8_t *)(pHdr+1), size);
   return res;
}

apc_error_t
CAPMSerializer::convert(converttype_t convertType, uint8_t cmdId,
                        uint8_t* payload, size_t size)
{
   apc_error_t res = APC_OK;
   //switch (cmdId) {
      // TODO: cast to message structure
      // TODO: handle conversion
      // case APC_NET_TX: 
      //    {
      //       apc_msg_net_tx_s * pMsg = bufferCast_p<apc_msg_net_tx_s>(payload, size);
      //       if (pMsg == nullptr) {
      //          res = APC_ERR_SIZE;
      //          break;
      //       }
      //       pMsg->txDoneId = CONVERT_S(convertType, pMsg->txDoneId);
      //       pMsg->slot     = CONVERT_L(convertType, pMsg->txDoneId);
      //       break;
      //    }
   //default:
   //   break;
   //}
   return res;
}

