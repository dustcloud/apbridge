/*
 * Copyright (c) 2010, Dust Networks Inc. 
 */

#include "HDLC.h"

// HDLC Constants
const uint8_t HDLC_PADDING = 0x7E;
const uint8_t HDLC_ESCCHAR = 0x7D;
const uint8_t HDLC_XORBYTE = 0x20;
const uint16_t CRC_MAGIC_NUMBER = 0xF0B8;

static int _fcstab[/*256*/] = {
   0x0000, 0x1189, 0x2312, 0x329b, 0x4624, 0x57ad, 0x6536, 0x74bf,
   0x8c48, 0x9dc1, 0xaf5a, 0xbed3, 0xca6c, 0xdbe5, 0xe97e, 0xf8f7,
   0x1081, 0x0108, 0x3393, 0x221a, 0x56a5, 0x472c, 0x75b7, 0x643e,
   0x9cc9, 0x8d40, 0xbfdb, 0xae52, 0xdaed, 0xcb64, 0xf9ff, 0xe876,
   0x2102, 0x308b, 0x0210, 0x1399, 0x6726, 0x76af, 0x4434, 0x55bd,
   0xad4a, 0xbcc3, 0x8e58, 0x9fd1, 0xeb6e, 0xfae7, 0xc87c, 0xd9f5,
   0x3183, 0x200a, 0x1291, 0x0318, 0x77a7, 0x662e, 0x54b5, 0x453c,
   0xbdcb, 0xac42, 0x9ed9, 0x8f50, 0xfbef, 0xea66, 0xd8fd, 0xc974,
   0x4204, 0x538d, 0x6116, 0x709f, 0x0420, 0x15a9, 0x2732, 0x36bb,
   0xce4c, 0xdfc5, 0xed5e, 0xfcd7, 0x8868, 0x99e1, 0xab7a, 0xbaf3,
   0x5285, 0x430c, 0x7197, 0x601e, 0x14a1, 0x0528, 0x37b3, 0x263a,
   0xdecd, 0xcf44, 0xfddf, 0xec56, 0x98e9, 0x8960, 0xbbfb, 0xaa72,
   0x6306, 0x728f, 0x4014, 0x519d, 0x2522, 0x34ab, 0x0630, 0x17b9,
   0xef4e, 0xfec7, 0xcc5c, 0xddd5, 0xa96a, 0xb8e3, 0x8a78, 0x9bf1,
   0x7387, 0x620e, 0x5095, 0x411c, 0x35a3, 0x242a, 0x16b1, 0x0738,
   0xffcf, 0xee46, 0xdcdd, 0xcd54, 0xb9eb, 0xa862, 0x9af9, 0x8b70,
   0x8408, 0x9581, 0xa71a, 0xb693, 0xc22c, 0xd3a5, 0xe13e, 0xf0b7,
   0x0840, 0x19c9, 0x2b52, 0x3adb, 0x4e64, 0x5fed, 0x6d76, 0x7cff,
   0x9489, 0x8500, 0xb79b, 0xa612, 0xd2ad, 0xc324, 0xf1bf, 0xe036,
   0x18c1, 0x0948, 0x3bd3, 0x2a5a, 0x5ee5, 0x4f6c, 0x7df7, 0x6c7e,
   0xa50a, 0xb483, 0x8618, 0x9791, 0xe32e, 0xf2a7, 0xc03c, 0xd1b5,
   0x2942, 0x38cb, 0x0a50, 0x1bd9, 0x6f66, 0x7eef, 0x4c74, 0x5dfd,
   0xb58b, 0xa402, 0x9699, 0x8710, 0xf3af, 0xe226, 0xd0bd, 0xc134,
   0x39c3, 0x284a, 0x1ad1, 0x0b58, 0x7fe7, 0x6e6e, 0x5cf5, 0x4d7c,
   0xc60c, 0xd785, 0xe51e, 0xf497, 0x8028, 0x91a1, 0xa33a, 0xb2b3,
   0x4a44, 0x5bcd, 0x6956, 0x78df, 0x0c60, 0x1de9, 0x2f72, 0x3efb,
   0xd68d, 0xc704, 0xf59f, 0xe416, 0x90a9, 0x8120, 0xb3bb, 0xa232,
   0x5ac5, 0x4b4c, 0x79d7, 0x685e, 0x1ce1, 0x0d68, 0x3ff3, 0x2e7a,
   0xe70e, 0xf687, 0xc41c, 0xd595, 0xa12a, 0xb0a3, 0x8238, 0x93b1,
   0x6b46, 0x7acf, 0x4854, 0x59dd, 0x2d62, 0x3ceb, 0x0e70, 0x1ff9,
   0xf78f, 0xe606, 0xd49d, 0xc514, 0xb1ab, 0xa022, 0x92b9, 0x8330,
   0x7bc7, 0x6a4e, 0x58d5, 0x495c, 0x3de3, 0x2c6a, 0x1ef1, 0x0f78
};


/**
 * Calculate incremental FCS 
 */
static uint32_t initFcs16()
{
    return 0xffff;
}
static uint32_t addOneByteToFcs16(uint32_t fcs, uint8_t byte)
{
    return ((fcs & 0xffff) >> 8) ^ _fcstab[(fcs ^ byte) & 0xff];
}
static uint16_t closeFcs16(uint32_t fcs)
{
   fcs = fcs ^ 0xffff;
   return (uint16_t)fcs & 0xffff;
}

/**
 * Calculate complete FCS
 */
uint16_t computeFCS16(const std::vector<unsigned char>& data)
{
   uint32_t fcs = initFcs16();
   for (size_t i = 0; i < data.size(); i++)
      fcs = ((fcs & 0xffff) >> 8) ^ _fcstab[(fcs ^ data[i]) & 0xff];

   return closeFcs16(fcs);
}


/**
 * Escape HDLC bytes
 *
 * Insert escape characters before special characters
 */
static void appendHDLC(std::vector<unsigned char>& dst, const std::vector<unsigned char>& src)
{
   for (size_t i = 0; i < src.size(); i++) {
      if (src[i] == HDLC_PADDING || src[i] == HDLC_ESCCHAR) {
         dst.push_back(HDLC_ESCCHAR);
         dst.push_back(src[i] ^ HDLC_XORBYTE);
      } else {
         dst.push_back(src[i]);
      }
   }
}


/**
 * Encode HDLC packet
 *
 * Param: src - input data
 * Returns: vector of encoded data
 */
std::vector<unsigned char> encodeHDLC(const std::vector<unsigned char>& src)
{
   uint16_t fcs = computeFCS16(src);
   std::vector<unsigned char> fcsbuf(2);
   fcsbuf[0] = fcs & 0xFF;
   fcsbuf[1] = (fcs>>8) & 0xFF;

   std::vector<unsigned char> result;
   result.push_back(HDLC_PADDING);

   // encode the source, escaping HDLC special characters
   appendHDLC(result, src);
   // encode the FCS
   appendHDLC(result, fcsbuf);

   result.push_back(HDLC_PADDING);

   return result;
}

// State transitions:
// PACKET_COMPLETE (ESC) -> HDLC_ESCAPE
// PACKET_COMPLETE (PAD) -> PACKET_COMPLETE, (callback)
// PACKET_COMPLETE (any) -> HDLC_DATA, append

// HDLC_DATA (ESC) -> HDLC_ESCAPE
// HDLC_DATA (PAD) -> PACKET_COMPLETE, callback
// HDLC_DATA (any) -> HDLC_DATA, append

// HDLC_ESCAPE (any) -> HDLC_DATA, append

// 
void CHDLC::addByte(uint8_t b)
{
   if (m_state == HDLC_ESCAPE) {
      uint8_t translated = b ^ HDLC_XORBYTE;
      append(translated);
      m_state = HDLC_DATA;
   }
   else if (b == HDLC_ESCCHAR) {
      m_state = HDLC_ESCAPE;
   }
   else if (b == HDLC_PADDING) {
      int len = m_buffer.size();
      if (len > 2) {
         uint16_t fcs = (m_buffer[len-2] * 256) + m_buffer[len-1];
         m_buffer.pop_back();
         m_buffer.pop_back();
         // validate checksum
         if (validateChecksum(fcs)) {
            callback();
         }
         // TODO: handle checksum error
      }
      // small packets are silently dropped
      reset();
   } else {
      append(b);
   }
}

// Private HDLC methods

bool CHDLC::validateChecksum(uint16_t frameFcs) {
   //UInt16 computedFcs = computeFCS16(m_buffer);
   //return (computedFcs == frameFcs);
   uint16_t computedFcs = closeFcs16(m_runningFCS);
   return computedFcs == 0xf47;
}

void CHDLC::append(uint8_t byte) {
   m_buffer.push_back(byte);
   m_runningFCS = addOneByteToFcs16(m_runningFCS, byte);
}

void CHDLC::callback() {
   if (m_handler && m_buffer.size() > 0) {
      m_handler->frameComplete(m_buffer);
   }
}

void CHDLC::reset() {
   m_state = HDLC_PACKET_COMPLETE;
   m_buffer.clear();
   m_runningFCS = initFcs16();
}
