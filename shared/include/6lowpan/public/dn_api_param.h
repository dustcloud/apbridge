/*
 * Copyright (c) 2010, Linear Technology. All rights reserved.
 */

#ifndef _DN_API_PARAM_H
#define _DN_API_PARAM_H

#include "dn_typedef.h"
#include "dn_api_common.h"
#include "dn_mesh.h"

/**
\addtogroup loc_intf
\{
*/

PACKED_START // All structures are packed starting here

//=========================== set/getParam formats ============================

/**
\addtogroup loc_intf_setgetparam_formats setParam/getParam formats
\brief Formats of the setParam/getParam command payloads.
\{
*/

//===============================================
//========== DN_API_PARAM_MACADDR ===============
//===============================================

//=== setParam

/**
\brief Payload of the request to set parameter #DN_API_PARAM_MACADDR.
*/
typedef struct {
   INT8U           paramId;                      ///< Parameter identifier. Set to #DN_API_PARAM_MACADDR.
   dn_macaddr_t    macAddr;                      ///< New MAC address to use.
} dn_api_set_macaddr_t;

/**
\brief Payload of the response when setting parameter #DN_API_PARAM_MACADDR.
*/
typedef dn_api_rsp_set_hdr_t dn_api_rsp_set_macaddr_t;

//=== getParam

/**
\brief Payload of the request to get parameter #DN_API_PARAM_MACADDR.
*/
typedef dn_api_get_hdr_t  dn_api_get_macaddr_t;

/**
\brief Payload of the response when getting parameter #DN_API_PARAM_MACADDR.
*/
typedef struct {
   INT8U           rc;                           ///< Return code, among values in #dn_api_rc_t.
   INT8U           paramId;                      ///< Parameter identifier. Set to #DN_API_PARAM_MACADDR.
   dn_macaddr_t    macAddr;                      ///< MAC address of the device.
} dn_api_rsp_get_macaddr_t;

//===============================================
//========== DN_API_PARAM_JOINKEY ===============
//===============================================

//=== setParam

/**
\brief Payload of the request to set parameter #DN_API_PARAM_JOINKEY.
*/
typedef struct {
   INT8U           paramId;                      ///< Parameter identifier. Set to #DN_API_PARAM_JOINKEY.
   dn_key_t        joinKey;                      ///< Join key.
} dn_api_set_joinkey_t;

/**
\brief Payload of the response when setting parameter #DN_API_PARAM_JOINKEY.
*/
typedef dn_api_rsp_set_hdr_t dn_api_rsp_set_joinkey_t;

//=== getParam

// Getting the #DN_API_PARAM_JOINKEY parameter is not supported.

//===============================================
//========== DN_API_PARAM_NETID =================
//===============================================

//=== setParam

/**
\brief Payload of the request to set parameter #DN_API_PARAM_NETID.
*/
typedef struct {
   INT8U           paramId;                      ///< Parameter identifier. Set to #DN_API_PARAM_NETID.
   dn_netid_t      netId;                        ///< Network ID.
} dn_api_set_netid_t;

/**
\brief Payload of the response when setting parameter #DN_API_PARAM_NETID.
*/
typedef dn_api_rsp_set_hdr_t dn_api_rsp_set_netid_t;

//=== getParam

/**
\brief Payload of the request to get parameter #DN_API_PARAM_NETID.
*/
typedef dn_api_get_hdr_t dn_api_get_netid_t;

/**
\brief Payload of the response when getting parameter #DN_API_PARAM_NETID.
*/
typedef struct {
   INT8U           rc;                           ///< Return code, among values in #dn_api_rc_t.
   INT8U           paramId;                      ///< Parameter identifier. Set to #DN_API_PARAM_NETID.
   dn_netid_t      netId;                        ///< Network Identifier.
} dn_api_rsp_get_netid_t;

//===============================================
//========== DN_API_PARAM_TXPOWER ===============
//===============================================

//=== setParam

/**
\brief Payload of the request to set parameter #DN_API_PARAM_TXPOWER.
*/
typedef struct {
   INT8U           paramId;                      ///< Parameter identifier. Set to #DN_API_PARAM_TXPOWER.
   INT8S           txPower;                      ///< Transmit Power.
} dn_api_set_txpower_t;

/**
\brief Payload of the response when setting parameter #DN_API_PARAM_TXPOWER.
*/
typedef dn_api_rsp_set_hdr_t dn_api_rsp_set_txpower_t;

//=== getParam

/**
\brief Payload of the request to get parameter #DN_API_PARAM_TXPOWER.
*/
typedef dn_api_get_hdr_t dn_api_get_txpower_t;

/**
\brief Payload of the response when getting parameter #DN_API_PARAM_TXPOWER.
*/
typedef struct {
   INT8U           rc;                           ///< Return code, among values in #dn_api_rc_t.
   INT8U           paramId;                      ///< Parameter identifier. Set to #DN_API_PARAM_TXPOWER.
   INT8S           txPower;                      ///< Transmit Power.
} dn_api_rsp_get_txpower_t;

//===============================================
//========== DN_API_PARAM_EVENTMASK =============
//===============================================

//=== setParam

/**
\brief Payload of the request to set parameter #DN_API_PARAM_EVENTMASK.
*/
typedef struct {
   INT8U           paramId;                      ///< Parameter identifier. Set to #DN_API_PARAM_EVENTMASK.
   INT32U          eventMask;                    ///< Event mask; <tt>0</tt>=unsubscribe, <tt>1</tt>=subscribe.
} dn_api_set_eventmask_t;

/**
\brief Payload of the response when setting parameter #DN_API_PARAM_EVENTMASK.
*/
typedef dn_api_rsp_set_hdr_t dn_api_rsp_set_eventmask_t;

//=== getParam

/**
\brief Payload of the request to get parameter #DN_API_PARAM_EVENTMASK.
*/
typedef dn_api_get_hdr_t dn_api_get_eventmask_t;

/**
\brief Payload of the response when getting parameter #DN_API_PARAM_EVENTMASK.
*/
typedef struct {
   INT8U           rc;                           ///< Return code, among values in #dn_api_rc_t.
   INT8U           paramId;                      ///< Parameter identifier. Set to #DN_API_PARAM_EVENTMASK.
   INT32U          eventMask;                    ///< Event mask; <tt>0</tt>=unsubscribed, <tt>1</tt>=subscribed.
} dn_api_rsp_get_eventmask_t;

//===============================================
//========== DN_API_PARAM_MOTEINFO ==============
//===============================================

//=== setParam

// Setting the #DN_API_PARAM_MOTEINFO parameter is not supported.

//=== getParam

/**
\brief Payload of the request to get parameter #DN_API_PARAM_MOTEINFO.
*/
typedef dn_api_get_hdr_t dn_api_get_moteinfo_t;

/**
\brief Payload of the response when getting parameter #DN_API_PARAM_MOTEINFO.
*/
typedef struct {
   INT8U           rc;                           ///< Return code, among values in #dn_api_rc_t.
   INT8U           paramId;                      ///< Parameter identifier. Set to #DN_API_PARAM_MOTEINFO.
   INT8U           apiVersion;                   ///< Version of the API protocol.
   INT8U           serialNumber[DN_SERNUM_SIZE]; ///< Serial number of the device.
   INT8U           hwModel;                      ///< Hardware model.
   INT8U           hwRev;                        ///< Hardware revision.
   dn_api_swver_t  swVer;                        ///< Network stack software version.
   INT8U           blSwVer;                      ///< Bootloader software version.
} dn_api_rsp_get_moteinfo_t;

//===============================================
//========== DN_API_PARAM_NETINFO ===============
//===============================================

//=== setParam

// Setting the #DN_API_PARAM_NETINFO parameter is not supported.

//=== getParam

/**
\brief Payload of the request to get parameter #DN_API_PARAM_NETINFO.
*/
typedef dn_api_get_hdr_t dn_api_get_netinfo_t;

/**
\brief Payload of the response when getting parameter #DN_API_PARAM_NETINFO.
*/
typedef struct {
   INT8U           rc;                           ///< Return code, among values in #dn_api_rc_t.
   INT8U           paramId;                      ///< Parameter identifier. Set to #DN_API_PARAM_NETINFO.
   dn_macaddr_t    macAddr;                      ///< MAC address of the device.
   dn_moteid_t     moteId;                       ///< Mote ID.
   dn_netid_t      netId;                        ///< Network ID.
   INT16U          slotSize;                     ///< Slot size, in micro-seconds.
} dn_api_rsp_get_netinfo_t;

//===============================================
//========== DN_API_PARAM_MOTESTATUS ============
//===============================================

//=== setParam

// Setting the #DN_API_PARAM_MOTESTATUS parameter is not supported.

//=== getParam

/**
\brief Payload of the request to get parameter #DN_API_PARAM_MOTESTATUS.
*/
typedef dn_api_get_hdr_t dn_api_get_motestatus_t;

/**
\brief Payload of the response when getting parameter #DN_API_PARAM_MOTESTATUS.
*/
typedef struct {
   INT8U           rc;                           ///< Return code, among values in #dn_api_rc_t.
   INT8U           paramId;                      ///< Parameter identifier. Set to #DN_API_PARAM_MOTESTATUS.
   INT8U           state;                        ///< Mote state.
   INT8U           stateReason;                  ///< <i>Reserved. Do not use.</i>
   INT16U          reserved_1;                   ///< <i>Reserved. Do not use.</i>
   INT8U           numParents;                   ///< Number of parents.
   INT32U          alarms;                       ///< Bitmap of current alarms.
   INT8U           reserved_2;                   ///< <i>Reserved. Do not use.</i>
} dn_api_rsp_get_motestatus_t;

//===============================================
//========== DN_API_PARAM_TIME ==================
//===============================================

//=== setParam

// Setting the #DN_API_PARAM_TIME parameter is supported on AP only.

/**
\brief Payload of the request to set parameter #DN_API_PARAM_TIME.
*/
typedef struct {
   INT8U           paramId;                      ///< Parameter identifier. Set to #DN_API_PARAM_TIME.
   INT8U           seqNum;                       ///< Sequence number. 
   dn_utc_time_t   utcTime;                      ///< UTC time.
} dn_api_set_time_t;

/**
\brief Payload of the response when setting parameter #DN_API_PARAM_TIME.
*/
typedef dn_api_rsp_set_hdr_t dn_api_rsp_set_time_t;

/**
\brief When serial API receives #DN_API_PARAM_TIME command, it 
       reconstructs dn_api_set_time_t payload into
       dn_api_ap_set_time_t payload
*/
typedef struct {
   INT8U           paramId;                      ///< Parameter identifier. Set to #DN_API_PARAM_TIME.
   INT8U           seqNum;                       ///< Sequence number. 
   dn_utc_time_t   utcTime;                      ///< UTC time.
   INT32U          uartTstamp;                   ///< system time when the sender sent the packet packet
} dn_api_ap_set_time_t;

//=== getParam

/**
\brief Payload of the request to get parameter #DN_API_PARAM_TIME.
*/
typedef dn_api_get_hdr_t dn_api_get_time_t;

/**
\brief Payload of the response when getting parameter #DN_API_PARAM_TIME.
*/
typedef struct {
   INT8U           rc;                           ///< Return code, among values in #dn_api_rc_t.
   INT8U           paramId;                      ///< Parameter identifier. Set to #DN_API_PARAM_TIME.
   INT32U          upTime;                       ///< Time since last reset, in seconds.
   dn_utc_time_t   utcTime;                      ///< UTC time.
   dn_asn_t        asn;                          ///< Time since start of network, in slots.
   INT16U          offset;                       ///< Time from the start of this slot, in micro-seconds.
} dn_api_rsp_get_time_t;

//===============================================
//========== DN_API_PARAM_CHARGE ================
//===============================================

//=== setParam

// Setting the #DN_API_PARAM_CHARGE parameter is not supported.

//=== getParam

/**
\brief Payload of the request to get parameter #DN_API_PARAM_CHARGE.
*/
typedef dn_api_get_hdr_t dn_api_get_charge_t;

/**
\brief Payload of the response when getting parameter #DN_API_PARAM_CHARGE.
*/
typedef struct {
   INT8U           rc;                           ///< Return code, among values in #dn_api_rc_t.
   INT8U           paramId;                      ///< Parameter identifier. Set to #DN_API_PARAM_CHARGE.
   INT32U          qTotal;                       ///< Charge since last reset, in milli-coulombs.
   INT32U          upTime;                       ///< Time since reset, in seconds.
   INT8S           tempInt;                      ///< Temperature (integral part, in C).
   INT8U           tempFrac;                     ///< Temperature (fractional part, in 1/255 of C).
} dn_api_rsp_get_charge_t;

//===============================================
//========== DN_API_PARAM_TESTRADIORXSTATS ======
//===============================================

//=== setParam

// Setting the #DN_API_PARAM_TESTRADIORXSTATS parameter is not supported.

//=== getParam

/**
\brief Payload of the request to get parameter #DN_API_PARAM_TESTRADIORXSTATS.
*/
typedef dn_api_get_hdr_t dn_api_get_rfrxstats_t;

/**
\brief Payload of the response when getting parameter #DN_API_PARAM_TESTRADIORXSTATS.
*/
typedef struct {
   INT8U           rc;                           ///< Return code, among values in #dn_api_rc_t.
   INT8U           paramId;                      ///< Parameter identifier. Set to #DN_API_PARAM_TESTRADIORXSTATS.
   INT16U          rxOkCnt;                      ///< Number of packets received successfully.
   INT16U          rxFailCnt;                    ///< Number of packets received with errors.
} dn_api_rsp_get_rfrxstats_t;

//===============================================
//========== DN_API_PARAM_MACMICKEY =============
//===============================================

//=== setParam

/**
\brief Payload of the request to set parameter #DN_API_PARAM_MACMICKEY.
*/
typedef struct {
   INT8U           paramId;                      ///< Parameter identifier. Set to #DN_API_PARAM_MACMICKEY.
   dn_key_t        key;                          ///< MAC MIC key.
} dn_api_set_macmickey_t;

/**
\brief Payload of the response when setting parameter #DN_API_PARAM_MACMICKEY.
*/
typedef dn_api_rsp_set_hdr_t dn_api_rsp_set_macmickey_t;

//=== getParam

// Getting the #DN_API_PARAM_MACMICKEY parameter is not supported.

//===============================================
//========== DN_API_PARAM_SHORTADDR =============
//===============================================

//=== setParam

/**
\brief Payload of the request to set parameter #DN_API_PARAM_SHORTADDR.
*/
typedef struct {
   INT8U           paramId;                      ///< Parameter identifier. Set to #DN_API_PARAM_SHORTADDR.
   dn_moteid_t     shortAddr;                    ///< Short address of the mote, between <tt>0x0001</tt> and <tt>0xFFFE</tt>, included.
} dn_api_set_shortaddr_t;

/**
\brief Payload of the response when setting parameter #DN_API_PARAM_SHORTADDR.
*/
typedef dn_api_rsp_set_hdr_t dn_api_rsp_set_shortaddr_t;

//=== getParam

/**
\brief Payload of the request to get parameter #DN_API_PARAM_SHORTADDR.
*/
typedef dn_api_get_hdr_t dn_api_get_shortaddr_t;

/**
\brief Payload of the response when getting parameter #DN_API_PARAM_SHORTADDR.
*/
typedef struct {
   INT8U           rc;                           ///< Return code, among values in #dn_api_rc_t.
   INT8U           paramId;                      ///< Parameter identifier. Set to #DN_API_PARAM_SHORTADDR.
   dn_moteid_t     shortAddr;                    ///< Short address of the mote, between <tt>0x0001</tt> and <tt>0xFFFE</tt>, included.
} dn_api_rsp_get_shortaddr_t;

//===============================================
//========== DN_API_PARAM_IP6ADDR ===============
//===============================================

//=== setParam

/**
\brief Payload of the request to set parameter #DN_API_PARAM_IP6ADDR.
*/
typedef struct {
   INT8U           paramId;                      ///< Parameter identifier. Set to #DN_API_PARAM_IP6ADDR.
   dn_ipv6_addr_t  ipAddr;                       ///< IPv6 address of the mote.
} dn_api_set_ip6addr_t;

/**
\brief Payload of the response when setting parameter #DN_API_PARAM_IP6ADDR.
*/
typedef dn_api_rsp_set_hdr_t dn_api_rsp_set_ip6addr_t;

//=== getParam

/**
\brief Payload of the request to get parameter #DN_API_PARAM_IP6ADDR.
*/
typedef dn_api_get_hdr_t dn_api_get_ip6addr_t;

/**
\brief Payload of the response when getting parameter #DN_API_PARAM_IP6ADDR.
*/
typedef struct {
   INT8U           rc;                           ///< Return code, among values in #dn_api_rc_t.
   INT8U           paramId;                      ///< Parameter identifier. Set to #DN_API_PARAM_IP6ADDR.
   dn_ipv6_addr_t  ipAddr;                       ///< IPv6 address of the mote.
} dn_api_rsp_get_ip6addr_t;

//===============================================
//========== DN_API_PARAM_CCAMODE ===============
//===============================================

//=== setParam

/**
\brief Payload of the request to set parameter DN_API_PARAM_CCAMODE.
*/
typedef struct {
   INT8U           paramId;                      ///< Parameter identifier. Set to DN_API_PARAM_CCAMODE.
   INT8U           mode;
} dn_api_set_ccamode_t;

/**
\brief Payload of the response when setting parameter DN_API_PARAM_CCAMODE.
*/
typedef dn_api_rsp_set_hdr_t dn_api_rsp_set_ccamode_t;

//=== getParam

/**
\brief Payload of the request to get parameter DN_API_PARAM_CCAMODE.
*/
typedef dn_api_get_hdr_t dn_api_get_ccamode_t;

/**
\brief Payload of the response when getting parameter DN_API_PARAM_CCAMODE.
*/
typedef struct {
   INT8U           rc;
   INT8U           paramId;
   INT8U           ccaMode;
} dn_api_rsp_get_ccamode_t;

//===============================================
//========== DN_API_PARAM_PATHALARMGEN ==========
//===============================================

//=== setParam

/**
\brief Payload of the request to set parameter DN_API_PARAM_PATHALARMGEN.
*/
typedef struct {
   INT8U           paramId;                      ///< Parameter identifier. Set to DN_API_PARAM_PATHALARMGEN.
   INT8U           isGenerate;                   ///< !=0 generate path alarm, otherwise don't generate.
} dn_api_set_pathalarmgen_t;

/**
\brief Payload of the response when setting parameter DN_API_PARAM_PATHALARMGEN.
*/
typedef dn_api_rsp_set_hdr_t dn_api_rsp_set_pathalarmgen_t;

//=== getParam

/**
\brief Payload of the request to get parameter DN_API_PARAM_PATHALARMGEN.
*/
typedef dn_api_get_hdr_t dn_api_get_pathalarmgen_t;

/**
\brief Payload of the response when getting parameter DN_API_PARAM_PATHALARMGEN.
*/
typedef struct {
   INT8U           rc;
   INT8U           paramId;                      ///< Parameter identifier. Set to DN_API_PARAM_PATHALARMGEN.
   INT8U           isGenerate;                   ///< !=0 generate path alarm, otherwise don't generate.
} dn_api_rsp_get_pathalarmgen_t;

//===============================================
//========== DN_API_PARAM_CHANNELS ==============
//===============================================

//=== setParam

/**
\brief Payload of the request to set parameter DN_API_PARAM_CHANNELS.
*/
typedef struct {
   INT8U           paramId;                      ///< Parameter identifier. Set to DN_API_PARAM_CHANNELS.
   INT16U          bitmap;                       ///< Bit0=channel 0, Bit 15= channel 15; 0=do not use; 1=use channel.
} dn_api_set_channels_t;

/**
\brief Payload of the response when setting parameter DN_API_PARAM_CHANNELS.
*/
typedef dn_api_rsp_set_hdr_t dn_api_rsp_set_channels_t;

//=== getParam

/**
\brief Payload of the request to get parameter DN_API_PARAM_CHANNELS.
*/
typedef dn_api_get_hdr_t dn_api_get_channels_t;

/**
\brief Payload of the response when getting parameter DN_API_PARAM_CHANNELS.
*/
typedef struct {
   INT8U           rc;
   INT8U           paramId;                      ///< Parameter identifier. Set to DN_API_PARAM_CHANNELS.
   INT16U          bitmap;                       ///< Bit0=channel 0, Bit 15= channel 15; 0=do not use; 1=use channel.
} dn_api_rsp_get_channels_t;

//===============================================
//========== DN_API_PARAM_ADVGRAPH ==============
//===============================================

//=== setParam

/**
\brief Payload of the request to set parameter DN_API_PARAM_ADVGRAPH.
*/
typedef struct {
   INT8U           paramId;                      ///< Parameter identifier. Set to DN_API_PARAM_ADVGRAPH.
   INT8U           advGraph;                     ///< Graph id to advertise.
} dn_api_set_advgraph_t;

/**
\brief Payload of the response when setting parameter DN_API_PARAM_ADVGRAPH.
*/
typedef dn_api_rsp_set_hdr_t dn_api_rsp_set_advgraph_t;

//=== getParam
/**
\brief Payload of the request to get parameter DN_API_PARAM_ADVGRAPH.
*/
typedef dn_api_get_hdr_t dn_api_get_advgraph_t;

/**
\brief Payload of the response when getting parameter DN_API_PARAM_ADVGRAPH.
*/
typedef struct {
   INT8U           rc;
   INT8U           paramId;
   INT8U           advGraph;
} dn_api_rsp_get_advgraph_t;

//===============================================
//========== DN_API_PARAM_HRTMR =================
//===============================================

//=== setParam

/**
\brief Payload of the request to set parameter #DN_API_PARAM_HRTMR.
*/
typedef struct {
   INT8U           paramId;                      ///< Parameter identifier. Set to #DN_API_PARAM_HRTMR.
   INT16U          hrTimer;                      ///< In seconds; 0=disabled.
} dn_api_set_hrtimer_t;

/**
\brief Payload of the response when setting parameter #DN_API_PARAM_HRTMR.
*/
typedef dn_api_rsp_set_hdr_t dn_api_rsp_set_hrtimer_t;

//=== getParam

// Getting the #DN_API_PARAM_HRTMR parameter is not supported.

//===============================================
//========== DN_API_PARAM_APPINFO ===============
//===============================================

//=== setParam

/**
\brief Payload of the request to set parameter #DN_API_PARAM_APPINFO.
*/
typedef struct {
   INT8U           paramId;                      ///< Parameter identifier. Set to #DN_API_PARAM_APPINFO.
   INT16U          vendorId;
   INT8U           appId;
   dn_api_swver_t  appVer;                       ///< App Version.
} dn_api_set_appinfo_t;

/**
\brief Payload of the response when setting parameter #DN_API_PARAM_APPINFO.
*/
typedef dn_api_rsp_set_hdr_t dn_api_rsp_set_appinfo_t;

//=== getParam

// Getting the #DN_API_PARAM_APPINFO parameter is not supported.

//===============================================
//========== DN_API_PARAM_HRTMR =================
//===============================================

//=== setParam

// Setting the #DN_API_PARAM_HRTMR parameter is not supported.

//=== getParam

/**
\brief Payload of the request to get parameter #DN_API_PARAM_HRTMR.
*/
typedef dn_api_get_hdr_t dn_api_get_hrTimer_t;

/**
\brief Payload of the response when getting parameter #DN_API_PARAM_HRTMR.
*/
typedef struct {
   INT8U           rc;                           ///< Return code, among values in #dn_api_rc_t.
   INT8U           paramId;                      ///< Parameter identifier. Set to #DN_API_PARAM_HRTMR.
   INT16U          hrTimer;                      ///< In seconds; 0=disabled.
} dn_api_rsp_get_hrTimer_t;

//===============================================
//========== DN_API_PARAM_APPINFO ===============
//===============================================

//=== setParam

// Setting the #DN_API_PARAM_APPINFO parameter is not supported.

//=== getParam

/**
\brief Payload of the request to get parameter #DN_API_PARAM_APPINFO.
*/
typedef dn_api_get_hdr_t dn_api_get_appinfo_t;

/**
\brief Payload of the response when getting parameter #DN_API_PARAM_APPINFO.
*/
typedef struct {
   INT8U           rc;                           ///< Return code, among values in #dn_api_rc_t.
   INT8U           paramId;                      ///< Parameter identifier. Set to #DN_API_PARAM_APPINFO.
   INT16U          vendorId;
   INT8U           appId;
   dn_api_swver_t  appVer;                       ///< App Version.
} dn_api_rsp_get_appinfo_t;

//===============================================
//========== DN_API_PARAM_PWRCOSTINFO ===========
//===============================================

//=== setParam

// Setting the #DN_API_PARAM_PWRCOSTINFO parameter is not supported.

//=== getParam

/**
\brief Payload of the request to get parameter #DN_API_PARAM_PWRCOSTINFO.
*/
typedef dn_api_get_hdr_t dn_api_get_pwrcostinfo_t;

/**
\brief Payload of the response when getting parameter #DN_API_PARAM_PWRCOSTINFO.
*/
typedef struct {
   INT8U           rc;                           ///< Return code, among values in #dn_api_rc_t.
   INT8U           paramId;                      ///< Parameter identifier. Set to #DN_API_PARAM_PWRCOSTINFO.
   dn_api_pwrcostinfo_t info;
} dn_api_rsp_get_pwrcostinfo_t;

//===============================================
//========== DN_API_PARAM_SIZEINFO ==============
//===============================================

//=== setParam

// Setting the #DN_API_PARAM_SIZEINFO parameter is not supported.

//=== getParam

/**
\brief Payload of the request to get parameter #DN_API_PARAM_SIZEINFO.
*/
typedef dn_api_get_hdr_t dn_api_get_sizeinfo_t;

/**
\brief Payload of the response when getting parameter #DN_API_PARAM_SIZEINFO.
*/
typedef struct {
   INT8U           rc;                           ///< Return code, among values in #dn_api_rc_t.
   INT8U           paramId;                      ///< Parameter identifier. Set to #DN_API_PARAM_SIZEINFO.
   INT8U           maxFrames;
   INT16U          maxLinks;
   INT8U           maxNbrs;
   INT8U           maxRoutes;
   INT8U           maxGraphs;
   INT8U           maxQSize;
} dn_api_rsp_get_sizeinfo_t;

//===============================================
//========== DN_API_PARAM_JOINDUTYCYCLE =========
//===============================================

//=== setParam

/**
\brief Payload of the request to set parameter #DN_API_PARAM_JOINDUTYCYCLE.
*/
typedef struct {
   INT8U           paramId;                      ///< Parameter identifier. Set to #DN_API_PARAM_JOINDUTYCYCLE.
   INT8U           dutyCycle;                    ///< Duty cycle (0-255), where 0=0.2% and 255=99.8%.
} dn_api_set_dutycycle_t;

/**
\brief Payload of the response when setting parameter #DN_API_PARAM_JOINDUTYCYCLE.
*/
typedef dn_api_rsp_set_hdr_t dn_api_rsp_set_dutycycle_t;

//=== getParam

/**
\brief Payload of the request to get parameter #DN_API_PARAM_JOINDUTYCYCLE.
*/
typedef dn_api_get_hdr_t dn_api_get_dutycycle_t;

/**
\brief Payload of the response when getting parameter #DN_API_PARAM_JOINDUTYCYCLE.
*/
typedef struct {
   INT8U           rc;                           ///< Return code, among values in #dn_api_rc_t.
   INT8U           paramId;                      ///< Parameter identifier. Set to #DN_API_PARAM_JOINDUTYCYCLE.
   INT8U           dutyCycle;                    ///< Duty cycle (0-255), where 0=0.2% and 255=99.8%.
} dn_api_rsp_get_dutycycle_t;

//===============================================
//========== DN_API_PARAM_OTAP_LOCKOUT ==========
//===============================================

//=== setParam

/**
\brief Payload of the request to set parameter #DN_API_PARAM_OTAP_LOCKOUT.
*/
typedef struct {
   INT8U           paramId;                      ///< Parameter identifier. Set to #DN_API_PARAM_OTAP_LOCKOUT.
   INT8U           mode;                         ///< OTAP mode.
} dn_api_set_otaplockout_t;

/**
\brief Payload of the response when setting parameter #DN_API_PARAM_OTAP_LOCKOUT.
*/
typedef dn_api_rsp_set_hdr_t dn_api_rsp_set_otaplockout_t;

//=== getParam

/**
\brief Payload of the request to get parameter #DN_API_PARAM_OTAP_LOCKOUT.
*/
typedef dn_api_get_hdr_t dn_api_get_otaplockout_t;

/**
\brief Payload of the response when getting parameter #DN_API_PARAM_OTAP_LOCKOUT.
*/
typedef struct {
   INT8U           rc;                           ///< Return code, among values in #dn_api_rc_t.
   INT8U           paramId;                      ///< Parameter identifier. Set to #DN_API_PARAM_OTAP_LOCKOUT.
   INT8U           mode;                         ///< OTAP mode.
} dn_api_rsp_get_otaplockout_t;

//===============================================
//========== DN_API_PARAM_ROUTINGMODE ===========
//===============================================

//=== setParam

/**
\brief Payload of the request to set parameter #DN_API_PARAM_ROUTINGMODE.
*/
typedef struct {
   INT8U           paramId;                      ///< Parameter identifier. Set to #DN_API_PARAM_ROUTINGMODE.
   INT8U           mode;                         ///< Routing mode.
} dn_api_set_rtmode_t;

/**
\brief Payload of the response when setting parameter #DN_API_PARAM_ROUTINGMODE.
*/
typedef dn_api_rsp_set_hdr_t dn_api_rsp_set_rtmode_t;

//=== getParam

/**
\brief Payload of the request to get parameter #DN_API_PARAM_ROUTINGMODE.
*/
typedef dn_api_get_hdr_t dn_api_get_rtmode_t;

/**
\brief Payload of the response when getting parameter #DN_API_PARAM_ROUTINGMODE.
*/
typedef struct {
   INT8U           rc;                           ///< Return code, among values in #dn_api_rc_t.
   INT8U           paramId;                      ///< Parameter identifier. Set to #DN_API_PARAM_ROUTINGMODE.
   INT8U           mode;                         ///< Routing mode.
} dn_api_rsp_get_rtmode_t;

//===============================================
//========== DN_API_PARAM_PWRSRCINFO ============
//===============================================

//=== setParam

/**
\brief Payload of the request to set parameter #DN_API_PARAM_PWRSRCINFO.
*/
typedef struct {
   INT8U           paramId;                      ///< Parameter identifier. Set to #DN_API_PARAM_PWRSRCINFO.
   dn_api_pwrsrcinfo_t info;                     ///< Power source information.
} dn_api_set_pwrsrcinfo_t;

/**
\brief Payload of the response when setting parameter #DN_API_PARAM_PWRSRCINFO.
*/
typedef dn_api_rsp_set_hdr_t dn_api_rsp_set_pwrsrcinfo_t;

//=== getParam

/**
\brief Payload of the request to get parameter #DN_API_PARAM_PWRSRCINFO.
*/
typedef dn_api_get_hdr_t dn_api_get_pwrsrcinfo_t;

/**
\brief Payload of the response when getting parameter #DN_API_PARAM_PWRSRCINFO.
*/
typedef struct {
   INT8U           rc;                           ///< Return code, among values in #dn_api_rc_t.
   INT8U           paramId;                      ///< Parameter identifier. Set to #DN_API_PARAM_PWRSRCINFO.
   dn_api_pwrsrcinfo_t info;                     ///< Power source information.
} dn_api_rsp_get_pwrsrcinfo_t;

//===============================================
//========== DN_API_PARAM_MOBILITY ==============
//===============================================

//=== setParam

/**
\brief Payload of the request to set parameter #DN_API_PARAM_MOBILITY.
*/
typedef struct {
   INT8U           paramId;                      ///< Parameter identifier. Set to #DN_API_PARAM_MOBILITY.
   INT8U           type;                         ///< Mobility type.
} dn_api_set_mobility_t;

/**
\brief Payload of the response when setting parameter #DN_API_PARAM_MOBILITY.
*/
typedef dn_api_rsp_set_hdr_t dn_api_rsp_set_mobility_t;

//=== getParam

/**
\brief Payload of the request to get parameter #DN_API_PARAM_MOBILITY.
*/
typedef dn_api_get_hdr_t dn_api_get_mobility_t;

/**
\brief Payload of the response when getting parameter #DN_API_PARAM_MOBILITY.
*/
typedef struct {
   INT8U           rc;                           ///< Return code, among values in #dn_api_rc_t.
   INT8U           paramId;                      ///< Parameter identifier. Set to #DN_API_PARAM_MOBILITY.
   INT8U           type;                         ///< Mobility type.
} dn_api_rsp_get_mobility_t;

//===============================================
//========== DN_API_PARAM_ADVKEY ================
//===============================================

//=== setParam

/**
\brief Payload of the request to set parameter #DN_API_PARAM_ADVKEY.
*/
typedef struct {
   INT8U           paramId;                      ///< Parameter identifier. Set to #DN_API_PARAM_ADVKEY.
   dn_key_t        key;                          ///< New advertisement key to use.
} dn_api_set_advkey_t;

/**
\brief Payload of the response when setting parameter #DN_API_PARAM_ADVKEY.
*/
typedef dn_api_rsp_set_hdr_t dn_api_rsp_set_advkey_t;

//=== getParam

/**
\brief Payload of the request to get parameter #DN_API_PARAM_ADVKEY.
*/
typedef dn_api_get_hdr_t dn_api_get_advkey_t;

/**
\brief Payload of the response when getting parameter #DN_API_PARAM_ADVKEY.
*/
typedef struct {
   INT8U           rc;                           ///< Return code, among values in #dn_api_rc_t.
   INT8U           paramId;                      ///< Parameter identifier. Set to #DN_API_PARAM_ADVKEY.
   dn_key_t        key;                          ///< Advertisement key.
} dn_api_rsp_get_advkey_t;

//===============================================
//========== DN_API_PARAM_AUTOJOIN ==============
//===============================================

//=== setParam

/**
\brief Payload of the request to set parameter #DN_API_PARAM_AUTOJOIN.
*/
typedef struct {
   INT8U           paramId;                      ///< Parameter identifier. Set to #DN_API_PARAM_AUTOJOIN.
   INT8U           mode;                         ///< Autojoin mode.
} dn_api_set_autojoin_t;

/**
\brief Payload of the response when setting parameter #DN_API_PARAM_AUTOJOIN.
*/
typedef dn_api_rsp_set_hdr_t dn_api_rsp_set_autojoin_t;

//=== getParam

/**
\brief Payload of the request to get parameter #DN_API_PARAM_AUTOJOIN.
*/
typedef dn_api_get_hdr_t dn_api_get_autojoin_t;

/**
\brief Payload of the response when getting parameter #DN_API_PARAM_AUTOJOIN.
*/
typedef struct {
   INT8U           rc;                           ///< Return code, among values in #dn_api_rc_t.
   INT8U           paramId;                      ///< Parameter identifier. Set to #DN_API_PARAM_AUTOJOIN.
   INT8U           mode;                         ///< Autojoin mode.
} dn_api_rsp_get_autojoin_t;


//===============================================
//========== DN_API_PARAM_AP_CLKSRC ==============
//===============================================

//=== setParam

/**
\brief Payload of the request to set parameter #DN_API_PARAM_AP_CLKSRC.
*/
typedef struct {
   INT8U           paramId;                      ///< Parameter identifier. Set to #DN_API_PARAM_AP_CLKSRC.
   INT8U           apClkSrc;                     ///< AP clock source.
} dn_api_set_ap_clksrc_t;

/**
\brief Payload of the response when setting parameter #DN_API_PARAM_AP_CLKSRC.
*/
typedef dn_api_rsp_set_hdr_t dn_api_rsp_set_ap_clksrc_t;

//=== getParam

/**
\brief Payload of the request to get parameter #DN_API_PARAM_AP_CLKSRC.
*/
typedef dn_api_get_hdr_t dn_api_get_ap_clksrc_t;

/**
\brief Payload of the response when getting parameter #DN_API_PARAM_AP_CLKSRC.
*/
typedef struct {
   INT8U           rc;                           ///< Return code, among values in #dn_api_rc_t.
   INT8U           paramId;                      ///< Parameter identifier. Set to #DN_API_PARAM_AP_CLKSRC.
   INT8U           apClkSrc;                     ///< AP clock source.
} dn_api_rsp_get_ap_clksrc_t;


//===============================================
//========== DN_API_PARAM_AP_STATS ======
//===============================================

//=== setParam

// Setting the #DN_API_PARAM_AP_STATS parameter is not supported.

//=== getParam

/**
\brief Payload of the request to get parameter #DN_API_PARAM_AP_STATS.
*/
typedef dn_api_get_hdr_t dn_api_get_ap_stats_t;

/**
\brief Payload of the response when getting parameter #DN_API_PARAM_AP_STATS.
*/
typedef struct {
   INT8U           rc;                           ///< Return code, among values in #dn_api_rc_t.
   INT8U           paramId;                      ///< Parameter identifier. Set to #DN_API_PARAM_AP_STATS.
   INT32S          lastPO;                       ///< Last parent time offset.
   INT32S          lastPFE;                      ///< Last parent frequency error.
   INT16U          numRxSetTimeErr;              ///< Number of received 'set time' commands with errors 
   INT16U          numSignalErr;                 ///< Number of PPS signals with errors
   INT8U           state;                        ///< PPS driver state 
} dn_api_rsp_get_ap_stats_t;


//===============================================
//========== DN_API_PARAM_AP_STATUS ============
//===============================================

//=== setParam

// Setting the #DN_API_PARAM_AP_STATUS parameter is not supported.

//=== getParam

/**
\brief Payload of the request to get parameter #DN_API_PARAM_AP_STATUS.
*/
typedef dn_api_get_hdr_t dn_api_get_apstatus_t;

/**
\brief Payload of the response when getting parameter #DN_API_PARAM_AP_STATUS.
*/
typedef struct {
   INT8U           rc;                           ///< Return code, among values in #dn_api_rc_t.
   INT8U           paramId;                      ///< Parameter identifier. Set to #DN_API_PARAM_AP_STATUS.
   INT8U           state;                        ///< AP state.
} dn_api_rsp_get_apstatus_t;


//===============================================
//========== DN_API_PARAM_ANT_GAIN =====
//===============================================

//=== setParam

/**
\brief Payload of the request to set parameter #DN_API_PARAM_ANT_GAIN.
*/
typedef struct {
   INT8U           paramId;                      ///< Parameter identifier. Set to #DN_API_PARAM_ANT_GAIN.
   INT8S           antGain;                      ///< Antenna gain.
} dn_api_set_ant_gain_t;

/**
\brief Payload of the response when setting parameter #DN_API_PARAM_ANT_GAIN.
*/
typedef dn_api_rsp_set_hdr_t dn_api_rsp_set_ant_gain_t;

//=== getParam

/**
\brief Payload of the request to get parameter #DN_API_PARAM_ANT_GAIN.
*/
typedef dn_api_get_hdr_t dn_api_get_ant_gain_t;

/**
\brief Payload of the response when getting parameter #DN_API_PARAM_ANT_GAIN.
*/
typedef struct {
   INT8U           rc;                           ///< Return code, among values in #dn_api_rc_t.
   INT8U           paramId;                      ///< Parameter identifier. Set to #DN_API_PARAM_ANT_GAIN.
   INT8S           antGain;                      ///< Antenna gain.
} dn_api_rsp_get_ant_gain_t;


//===============================================
//========== DN_API_PARAM_EU_COMPLIANT_MODE =====
//===============================================

//=== setParam

/**
\brief Payload of the request to set parameter #DN_API_PARAM_EU_COMPLIANT_MODE.
*/
typedef struct {
   INT8U           paramId;                      ///< Parameter identifier. Set to #DN_API_PARAM_EU_COMPLIANT_MODE.
   INT8U           compMode;                     ///< EU compliant mode.
} dn_api_set_comp_mode_t;

/**
\brief Payload of the response when setting parameter #DN_API_PARAM_EU_COMPLIANT_MODE.
*/
typedef dn_api_rsp_set_hdr_t dn_api_rsp_set_comp_mode_t;

//=== getParam

/**
\brief Payload of the request to get parameter #DN_API_PARAM_EU_COMPLIANT_MODE.
*/
typedef dn_api_get_hdr_t dn_api_get_comp_mode_t;

/**
\brief Payload of the response when getting parameter #DN_API_PARAM_EU_COMPLIANT_MODE.
*/
typedef struct {
   INT8U           rc;                           ///< Return code, among values in #dn_api_rc_t.
   INT8U           paramId;                      ///< Parameter identifier. Set to #DN_API_PARAM_EU_COMPLIANT_MODE.
   INT8U           compMode;                     ///< EU compliant mode.
} dn_api_rsp_get_comp_mode_t;


//===============================================
//========== DN_API_PARAM_SIZEINFOEXT ===========
//===============================================

//=== setParam

// Setting the #DN_API_PARAM_SIZEINFOEXT parameter is not supported.

//=== getParam

/**
\brief Payload of the request to get parameter #DN_API_PARAM_SIZEINFOEXT.
*/
typedef dn_api_get_hdr_t dn_api_get_sizeinfoext_t;

/**
\brief Payload of the response when getting parameter #DN_API_PARAM_SIZEINFOEXT.
*/
typedef struct {
   INT8U           rc;                           ///< Return code, among values in #dn_api_rc_t.
   INT8U           paramId;                      ///< Parameter identifier. Set to #DN_API_PARAM_SIZEINFOEXT.
   INT8U           maxFrames;
   INT16U          maxLinks;
   INT16U          maxNbrs;
   INT8U           maxRoutes;
   INT8U           maxGraphs;
   INT8U           maxQSize;
} dn_api_rsp_get_sizeinfoext_t;

/**
// end of loc_intf_setgetparam_formats
\}
*/

PACKED_STOP // back to default packing

/**
// end of loc_intf
\}
*/

#endif /* _DN_API_PARAM_H */
