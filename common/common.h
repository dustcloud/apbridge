/*
 * Copyright (c) 2014, Linear Technology. All rights reserved.
 */

#pragma once

/**
 * \file common.h
 * Common utility functions used across Voyager components
 */

#ifdef WIN32
   #include <winsock2.h>
#else
   #include <arpa/inet.h>
#endif

#include <stdlib.h>
#include <stdint.h>

#include <boost/chrono.hpp>
#include <iostream>

class    IFmtOutput;

#ifdef VLD
   #include "vld.h"
#endif

char const EV_VOYAGER_HOME[] = "VOYAGER_HOME"; 
const uint32_t MAX_NET_PKT_SIZE = 1024;

typedef boost::chrono::microseconds usec_t;  // Time definition microseconds (10-6)
typedef boost::chrono::milliseconds msec_t;  // Time definition milliseconds (10-3)
typedef boost::chrono::seconds      sec_t;
typedef boost::chrono::steady_clock::time_point mngr_time_t;
typedef boost::chrono::system_clock::time_point sys_time_t;
sys_time_t time_mngr2sys(mngr_time_t t);
mngr_time_t     time_sys2mngr(sys_time_t t);

#define TO_USEC(_D_)   (boost::chrono::duration_cast<usec_t>(_D_))
#define TO_MSEC(_D_)   (boost::chrono::duration_cast<msec_t>(_D_))
#define TO_SEC(_D_)    (boost::chrono::duration_cast<sec_t> (_D_))
#define TIME_NOW()     (boost::chrono::steady_clock::now())
#define SYSTIME_NOW()  (boost::chrono::system_clock::now())
const mngr_time_t      TIME_EMPTY    = mngr_time_t(sec_t(0));
const sys_time_t       SYSTIME_EMPTY = sys_time_t(sec_t(0));

// Convert duration to the length in micro/milli/seconds
#define TIME_D2SEC(_D_)   (boost::chrono::duration_cast<sec_t>(_D_).count())
#define TIME_D2MSEC(_D_)  (boost::chrono::duration_cast<msec_t>(_D_).count())
#define TIME_D2USEC(_D_)  (boost::chrono::duration_cast<usec_t>(_D_).count())

// Print current time
void printCurTime(IFmtOutput& fo);
void printTime(IFmtOutput& fo, const mngr_time_t& time);

std::ostream& operator << (std::ostream&, const mngr_time_t& time);
std::ostream& operator << (std::ostream& os, const sys_time_t& utc);

std::string toString(const mngr_time_t& time);
std::string toString(const sys_time_t& time);
std::string toStringISO(const sys_time_t& utc);

// Monitor task IDs
typedef uint32_t monitorid_t;
const   monitorid_t MONITORID_EMPTY = 0;

// Convert HEX char to binary
uint8_t hexCh2bin(const char ch);

// Convert HEX string to binary
// WARNING: pRes is filled from the end, so the caller must know the length of the expected data
bool hexStr2bin(const char * str, uint8_t * pRes, uint32_t size);

/**
 * Convert hex string (two ASCII characters per byte) into binary data
 * The output buffer is filled starting at the beginning of the buffer.
 * 
 * 
 * \return length of the converted binary data or 0 if an error is encountered, i.e. if the string contains invalid chars
//          or the output buffer is not large enough
*/
int hexToBin(const char * str, uint8_t * pRes, size_t len);

// For print buffer as HEX use: os << show_hex(buf, len) ...
struct show_hex { 
   show_hex(const void * ar, int l, char div = ':')
      : m_ar(static_cast<const uint8_t *>(ar)), m_len(l) { m_div[0] = div; m_div[1] = 0; }
   const uint8_t * m_ar;
   uint32_t m_len;
   char     m_div[2];
};
std::ostream& operator << (std::ostream&, const show_hex&);
std::string dumpToString(const uint8_t * data, uint32_t len, char d = '-');

// Address definitions
#define MAC_LENGTH 8
typedef uint16_t moteid_t;
typedef uint8_t  macaddr_t[MAC_LENGTH];

const   moteid_t   MOTEID_EMPTY     = 0;
const   moteid_t   MOTEID_BROADCAST = 0xFFFF;
bool    isMacBroadcast(const macaddr_t& mac);
bool    isMacEmpty(const macaddr_t& mac);
void    setMacBroadcast(macaddr_t& mac);
void    setMacEmpty(macaddr_t& mac);
void    copyMac(macaddr_t& dst, const macaddr_t& src);
bool    isMacEqu(const macaddr_t& mac1, const macaddr_t& mac2);

// Prototype for function ENUM to String
#define ENUM2STR(_T_) const char * toString(_T_ aVal, bool isComment = true); \
   bool toEnum(const char * str, _T_ & res);

// Calculate the number of objects in a static array
#define ARRAY_LENGTH(ary) (sizeof(ary)/sizeof(*ary))
#define ARRAY_END(ary)    (ary + ARRAY_LENGTH(ary))

#define KEY128_LENGTH  16
typedef uint8_t  key128_t[KEY128_LENGTH];

#define IP6_LENGTH 16
typedef uint8_t  ip6addr_t[IP6_LENGTH];

#ifndef htonll
  /**
   * Macro for swapping byte order on 64-bit integer
   */
  #define htonll(l)                                           \
             ( ( ((l) >> 56) & 0x00000000000000FFLL ) |       \
               ( ((l) >> 40) & 0x000000000000FF00LL ) |       \
               ( ((l) >> 24) & 0x0000000000FF0000LL ) |       \
               ( ((l) >>  8) & 0x00000000FF000000LL ) |       \
               ( ((l) <<  8) & 0x000000FF00000000LL ) |       \
               ( ((l) << 24) & 0x0000FF0000000000LL ) |       \
               ( ((l) << 40) & 0x00FF000000000000LL ) |       \
               ( ((l) << 56) & 0xFF00000000000000LL ) )
   #define ntohll(l) htonll(l)
#endif

enum converttype_t {
   HOST_TO_NET,
   NET_TO_HOST,
};

/**
 * Macros for converting 16 bit (_S), 32 bit (_L), 64 bit (_LL) integers
 */
#define CONVERT_S(_C_, _V_)  (_C_ == NET_TO_HOST ? ntohs(_V_)  : htons(_V_))
#define CONVERT_L(_C_, _V_)  (_C_ == NET_TO_HOST ? ntohl(_V_)  : htonl(_V_))
#define CONVERT_LL(_C_, _V_) (_C_ == NET_TO_HOST ? ntohll(_V_) : htonll(_V_))

typedef int64_t  asn_t;
/**
 * Serialize a network buffer containing an ASN value into the host data type
 * \param netAsn      Pointer to the network buffer
 * \param netAsnSize  Length of the ASN field (up to 8 bytes)
 * \return ASN (as a host integer)
 */
asn_t     ntoh_asn(const uint8_t * netAsn, uint32_t netAsnSize);
/**
 * Write a host ASN value to a network buffer in network byte order
 * \param hostAsn     Host ASN value
 * \param netAsn      Pointer to the network buffer
 * \param netAsnSize  Length of the ASN field (up to 8 bytes)
 * \return Pointer to the network buffer
 */
uint8_t * hton_asn(const asn_t hostAsn, uint8_t * netAsn, uint32_t netAsnSize);
/**
 * Print the ASN into the FmtOutput instance
 */
void      printAsn(IFmtOutput& fo, const asn_t asn);

typedef uint32_t ap_intf_id_t;    // AP interface ID
const   ap_intf_id_t APINTFID_EMPTY = 0;

/* Get public IP Address from host.
 */

/**
 * Get public IP Address from host.
 *
 * \param [out]   sAddress    IP address
 * \param [out]   sHostName   Host name 
 *
 * \return  0 if successful otherwise -1
 */
int getIPAddress(std::string* sAddress, std::string* sHostName);

/**
 * API protocols.
 */
enum apiproto_t {
   APIPROTO_NA,
   APIPROTO_IPC,     // "IPC"
   APIPROTO_TCP,     // "TCP"
};
ENUM2STR(apiproto_t);

/**
 * Gets a random number [0..maxVal)
 *
 * \param   maxVal   The maximum value of random number + 1
 *
 * \return  The random number .
 */
uint32_t getRand(uint32_t mavVal);

/**
 * Gets full file name. 
 *
 * \param   subDir   The sub dir.
 * \param   fileName Filename of the file.
 *
 * \return  The full file name.
 */

std::string getFullFileName(const std::string& subDir, const std::string& fileName);

/**
 * Base64 encoder
 * \return  The input string encoded in base64 (including padding)
 */
std::string base64Encoder(std::string data);
