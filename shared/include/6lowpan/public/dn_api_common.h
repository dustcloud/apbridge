/*
Copyright (c) 2010, Dust Networks.  All rights reserved.
*/

#ifndef _DN_API_COMMON_H
#define _DN_API_COMMON_H
#include "dn_typedef.h"
#include "dn_pack.h"

/**
\addtogroup loc_intf
\{
*/

/**
\addtogroup loc_intf_common Common
\brief Common definitions and header formats of the \ref loc_intf.
\{
*/

PACKED_START // All structures are packed starting here

//=========================== defines/enums ===================================

/**
\name Command and notification identifiers
\anchor dn_api_common_cmdId
\{
*/
#define DN_API_LOC_CMD_SETPARAM                  0x01      ///< Identifier of the "set parameter" command.
#define DN_API_LOC_CMD_GETPARAM                  0x02      ///< Identifier of the "get parameter" command.
//                                               0x03      unused
//                                               0x04      unused
//                                               0x05      unused
#define DN_API_LOC_CMD_JOIN                      0x06      ///< Identifier of the "join" command.
#define DN_API_LOC_CMD_DISCONNECT                0x07      ///< Identifier of the "disconnect" command.
#define DN_API_LOC_CMD_RESET                     0x08      ///< Identifier of the "reset" command.
#define DN_API_LOC_CMD_LOWPWRSLEEP               0x09      ///< Identifier of the "low power sleep" command.
//                                               0x0A      unused
//                                               0x0B      unused
#define DN_API_LOC_CMD_TESTRADIORX               0x0C      ///< Identifier of the "test radio RX" command.
#define DN_API_LOC_NOTIF_TIME                    0x0D      ///< Identifier of the "time" notification.
#define DN_API_LOC_NOTIF_EVENTS                  0x0F      ///< Identifier of the "events" notification.
#define DN_API_LOC_CMD_CLEARNV                   0x10      ///< Identifier of the "clear non-volatile memory" command.
#define DN_API_LOC_CMD_SERVICE_REQUEST           0x11      ///< Identifier of the "request service" command.
#define DN_API_LOC_CMD_GET_SVC_INFO              0x12      ///< Identifier of the "get service info" command.
//                                               0x13      unused
//                                               0x14      unused
#define DN_API_LOC_CMD_OPEN_SOCKET               0x15      ///< Identifier of the "open socket" command.
#define DN_API_LOC_CMD_CLOSE_SOCKET              0x16      ///< Identifier of the "close socket" command.
#define DN_API_LOC_CMD_BIND_SOCKET               0x17      ///< Identifier of the "bind socket" command.
#define DN_API_LOC_CMD_SENDTO                    0x18      ///< Identifier of the "send to" command.
#define DN_API_LOC_NOTIF_RECEIVED                0x19      ///< Identifier of the "data reception" notification.
#define DN_API_LOC_CMD_TUNNEL                    0x21      ///< <i>Reserved. Do not use.</i>
//                                               0x22      unused
#define DN_API_LOC_CMD_AP_SEND                   0x23      ///< Send packet into network (used by AP only, not implemented on motes)
#define DN_API_LOC_CMD_SEARCH                    0x24      ///< Identifier of the "search" notification.
#define DN_API_LOC_NOTIF_TXDONE                  0x25      ///< Identifier of the "transmission done" notification.
#define DN_API_LOC_NOTIF_ADVRX                   0x26      ///< Identifier of the "advertisement received" notification.
#define DN_API_LOC_NOTIF_AP_RECEIVE              0x27      ///< Received packet from network (used by AP only, not implemented on motes)
#define DN_API_LOC_CMD_TESTRADIOTX               0x28      ///< Identifier of the "test radio TX" command.
#define DN_API_LOC_CMD_ZEROIZE                   0x29      ///< Identifier of the "zeroize" command.
#define DN_API_LOC_CMD_SEND_RAW                  0x2A      ///< used in dnm_local, never sent to stack
#define DN_API_LOC_CMD_SOCKET_INFO               0x2B      ///< Get info about the previously opened socket
#define DN_API_LOC_NOTIF_READY_FOR_TIME          0x2C      ///< Ready for time notification (used by AP only, not implemented on motes) 
#define DN_API_LOC_CMD_WRITE_GPS_STATUS          0x2D      ///< Write GPS status (used by AP only, not implemented on motes) 
/**
\}
*/

/**
\name Event Flags
\anchor dn_api_common_event_flags
 
These are the possible flags associated with the "events" notification, see
#DN_API_LOC_NOTIF_EVENTS.
\{
*/
#define DN_API_LOC_EV_BOOT                       0x01      ///< The mote booted up.
#define DN_API_LOC_EV_ALARMS_CHG                 0x02      ///< The value of the alarms field has changed.
#define DN_API_LOC_EV_TIME_CHG                   0x04      ///< The UTC time on the mote has changed.
#define DN_API_LOC_EV_JOINFAIL                   0x08      ///< A join operation failed.
#define DN_API_LOC_EV_DISCON                     0x10      ///< The mote disconnected from the network.
#define DN_API_LOC_EV_OPERATIONAL                0x20      ///< The mote is ready to send data.
//                                               0x40      unused
#define DN_API_LOC_EV_SVC_CHG                    0x80      ///< The mote's service(s) changed.
#define DN_API_LOC_EV_JOINSTART                  0x100     ///< The mote initiated a joining sequence.
#define DN_API_LOC_EV_AP_SYNC_LOST               0x200     ///< AP lost sync with PPS signal
/**
\}
*/

/**
\name Set/Get Parameters
\anchor dn_api_common_setgetparams
\{
*/
#define DN_API_PARAM_MACADDR                     0x01      ///< Identifier of the "MAC address of the mote" parameter.
#define DN_API_PARAM_JOINKEY                     0x02      ///< Identifier of the "security join key" parameter.
#define DN_API_PARAM_NETID                       0x03      ///< Identifier of the "network identifier" parameter.
#define DN_API_PARAM_TXPOWER                     0x04      ///< Identifier of the "transmit power" parameter.
//                                               0x05      unused
#define DN_API_PARAM_JOINDUTYCYCLE               0x06      ///< Identifier of the "join duty cycle" parameter.
//                                               0x07      unused
//                                               0x08      unused
//                                               0x09      unused
//                                               0x0A      unused
#define DN_API_PARAM_EVENTMASK                   0x0B      ///< Identifier of the "event mask" parameter.
#define DN_API_PARAM_MOTEINFO                    0x0C      ///< Identifier of the "mote information" parameter.
#define DN_API_PARAM_NETINFO                     0x0D      ///< Identifier of the "network information" parameter.
#define DN_API_PARAM_MOTESTATUS                  0x0E      ///< Identifier of the "mote status" parameter.
#define DN_API_PARAM_TIME                        0x0F      ///< Identifier of the "time information" parameter.
#define DN_API_PARAM_CHARGE                      0x10      ///< Identifier of the "charge" parameter.
#define DN_API_PARAM_TESTRADIORXSTATS            0x11      ///< Identifier of the "statistics from radio test" parameter.
//                                               0x12      unused
//                                               0x13      unused
//                                               0x14      unused
#define DN_API_PARAM_OTAP_LOCKOUT                0x15      ///< Identifier of the "OTAP lockout" parameter.
#define DN_API_PARAM_MACMICKEY                   0x16      ///< Identifier of the "MAC MIC key" parameter.
#define DN_API_PARAM_SHORTADDR                   0x17      ///< Identifier of the "mote's short address" parameter.
#define DN_API_PARAM_IP6ADDR                     0x18      ///< Identifier of the "mote's IPv6 address" parameter.
#define DN_API_PARAM_CCAMODE                     0x19      ///< Identifier of the "Clear Channel Assessment (CCA) mode" parameter.
#define DN_API_PARAM_CHANNELS                    0x1A      ///< Identifier of the "channels" parameter.
#define DN_API_PARAM_ADVGRAPH                    0x1B      ///< Identifier of the "advertisement graph" parameter.
#define DN_API_PARAM_HRTMR                       0x1C      ///< Identifier of the "health report timer" parameter.
#define DN_API_PARAM_ROUTINGMODE                 0x1D      ///< Identifier of the "routing mode" parameter.
#define DN_API_PARAM_APPINFO                     0x1E      ///< Identifier of the "application information" parameter.
#define DN_API_PARAM_PWRSRCINFO                  0x1F      ///< Identifier of the "power source information" parameter.
#define DN_API_PARAM_PWRCOSTINFO                 0x20      ///< Identifier of the "power cost information" parameter.
#define DN_API_PARAM_MOBILITY                    0x21      ///< Identifier of the "mobility" parameter.
#define DN_API_PARAM_ADVKEY                      0x22      ///< Identifier of the "advertisement key" parameter.
#define DN_API_PARAM_SIZEINFO                    0x23      ///< Identifier of the "size information" parameter.
#define DN_API_PARAM_AUTOJOIN                    0x24      ///< Identifier of the "auto-join mode" parameter.
#define DN_API_PARAM_PATHALARMGEN                0x25      ///< Identifier of the "path alarm generation" parameter.
#define DN_API_PARAM_AP_CLKSRC                   0x26      ///< Identifier of the "AP clock source" parameter.
#define DN_API_PARAM_AP_STATS                    0x27      ///< Identifier of the "AP statistics" parameter.
#define DN_API_PARAM_AP_STATUS                   0x28      ///< Identifier of the "AP status" parameter.
#define DN_API_PARAM_ANT_GAIN                    0x29      ///< Identifier of the "Antenna gain" parameter.
#define DN_API_PARAM_EU_COMPLIANT_MODE           0x2A      ///< Identifier of the "EU compliant mode" parameter.
#define DN_API_PARAM_SIZEINFOEXT                 0x2B      ///< Identifier of the "size information extended" parameter.
/**
\}
*/

/**
\brief Common return codes.
\anchor dn_api_common_rc
*/
typedef enum {
   DN_API_RC_OK                                  = 0x00,   ///< The command completed successfully.
   DN_API_RC_ERROR                               = 0x01,   ///< Processing error.
   DN_API_RC_RSVD1                               = 0x02,   ///< <i>Reserved. Do not use.</i>
   DN_API_RC_BUSY                                = 0x03,   ///< The device is  currently unavailable to perform the operation.
   DN_API_RC_INVALID_LEN                         = 0x04,   ///< Invalid length.
   DN_API_RC_INV_STATE                           = 0x05,   ///< Invalid state.
   DN_API_RC_UNSUPPORTED                         = 0x06,   ///< Unsupported command or operation.
   DN_API_RC_UNKNOWN_PARAM                       = 0x07,   ///< Unknown parameter.
   DN_API_RC_UNKNOWN_CMD                         = 0x08,   ///< Unknown command.
   DN_API_RC_WRITE_FAIL                          = 0x09,   ///< Could not write to persistent storage.
   DN_API_RC_READ_FAIL                           = 0x0A,   ///< Could not read from persistent storage.
   DN_API_RC_LOW_VOLTAGE                         = 0x0B,   ///< Low voltage detected.
   DN_API_RC_NO_RESOURCES                        = 0x0C,   ///< Could not process command due to low resources (e.g. no buffers).
   DN_API_RC_INCOMPLETE_JOIN_INFO                = 0x0D,   ///< Incomplete configuration to start joining.
   DN_API_RC_NOT_FOUND                           = 0x0E,   ///< Resource not found.
   DN_API_RC_INVALID_VALUE                       = 0x0F,   ///< Access to resource or command is denied.
   DN_API_RC_ACCESS_DENIED                       = 0x10,   ///< Access to this command/variable is denied.
   DN_API_RC_DUPLICATE                           = 0x11,   ///< Duplicate.
   DN_API_RC_ERASE_FAIL                          = 0x12,   ///< Erase operation failed.
} dn_api_rc_t;

/**
\name Transport protocol
\anchor dn_api_common_transport

Options for:
- #dn_api_loc_open_socket_t::protocol.
\{
*/
#define DN_API_PROTO_UDP                         0         ///< User Datagram Protocol (UDP).
/**
\}
*/

/**
\name Routing mode
\anchor dn_api_common_routing_mode

Options for:
- #dn_api_set_rtmode_t::mode
- #dn_api_rsp_get_rtmode_t::mode.
\{
*/
#define DN_API_ROUTING_ENABLED                   0         ///< Routing is enabled.
#define DN_API_ROUTING_DISABLED                  1         ///< Routing is disabled.
/**
\}
*/

/**
\name Mobility type
\anchor dn_api_common_mobility_type

Options for:
- #dn_api_set_mobility_t::type
- #dn_api_rsp_get_mobility_t::type.
\{
*/
#define DN_API_LOCATION_UNUSED                   0         ///< The mote's location is not used.
#define DN_API_LOCATION_KNOWN                    1         ///< The mote's location is known.
#define DN_API_LOCATION_UNKNOWN                  2         ///< The mote's location is unknown.
#define DN_API_LOCATION_MOBILE                   3         ///< The mote is mobile.
/**
\}
*/

/**
\name Service type
\anchor dn_api_common_service_type

Options for:
- #dn_api_loc_sendto_t::serviceType
- #dn_api_loc_svcrequest_t::type
- #dn_api_loc_get_service_t::type.
- #dn_api_loc_rsp_get_service_t::type.
\{
*/
#define DN_API_SERVICE_TYPE_BW                   0         ///< "Bandwidth" service type.
#define DN_API_SERVICE_TYPE_LATENCY              1         ///< "Latency" service type.
/**
\}
*/

/**
\name Service state
\anchor dn_api_common_service_state

Options for:
- #dn_api_loc_rsp_get_service_t::status
\{
*/
#define DN_API_SERVICE_STATE_REQ_COMPLETED       0         ///< The service request is completed.
#define DN_API_SERVICE_STATE_REQ_PENDING         1         ///< The service request is pending.
/**
\}
*/

/**
\name Packet priority
\anchor dn_api_common_pkt_prio

Options for:
- #dn_api_loc_sendto_t::priority
\{
*/
#define DN_API_PRIORITY_LOW                      0         ///< This is a low priority packet.
#define DN_API_PRIORITY_MED                      1         ///< This is a medium priority packet.
#define DN_API_PRIORITY_HIGH                     2         ///< This is a high priority packet.
#define DN_API_PRIORITY_CTRL                     3         ///< This is a packet at control priority.
/**
\}
*/

/**
\name Mote state
\anchor dn_api_common_mote_state

Options for:
- #dn_api_loc_notif_events_t::state
- #dn_api_rsp_get_motestatus_t::state
\{
*/
#define DN_API_ST_INIT                           0         ///< Initializing.
#define DN_API_ST_IDLE                           1         ///< Idle, ready to be configured or join a network.
#define DN_API_ST_SEARCHING                      2         ///< Received join command, searching for network.
#define DN_API_ST_NEGO                           3         ///< Sent a join request.
#define DN_API_ST_CONNECTED                      4         ///< Received at least one packet from the manager.
#define DN_API_ST_OPERATIONAL                    5         ///< Configured by the manager and ready to send/receive data.
#define DN_API_ST_DISCONNECTED                   6         ///< Disconnected from the network.
#define DN_API_ST_RADIOTEST                      7         ///< Radio test.
#define DN_API_ST_PROMISC_LISTEN                 8         ///< Received search command, listening for nearby networks
/**
\}
*/

/// \brief Number of elements in #dn_api_pwrsrcinfo_t::limit.
#define DN_API_PWR_LIMIT_CNT                     3

// power cost defs for DN_API_PARAM_PWRCOSTINFO
#define DN_API_PWRCOSTINFO_LINE             0xFFFF    
#define DN_API_PWRCOSTINFO_UNLIM            0xFFFE

/**
\name Clear Channel Assessment (CCA) mode
\anchor dn_api_common_cca_mode

Options for:
- #dn_api_set_ccamode_t::mode
\{
*/
#define DN_API_CCA_MODE_OFF                      0         ///< CCA is off.
#define DN_API_CCA_MODE_ON                       1         ///< CCA is on.
/**
\}
*/

/**
\name Transmission power
\anchor dn_api_common_tx_power

Options for:
- #dn_api_set_txpower_t::txPower
- #dn_api_rsp_get_txpower_t::txPower
\{
*/
#define DN_API_TXPOWER_PA_OFF                    0         ///< Have the radio transmit at  0dBm.
#define DN_API_TXPOWER_PA_ON                     8         ///< Have the radio transmit at +8dBm
/**
\}
*/

/// \brief Maximum join duty cycle (corresponding to 100%).
#define DN_API_MAX_JOINDC                        0xFF

/**
\name Transmission status
\anchor dn_api_common_tx_status

Options for:
- #dn_api_loc_notif_txdone_t::status
\{
*/
#define DN_API_TXSTATUS_OK                       0         ///< Transmission succeeded.
#define DN_API_TXSTATUS_FAIL                     1         ///< Transmission failed.
/**
\}
*/

/// \brief Default route for all services.
#define  DN_API_ROUTE_ALLSERVICES                0xFF

/**
\name Alarm flags
\anchor dn_api_common_alarm_flags

Options for:
- #dn_api_loc_notif_events_t::alarms
- #dn_api_rsp_get_motestatus_t::alarms
\{
*/
#define DN_API_ALARM_NVERR                       0x1       ///< Error in non-volatile (NV) configuration storage detected.
#define DN_API_ALARM_LOWVOLTAGE                  0x2       ///< Low voltage detected.
#define DN_API_ALARM_OTPERR                      0x4       ///< Error in calibration or bsp data detected.
#define DN_API_ALARM_SEND_NOTREADY               0x8       ///< Not ready to send. This alarm will be declared while the mote is not ready to accept packets into the network.
/**
\}
*/

/**
\name Type of radiotest transmission
\anchor dn_api_common_radioTx_type

Options for:
- dn_api_loc_testrftx_t::type
\{
*/
#define DN_API_RADIOTX_TYPE_PKT                  0         ///< Packet transmission.
#define DN_API_RADIOTX_TYPE_CM                   1         ///< Continuous modulation.
#define DN_API_RADIOTX_TYPE_CW                   2         ///< Continuous wave.
#define DN_API_RADIOTX_TYPE_PKCCA                3         ///< Packet transmission with CCA mode 
/**
\}
*/

/**
\name GPS status

Options for:
- #dn_api_loc_write_gps_status_t::status 
\{
*/
#define DN_API_GPS_STATUS_SYNCHED               0
#define DN_API_GPS_STATUS_LOST                  1
/**
\}
*/

/**
\name clock source status

Options for:
- #dn_api_net_rsp_read_clk_src_status_t::status 
\{
*/
#define DN_API_CLK_SRC_STATUS_VALID               0
#define DN_API_CLK_SRC_STATUS_INVALID             1
/**
\}
*/

/**
\name EU compliant mode

Options for:
- #dn_api_set_comp_mode_t::compMode
- #dn_api_rsp_get_comp_mode_t::compMode
\{
*/
#define DN_API_EU_COMP_MODE_OFF                   0         ///< EU compliant mode off.
#define DN_API_EU_COMP_MODE_ON                    1         ///< EU compliant mode on.
/**
\}
*/

/// \brief len value in dn_api_cmd_hdr_t for extended length
#define  DN_API_LOC_EXT_HDR_LEN                 0xFF

//=========================== structs =========================================

/**
\brief Command header.
*/
typedef struct {
   INT8U                cmdId;                   ///< Command id.
   INT8U                len;                     ///< Number of bytes in this request, excluding this header.
} dn_api_cmd_hdr_t;

/**
\brief Format of a reply without payload.
*/
typedef struct {
   dn_api_cmd_hdr_t     hdr;                     ///< Header of the reply.
   INT8U                rc;                      ///< Return code.
} dn_api_empty_rsp_t;

/**
\brief Payload of a response containing a return code.
*/
typedef struct {
  INT8U                 rc;                      ///< Return code.
} dn_api_rc_rsp_t;

/**
\brief Header of a getParameter request.
*/
typedef struct {
   INT8U                paramId;                 ///< Identifier of the parameter to get.
} dn_api_get_hdr_t;

/**
\brief Payload for a getParameter response.
*/
typedef struct {
   INT8U                rc;                      ///< Return code.
   INT8U                paramId;                 ///< Identifier of the parameter.
} dn_api_rsp_get_hdr_t;

/**
\brief Header of a setParameter request.
*/
typedef struct {
   INT8U                paramId;                 ///< Identifier of the parameter to set.
} dn_api_set_hdr_t;

/**
\brief Payload for a setParameter response.
*/
typedef struct {
   INT8U                rc;                      ///< Return code.
   INT8U                paramId;                 ///< Identifier of the parameter.
} dn_api_rsp_set_hdr_t;

/**
\brief Software version information.

The software version is represented by a number of format
<tt>MAJOR.MINOR.PATCH.BUILD</tt>.
*/
typedef struct {
   INT8U                major;                   ///< The <tt>MAJOR</tt> component of the software version number.
   INT8U                minor;                   ///< The <tt>MINOR</tt> component of the software version number.
   INT8U                patch;                   ///< The <tt>PATCH</tt> component of the software version number.
   INT16U               build;                   ///< The <tt>BUILD</tt> component of the software version number.
} dn_api_swver_t;

/**
\brief Power limit information.
*/
typedef struct {
   INT16U               currentLimit;            ///< Current limit, in micro-amps (uA). Set to 0 for an empty limit entry.
   INT16U               dischargePeriod;         ///< Discharge period, in seconds.
   INT16U               rechargePeriod;          ///< Recharge period, in seconds.
} dn_api_pwrlim_t;

/**
\brief Power source information.
*/
typedef struct {
   INT16U               maxStCurrent;            ///< Maximum steady-state current. Set to <tt>0xffff</tt> to indicate "no limit".
   INT8U                minLifeTime;             ///< Minimum lifetime, in months. Set to <tt>0</tt> to indicate "no limit".
   dn_api_pwrlim_t      limit[DN_API_PWR_LIMIT_CNT];  ///< Temporary current limits.
} dn_api_pwrsrcinfo_t;

/**
\brief Power cost information.
*/
typedef struct {
   INT8U                maxTxCost;               ///< Cost of maximum PA transmission, in uC, in 7.1 format.
   INT8U                maxRxCost;               ///< Cost of maximum PA reception, in uC, in 7.1 format.
   INT8U                minTxCost;               ///< Cost of minimum PA transmission, in uC, in 7.1 format.
   INT8U                minRxCost;               ///< Cost of minimum PA reception, in uC, in 7.1 format.
} dn_api_pwrcostinfo_t;

/**
\brief Command header for packets with request length exceeding 
*      one byte.
*/
typedef struct {
   INT8U                cmdId;                   ///< Command id.
   INT8U                len;                     ///< Always set to DN_API_LOC_EXT_HDR_LEN.
   INT16U               extLen;                  ///< Number of bytes in this request, excluding this header.
} dn_api_cmd_exthdr_t;

PACKED_STOP // back to default packing

/**
// end of loc_intf_common
\}
*/

/**
// end of loc_intf
\}
*/

#endif // DN_API_COMMON_H
