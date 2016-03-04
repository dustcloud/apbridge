/*
 * Copyright (c) 2013, Linear Technology.  All rights reserved.
 */

#pragma once

#include <stdint.h>
#include <string>
#include <iostream>
#include "zmq.hpp"
#include "czmq.h"

#include "Logger.h"

/**
 * ZeroMQ helper classes built around the CZMQ library
 */
namespace zmqUtils {

   // TODO: ZFrame
   // must transfer ownership correctly when interacting with ZMessage
   
   /**
    * Multi-part message class
    *
    * This class provides a C++ wrapper around the CZMQ multi-part message structure. 
    */
   class zmessage 
   {
   public:
      /**
       * Initialize an empty message
       */ 
      zmessage() : m_msg(zmsg_new()) { ; }
      
      /**
       * Initialize message from data using zero-copy
       *
       * \param  data     buffer of data for the first frame
       * \param  length   length of the data
       * \param  free_fn  function to call to free the the data
       * \param  arg      arbitrary parameter to pass to free_fn
       
       */ 
      //zmessage(void* data, size_t length, zframe_free_fn* free_fn, void* arg)
      //   : m_msg(zmsg_new())
      //{
      //   zframe_t* frame = zframe_new_zero_copy(data, length, free_fn, arg);
      //   zmsg_add(m_msg, frame);
      //}
      
      /**
       * Duplicate a message
       *
       * \param msg The zmessage to copy
       */ 
      explicit zmessage(zmessage* msg) : m_msg(NULL)
      {
         m_msg = zmsg_dup(msg->m_msg);
      }
      
      /**
       * Construct a message by reading from a ZMQ socket.
       *
       * \param sock ZMQ socket to read from
       */ 
      explicit zmessage(zmq::socket_t* sock) : m_msg(NULL)
      {
         m_msg = zmsg_recv(*sock);
      }
      
      /**
       * Destroy the message
       */ 
      ~zmessage() { if (m_msg) zmsg_destroy(&m_msg); }

      /**
       * Get the front frame of the message. The frame remains part of the
       * message. The zmessage class maintains responsibility for destroying
       * the frame.
       *
       * \return the front frame. 
       */
      inline zframe_t* front() { return zmsg_first(m_msg); }

      /**
       * Return the next frame of the message. 
       * The caller is responsible for destroying the frame if needed.
       * 
       * \return the front frame. 
       */
      zframe_t* next() {
         return zmsg_next(m_msg);
      }

      /**
       * Return the last frame of the message. 
       * The caller is responsible for destroying the frame if needed.
       * 
       * \return the front frame. 
       */
      zframe_t* last() {
         return zmsg_last(m_msg);
      }

      /**
       * Pop the front frame of the message. The caller is responsible for
       * destroying the frame.
       *
       * \return the front frame. 
       */
      zframe_t* pop() {
         return zmsg_pop(m_msg);
      }

      /**
       * Push the frame onto the front of the message.
       *
       * \return nothing
       */
      int push(zframe_t* frame) {
         return zmsg_push(m_msg, frame);
      }
      
      /**
       * Add the frame onto the end of the message. 
       *
       * \return nothing
       */
      int add(zframe_t* frame) {
         return zmsg_add(m_msg, frame);
      }
      
      /**
       * Pop (and destroy) the front frame of the message.
       * 
       * \return string containing the data of the first frame. The result is empty if the message contains no further frames.
       */ 
      std::string popstr() {
         zframe_t* frame = zmsg_pop(m_msg);
         std::string result;
         if (frame != NULL) {
            result = std::string((char*)zframe_data(frame), zframe_size(frame));
            zframe_destroy(&frame);
         }
         return result;
      }
      
      /**
       * Convenience method to push a string to the front of the message.
       *
       * \param value string to append
       */
      int pushstr(std::string value) {
         return zmsg_pushstr(m_msg, value.c_str());
      }
      
      /**
       * Convenience method to add a string to the end of the message.
       *
       * \param value string to append
       */
      int addstr(std::string value) {
         return zmsg_addstr(m_msg, value.c_str());
      }

      /**
       * Pop (and destroy) the front frame of the message.
       * 
       * \attention The integer is converted from network byte order.
       *
       * \return integer in the data of the first frame. The result is -1 if the message contains no further frames.
       */ 
      int popint() {
         zframe_t* frame = zmsg_pop(m_msg);
         int result = -1;
         if (frame != NULL && zframe_size(frame) >= sizeof(int)) {
            int* buf = (int*)zframe_data(frame);
            result = ntohl(buf[0]);
            zframe_destroy(&frame);
         }
         return result;
      }
      
      /**
       * Convenience method to add an integer to the end of the message.
       *
       * \attention The integer is converted to network byte order.
       *
       * \param value integer to append
       */
      int addint(int value) {
         int nValue = htonl(value);
         return zmsg_addmem(m_msg, &nValue, sizeof(value));
      }      
      
      /**
       * Convenience method to add a byte array to the end of the 
       * message.
       *
       * \param buf      Buffer to append
       * \param bufSize  Buffer size
       */
      int addbytearray(const uint8_t* buf, int bufSize) {
         return zmsg_addmem(m_msg, buf, bufSize);
      }

      /**
       * Get the number of frames in the message.
       * \return number of frames in the message
       */
      inline size_t size() const { return zmsg_size(m_msg); }

      // TODO remove wrap / unwrap -- deprecated in czmq library
      /**
       * Wrap a frame with a string value around the message. Typically, this
       * is used to wrap a ZMQ identifier around a message.
       *
       * \param  value  the string value
       * \param  delimiter  additional value to insert before wrapping
       */
      void wrap(std::string value, std::string delimiter)
      {
         if (!delimiter.empty()) {
            zmsg_pushstr(m_msg, delimiter.c_str());
         }
         zframe_t* frame = zframe_new(value.data(), value.size());
         zmsg_wrap(m_msg, frame);
      }

      /**
       * Unwrap a frame containing an identifier
       */
      zframe_t* unwrap() 
      {
         return zmsg_unwrap(m_msg);
      }

      /**
       * Replace the message with a message received from a socket
       *
       * \param  sock  the socket to receive from
       * \return 0
       */
      int recv(zmq::socket_t* sock) 
      {
         if (m_msg != NULL)
            zmsg_destroy(&m_msg);

         m_msg = zmsg_recv(*sock);
         return 0;
      }

      /**
       * Send the message to a socket
       *
       * \attention The socket is responsible for destroying the underlying
       * zmsg structure. The caller is left with an empty zmessage instance,
       * which must be deleted (if on the heap).
       * 
       * \param  sock  the socket to send the message on
       * \return 0
       */
      int send(zmq::socket_t* sock) 
      {
         int result = zmsg_send(&m_msg, *sock);
         m_msg = NULL;
         return result;
      }

      /**
       * Print the message to an ostream
       * 
       * \attention The dump method iterates over the frames of a
       * message. This will interrupt any other iteration over the frames.
       *
       * \param  os  output stream to write to
       * \param  delimiter  string to separate frames with
       * \return output stream
       */
      std::ostream& dump(std::ostream& os, const std::string& delimiter = " | ") 
      {
         // TODO: save flags
         zframe_t* frame = front();
         for (size_t i = 0; frame != NULL; frame = zmsg_next(m_msg), i++) {
            size_t len = zframe_size(frame);
            char* data = (char*)zframe_data(frame);
            
            bool useAscii = true;
            for (size_t j = 0; j < len && useAscii; j++) {
               useAscii = (isprint(data[j])) ? true : false;
            }
            os << '[' << len << "] ";
            // change output based on text or binary
            if (len > 0) {
               if (useAscii) {
                  os << std::string(data, len);
               }
               else {
                  char* hexdata = (char*)zframe_strhex(frame);
                  os << hexdata;
                  free(hexdata);
               }
            }
            os << delimiter;
         }
         // TODO restore flags
         return os;
      }      

   private:
      zmsg_t* m_msg;
      
      // TODO: allow copy operations
      zmessage(const zmessage& rhs);
      zmessage& operator=(const zmessage& rhs);
   };

   /**
    * Create new ZMQ client
    * 
    * \param ctx      ZMQ context
    * \param identity Client identity
   */
   zmq::socket_t* createClient(zmq::context_t* ctx,
                               const std::string identity);
   
}


inline std::ostream& operator<<(std::ostream& os, const zmqUtils::zmessage& zmsg)
{
   return const_cast<zmqUtils::zmessage&>(zmsg).dump(os);
}

#define DUSTLOG_MSGDEBUG(logger, prefix, msg) \
   if (Logger::isEnabled(Logger::getLog(logger), log4cxx::Level::getDebug())) \
   {\
      std::ostringstream os; os << *msg;\
      DUSTLOG_DEBUG(logger, prefix << os.str());   \
   }

#define DUSTLOG_MSGTRACE(logger, prefix, msg) \
   if (Logger::isEnabled(Logger::getLog(logger), log4cxx::Level::getTrace())) \
   {\
      std::ostringstream os; os << *msg;\
      DUSTLOG_TRACE(logger, prefix << os.str());   \
   }


#if 0
#include "log4cxx/stream.h"
// TODO: still can't get operator<< to work with logstream for use
// in DUSTLOG macros
// the following still results in template resolution errors
inline log4cxx::logstream& operator<<(log4cxx::logstream& os,
                                      const zmqUtils::zmessage& zmsg)
{
   std::basic_ostream<char>& bos = (std::basic_ostream<char>&)os;
   const_cast<zmqUtils::zmessage&>(zmsg).dump(bos);
   return os;
}
#endif

