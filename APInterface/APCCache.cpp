#include "APCCache.h"

CAPCCache::CAPCCache(uint32_t cacheSize) : m_outpkts(cacheSize)
{
   clear();
}

CAPCCache::~CAPCCache() {;}

void  CAPCCache::clear() 
{
   m_lastSentSeqNum = m_getNextSeqNum = m_pos = m_numPackets = 0;
}

apc_error_t CAPCCache::addPacket(apc_msg_type_t type, const uint8_t * p1, uint32_t s1, 
                                 const uint8_t * p2, uint32_t s2, uint32_t * pSeqNum)
{
   boost::unique_lock<boost::mutex>  lock(m_lock);
   BOOST_ASSERT(!((p1 == NULL && s1 !=0) || (p2 == NULL && s2 !=0 )));
   if (s1+ s2 > APC_MAX_MSG_SIZE)
      return APC_ERR_SIZE;

   apc_cache_pkt_s& pkt = m_outpkts[m_pos];
   pkt.m_type = type;
   pkt.m_size = s1 + s2;

   if (p1 && s1 > 0)
      memcpy(pkt.m_payload.data(), p1, s1);
   if (p2 && s2 > 0)
      memcpy(pkt.m_payload.data()+s1, p2, s2);

   *pSeqNum = pkt.m_seqNumb = (int32_t)++m_lastSentSeqNum;
   if (++m_pos >= m_outpkts.size()) m_pos = 0;  // Next position

   if (m_numPackets >= m_outpkts.size())
      return APC_ERR_OUTBUFOVERFLOW;
   ++m_numPackets;

   return APC_OK;
}

apc_error_t CAPCCache::confirmedSeqNum(uint32_t a_confirmedNum, bool isInit)
{
   boost::unique_lock<boost::mutex>  lock(m_lock);
   apc_error_t res = APC_OK;
   uint64_t confirmedNum = a_confirmedNum | (m_lastSentSeqNum & 0xFFFFFFFF00000000);
   if (a_confirmedNum > (m_lastSentSeqNum & 0xFFFFFFFF))
      confirmedNum -= 0x100000000;

   if (confirmedNum > m_lastSentSeqNum)
      return APC_ERR_PROTOCOL;
   if (m_lastSentSeqNum - confirmedNum > m_outpkts.size()) {
      confirmedNum = m_lastSentSeqNum - m_outpkts.size();
      res = APC_ERR_OUTBUFOVERFLOW;
   }
   uint32_t numPkt = (uint32_t)(m_lastSentSeqNum - confirmedNum);
   if (m_numPackets > numPkt || isInit)  // After confirmation number packet in cache can only decrease
      m_numPackets = numPkt;
   return res;
}

void CAPCCache::prepForGet()
{
   boost::unique_lock<boost::mutex>  lock(m_lock);
   m_getNextSeqNum = m_lastSentSeqNum - m_numPackets;
}

apc_error_t CAPCCache::getNextPacket(apc_cache_pkt_s * pPkt)
{
   boost::unique_lock<boost::mutex>  lock(m_lock);

   if (m_getNextSeqNum >= m_lastSentSeqNum)
      return APC_ERR_NOTFOUND;

   apc_error_t res = APC_OK;
   uint64_t offset = m_lastSentSeqNum - m_getNextSeqNum;
   if (offset > m_numPackets) {
      offset  = m_numPackets;
      m_getNextSeqNum = m_lastSentSeqNum - offset;
      res = APC_ERR_OUTBUFOVERFLOW;
   }
   
   uint32_t pos = m_pos;
   if (offset > m_pos)
      pos += m_outpkts.size();
   pos -= (uint32_t)offset;

   *pPkt = m_outpkts[pos];
   m_getNextSeqNum++;
   return res;
}
