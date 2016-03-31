#pragma once
#include "common.h"
#include "APCConnector.h"
#include "public/APCError.h"

#include <boost/thread.hpp>

class CAPCCache
{
public:
   struct apc_cache_pkt_s 
   {
      uint32_t                   m_seqNumb;
      apc_msg_type_t             m_type;              // Type of packet
      uint32_t                   m_size;              // Packet size
      CAPCConnector::iobuf_t     m_payload;           // Payload of packet
   };
   CAPCCache(uint32_t cacheSize);
   ~CAPCCache();
   void        clear();

   /**
    * Adds a packet to cache.
    *
    * \param       type     Packet type.
    * \param       p1, p2   Packet payload
    * \param       s1, s2   Size of payload
    * \param [out] pSeqNum  Sequence number.
    *
    * \return  AP_OK- successful, 
    *          APC_ERR_OUTBUFOVERFLOW - return when cache is full, and next write can rewrites old packets
    *          APC_ERR_SIZE - size of packet wrong, packet is not saved
    */
   apc_error_t addPacket(apc_msg_type_t type, const uint8_t * p1, uint32_t s1, 
                         const uint8_t * p2, uint32_t s2, uint32_t * pSeqNum);

   /**
    * Remove packets up to (and including) confirmedNum from cache.
    *
    * \param   confirmedNum   The confirmed number.
    *
    * \return  .
    */
   apc_error_t confirmedSeqNum(uint32_t confirmedNum, bool isInitialization = false);

   /**
    * Prepare to get packets from cache.
    */
   void       prepForGet();

   /**
    * Gets a next packet from cache.
    *
    * \param [out]      pPkt    packet
    *
    * \return  APC_OK if successful, 
    *          APC_ERR_OUTBUFOVERFLOW - some packet lost
    *          APC_ERR_NOTFOUND - no packet in cache otherwise error code
    */
   apc_error_t getNextPacket(apc_cache_pkt_s * pPkt);

   uint32_t    getLastSent() const { return (uint32_t)m_lastSentSeqNum; }

   uint32_t    getNumCachedPkts() const { return m_numPackets; }
   size_t      getCacheSize() const { return m_outpkts.size(); }

private:
   std::vector<apc_cache_pkt_s>  m_outpkts;           // Cached data
   uint64_t                      m_lastSentSeqNum;    // Last sent packet
   uint32_t                      m_numPackets;       // Number packet in cache
   uint32_t                      m_pos;               // Current position in cache
   uint64_t                      m_getNextSeqNum;     // seq. number of packet processing getNextPacket function
   boost::mutex                  m_lock;
   
};

