/*
 * Copyright (c) 2014, Linear Technology.  All rights reserved.
 */
#include "public/RpcProtocol.h"
#include <functional>
#include "public/SerializerUtility.h"

using namespace std;

// Returns: timestamp for use in RPC and OAuth authentication
int64_t getRpcTimestamp()
{
   // The OAuth methods require timestamps in seconds and there seems to be
   // something coupled between OAuth and RPC requests -- changing the
   // resolution here results in errors in the OAuth unit tests.
   return getCurrentTimeSecs();
}

// Returns: whether the timestamp is within the acceptable window
bool inTimestampWindow(int64_t timestamp, int64_t refTime)
{
   if (refTime == 0) {
      refTime = getRpcTimestamp();
   }
   uint64_t diff = abs(refTime - timestamp);
   return diff < TIMESTAMP_WINDOW_SIZE;
}

/**
 * Purge expired timestamp and nonce pairs
 */
void purgeTimestampNonceLog(timestampNonceLog_t& timestampNonceLog)
{
   int64_t currentTime = getRpcTimestamp();

   // The TimestampNonceLog is in order of the time the RPC calls are
   // received, which is similar but not the same as the timestamp value.
   // New timestamps are inserted at the end.

   // We can safely remove all timestamps that fall outside the window.
   // For simplicity, we iterate through the log until we find a timestamp
   // within the window and remove entries up to (but not including) that entry.
   
   timestampNonceLog_i iter;   
   for (iter = timestampNonceLog.begin();
        iter != timestampNonceLog.end(); iter++) {
      if (inTimestampWindow(iter->timeS, currentTime)) {
         break;
      }
   }
   
   if (iter != timestampNonceLog.begin()) {
      //DUSTLOG_DEBUG("rpc.server", "Purging timestamps up to " << iter->timeS << ", now=" << currentTime);
      timestampNonceLog.erase(timestampNonceLog.begin(), iter);
   }
}

/**
 * Validate timestamp and nonce values
 */
bool validateTimestampNoncePair(timestampNonce_t newTimestampNonce,
                                timestampNonceLog_t& timestampNonceLog)
{
   // Purge all timestamp and nonce pairs from local time
   purgeTimestampNonceLog(timestampNonceLog);

   // Validate timestamp
   if (!inTimestampWindow(newTimestampNonce.timeS)) {
      //DUSTLOG_ERROR("rpc.server", "Bad timestamp now=" << getRpcTimestamp()
      //              << " ts=" << newTimestampNonce.timeS);
      return false;
   }      
   
   // Validate if nonce at the same timestamp is duplicated
   timestampNonceLog_t::const_iterator iter;
   for (iter = timestampNonceLog.begin(); 
        iter != timestampNonceLog.end(); iter++) {
      if (newTimestampNonce.timeS == iter->timeS) {
         if (newTimestampNonce.nonce == iter->nonce) {
            //DUSTLOG_ERROR("rpc.server", "Duplicate timestamp, nonce pair");
            return false;
         }
      }
   }

   // insert the new timestamp nonce entry
   timestampNonceLog.push_back(newTimestampNonce);
   
   return true;
}
