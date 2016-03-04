#include "StatDelaysCalc.h"
#include <algorithm>
#include <boost/assign/list_of.hpp> 

using namespace std;

static const vector<usec_t> DEFAULT_THRESHOLDS  = boost::assign::list_of 
   (usec_t(5000))
   (usec_t(7000))
   (usec_t(10000))
   (usec_t(50000))
;

CStatDelaysCalc::CStatDelaysCalc() 
{
   clear();
   setThresholds(DEFAULT_THRESHOLDS);
}

void  CStatDelaysCalc::clear() 
{
   m_maxDelay = usec_t(0);
   m_allEvents = 0;
   m_numOutThresholds.m_number = 0;
   for_each(m_threshold.begin(), m_threshold.end(), [](numthevent_s& e) ->void {e.m_number = 0; });
}

void CStatDelaysCalc::setThresholds(const vector<usec_t>& delays) 
{
   m_threshold.clear();
   for(const usec_t& d : delays) {
      numthevent_s item = {d, 0};
      m_threshold.push_back(item);
   }
   m_numOutThresholds.m_threshold = delays.back();
   m_numOutThresholds.m_number = 0;
}

void  CStatDelaysCalc::getStat(statdelays_s * pStat)
{
   boost::unique_lock<boost::mutex> lock(m_lock);
   pStat->m_numThresholdEvents.clear();
   pStat->m_numThresholdEvents = m_threshold;
   pStat->m_maxDelay              = m_maxDelay;
   pStat->m_numOutThresholdEvents = m_numOutThresholds;
   pStat->m_numEvents             = m_allEvents;
}

uint64_t CStatDelaysCalc::getNumEvents()
{
   boost::unique_lock<boost::mutex> lock(m_lock);
   return m_allEvents;
}

void CStatDelaysCalc::addEvent(usec_t delay) 
{
   boost::unique_lock<boost::mutex> lock(m_lock);
   bool isAdded = false;
   for(numthevent_s& i : m_threshold) {
      if (delay <= i.m_threshold) {
         i.m_number++;
         isAdded = true;
         break;
      }
   }
   if (!isAdded)
      m_numOutThresholds.m_number++;
   if (delay > m_maxDelay)
      m_maxDelay = delay;
   m_allEvents++;
}

ostream& operator << (ostream& os, const statdelays_s& stat) 
{
   for(const numthevent_s& e : stat.m_numThresholdEvents)  {
      os << TO_MSEC(e.m_threshold).count() << "ms: " << e.m_number << ", ";
   }
   os << ">" << TO_MSEC(stat.m_numOutThresholdEvents.m_threshold).count() << "ms: " 
      << stat.m_numOutThresholdEvents.m_number << "; ";
   os << "All: " << stat.m_numEvents << "; Max delay: " << stat.m_maxDelay.count() << "usec."; 
   return os;
}

std::ostream& operator << (std::ostream& os, CStatDelaysCalc& stat) {
   statdelays_s s; 
   stat.getStat(&s);
   os << s;
   return os;
}
