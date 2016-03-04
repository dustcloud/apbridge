/*
 * Copyright (c) 2009, Dust Networks.  All rights reserved.
 *
 * $HeadURL: https://svn/software/trunk/shared/include/6lowpan/6lowpanhdr.h $
 * $Rev: 64147 $
 * $Author: alushin $
 * $LastChangedDate: 2015-01-27 14:06:31 -0800 (Tue, 27 Jan 2015) $ 
 */

#ifndef __6LOWPANHDR_H__
#define __6LOWPANHDR_H__

#include "dn_typedef.h"
#include "dn_pack.h"
#include "public/dn_mesh.h"

#define IP6ADDR_LENGTH  16

#ifndef MAC_LONG_ADDR
#define MAC_LONG_ADDR            DN_MESH_LONG_ADDR_SIZE
#endif

#define MAC_SHORT_ADDR           2

#define  MIN_SRCROUTE_HOPS       4
#define  MAX_SRCROUTE_HOPS       8

#define  MESHSPEC_ROUTE_MIN_VAL              0x2
#define  MESHSPEC_ROUTE_MAX_VAL              0x3

PACKED_START /* all structures that follow are packed */

//Bit map of mesh specifier: jdDsSpRrvvv000pp
typedef struct {
#ifdef L_ENDIAN
   INT8U fSrcRoute:2;     //Source Route type 00 and 01: no source route, 10:
                             //4 hop, 11:8 hops.
   INT8U fProxy:1;        //1:proxy included
   INT8U fLongSrc:1;      //1:8 bytes long OUI address
   INT8U srcElided:1;     //1:Elided
   INT8U fLongDst:1;      //1:8 bytes
   INT8U dstElided:1;     //1:Elided
   INT8U secType:1;       //1:join key

   INT8U priority:2;      //11:cmd, 10:data, 01:normal, 00:lowest
   INT8U reserved:3;      //reserved. always 0b000
   INT8U version:3;       //version information

#else
   INT8U secType:1;       //1:join key
   INT8U dstElided:1;     //1:Elided
   INT8U fLongDst:1;      //1:8 bytes
   INT8U srcElided:1;     //1:Elided
   INT8U fLongSrc:1;      //1:8 bytes
   INT8U fProxy:1;        //1:proxy included
   INT8U fSrcRoute:2;     //Source Route type 00 and 01: no source route, 10:
                          //4 hop, 11:8 hops.

   INT8U version:3;       //version information
   INT8U reserved:3;      //reserved. always 0b000
   INT8U priority:2;      //11:cmd, 10:data, 01:normal, 00:lowest
#endif   
}  mesh_spec_t;

/*
   Net Packet Format:

   Net:
      INT8U      dispatch;     // always 0x00
      mesh_spec_t  meshSpec;     
      INT8U     ttl;
      INT16U    asn;
      INT8U     graphId;
      union {
             EMPTY;
             INT16U   srcShort;
             INT8U    srcLong[8];
      }
      union {
             EMPTY;
             INT16U   dstShort;
             INT8U    dstLong[8];
      }
      
      union {               
         EMPTY;
         INT16U proxy;
      };
      union {                 // 0/8/16
         INT16U route[MIN_SRCROUTE_HOPS];
         INT16U route[MAX_SRCROUTE_HOPS];
         EMPTY;
      };
   Security:   
      union {                 // 1/4. Nonce
         INT8U  nonce;
         INT32U nonce;
      };
      INT8U netMic[NET_MIC_LENGTH];  // 4. Net MIC

   Transport:
      INT8U  transport;    
*/
#define NET_DFLT_TTL 127


// Transport Header
#define NETTR_RLBL_FL            0x80
#define NETTR_RESP_FL            0x40
#define NETTR_BCAST_FL           0x20
#define NETTR_SQN_MASK           0x1F

// long address: HCF_OUI_TITLE | NET_ADDRESS |(SESSION_ID+1)
#define HCF_OUI_BYTE5            0x1E
#define HCF_OUI_BYTE6            0x1B
#define HCF_OUI_BYTE7            0x00


// Array constants
#define  MAX_SVC_NUM             16
//#define  MAX_ROUTE_TBL           8
#define  MAX_SRC_ROUTE           2
#define  NET_MIC_LENGTH          4

// HART_spec_075, pp.39, assume MSB first
#define WELL_KNOWN_KEY {0x77, 0x77, 0x77, 0x2E, 0x68, 0x61, 0x72, 0x74,0x63, 0x6F, 0x6D, 0x6D, 0x2E, 0x6F, 0x72, 0x67} 

#define PKT_PRIORITY_LEVEL0        0              //lowest priority
#define PKT_PRIORITY_LEVEL1        1              //
#define PKT_PRIORITY_LEVEL2        2              //
#define PKT_PRIORITY_LEVEL3        3              //highest priority

#define PKT_PRIORITY_LOWEST      PKT_PRIORITY_LEVEL0
#define PKT_PRIORITY_NORMAL      PKT_PRIORITY_LEVEL1
#define PKT_PRIORITY_DATA        PKT_PRIORITY_LEVEL2
#define PKT_PRIORITY_CMD         PKT_PRIORITY_LEVEL3

#define SEC_TYPE_JOIN            1
#define SEC_TYPE_NORMAL          0

#define MESH_SPEC_ROUTE8_VAL      0x3
#define MESH_SPEC_ROUTE4_VAL      0x2
#define MESH_SPEC_NOROUTE_VAL     0x0

#define SEC_CTRL_SES_VAL         0
#define SEC_CTRL_JOIN_VAL        1
   

// Static parts of net headers
typedef struct {
   mesh_spec_t  meshSpec;
   INT8U        ttl;
   INT16U       asn;
   INT8U        graphId;
}   mesh_hdr_bgn_t;

// Transport header
typedef struct {
#ifdef L_ENDIAN
   INT8U sqnNum:5;
   INT8U reserv:1;
   INT8U fResp:1;
   INT8U fRlbl:1;
#else
   INT8U fRlbl:1;
   INT8U fResp:1;
   INT8U reserv:1;
   INT8U sqnNum:5;
#endif
}   transport_hdr_t;

#define TRANSP_HDR_SIZE sizeof(transport_hdr_t)
#define TRANSP_MAX_SEQ 31


// Security header
typedef struct {
#ifdef L_ENDIAN
   INT8U        encryptOffset:7;
   INT8U        isTrusted:1;
#else
   INT8U        isTrusted:1;
   INT8U        encryptOffset:7;
#endif
   INT8U        nonce;
   INT8U        mic[NET_MIC_LENGTH];
}   sec_hdr_short_t;

typedef struct {
#ifdef L_ENDIAN
   INT8U        encryptOffset:7;
   INT8U        isTrusted:1;
#else
   INT8U        isTrusted:1;
   INT8U        encryptOffset:7;
#endif
   INT32U       nonce;
   INT8U        mic[NET_MIC_LENGTH];
}   sec_hdr_long_t;


// Several predifined lowpan_iphc headers
#define  LOWPAN_IPHC_VAL_MOTE2AP       0x7E77
#define  LOWPAN_IPHC_VAL_AP2MOTE_REG   0x7E77
#define  LOWPAN_IPHC_VAL_AP2MOTE_BCAST 0x7E7F
//TODO set corect value
#define  LOWPAN_IPHC_VAL_BCASTADDR     0x01

typedef struct{
#ifdef L_ENDIAN
   INT8U hopLimit:2;      /**0b10: The hop limit field is elided and the hop
                            *limit is 64*/
   INT8U fNextHdr :1;     //1: the next header is compressed using LOWPAN_NHC
   INT8U trafficFlow:2;   /**always 0b11 (version, traffic class and flow label
                            *are compressed)*/
   INT8U dispatch:3;      //reserved dispatch type 0b011

   INT8U dam:2;           //Destination Address Mode
   INT8U dac:1;           //Destination Address Compression
   INT8U mc: 1;           //Multicast Compression Flag
   INT8U sam:2;           //Source Address Mode
   INT8U sac:1;           //Source Address Compression
   INT8U fContext:1;      /**Context identifier. 0: No additional 8-bit context
                            *identifier extension is used.*/
#else
   INT8U dispatch:3;      //reserved dispatch type 0b011
   INT8U trafficFlow:2;   /**always 0b11 (version, traffic class and flow label
                            *are compressed)*/
   INT8U fNextHdr :1;     //1: the next header is compressed using LOWPAN_NHC
   INT8U hopLimit:2;      /**0b10: The hop limit field is elided and the hop
                            *limit is 64*/
   INT8U fContext:1;      /**Context identifier. 0: No additional 8-bit context
                            *identifier extension is used.*/
   INT8U sac:1;           //Source Address Compression
   INT8U sam:2;           //Source Address Mode
   INT8U mc: 1;           //Multicast Compression Flag
   INT8U dac:1;           //Destination Address Compression
   INT8U dam:2;           //Destination Address Mode
#endif   
} lowpan_iphc_t;

#define LOWPAN_FLAG_CONTEXT_ID_ELIDED   0

#define LOWPAN_FLAG_ADDR_COMPRESSED     1

#define LOWPAN_MODE_ADDR_128BITS        0
#define LOWPAN_MODE_ADDR_64BITS         1
#define LOWPAN_MODE_ADDR_16BITS         2
#define LOWPAN_MODE_ADDR_0BITS          3

//These codes are used when MC=1 and DAC=0
#define LOWPAN_MC_MODE_ADDR_128BITS     0    
#define LOWPAN_MC_MODE_ADDR_48BITS      1
#define LOWPAN_MC_MODE_ADDR_32BITS      2
#define LOWPAN_MC_MODE_ADDR_8BITS       3
#define LOWPAN_MC_DAC_MODE_ADDR_48BITS  0

#define LOWPAN_FLAG_MC_COMPRESSED       1

#define LOWPAN_NHC_FIXED_VALUE          0xEF  //always 0xEF in our 6lowpan stack

#define UDP_LOWPAN_HDR_ID               0x1E

// values of NHC UDP byte
#define NHC_UDP_MASK                    0xF8
#define NHC_UDP_VAL                     0xF0


typedef struct{
#ifdef L_ENDIAN
   INT8U fDstPort:1;      //0: full (16 bits), 1 compressed (4 bits)
   INT8U fSrcPort:1;      //0: full (16 bits), 1 compressed (4 bits)
   INT8U fChecksum:1;     /**0: All 16 bits checksum are carried in line*/
   INT8U fUDP:5;          /**always 0b11110 */
#else
   INT8U fUDP:5;          //always 0b11110 
   INT8U fChecksum:1;     //0: All 16 bits checksum are carried in line
   INT8U fSrcPort:1;      //0: full (16 bits), 1 compressed (4 bits)
   INT8U fDstPort:1;      //0: full (16 bits), 1 compressed (4 bits)
#endif   
} udp_lowpan_nhc_t;


typedef struct {
#ifdef L_ENDIAN
   INT8U dstPort:4;
   INT8U srcPort:4;
#else
   INT8U srcPort:4;
   INT8U dstPort:4;
#endif
}  udp_compressed_port_t;

#define NEXT_HDR_TYPE_UDP                   0x1E   //UDP header: 0b11110
#define UDP_NHC_FLAG_CS_ELIDED   1                 //Checksum elided

//keep consistent with IPV6 next header value
#define NET_PKT_TYPE_TCP                0x06
#define NET_PKT_TYPE_UDP                0x11
#define NET_PKT_TYPE_ICMPV6             0x3A

#define  MAX_SRCROUTE_HOPS       8
#define  MIN_SRCROUTE_HOPS       4

/* Structures and constants per 802.15.4e, used in mesh header;
   structures used for sizeof only, access using functions
   mac_writeHeaderIE, mac_writePaylIE */

typedef struct mac_hdr_info_element {
   INT16U    type:1;
   INT16U    elemId:8;
   INT16U    contentLen:7;
   #ifndef __cplusplus
   INT8U     content[];
   #endif
} mac_hdr_info_element_t;

typedef struct mac_payload_info_element {
   INT16U    type:1;
   INT16U    elemId:4;
   INT16U    contentLen:11;
   #ifndef __cplusplus
   INT8U     content[];
   #endif
} mac_payload_info_element_t;

#define MAC_HDR_IE_TYPE          0
#define MAC_PAYLOAD_IE_TYPE      1

/* Header element ID range 0 - 0x19 is used for vendor-specific IDs */
#define MAC_HDR_ELEMID_TOF_ACK			0x18
#define MAC_HDR_ELEMID_MESH_HDR			0x19
#define MAC_HDR_ELEMID_ACK_TIMECORR		0x1E
#define MAC_HDR_ELEMID_TERMINATION		0x7F

#define MAC_PAYL_ELEMID_MLME			0x1
#define MAC_PAYL_ELEMID_TERMINATION		0xF


PACKED_STOP /* back to default packing */

#endif

