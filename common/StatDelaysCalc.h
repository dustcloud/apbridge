#pragma once

#include "StatDelays.h"
#include <boost/thread.hpp>

/**
 * Calculate number of events with delay <= threshold delay.
 * Default thresholds: 5ms, 7ms, 10 ms
 */
class CStatDelaysCalc {
public:
   CStatDelaysCalc();
   ~CStatDelaysCalc(){;}
   void         clear();

   // Set new delay thresholds.
   // 'delays' must be sorted
   void         setThresholds(const std::vector<usec_t>& delays);

   // Get delay statistics
   void         getStat(statdelays_s * pStat);

   uint64_t     getNumEvents();

   // Add new event delay 'delay' usec
   void         addEvent(usec_t dealy);
   void         addEvent(const mngr_time_t& startTime) { addEvent(TO_USEC(TIME_NOW() - startTime)); }

private:
   usec_t            m_maxDelay;
   numtheventlist_t  m_threshold;
   numthevent_s      m_numOutThresholds;
   uint64_t          m_allEvents;
   boost::mutex      m_lock;
};

std::ostream& operator << (std::ostream& os, CStatDelaysCalc& stat);