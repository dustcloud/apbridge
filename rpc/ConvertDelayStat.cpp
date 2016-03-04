#include "rpc/public/ConvertDelayStat.h"

void convertDelayStat(const statdelays_s& stat, common::DelayStat * pRpcStat) {
   for(const numthevent_s & e : stat.m_numThresholdEvents) {
      common::DelayStatItem *item = pRpcStat->add_delays();
      item->set_threshold(TO_MSEC(e.m_threshold).count());
      item->set_numevents(e.m_number);
   }
   common::DelayStatItem * pOutTh = pRpcStat->mutable_outthresholdevents(); 
   pOutTh->set_threshold(TO_MSEC(stat.m_numOutThresholdEvents.m_threshold).count());
   pOutTh->set_numevents(stat.m_numOutThresholdEvents.m_number);
   pRpcStat->set_maxdelay(static_cast<double>(stat.m_maxDelay.count()) / 1000.0);
   pRpcStat->set_numevents(stat.m_numEvents);
}