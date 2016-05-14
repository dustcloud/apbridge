#pragma once
#include "common.h"
#include "dn_pack.h"

/** \file APCProto.h
 * Definitions and data structures for the Manager-APC Protocol
 */

// Max size of string representation of software version
const uint32_t SIZE_STR_VER = 80;

// Protocol version
const uint8_t  APC_PROTO_VER = 0;

/**
 * Values that represent types of APC interface messages
 */
enum apc_msg_type_t : uint8_t
{
   APC_NA            = 0,
   APC_CONNECT       = 1,  ///< bi-dir First message sent by each side, carries identity of the peer
   APC_DISCONNECT    = 2,  ///<  bi-dir close connection
   APC_NET_TX        = 3,  ///<  Mgr->APC Manager sends network packet to AP
   APC_NET_RX        = 4,  ///<  APC->Mgr AP sends network packet to Manager
   APC_NET_TXDONE    = 5,  ///<  APC->Mgr Tx done notification from the AP
   APC_NET_TX_PAUSE  = 6,  ///<  bi-dir Pause network traffic to originator of this message
   APC_NET_TX_RESUME = 7,  ///<  bi-dir Resume network traffic to originator of this message
   APC_RESET_AP      = 8,  ///<  Mgr->APC Manager requests h/w reset of AP
   APC_KA            = 9,  ///<  bi-dir Keep-alive message
   APC_AP_LOST       = 10, ///<  APC->Mgr Indication that APC lost communication with the AP
   APC_GET_TIME      = 11, ///<  Mgr->APC Request UTC/ASN time map from AP
   APC_TIME_MAP      = 12, ///<  APC->Mgr Notification that contains UTC/ASN time mapping
   APC_GPS_LOCK      = 13, ///<  APC->Mgr Notification that contains GPS Lock state
   APC_DISCONNECT_AP = 14, ///<  Mgr->APC Manager requests s/w reset of AP
};
ENUM2STR(apc_msg_type_t);

const uint32_t APC_COOKIE = 0x7E7E7E7E;

/**
 * Message flags
 */
enum apc_hdr_flags_t
{
   APC_HDR_FLAGS_NOTRACK = 0x1, ///< Do not track the sequence number for this message
};

/**
 * Flags of NetTx message (apc_msg_net_tx_s)
 */
enum apc_nettx_flags
{
   APC_NETTX_FL_USETXINFO = 0x1,    ///< use TX link information (frame, slot, channel, dst fields)
};
const uint16_t APC_NETTX_NOTXDONE = 0xFFFF;  ///< Suppress generation Tx Done message

PACKED_START
/**
 * Header of APC interface messages
 */
struct apc_hdr_s
{
   uint32_t       cookie;  ///< A fixed value of 0x7E7E7E7E that designates beginning of a packet.
   uint8_t        flags;   ///< See \ref apc_hdr_flags_t
   uint32_t       mySeq;   ///< The sender's sequence number. Starts with 1
   uint32_t       yourSeq; ///< Last sequence number received. 
   apc_msg_type_t type;    ///< Message type
   uint16_t       length;  ///< Payload length (or 0 if no payload).
   // Payload
};

///////////////////////////////////////////////
//          APC interface messages
///////////////////////////////////////////////

/**
 * APC_GPS_LOCK: APC GPS LOCK message
 */
struct apc_msg_net_gpslock_s
{
   uint8_t    state;   ///< state of gps lock. 0=no lock, 1=lock
};

/**
 * APC_CONNECT. APC connection message
 */
struct apc_msg_connect_s
{
   uint8_t  ver;           ///< APC protocol version. 0=initial version
   uint32_t flags;         ///< Reserved; Set to 0.
   char     identity[32];  ///< String that uniquely describes the sender
   uint32_t sesId;         ///< Unique session id assigned by the Manager. . 0 - used client for start new session
   apc_msg_net_gpslock_s gpsstate; ///< GPS lock status 
   uint32_t netId;         /// < Network ID (send by manager to APC)
};

/**
 * APC_CONNECT. APC connection message
 */
struct apc_msg_connect_s_v1
{
   uint8_t  ver;           ///< APC protocol version. 0=initial version
   uint32_t flags;         ///< Reserved; Set to 0.
   char     identity[32];  ///< String that uniquely describes the sender
   uint32_t sesId;         ///< Unique session id assigned by the Manager. . 0 - used client for start new session
   apc_msg_net_gpslock_s gpsstate; ///< GPS lock status 
   uint32_t netId;         ///< Network ID (send by manager to APC)
   char     version[SIZE_STR_VER];   ///< APC/ Manger software version
};

/**
 * APC_NET_TX: APC transmit message.
 */
struct apc_msg_net_tx_s
{
   uint8_t    priority; ///<  priority of the packet; 0(lowest)-3(highest)
   uint16_t   txDoneId; ///<  id to include in TX confirmation. 0xFFFF means no confirmation should be sent.
   uint8_t    flags;    ///< 0x01: fUseLinkInfo - indicates whether to use TX link information
   uint8_t    frame;    ///< Frame id for the TX link
   uint32_t   slot;     ///< Slot for the TX link
   uint8_t    offset;   ///< Offset for the TX link
   uint16_t   dst;      ///< First Hop Mote
   // payload bytes packet payload, starting with mesh header
};

/**
 * APC_NET_TXDONE: APC TX done message
 */
struct apc_msg_net_txdone_s
{
   uint16_t   txDoneId; ///< packet identifier submitted with APC_NET_TX_REQ
   uint8_t    status;
};

/**
 * APC_TIME_MAP: APC time map message
 */
struct apc_msg_timemap_s
{
   uint64_t utcSeconds;       ///< time of the mapping, in number of seconds since midnight of January 1, 1970
   uint32_t utcMicroseconds;  ///< microseconds since the beginning of the current second
   uint64_t asn;              ///< ASN that is mapped to the system time in this message
   uint16_t asnOffset;        ///< microseconds since the beginning of the ASN slot};
};
PACKED_STOP

const uint32_t MAX_APC_HDR_SIZE = sizeof(apc_hdr_s) + sizeof(apc_msg_net_tx_s);
