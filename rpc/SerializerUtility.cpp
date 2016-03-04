/*
 * Copyright (c) 2013, Linear Technology.  All rights reserved.
 */

#include "public/SerializerUtility.h"

#include "public/RpcCommon.h"
#include <vector>
int serializeProtobuf(const google::protobuf::Message& msg, void* data, int length)
{
   int len = msg.ByteSize();
   // TODO: validate buffer length >= msg length
   ostreambuf<char> buffer(static_cast<char*>(data), length);
   std::ostream respStream(&buffer);
   msg.SerializeToOstream(&respStream);
   
   return len;
}


int serialize2zmsg(const google::protobuf::Message& msg, zmqUtils::zmessage& zmsg)
{
   uint8_t              bufArray[1024];   // 90% of messages < 1K
   std::vector<uint8_t> buvVector;        // vector will reallocate very rare
   uint8_t           *  data = NULL;

   size_t len = msg.ByteSize();
   zframe_t* frame;
   if (len > 0) {
      if (len <= sizeof(bufArray)) {
         data = bufArray;
      } else {
         buvVector.resize(len);
         data = buvVector.data();
      }
      serializeProtobuf(msg, data, len);
   }
   frame = zframe_new(data, len);
   zmsg.add(frame);
   return 0;
}

void deserialize2pb(google::protobuf::Message& msg, zmqUtils::zmessage& resp)
{
   msg.ParseFromString(resp.popstr());
}
