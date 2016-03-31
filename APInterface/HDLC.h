/*
 * Copyright (c) 2016, Linear Technology. All rights reserved.
 */
#pragma once

#ifndef HDLC_H_
#define HDLC_H_

/*
 * HDLC Parser and Generator
 */
#include "common.h"
#include <vector>


/**
 * HDLC packet generator
 */
std::vector<uint8_t> encodeHDLC(const std::vector<uint8_t>& src);

/**
 * HDLC FCS calculation
 */
uint16_t computeFCS16(const std::vector<uint8_t>& data);

// TODO: needs a better name
class IHDLCCallback {
public:
   virtual void frameComplete(const std::vector<uint8_t>& packet) = 0;
};


class CHDLC {
   enum ParseState {
      HDLC_PACKET_COMPLETE,
      HDLC_DATA,
      HDLC_ESCAPE,
   };

public:
   CHDLC(int inputLength, IHDLCCallback* handler) 
      : m_handler(handler),
        m_state(HDLC_PACKET_COMPLETE),
        m_buffer(inputLength),
        m_runningFCS(0)
   { reset(); }

   void addByte(uint8_t b);

private:
   bool validateChecksum(uint16_t frameFcs);
   void append(uint8_t byte);
   void callback();
   void reset();

   IHDLCCallback* m_handler;

   ParseState   m_state;
   std::vector<uint8_t> m_buffer;
   uint32_t     m_runningFCS;
};


#endif /* ! HDLC_H_ */
