/*
 * Copyright (c) 2016, Linear Technology. All rights reserved.
 */

#ifndef _DN_API_LOCAL_H
#define _DN_API_LOCAL_H

#include "dn_typedef.h"
#include "dn_api_common.h"
#include "dn_mesh.h"

/**
\addtogroup loc_intf
\{
*/

PACKED_START // All structures are packed starting here

//=========================== request/response formats ========================

/**
\addtogroup loc_intf_reqresp_formats Request/response formats
\brief Formats of the command requests/responses sent over the local interface.
\{
*/

//===== DN_API_LOC_CMD_SETPARAM

/**
\brief Format of the request when issuing a #DN_API_LOC_CMD_SETPARAM command.
*/
typedef struct {
   INT8U           paramId;                      ///< Identifier of the parameter to set.
   #ifndef __cplusplus
   INT8U           payload[];                    ///< Parameter-specific payload.
   #endif
} dn_api_loc_setparam_t;

/**
\brief Format of the response when issuing a #DN_API_LOC_CMD_SETPARAM command.
*/
typedef struct {
   INT8U           rc;                           ///< Return code, among values in #dn_api_rc_t.
   INT8U           paramId;                      ///< Identifier of the parameter.
   #ifndef __cplusplus
   INT8U           payload[];                    ///< Parameter-specific payload.
   #endif
} dn_api_loc_rsp_setparam_t;

//===== DN_API_LOC_CMD_GETPARAM

/**
\brief Format of the request when issuing a #DN_API_LOC_CMD_GETPARAM command.
*/
typedef struct {
   INT8U           paramId;                      ///< Identifier of the parameter to get.
   #ifndef __cplusplus
   INT8U           payload[];                    ///< Parameter-specific payload.
   #endif
} dn_api_loc_getparam_t;

/**
\brief Format of the response when issuing a #DN_API_LOC_CMD_GETPARAM command.
*/
typedef struct {
   INT8U           rc;                           ///< Return code, among values in #dn_api_rc_t.
   INT8U           paramId;                      ///< Identifier of the parameter.
   #ifndef __cplusplus
   INT8U           payload[];                    ///< Parameter-specific payload.
   #endif
} dn_api_loc_rsp_getparam_t;


//===== DN_API_LOC_CMD_JOIN

// note: the request when issuing a #DN_API_LOC_CMD_JOIN command has no payload.

/**
\brief Format of the response when issuing a #DN_API_LOC_CMD_JOIN command.
*/
typedef dn_api_rc_rsp_t dn_api_loc_rsp_join_t;

//===== DN_API_LOC_CMD_DISCONNECT

// note: the request when issuing a #DN_API_LOC_CMD_DISCONNECT command has no payload.

/**
\brief Format of the response when issuing a #DN_API_LOC_CMD_DISCONNECT command.
*/
typedef dn_api_rc_rsp_t dn_api_loc_rsp_discon_t;

//===== DN_API_LOC_CMD_RESET

// note: the request when issuing a #DN_API_LOC_CMD_RESET command has no payload.

/**
\brief Format of the response when issuing a #DN_API_LOC_CMD_RESET command.
*/
typedef dn_api_rc_rsp_t dn_api_loc_rsp_reset_t;

//===== DN_API_LOC_CMD_LOWPWRSLEEP

// note: the request when issuing a #DN_API_LOC_CMD_LOWPWRSLEEP command has no payload.

/**
\brief Format of the response when issuing a #DN_API_LOC_CMD_LOWPWRSLEEP command.
*/
typedef dn_api_rc_rsp_t dn_api_loc_rsp_lpsleep_t;

//===== DN_API_LOC_CMD_TESTRADIOTX

/// Maximum value for dn_api_loc_testrftx_t::numSubtests.
#define DN_API_RTEST_MAX_SUBTESTS  10

/**
\brief A test radio TX sequence definition.
*/
typedef struct {
   INT8U           pkLen;                        ///< Length of the packet to send. You must use a length between 2 and 125 bytes.
   INT16U          gap;                          ///< Delay between this packet and the next one, in micro-seconds.
} PACKED_ATTR dn_api_loc_testrftx_subtestparam_t;

/**
\brief Format of the request when issuing a #DN_API_LOC_CMD_TESTRADIOTX command.
*/
typedef struct {
   INT8U           type;                         ///< Test type.
   INT16U          mask;                         ///< Mask of channels (0–15) enabled for the test. Bit 0 corresponds to channel 0. For continuous wave and continuous modulation tests, only one channel should be enabled.
   INT16U          numRepeats;                   ///< Number of times to repeat the packet sequence (0=do not stop). Applies only to packet transmission tests.
   INT8S           txPower;                      ///< Transmit power, in dB.
   INT8U           numSubtests;                  ///< Number of packets in each sequence. This parameter is only used for packet test. It can not exceed #DN_API_RTEST_MAX_SUBTESTS.
   #ifndef __cplusplus
   dn_api_loc_testrftx_subtestparam_t subtestParam[]; ///< Array of sequence definitions.
   #endif
} PACKED_ATTR dn_api_loc_testrftx_part1_t;

typedef struct {
   INT8U           stationId;                    ///< Device stationId
} PACKED_ATTR dn_api_loc_testrftx_part2_t;

/**
\brief Format of the response when issuing a #DN_API_LOC_CMD_TESTRADIOTX command.
*/
typedef dn_api_rc_rsp_t dn_api_loc_rsp_testrftx_t;

//===== DN_API_LOC_CMD_ZEROIZE

/**
\brief Format of the request when issuing a #DN_API_LOC_CMD_ZEROIZE command.
*/
typedef struct {
    INT32U         password;                    ///< Password to authenticate zeroize
} PACKED_ATTR dn_api_loc_req_zeroize_t;
/**
\brief Format of the response when issuing a #DN_API_LOC_CMD_ZEROIZE command.
*/
typedef dn_api_rc_rsp_t dn_api_loc_rsp_zeroize_t;

//===== DN_API_LOC_CMD_TESTRADIORX

/**
\brief Format of the request when issuing a #DN_API_LOC_CMD_TESTRADIORX command.
*/
typedef struct {
   INT16U          mask;                         ///< Mask of channels (0–14) enabled for the test. Bit 0 corresponds to channel 0. Only one channel should be enabled.
   INT16U          timeSeconds;                  ///< Test duration (in seconds).
} dn_api_loc_testrfrx_part1_t;

typedef struct {
   INT8U           stationId;                    ///< Device stationId
} dn_api_loc_testrfrx_part2_t;

/**
\brief Format of the response when issuing a #DN_API_LOC_CMD_TESTRADIORX command.
*/
typedef dn_api_rc_rsp_t dn_api_loc_rsp_testrfrx_t;

//===== DN_API_LOC_CMD_CLEARNV

// note: the request when issuing a #DN_API_LOC_CMD_CLEARNV command has no payload.

/**
\brief Format of the response when issuing a #DN_API_LOC_CMD_CLEARNV command.
*/
typedef dn_api_rc_rsp_t dn_api_loc_rsp_clearnv_t;

//===== DN_API_LOC_CMD_SERVICE_REQUEST

/**
\brief Format of the request when issuing a #DN_API_LOC_CMD_SERVICE_REQUEST command.
*/
typedef struct {
   dn_moteid_t     dest;                         ///< Address of in-mesh destination of service. The manager address (<tt>0xFFFE</tt>) is the only value currently supported.
   INT8U           type;                         ///< Service type.
   INT32U          value;                        ///< Inter-packet interval (in ms).
} dn_api_loc_svcrequest_t;

/**
\brief Format of the response when issuing a #DN_API_LOC_CMD_SERVICE_REQUEST command.
*/
typedef dn_api_rc_rsp_t dn_api_loc_rsp_svcrequest_t;

//===== DN_API_LOC_CMD_GET_SVC_INFO

/**
\brief Format of the request when issuing a #DN_API_LOC_CMD_GET_SVC_INFO command.
*/
typedef struct {
   dn_moteid_t     dest;                         ///< Address of in-mesh destination of service. The manager address (<tt>0xFFFE</tt>) is the only value currently supported.
   INT8U           type;                         ///< Service type.
} dn_api_loc_get_service_t;

/**
\brief Format of the response when issuing a #DN_API_LOC_CMD_GET_SVC_INFO command.
*/
typedef struct {
   INT8U           rc;                           ///< Return code, among values in #dn_api_rc_t.
   dn_moteid_t     dest;                         ///< Short address of in-mesh service destination.
   INT8U           type;                         ///< Type of the service.
   INT8U           status;                       ///< State of the service.
   INT32U          value;                        ///< Inter-packet interval (in ms)
} dn_api_loc_rsp_get_service_t;


//===== DN_API_LOC_CMD_OPEN_SOCKET

/**
\brief Format of the request when issuing a #DN_API_LOC_CMD_OPEN_SOCKET command.
*/
typedef struct {
   INT8U           protocol;                     ///< Transport protocol.
} dn_api_loc_open_socket_t;

/**
\brief Format of the response when issuing a #DN_API_LOC_CMD_OPEN_SOCKET command.
*/
typedef struct {
   INT8U           rc;                           ///< Return code, among values in #dn_api_rc_t.
   INT8U           socketId;                     ///< Socket ID.
} dn_api_loc_rsp_open_socket_t;

//===== DN_API_LOC_CMD_CLOSE_SOCKET

/**
\brief Format of the request when issuing a #DN_API_LOC_CMD_CLOSE_SOCKET command.
*/
typedef struct {
   INT8U           socketId;                     ///< Socket ID.
} dn_api_loc_close_socket_t;

/**
\brief Format of the response when issuing a #DN_API_LOC_CMD_CLOSE_SOCKET command.
*/
typedef dn_api_rc_rsp_t dn_api_loc_rsp_close_socket_t;

//===== DN_API_LOC_CMD_BIND_SOCKET

/**
\brief Format of the request when issuing a #DN_API_LOC_CMD_BIND_SOCKET command.
*/
typedef struct {
   INT8U           socketId;                     ///< Socket ID.
   INT16U          port;                         ///< Port number to bind the socket to.
} dn_api_loc_bind_socket_t;

/**
\brief Format of the response when issuing a #DN_API_LOC_CMD_BIND_SOCKET command.
*/
typedef dn_api_rc_rsp_t dn_api_loc_rsp_bind_socket_t;


//===== DN_API_LOC_CMD_SOCKET_INFO

/**
\brief Format of the request when issuing a #DN_API_LOC_CMD_SOCKET_INFO command.
*/
typedef struct {
   INT8U           index;                     // index of requested socket (i.e. 0=first, 1=second, etc)
} dn_api_loc_socket_info_t;

/**
\brief Format of the response when issuing a #DN_API_LOC_CMD_SOCKET_INFO command.
*/
typedef struct {
   INT8U           rc;                           ///< Return code, among values in #dn_api_rc_t.
   INT8U           index;                        ///< Index of the socket.
   INT8U           socketId;                     ///< Socket ID.
   INT8U           protocol;                     ///< Socket protocol.
   INT8U           bindState;                    ///< Socket bind state.
   INT16U          port;                         ///< Socket port number.
} dn_api_loc_rsp_socket_info_t;

//===== DN_API_LOC_CMD_SENDTO

/**
\brief Format of the request when issuing a #DN_API_LOC_CMD_SENDTO command.
*/
typedef struct {
   INT8U           socketId;                     ///< Socket ID.
   dn_ipv6_addr_t  destAddr;                     ///< Destination IPv6 address.
   INT16U          destPort;                     ///< Destination port.
   INT8U           serviceType;                  ///< Service type.
   INT8U           priority;                     ///< Priority of the packet.
   INT16U          packetId;                     ///< User-defined packet ID for txDone notification; <tt>0xFFFF</tt>=do not generate notification.
   #ifndef __cplusplus
   INT8U           payload[];                    ///< Payload of the packet.
   #endif
} dn_api_loc_sendto_t;

/**
\brief Format of the response when issuing a #DN_API_LOC_CMD_SENDTO command.
*/
typedef dn_api_rc_rsp_t dn_api_loc_rsp_sendto_t;

#define LOC_SENDTO_DATA_OFFSET ((((dn_api_loc_sendto_t*)0)->payload) - ((INT8U *)(dn_api_loc_sendto_t*)0)) 

//===== DN_API_LOC_CMD_TUNNEL

//typedef struct{
//   INT8U   data[1];
//} dn_api_loc_tun_t;

/**
\brief Format of the response when issuing a #DN_API_LOC_CMD_TUNNEL command.
*/
typedef struct{
   INT8U           rc;                           ///< Return code, among values in #dn_api_rc_t.
   #ifndef __cplusplus
   INT8U           data[];
   #endif
} dn_api_loc_rsp_tun_t;

//===== DN_API_LOC_CMD_AP_SEND

/**
\brief apsend control fields.
*/
typedef struct{
#ifdef L_ENDIAN
   INT8U           priority:2;                   ///< 00=low, 01=med, 10=high, 11=ctrl
   INT8U           linkCtrl:1;                   ///< 0=default link selection. 1=use frame/slot/offset
   INT8U           reserved:5;
#else
   INT8U           reserved:5;
   INT8U           linkCtrl:1;                   ///< 0=default link selection. 1=use frame/slot/offset
   INT8U           priority:2;                   ///< 00=low, 01=med, 10=high, 11=ctrl
#endif

   INT16U          packetId;                     ///< Packet ID (used by txDone notification); 0xFFFF=no notif
   INT8U           frameId;                      ///< Output link definition
   INT32U          timeslot;
   INT8U           channel;
   INT16U          dest;
} dn_api_loc_apsend_ctrl_t;

/**
\brief Format of the request when issuing a #DN_API_LOC_CMD_AP_SEND command. 
*/
typedef struct {
   dn_api_loc_apsend_ctrl_t control;          ///< control
   INT8U           payload[DN_MAX_MAC_PAYLOAD];  ///< max payload (includes mesh hdr, 6lowpan, user payload)
} dn_api_loc_apsend_t;

/**
\brief Format of the response when issuing a #DN_API_LOC_CMD_AP_SEND command.
*/
typedef dn_api_rc_rsp_t dn_api_loc_rsp_apsend_t;

//===== DN_API_LOC_CMD_SEARCH

// note: the request when issuing a #DN_API_LOC_CMD_SEARCH command has no payload.

/**
\brief Format of the response when issuing a #DN_API_LOC_CMD_SEARCH command.
*/
typedef dn_api_rc_rsp_t dn_api_loc_rsp_search_t;

//===== DN_API_LOC_CMD_WRITE_GPS_STATUS_

/**
\brief Format of the request when issuing a #DN_API_LOC_CMD_WRITE_GPS_STATUS command.
*/
typedef struct {
   INT8U           status;                    ///< GPS status.
} dn_api_loc_write_gps_status_t;

/**
\brief Format of the response when issuing a #DN_API_LOC_CMD_WRITE_GPS_STATUS command.
*/
typedef dn_api_rc_rsp_t dn_api_loc_rsp_write_gps_status_t;


/**
\}
*/

//=========================== notification formats ============================

/**
\addtogroup loc_intf_notif_formats Notification formats
\brief Formats of the notifications received over the local interface.
\{
*/

//===== DN_API_LOC_NOTIF_EVENTS

/**
\brief Format of a #DN_API_LOC_NOTIF_EVENTS notification.
*/
typedef struct {
   INT32U          events;                       ///< Bitmap of recent events.
   INT8U           state;                        ///< Current mote state.
   INT32U          alarms;                       ///< Bitmap of current alarms.
} dn_api_loc_notif_events_t;

//===== DN_API_LOC_NOTIF_RECEIVED

/**
\brief Format of a #DN_API_LOC_NOTIF_RECEIVED notification.
*/
typedef struct {
   INT8U           socketId;                     ///< Socket ID on which the packet was received.
   dn_ipv6_addr_t  sourceAddr;                   ///< Source IPv6 address of the packet.
   INT16U          sourcePort;                   ///< Source port of the packet.
   #ifndef __cplusplus
   INT8U           data[];                       ///< Payload of the packet.
   #endif
} dn_api_loc_notif_received_t;

//define dn_api_loc_notif__MACRX
//typedef struct {
//   INT8U   data[1];     /* packet payload */
//}  dn_api_loc_macrx_t;

//===== DN_API_LOC_NOTIF_TXDONE

/**
\brief Format of a #DN_API_LOC_NOTIF_TXDONE notification.
*/
typedef struct {
   INT16U          packetId;                     ///< Packet id provided in #dn_api_loc_sendto_t::packetId.
   INT8U           status;                       ///< Packet transmission status.
} dn_api_loc_notif_txdone_t;

//===== DN_API_LOC_NOTIF_TIME

/**
\brief Format of a #DN_API_LOC_NOTIF_TIME notification.
*/
typedef struct {
   INT32U          upTime;                       ///< Time since last reset, in seconds.
   dn_utc_time_t   utcTime;                      ///< UTC time when the TIMEn pin was asserted.
   dn_asn_t        asn;                          ///< Absolute Slot Number (ASN) value when the TIMEn pin was asserted.
   INT16U          offset;                       ///< Number of micro-seconds since the start of the time slot when the TIMEn pin was asserted.
   INT16U          asnSubOffset;                 ///< Number of nano-seconds since the start of the time slot when the TIMEn pin was asserted.
} dn_api_loc_notif_time_t;

//===== DN_API_LOC_NOTIF_ADVRX

/**
\brief Format of a #DN_API_LOC_NOTIF_ADVRX notification.
*/
typedef struct {
   INT16U          netId;                        ///< The Network ID contained in the advertisement just received.
   INT16U          moteId;                       ///< The source address contained in the advertisement just received, i.e. the ID of its sender.
   INT8S           rssi;                         ///< The Received Signal Strength Indicator (RSSI) at which the advertisement was received.
   INT8U           joinPri;                      ///< The join priority (hop depth) contained in the advertisement just received.
} dn_api_loc_notif_adv_t;

//===== DN_API_LOC_NOTIF_READY_FOR_TIME

/**
\brief Format of a #DN_API_LOC_NOTIF_READY_FOR_TIME 
*      notification.
*/
typedef struct {
   INT8U           seqNum;                       ///< Sequence number.
} dn_api_loc_notif_ready_for_time_t;

/**
\}
*/

//=========================== defines =========================================

/// \brief Maximum number of bytes in a message exchanged over the local interface.
#define DN_API_LOC_MAXMSG_SIZE              (sizeof(dn_api_cmd_hdr_t) + sizeof(dn_api_loc_apsend_t))

/// \brief Maximum number of bytes in a request sent over the local interface.
#define DN_API_LOC_MAX_REQ_SIZE             DN_API_LOC_MAXMSG_SIZE

/// \brief Maximum number of bytes in a response received over the local interface.
#define DN_API_LOC_MAX_RESP_SIZE            DN_API_LOC_MAXMSG_SIZE

/// \brief Maximum number of bytes in a notification received over the local interface.
#define DN_API_LOC_MAX_NOTIF_SIZE           DN_API_LOC_MAXMSG_SIZE

/// \brief Maximum number of application-level payload bytes in a packet.
#define DN_API_LOC_MAX_USERPAYL_SIZE        90

PACKED_STOP // back to default packing

/**
// end of loc_intf
\}
*/

#endif /* _DN_API_LOCAL_H */
