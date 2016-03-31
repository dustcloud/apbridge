/*
 * Copyright (c) 2013, Linear Technology.  All rights reserved.
 */

#pragma once

#include <ostream>
#include <streambuf>
#include <google/protobuf/message.h>
#include "zmqUtils.h"

/**
 * Simple ostream wrapper to write protobuf serialization into a character array.
 */
template <typename char_type>
struct ostreambuf : public std::basic_streambuf<char_type, std::char_traits<char_type> >
{
   ostreambuf(char_type* buffer, std::streamsize bufferLength)
   {
      // set the "put" pointer the start of the buffer and records its length.
      this->setp(buffer, buffer + bufferLength);
   }
};

/**
 * Serialize protobuf message into memory
 *
 * \return length of the serialized data
 */
int serializeProtobuf(const google::protobuf::Message& msg, void* data, int length);

/**
 * Serialize protobuf message and append a new frame to a zmessage
 *
 * \return error code
 */
int serialize2zmsg(const google::protobuf::Message& msg, zmqUtils::zmessage& out);

/**
 * Deserialize a protobuf message from the first frame of a zmessage. This
 * call destroys the first frame of the zmessage.
 *
 * \attention The caller is responsible for creating an empty protobuf
 * instance of the appropriate type.
 */
void deserialize2pb(google::protobuf::Message& msg, zmqUtils::zmessage& resp);

