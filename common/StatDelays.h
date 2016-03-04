#pragma once
#include "common.h"
#include <vector>
#include <iostream>

struct numthevent_s {
   usec_t   m_threshold;   // Threshold 
   uint64_t m_number;      // Number of events
};
typedef std::vector<numthevent_s>  numtheventlist_t;
typedef numtheventlist_t::iterator numtheventlist_i;
typedef numtheventlist_t::const_iterator numtheventlist_c;

struct statdelays_s {
   numtheventlist_t m_numThresholdEvents;
   numthevent_s     m_numOutThresholdEvents;
   uint64_t         m_numEvents;
   usec_t           m_maxDelay;
};

std::ostream& operator << (std::ostream& os, const statdelays_s& stat);

