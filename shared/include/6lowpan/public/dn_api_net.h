/*
 * Copyright (c) 2009, Dust Networks.  All rights reserved.
 *
 * $HeadURL: https://subversion/software/trunk/shared/include/6lowpan/dn_api_net.h $
 * $Rev: 42425 $
 * $Author: alushin $
 * $LastChangedDate: 2011-01-07 17:34:14 -0800 (Fri, 07 Jan 2011) $ 
 */

#ifndef _DN_API_NET_H
#define _DN_API_NET_H

#include "dn_pack.h"

#include "dn_mesh.h"
#include "dn_api_common.h"


PACKED_START /** all structures that follow are packed */


/**
\name Network API commands identifiers
\{
*/
#define DN_API_NET_CMD_SET_PARAM                 0x01      ///< Identifier of the "set parameter" command.
#define DN_API_NET_CMD_GET_PARAM                 0x02      ///< Identifier of the "get parameter" command.
#define DN_API_NET_CMD_RESERVED1                 0x03      ///< <i>Reserved. Do not use.</i>
#define DN_API_NET_CMD_RESERVED2                 0x04      ///< <i>Reserved. Do not use.</i>
#define DN_API_NET_CMD_ADD_FRAME                 0x05      ///< Identifier of the "add frame" command.
#define DN_API_NET_CMD_DEL_FRAME                 0x06      ///< Identifier of the "delete frame" command.
#define DN_API_NET_CMD_CH_SIZE_FRAME             0x07      ///< Identifier of the "change frame size" command.
#define DN_API_NET_CMD_CTRL_FRAME                0x08      ///< Identifier of the "control frame" command.
#define DN_API_NET_CMD_ADD_LINK                  0x09      ///< Identifier of the "add link" command.
#define DN_API_NET_CMD_DEL_LINK                  0x0A      ///< Identifier of the "delete link" command.
#define DN_API_NET_CMD_ADD_MUL_LINK              0x0B      ///< Identifier of the "add multiple links" command.
#define DN_API_NET_CMD_DEL_MUL_LINK              0x0C      ///< Identifier of the "delete multiple links" command.
#define DN_API_NET_CMD_ADD_SESSION               0x0D      ///< Identifier of the "add session" command.
#define DN_API_NET_CMD_DEL_SESSION               0x0E      ///< Identifier of the "delete session" command.
#define DN_API_NET_CMD_ADD_CONN                  0x0F      ///< Identifier of the "add connection" command.
#define DN_API_NET_CMD_DEL_CONN                  0x10      ///< Identifier of the "delete connection" command.
#define DN_API_NET_CMD_ACTIVATE                  0x11      ///< Identifier of the "activate" command.
#define DN_API_NET_CMD_SET_NBR_PROP              0x12      ///< Identifier of the "set neighbor property" command.
#define DN_API_NET_CMD_ADD_GR_ROUTE              0x13      ///< Identifier of the "add a graph route" command.
#define DN_API_NET_CMD_DEL_GR_ROUTE              0x14      ///< Identifier of the "delete a graph route" command.
#define DN_API_NET_CMD_ECHO_REQ                  0x15      ///< Identifier of the "echo request" command.
#define DN_API_NET_CMD_OTAP_HANDSHAKE            0x16      ///< Identifier of the "OTAP handshake" command.
#define DN_API_NET_CMD_OTAP_DATA                 0x17      ///< Identifier of the "OTAP data" command.
#define DN_API_NET_CMD_OTAP_STATUS               0x18      ///< Identifier of the "OTAP status" command.
#define DN_API_NET_CMD_OTAP_COMMIT               0x19      ///< Identifier of the "OTAP commit" command.
#define DN_API_NET_CMD_GET_LOG                   0x1A      ///< Identifier of the "get log" command.
#define DN_API_NET_CMD_GET_REQ_SRV               0x1B      ///< Identifier of the "get requested service" command.
#define DN_API_NET_CMD_ASSIGN_SRV                0x1C      ///< Identifier of the "assign service" command.
#define DN_API_NET_CMD_CH_NET_KEY                0x1D      ///< Identifier of the "change network key" command.
#define DN_API_NET_CMD_DISCONNECT                0x1E      ///< Identifier of the "disconnect" command.
#define DN_API_NET_CMD_TUN_LOC_CMD               0x1F      ///< Identifier of the "tunnel" command.
#define DN_API_NET_CMD_REFRESH_ADV               0x20      ///< Identifier of the "refresh the advertisement" command.
#define DN_API_NET_CMD_READ_FRAME                0x21      ///< Identifier of the "read frame" command.
#define DN_API_NET_CMD_READ_LINK                 0x22      ///< Identifier of the "read link" command.
#define DN_API_NET_CMD_READ_SESSION              0x23      ///< Identifier of the "read session" command.
#define DN_API_NET_CMD_READ_GR_ROUTE             0x24      ///< Identifier of the "read graph route" command.
#define DN_API_NET_CMD_SET_ADV_TIME              0x25      ///< Set min time between advertisement packet (msec)
#define DN_API_NET_CMD_READ_CLK_SRC_STATUS       0x26      ///< Identifier of the "read clock source status" command. 
#define DN_API_NET_CMD_READ_LINK_EXT             0x27      ///< Identifier of the "read link extended" command.
/**
\}
*/

/**
\name Network API notification identifiers
\{
*/
#define DN_API_NET_NOTIF_DEV_HR                  0x80      ///< Device Health Report.
#define DN_API_NET_NOTIF_NBR_HR                  0x81      ///< Neighbor Health Report.
#define DN_API_NET_NOTIF_DSCV_NBR                0x82      ///< Discovery Health Report.
#define DN_API_NET_NOTIF_PATH_ALARM              0x83      ///< Path alarm.
#define DN_API_NET_NOTIF_SR_FAIL_ALARM           0x84      ///< Source Route Failed alarm.
#define DN_API_NET_NOTIF_ECHO_RESP               0x85      ///< Echo response.
#define DN_API_NET_NOTIF_REQ_SERVICE             0x86      ///< Request service.
#define DN_API_NET_NOTIF_ACCEPT_CONTEXT          0x87      ///< Context accepted.
#define DN_API_NET_NOTIF_TRACE                   0x88      ///< Trace notification.
#define DN_API_NET_NOTIF_MOTE_JOIN               0x89      ///< Mote joined notification.
#define DN_API_NET_NOTIF_AP_JOIN                 0x90      ///< Access-Point joined notification.
#define DN_API_NET_NOTIF_HR                      0x91      ///< Extended health report
#define DN_API_NET_NOTIF_AP_RANDOM               0x92      ///< Random number generated by AP 
#define DN_API_NET_NOTIF_STATUS_CHANGE           0x93      ///< Status change 
#define DN_API_NET_NOTIF_BLINK                   0x94      ///< Blink
#define DN_API_NET_NOTIF_DSCV_NBR_REDUCED        0x95      ///< Reduced Discovery Health Report.
#define DN_API_NET_NOTIF_RSSI                    0x96      ///< RSSI report 

#define DN_API_NET_NOTIF_FIRST                   DN_API_NET_NOTIF_DEV_HR
#define DN_API_NET_NOTIF_LAST                    DN_API_NET_NOTIF_RSSI
/**
\}
*/

/**
\brief OTAP return codes.
*/
typedef enum {
   DN_API_OTAP_RC_OK                             = 0,
   DN_API_OTAP_RC_LOWBATT                        = 1,      ///< Battery voltage too low.
   DN_API_OTAP_RC_FILE                           = 2,      ///< File size, block size, start address, or execute size is wrong.
   DN_API_OTAP_RC_INVALID_PARTITION              = 3,      ///< Invalid partition information (ID, file size).
   DN_API_OTAP_RC_INVALID_APP_ID                 = 4,      ///< AppId is not correct.
   DN_API_OTAP_RC_INVALID_VER                    = 5,      ///< SW versions are not compatible for OTAP.
   DN_API_OTAP_RC_INVALID_VENDOR_ID              = 6,      ///< Invalid vendor ID.
   DN_API_OTAP_RC_RCV_ERROR                      = 7,
   DN_API_OTAP_RC_FLASH                          = 8,
   DN_API_OTAP_RC_MIC                            = 9,      ///< MIC failed on uploaded data.
   DN_API_OTAP_RC_NOT_IN_OTAP                    = 10,     ///< No OTAP handshake is initiated.
   DN_API_OTAP_RC_IOERR                          = 11,     ///< IO error.
   DN_API_OTAP_RC_CREATE                         = 12,     ///< Can not create OTAP file.
   DN_API_OTAP_RC_INVALID_EXEPAR_HDR             = 13,     ///< Wrong value for exe-partition header fieds 'signature' or 'upgrade'.
   DN_API_OTAP_RC_RAM                            = 14,     ///< Can not allocate memory.
   DN_API_OTAP_RC_UNCOMPRESS                     = 15,     ///< Uncompression error.
   DN_API_OTAP_IN_PROGRESS                       = 16,     ///< OTAP in progress.
   DN_API_OTAP_LOCK                              = 17,     ///< OTAP lock.
} dn_api_otap_rc_t;

/**
\name Frame modes
\{
*/
#define DN_API_FRAME_MODE_INACTIVE               0         ///< Frame is inactive.
#define DN_API_FRAME_MODE_ACTIVE                 1         ///< Frame is active.
/**
\}
*/

/**
\name Link types
\{
*/
#define DN_API_LINK_TYPE_NORMAL                  0         ///< Normal link.
#define DN_API_LINK_TYPE_JOIN                    1         ///< Join link.
#define DN_API_LINK_TYPE_DSCV                    2         ///< Discovery link.
#define DN_API_LINK_TYPE_ADV                     3         ///< Advertisement link.
/**
\}
*/

/**
\name Link flags
\{
*/
#define DN_API_LINK_OPT_TX                       0x01      ///< Transmit link.
#define DN_API_LINK_OPT_RX                       0x02      ///< Receive link.
#define DN_API_LINK_OPT_SHARED                   0x04      ///< Shared link.
#define DN_API_LINK_OPT_TOF                      0x08      ///< TOF ranging.
#define DN_API_LINK_OPT_NO_PF                    0x10      ///< Do not monitor path failure on this link.
/**
\}
*/

/**
\name Active flags
\{
*/
#define DN_API_ACTFL_CONRDY                      0x1       ///< Connection is ready.
#define DN_API_ACTFL_ADVRDY                      0x2       ///< Advertisment links is creatd.
#define DN_API_ACTFL_OPER                        0x4       ///< The mote is ready.
/**
\}
*/

/**
\name Session types
\{
*/
#define DN_API_SESSION_TYPE_UCAST                0         ///< Unicast session.
#define DN_API_SESSION_TYPE_BCAST                1         ///< Broadcast session.
/**
\}
*/

/**
\name Neighbor property flags
\{
*/
#define DN_API_NBR_FLAG_TIME_SOURCE              0x01      ///< This neighbor is a time source.
/**
\}
*/

/// OTAP data size.
#define  DN_API_OTAP_DATA_SIZE                   84

/// OTAP max lost blocks size
#define  DN_API_OTAP_MAXLOSTBLOCKS_SIZE          40


/**
\name Trace types
\{
*/
#define  DN_API_NET_NOTIF_TRACETYPE_CLIPRINT     0         ///< CLI print trace.
#define  DN_API_NET_NOTIF_TRACETYPE_LOGTRACE     1         ///< Log trace.
/**
\}
*/

/// Maximum size of trace string in #DN_API_NET_NOTIF_TRACE notification.
#define  DN_API_NET_NOTIF_TRACE_MAXSTRINGSIZE    80

/// AP clock sources
#define DN_API_AP_CLK_SOURCE_INTERNAL  0
#define DN_API_AP_CLK_SOURCE_NETWORK   1
#define DN_API_AP_CLK_SOURCE_PPS       2

#define DN_API_AP_NUM_RAND_BYTES    64

/// status bitmap for dn_api_net_notif_status_change_t
#define DN_API_STATUS_CHANGED_GPS   0x1

// common link definition
typedef struct {
   INT8U   frameId;    // Frame ID
   INT32U  slot;       // Timeslots
   INT8U   channel;    // Channel offset
   INT16U  moteId;     // Destination (0xFFFF - broadcast)
   INT8U   opt;        // Link options. See Common Table 46
   INT8U   type;       // Link type. See Common Table 45
}  dn_api_net_link_t;

// DN_API_NET_SETPARAM
typedef struct {
   INT8U paramId;
   #ifndef __cplusplus
   INT8U payload[]; // param-specific payload
   #endif
}  dn_api_net_setparam_t;

typedef struct {
   INT8U rc;
   INT8U paramId;
   #ifndef __cplusplus
   INT8U payload[];
   #endif
}  dn_api_net_rsp_setparam_t;


// DN_API_NET_GETPARAM
typedef struct {
   INT8U paramId;
   #ifndef __cplusplus
   INT8U payload[]; // param-specific payload
   #endif
}  dn_api_net_getparam_t;

typedef struct {
   INT8U rc;
   INT8U paramId;
   #ifndef __cplusplus
   INT8U payload[];
   #endif
}  dn_api_net_rsp_getparam_t;


// DN_API_NET_ADD_FRAME - addFrame
typedef struct {
   INT8U   frameId;       // Frame ID
   INT8U   options;       // options (bitmap) 
   INT32U  numSlots;      // Number of timeslots
}  dn_api_net_add_frame_t;

typedef dn_api_rc_rsp_t dn_api_net_rsp_add_frame_t;


// DN_API_NET_DEL_FRAME Delete Frame
typedef struct {   
   INT8U   frameId;
}   dn_api_net_del_frame_t;

typedef dn_api_rc_rsp_t dn_api_net_rsp_del_frame_t;


// DN_API_NET_CMD_READ_FRAME read Frame
typedef struct {   
   INT8U   frameId;
}   dn_api_net_read_frame_t;

typedef struct {
   INT8U   rc;                   //result code
   INT8U   numFree;              //number of free entries      
   INT8U   frameId;              //frame id to be read
   INT32U  frameSize;            //frame size
   INT8U   flags;                //0-inactive 1-active
   INT8U   firstFreeIdx;         // first free index
}   dn_api_net_rsp_read_frame_t;

// DN_API_NET_CMD_READ_LINK read link
typedef struct {   
   INT8U   linkId;
}   dn_api_net_read_link_t;

typedef struct {
   INT8U   rc;                   //result code
   INT8U   linkIdx;              //link to be read      
   INT8U   frameId;              //frame id to be read
   INT32U  slot;                 //frame size
   INT8U   chOffset;             //Channel offset
   INT16U  shortAddr;            //dest. address
   INT8U   linkOptions;          //link options
   INT8U   linkType;             //link type
   INT8U   linkQuality;          //link quality
}   dn_api_net_rsp_read_link_t;

// DN_API_NET_CMD_READ_LINK_EXT read link
typedef struct {   
   INT16U   linkId;
}   dn_api_net_read_link_ext_t;

typedef struct {
   INT8U   rc;                   //result code
   INT16U  linkIdx;              //link to be read      
   INT8U   frameId;              //frame id to be read
   INT32U  slot;                 //frame size
   INT8U   chOffset;             //Channel offset
   INT16U  shortAddr;            //dest. address
   INT8U   linkOptions;          //link options
   INT8U   linkType;             //link type
   INT8U   linkQuality;          //link quality
}   dn_api_net_rsp_read_link_ext_t;

// DN_API_NET_CMD_READ_SESSION read session
typedef struct {   
   INT16U   peerAddr;            //peer address of the session
   INT8U    sessionType;         //type of session
}   dn_api_net_read_session_t;

typedef struct {
   INT8U   rc;                   //result code
   INT16U  peerAddr;             //peer address of the session    
   INT8U   sessionType;          //type of session
   dn_macaddr_t peerMac;         // Peer MAC Addr
   INT32U       nonceCtr;        // Nonce counter
}   dn_api_net_rsp_read_session_t;

// DN_API_NET_CMD_READ_GR_ROUTE read graph route
typedef struct {   
   INT8U    graphIndex;         //index of graph table
}   dn_api_net_read_graph_route_t;

typedef struct {
   INT8U   rc;                   //result code
   INT8U  graphId;               //graph Id
}   dn_api_net_rsp_read_graph_route_t;

// DN_API_NET_CMD_SET_ADV_TIME set time between adv packet
typedef struct {   
   INT32U    advTime;         // Min time between advertisement packets (msec)
}   dn_api_net_set_advtime_t;

typedef dn_api_rc_rsp_t dn_api_net_rsp_set_advtime_t;

// DN_API_NET_CH_SIZE_FRAME  changeFrameSize
typedef struct {
   INT8U     frameId;
   INT32U    numSlots;            /* Size in number of slots. 0: frame is not active */
   dn_asn_t  execAsn;             //ASN for execution (0:immediately)
}   dn_api_net_ch_frame_t;

typedef dn_api_rc_rsp_t dn_api_net_rsp_ch_frame_t;

// DN_API_NET_CTRL_FRAME 
typedef struct {
   INT8U     frameId;
   dn_asn_t  actAsn;              //ASN of activation (0:immediately, 0xFFFFFFFFFF:never)
   dn_asn_t  deactAsn;            //ASN of deactivation (0:immediately, 0xFFFFFFFFFF: never)
}   dn_api_net_ctrl_frame_t;

typedef dn_api_rc_rsp_t dn_api_net_rsp_ctrl_frame_t;

// DN_API_NET_ADD_LINK addLink
typedef struct {
   dn_api_net_link_t    link;
}  dn_api_net_add_link_t;

typedef dn_api_rc_rsp_t dn_api_net_rsp_add_link_t;

// DN_API_NET_DEL_LINK delLink
typedef struct {
   dn_api_net_link_t    link;
}   dn_api_net_del_link_t;

typedef dn_api_rc_rsp_t dn_api_net_rsp_del_link_t;

// DN_API_NET_ADD_MUL_LINK - add Multiple Links
typedef struct {
   INT8U  numLinks;
   dn_api_net_link_t linkTmp;     //Link template
}   dn_api_net_add_mul_link_t;

typedef dn_api_rc_rsp_t dn_api_net_rsp_add_mul_link_t;

// DN_API_NET_DEL_MUL_LINK - delMulLink
typedef struct {
   INT8U  numLinks;
   dn_api_net_link_t linkTmp;
}   dn_api_net_del_mul_link_t;

typedef dn_api_rc_rsp_t dn_api_net_rsp_del_mul_link_t;

// DN_API_NET_ADD_SESSION - addSession
typedef struct {
   INT16U       peerId;           // Peer device ID
   INT8U        sessionType;      // Session type SESSION_TYPE_xxx
   dn_key_t     key;
   dn_macaddr_t peerMac;          // Peer MAC Addr
   INT32U       nonceCtr;         // Nonce counter
}  dn_api_net_add_session_t;

typedef dn_api_rc_rsp_t dn_api_net_rsp_add_session_t;

// DN_API_NET_DEL_SESSION  - delete Session
typedef struct {
   INT16U  peerId;           // Peer device ID
   INT8U   sessionType;      // Session type (see Common Table 48)
}  dn_api_net_del_session_t;

typedef dn_api_rc_rsp_t  dn_api_net_rsp_del_session_t;

// DN_API_NET_ADD_CONN - add Connection
typedef struct {
   INT8U   graphId;    
   INT16U  nbrId;
}  dn_api_net_add_con_t;

typedef dn_api_rc_rsp_t  dn_api_net_rsp_add_con_t;


// DN_API_NET_DEL_CONN - delete Connection
typedef struct {
   INT8U   graphId;    
   INT16U  nbrId;
}  dn_api_net_del_con_t;

typedef dn_api_rc_rsp_t dn_api_net_rsp_del_con_t;

// DN_API_NET_ACTIVATE - activate command
typedef struct {
   INT16U  flags;  // See DN_API_ACTFL_xxx
}    dn_api_net_activate_t;

typedef dn_api_rc_rsp_t dn_api_net_rsp_activate_t;


// DN_API_NET_SET_NBR_PROP - set Neighbor Property
typedef struct {
   INT16U  nbrId;
   INT8U   nbrProp;
}   dn_api_net_set_nbr_prop_t;

typedef dn_api_rc_rsp_t dn_api_net_rsp_set_nbr_prop_t;


// DN_API_NET_ADD_GR_ROUTE - add Graph Route
typedef struct {
   INT16U  peerId;
   INT8U   serviceType; // 0xFF for all services
   INT8U   graphId;    
}  dn_api_net_add_graph_route_t;

typedef dn_api_rc_rsp_t dn_api_net_rsp_add_graph_route_t;

// DN_API_NET_DEL_GR_ROUTE - Delete Route
typedef struct {
   INT16U  peerId;
   INT8U   serviceType;
}   dn_api_net_del_graph_route_t;

typedef dn_api_rc_rsp_t dn_api_net_rsp_del_graph_route_t;

// dn_api_net_ECHO_REQ - Echo Request
typedef struct {
   INT8U    payload[1]; // payload is optional, could be 0 bytes
}   dn_api_net_echo_req_t;

typedef dn_api_rc_rsp_t  dn_api_net_echo_resp_t;

// DN_API_NET_REFRESH_ADV - refresh advertisement payload - empty request
typedef dn_api_rc_rsp_t dn_api_net_rsp_refresh_adv_t;

// ======================== OTAP ==============================================
// OTAP File Header
typedef struct {
   INT8U          partitionId;
#ifdef L_ENDIAN
   INT8U          isExe:1;       // File is executable 
   INT8U          isCompress:1;  // File is not comressed
   INT8U          fSkipValidation:1; // Skip exeAppId+exeVendorID validation
   INT8U          reserv:5;
#else
   INT8U          reserv:5;
   INT8U          fSkipValidation:1; // Skip exeAppId+exeVendorID validation
   INT8U          isCompress:1;  // File is not comressed
   INT8U          isExe:1;       // File is executable 
#endif
   INT32U         realSize;      // Size of uncompressed file image
   // Below parameters used only for exe-file. 
   INT32U         exeStartAddr;  // Address at which the image must start; 0xFFs=don’t care
   dn_api_swver_t exeVersion;    // Software version of this executable
   dn_api_swver_t exeDependsVer; // Requires current exe version < dependsVersion; 0=don't care
   INT8U          exeAppId;      // Application ID
   INT16U         exeVendorID;   // Vendor ID. 
   INT8U          exeHwId;       // Hardware ID. 0=don't care
}  dn_api_otap_fileinfo_t;

//OTAP Handshake  NETAPI_CMD_OTAP_START ----------------------------------------
typedef struct {
#ifdef L_ENDIAN
   INT8U      isOTAP:1;       // OTAP file
   INT8U      isOverwrite:1;  // Overwrte file. OTAP-files is overwite-file
   INT8U      isTmp:1;        // Temporary file. OTAP-files is tmp-file
   INT8U      reserv:5;
#else
   INT8U      reserv:5;
   INT8U      isTmp:1;        // Temporary file
   INT8U      isOverwrite:1;  // Overwrte file
   INT8U      isOTAP:1;       // OTAP file
#endif
   INT32U     otapMIC;        // OTAP mic (use as OTAP ID)
   INT32U     fileSize;       // File size
   INT8U      blkSize;        // Size of data block 
   // char[13] for regular and dn_api_otap_fileinfo_t for OTAP
}  dn_api_net_otap_handshake_t;

typedef struct {
   INT8U      rc;         /** Command return code (see DN_API_RC_xxx */
   INT8U      otapRC;     /** Return Code (see DN_API_OTAP_RC_xxx) */
   INT32U     otapMIC;    /** OTAP id */ 
   INT32U     delay;      /** Delay between sending data packets (in ms) */
}  dn_api_net_rsp_otap_handshake_t;

//OTAP Data  NETAPI_CMD_OTAP_DATA ----------------------------------------------
typedef struct {
   INT32U     otapMIC;     /** OTAP id */ 
   INT16U     blkNum;      /** Block number */
   #ifndef __cplusplus
   INT8U      data[];      /** OTAP data (max size DN_API_OTAP_DATA_SIZE)*/
   #endif
}  dn_api_net_otap_data_t;

typedef dn_api_rc_rsp_t  dn_api_net_otap_data_resp_t;

//OTAP Status NETAPI_CMD_OTAP_SATUS --------------------------------------------
typedef struct {
   INT32U      otapMIC;     /** OTAP id */ 
}   dn_api_net_otap_status_t;

typedef struct {
   INT8U      rc;         /** Command return code (see DN_API_RC_xxx */
   INT8U      otapRC;     /** Return Code (see DN_API_OTAP_RC_xxx) */
   INT32U     otapMIC;     /** OTAP id */ 
   #ifndef __cplusplus
   INT16U     lostBlks[]; /** List of lost blocks (max DN_API_OTAP_MAXLOSTBLOCKS_SIZE)*/
   #endif
}  dn_api_net_rsp_otap_status_t;
 
//OTAP Commit NETAPI_CMD_OTAP_COMMIT -------------------------------------------
typedef struct {
   INT32U     otapMIC;     /** OTAP id */ 
}  dn_api_net_otap_commit_t;

typedef struct {
   INT8U      rc;         /** Command return code (see DN_API_RC_xxx */
   INT8U      otapRC;     /** Return Code (see DN_API_OTAP_RC_xxx) */
   INT32U     otapMIC;     /** OTAP id */ 
}  dn_api_net_rsp_otap_commit_t;
//] ===========================================================================

// DN_API_NET_GET_REQ_SRV  - get service 
typedef struct {
   dn_moteid_t    destId;     /* address  of in-mesh  destination of service */
   INT8U          type;       /* service  type */
}  dn_api_net_get_req_srv_t;

typedef struct {
   INT8U          rc;         /* result code */
   dn_moteid_t    destId;     /* address  of in-mesh  destination of service */
   INT8U          type;       /* service  type */
   INT8U          status;     /* state of the indicated service */   
   INT32U         info;       /* service type-specific information (bw or latency) */
}  dn_api_net_rsp_get_req_srv_t;

// DN_API_NET_ASSIGN_SRV   - Assign Service
typedef struct {
   dn_moteid_t     destId;
   INT8U           type;        // See SRV_TYPE_xxx
   INT32U          value;       
}  dn_api_net_assign_srv_t;

typedef dn_api_rc_rsp_t dn_api_net_rsp_assign_srv_t;

// DN_API_NET_CH_NET_KEY - Change NetKey
typedef struct {
   dn_key_t        key;
   dn_asn_t        asn;       //ASN at which key takes effect
}  dn_api_net_ch_net_key_t;

typedef dn_api_rc_rsp_t dn_api_net_rsp_ch_net_key_t;

// DN_API_NET_GET_LOG - get log
// cmd is empty
typedef dn_api_rc_rsp_t dn_api_net_rsp_getlog_t;

// DN_API_NET_RESET - disconnect command
// cmd is empty
typedef dn_api_rc_rsp_t dn_api_net_rsp_discon_t;


// DN_API_NET_NOTIF_DEV_HR - Device Health Report
typedef struct {
   INT32U  charge;        // Lifetime charge consumption (mC)

#ifdef L_ENDIAN
   INT8U   avgQOcc:4;     // Mean Queue Occupancy 
   INT8U   maxQOcc:4;     // Max queue occupancy
#else
   INT8U   maxQOcc:4;     // Max Queue Occupancy 
   INT8U   avgQOcc:4;     // Mean queue occupancy
#endif
   INT8S   temperature;   //Temperature (Celsius)
   INT16U  batteryVolt;   //Battery Voltage (mV)   

   INT16U  numTxOk;       // Number of pkts from net to MAC
   INT16U  numTxFail;     // Number of pkts not sent due to congestion and failure to allocate a pk
   INT16U  numRxOk;       // Number of received packets  
   INT16U  numRxLost;     // Number of packet lost (discarded by NET layer due to misc. errors in mesh/IP/UDP/transport/security/cmd
   INT8U   numMacDrop;    // Number of pkts dropped by MAC due to retry count, or PDU age, or no route
   INT8U   numTxBad;      //Tx failure counter for bad link
   struct {
      INT8U   frameId;
      INT32U  slot;
      INT8U   offset;
   } badLink;
}   dn_api_net_notif_devhr_t;

typedef struct {
   INT16U  nbrId;         // neighbor ID 
   INT8U   nbrFlag;       // 
   INT8S   rsl;           // RSL
   INT16U  numTxPk;       // Number of transmitted pakets
   INT16U  numTxFail;     // Number of failed transmission
   INT16U  numRxPk;       // Number of received packets
}  netapi_nbrhr_t;

// DN_API_NET_NOTIF_NBR_HR   - Neighbors' Health Report
typedef struct {
   INT8U        numItems;            // Number of neighbors read
   #ifndef __cplusplus
   netapi_nbrhr_t  nbr[];
   #endif
}   dn_api_net_notif_nbrhr_t;

typedef struct {
   INT16U  moteId;     // Neighbor ID
   INT8S   rsl;        // RSL of neighbor
   INT8U   numRx;      // Number of times neighbor was heard.
} netapi_nbr_t;

// DN_API_NET_NOTIF_DSCV_NBR - Discovered Neighbors
typedef struct { 
   INT8U         numJoinParents;      // Number of parents that the joining mote is connected to
   INT8U         numItems;            // Number of neighbors reported
   #ifndef __cplusplus
   netapi_nbr_t  nbr[];
   #endif
}   dn_api_net_notif_dscv_nbr_t;

// DN_API_NET_NOTIF_PATH_ALARM - Path Down Alarm
typedef struct {
   INT16U        nbrId;
}   dn_api_net_notif_path_alarm_t;

// DN_API_NET_NOTIF_SR_FAIL_ALARM - Source Route Failure Alarm
typedef struct {
   INT16U        nbrId;           //Short address of the neighbor
   INT16U        srcId;           //The packet's source (0 - elided)
   INT16U        dstId;           //The packet's destination
   INT32U        netMic;          //Net MIC of the failed packet
}   dn_api_net_notif_sr_alarm_t;

// DN_API_NET_NOTIF_ECHO_RESP  - Echo Response
typedef struct {
   INT8S         temperature;     //In Celsius
   INT16U        batteryVolt;     //Battery Voltage in mV
   #ifndef __cplusplus
   INT8U         payload[];       //Copy of payload received in request
   #endif
}   dn_api_net_notif_echo_resp_t;

// DN_API_NET_NOTIF_REQ_SERVICE - Request Service
typedef struct {
   dn_moteid_t  destId;
   INT8U        type;
}  dn_api_net_notif_req_srv_t;

// DN_API_NET_NOTIF_ACCEPT_CONTEXT - Accept IPv6 Address mapping with context
typedef struct {
   INT16U           context;
   dn_ipv6_addr_t   ipAddr;
}   dn_api_net_notif_accept_context_t;

// DN_API_NET_NOTIF_TRACE - Mote Trace Strings
typedef struct {
   INT8U                traceType;    // See DN_API_NET_NOTIF_TRACETYPE_xxx
   #ifndef __cplusplus
   char                 str[];        // Null-terminated string
   #endif
}   dn_api_net_notif_trace_t;

// DN_API_NET_NOTIF_MOTE_JOIN - Mote Join
typedef struct {
   dn_api_pwrsrcinfo_t  pwrSource;
   dn_api_pwrcostinfo_t pwrCost;
   INT8U                mobility;
   INT8U                route;
}   dn_api_net_notif_join_t;

// DN_API_NET_NOTIF_AP_JOIN - AP Join
typedef struct {
   INT8U  clkSource;
}   dn_api_net_notif_ap_join_t;

// DN_API_NET_NOTIF_AP_RANDOM - AP Random
typedef struct {
   INT8U  numRandBytes;                         // number of random bytes that follows
   INT8U  randData[DN_API_AP_NUM_RAND_BYTES];   // random data for Manager to generate key
}   dn_api_net_notif_ap_random_t;

// DN_API_NET_NOTIF_STATUS_CHANGE - Status change
typedef struct {
   INT32U curStatusChangePending;
}   dn_api_net_notif_status_change_t;

// DN_API_NET_CMD_READ_CLK_SRC_STATUS - Read clock source status
// cmd is empty

typedef struct {
   INT8U rc;
   INT8U status;
}   dn_api_net_rsp_read_clk_src_status_t;

#define DN_API_MAX_SUPPORTED_MAC_CHANNELS    15

typedef struct {
   INT8S  avgIdleRSSI;        // average rssi measured during idle listen on this channel
   INT16U txUnicastCnt;       // number of unicast attempts on this channel
   INT16U txUnicastFailCnt;   // number of no-acks on this channel
} netapi_rssi_report_t;

// DN_API_NET_NOTIF_RSSI - RSSI report
typedef struct { 
   netapi_rssi_report_t  report[DN_API_MAX_SUPPORTED_MAC_CHANNELS];
}   dn_api_net_notif_rssi_report_t;

PACKED_STOP


#endif /*_DN_API_NET_H */

