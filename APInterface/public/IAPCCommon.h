#pragma once
#include "common.h"
#include "StatDelays.h"

/**
 * \file IAPCCommon.h
 */

/**
 * Interface information.
 */
struct ap_intf_info_t
{
   std::string  name;       ///< Name of interface
   std::string  ipAddress;   ///< IP address of APC
   uint16_t     port;       ///< TCP port of APC
};

struct ap_intf_stat_t
{
   uint16_t     curAllocOutBuf;     ///< Number of currently allocated output buffers
   uint16_t     maxAllocOutBuf;     ///< Maximum number of output buffers
   uint32_t     numReceivedPkt;     ///< Number of received packets
   statdelays_s sendStat;
};

/**
 * Priority of sending packets
 */
enum ap_intf_priority_t 
{
   APINTF_LOW = 0,
   APINTF_MED = 1,
   APINTF_HI  = 2,
   APINTF_CMD = 3,
};
ENUM2STR(ap_intf_priority_t);

/**
 * Header of packet to send
 */
struct ap_intf_sendhdr_t {
   ap_intf_priority_t priority;	             ///< priority of the packet
   bool                isTxDoneRequested;    ///< Flag - generate TX Done notification
   uint16_t            txDoneId;             ///< Packet ID. Included in txDone notification
   bool                isLinkInfoSpecified;  ///< flag that indicates whether to use TX link information
   uint8_t             frId;                 ///< Frame id for the TX link
   uint32_t            slot;                 ///< Slot for the TX link
   uint8_t             offset;               ///< Offset for the TX link
   uint16_t            dst;                  ///< Destination mote ID
};

/**
 * Transmit status
 */
enum ap_intf_txstatus_t
{
   APINTF_TXDONE_OK,
   APINTF_TXDONE_ERR,
};
ENUM2STR(ap_intf_txstatus_t);

/**
 * TX done notification parameters
 */
struct ap_intf_txdone_t {
   uint16_t           txDoneId;  ///< Packet ID (see ap_intf_sendhdr_t)
   ap_intf_txstatus_t status;    ///< Status of finished transmission
};


/**
 * AP time map (System time to ASN)
 */
struct ap_time_map_t
{
   int64_t   timeSec;       ///< Number of seconds since midnight of January 1, 1970
   uint32_t  timeUsec;      // < Microseconds since the beginning of the current second.
   asn_t     asn;           ///< Current network ASN  
   uint16_t  asnOffset;     ///< Microseconds since the beginning of the current ASN
};

/**
 * GPS lock status
 */
enum ap_int_gpslockstat_t
{
   APINTF_GPS_LOCK   =  0,
   APINTF_GPS_NOLOCK =  1,
};
ENUM2STR(ap_int_gpslockstat_t)

/**
 * GPS satellite lock notification parameters
 */
struct ap_intf_gpslock_t {
   ap_int_gpslockstat_t           gpsstate;   ///< state of gps lock. 0=no lock, 1=lock
};