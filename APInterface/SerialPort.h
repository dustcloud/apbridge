/*
 * Copyright (c) 2014, Linear Technology. All rights reserved. 
 */

#pragma once

#include <boost/asio.hpp>
#include <boost/asio/serial_port.hpp>
#include <vector>

#include "common.h"
#include "StatDelaysCalc.h"
#include "HDLC.h"
#include "public/APCError.h"


enum EAPResetSignal {
   AP_RESET_SIGNAL_TX  = 1,        // use TX line to reset AP
   AP_RESET_SIGNAL_DTR = 2,        // use DTR to reset AP
};

/**
 * Interface for sending data to the Serial Port
 */
class IInputHandler
{
public:
   virtual ~IInputHandler() { ; }
   /**
    * Send data
    */
   virtual int handleData(const std::vector<uint8_t>& data) = 0;

   /**
    * Reset AP
    */
   virtual void sendResetAP() = 0;

   /**
    * Check if port is ready.
    */
   virtual bool isReady() = 0;

   //open APM Port
   virtual apc_error_t openPort() = 0;

   //close APM Port
   virtual apc_error_t closePort() = 0;

   //restart APM Port
   virtual apc_error_t restart() = 0;
};

/**
 * Interface for receiving data from the Serial Port
 */
class IAPMMsgHandler
{
public:
   virtual ~IAPMMsgHandler() { ; }
   virtual apc_error_t dataReceived(const uint8_t* data, size_t size) = 0;
};


extern const uint32_t DEFAULT_BAUD_RATE;

/**
 * Serial Port class for ASIO
 */
class CSerialPort : public IInputHandler, public IHDLCCallback {
public:
   /**
    */
   CSerialPort(boost::asio::io_service& io_service,
               std::string apiPort, std::string resetPort,
               EAPResetSignal resetSignal,
              uint32_t baud = DEFAULT_BAUD_RATE, bool useHDLC = true);
   
   virtual ~CSerialPort();

   apc_error_t start(IAPMMsgHandler& handler);
   void stop();
   
   void setReadTimeout(int readTimeout) { m_readTimeout = readTimeout; }

   virtual void sendResetAP();

   // this handler is called with data to be sent
   virtual int handleData(const std::vector<uint8_t>& data);

   // this handler is called when data is read from the serial port
   void handleRead(const boost::system::error_code& error, size_t length);

   // callback when data is written
   //void handleWriteComplete(const boost::system::error_code& error);

   // this handler is called when a complete frame is received by the HDLC decoder
   virtual void frameComplete(const std::vector<uint8_t>& data);

   //to check if serialport is ready
   virtual bool isReady();

   //open APM Port
   apc_error_t openPort();

   //close APM Port
   apc_error_t closePort();

   //restart APM Port
   apc_error_t restart();

   statdelays_s getStatToAP();
   statdelays_s getStatFromAP();
   uint32_t     getNumOutBuffers() const { return m_outbufPool.getNumOutBuffers(); }

private:
   typedef std::vector<uint8_t> iobuffer_t;
   class CBufferPool {
   public:
      typedef uint32_t iobufid_t;
      CBufferPool(){;}
      iobufid_t    alloc();
      void         free(iobufid_t bufId);
      iobuffer_t&  get(iobufid_t bufId);
      uint32_t getNumOutBuffers() const { return m_pool.size(); }
   private:
      boost::mutex  m_lock;
      std::vector<std::pair<bool, iobuffer_t>> m_pool;
   };

   // internal implementation to write data
   void write(const iobuffer_t& data);
   void handleWriteComplete(CBufferPool::iobufid_t bufId, const boost::system::error_code& error);

   // internal implementation to reset AP
   void resetDtrLine();
   void resetTxLine();

private:
   boost::asio::io_service& m_io_service;

   IAPMMsgHandler* m_inputHandler;
   
   // serial port options
   int m_readTimeout; // millisecond timeout for read operations
   CHDLC* m_encoder;
   
   // serial port
   std::string m_apiPort;
   std::string m_resetPort;
   boost::asio::serial_port m_serial;

   // reset signal, choose TX line or DTR line
   EAPResetSignal m_resetSignal;

   //baud
   uint32_t m_baud;
      
   iobuffer_t   m_input;
   CBufferPool  m_outbufPool;
   //std::vector<uint8_t> m_output;

   CStatDelaysCalc m_statToAP;
   CStatDelaysCalc m_statFromAP;
};
