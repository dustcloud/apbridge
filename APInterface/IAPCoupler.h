/*
 * Copyright (c) 2014, Linear Technology. All rights reserved.
 */

#pragma once

#include "common.h"
#include "public/APCError.h"

#include "6lowpan/public/dn_api_common.h"
#include "6lowpan/public/dn_api_local.h"
#include "6lowpan/public/dn_api_param.h"
#include "public/IAPCCommon.h"
#include <boost/function.hpp>

typedef boost::function<void(uint8_t cmdId, const uint8_t* response, size_t size)> ResponseCallback;
typedef boost::function<void(uint8_t cmdId, uint8_t rc)> ErrorResponseCallback;

/**
 * AP Command Handler
 *
 * Send commands to the AP
 */
class IAPMOutputHandler
{
public:
   virtual ~IAPMOutputHandler() { ; }

   /**
    * Send a generic command and payload
    */
   virtual apc_error_t sendCmd(uint8_t cmdId, const uint8_t* data, size_t size,ResponseCallback resCallback = NULL,
                               ErrorResponseCallback errRespCallback = NULL) = 0;

   /**
    * Send the join command
    */
   virtual apc_error_t sendJoin() = 0;
   
   // TODO: define methods for each AP command
};


/**
 * AP Notif Handler
 *
 * Receive command responses and notifications from the AP
 */
class IAPMNotifHandler
{
public:
   virtual ~IAPMNotifHandler() { ; }

   // TODO: return apc_error_t 

   /**
    * Receive data from the AP
    */
   virtual void handleAPReceive(const uint8_t* data, size_t length) = 0;

   /**
    * Handle the event notification from the AP
    */
   virtual void handleEvent(const dn_api_loc_notif_events_t& event) = 0;

   /**
    * Handle the Time Indication notification from the AP
    */
   virtual void handleTimeIndication(const dn_api_loc_notif_time_t& timeMap) = 0;

   /**
    * Handle the Ready For Time notification from the AP
    */
   virtual void handleReadyForTime(const dn_api_loc_notif_ready_for_time_t& ready) = 0;

   /**
    * Handle the TX Done notification from the AP
    */
   virtual void handleTXDone(const dn_api_loc_notif_txdone_t& txDone) = 0;

   /**
    * Handle the Get Parameter Mac Address command response
    */
   virtual void handleParamMacAddress(const dn_api_rsp_get_macaddr_t& getParam) = 0;
   
   /**
    * Handle the Get Parameter Mote Info command response
    */
   virtual void handleParamMoteInfo(const dn_api_rsp_get_moteinfo_t& getMoteInfo) = 0;

   /**
    * Handle the Get Parameter App Info command response
    */
   virtual void handleParamAppInfo(const dn_api_rsp_get_appinfo_t& getAppInfo) = 0;

   /**
    * Handle the Get Parameter Clk Src command response
    */
   virtual void handleParamClkSrc(const dn_api_rsp_get_ap_clksrc_t& getClkSrc) = 0;

   /**
    * Handle the Get Parameter Net Id command response
    */
   virtual void handleParamNetId(const dn_api_rsp_get_netid_t& getNetId) = 0;

   /**
    * Handle the Get Parameter Time command response
    */
   virtual void handleParamGetTime(const dn_api_rsp_get_time_t& getTime) = 0;

   /**
    * Handle the Get Parameter Ap Status command response
    */
   virtual void handleParamApStatus(const dn_api_rsp_get_apstatus_t& getApStatus) = 0;
   
   /**
    * Handle APLost Notification
    */
   virtual void handleAPLost() = 0;

   /**
    * Handle APPause Notification
    */
   virtual void handleAPPause() = 0;

   /**
    * Handle APResume Notification
    */
   virtual void handleAPResume() = 0;

   /**
    * Handle AP Boot Notification
    */
   virtual void handleAPBoot() = 0;

   /**
    * Handle AP Reboot Notification
    */
   virtual void handleAPReboot() = 0;


   /**
    * Handle of command error
    */
   virtual void handleError(uint8_t cmdId, uint8_t rc) = 0;

};
