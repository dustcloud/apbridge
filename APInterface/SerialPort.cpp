/*
 * Copyright (c) 2014, Linear Technology. All rights reserved. 
 */

#ifdef WIN32
   #pragma warning( disable : 4996 )
#endif

#include "SerialPort.h"
#include "apc_common.h"
#include "Logger.h"

#ifndef WIN32
   #include <sys/ioctl.h>
   #include <unistd.h>
#endif

#include <fcntl.h>
#include <errno.h>
#include <boost/bind.hpp>
#include <boost/exception/diagnostic_information.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

using namespace boost::asio;

const size_t MAX_HDLC_BUFFER_LEN = 256;

const int DEFAULT_READ_TIMEOUT = 0;

const char RESET_PORT_DATA[] = { 0 };
const int  RESET_PORT_BAUD = 9600;

const char SERIAL_LOGGER[] = "apm.io.serial";


CSerialPort::CSerialPort(boost::asio::io_service& io_service,
                         std::string apiPort, std::string resetPort,
                         EAPResetSignal resetSignal,
                         uint32_t  baud, bool useHDLC)
   : m_io_service(io_service),
     m_inputHandler(nullptr),
     m_readTimeout(DEFAULT_READ_TIMEOUT),
     m_encoder(NULL),
     m_apiPort(apiPort),
     m_resetPort(resetPort),
     m_serial(io_service),
     m_resetSignal(resetSignal),
     m_baud(baud),
     m_input(INPUT_BUFFER_LEN)
{
   if (useHDLC) {
      m_encoder = new CHDLC(MAX_HDLC_BUFFER_LEN, this);
   }

#if 0
   // TODO: do we need to manually set CTS?
   int fd = open(m_apiPort.c_str(), O_RDWR);
   int linemask = TIOCM_CTS; // CTS bit
   if (ioctl(fd, TIOCMBIS, &linemask) == -1)
      DUSTLOG_ERROR(SERIAL_LOGGER, "AP CTS failed: " << strerror(errno));
   close(fd);
#endif
  
}
apc_error_t CSerialPort::openPort()
{
   try
   {
      boost::system::error_code err;
      DUSTLOG_INFO(SERIAL_LOGGER,"Opening Serial Port: " << m_apiPort);
      m_serial.open(m_apiPort);
   
      // set parameters: baud, no flow control, 8N1
      m_serial.set_option(serial_port_base::baud_rate(m_baud), err);
      m_serial.set_option(serial_port_base::flow_control(serial_port_base::flow_control::none), err);
      m_serial.set_option(serial_port_base::character_size(8), err);
      m_serial.set_option(serial_port_base::parity(serial_port_base::parity::none), err);
      m_serial.set_option(serial_port_base::stop_bits(serial_port_base::stop_bits::one), err);
   } catch(boost::exception const&  ex) {
     //SerialPort not avaiable , Try to open it periodically.
      DUSTLOG_WARN(SERIAL_LOGGER,"Failed to open : "<< m_apiPort <<" Boost error: "<< boost::diagnostic_information(ex) <<" Trying to Reconnect...");
      return APC_ERR_IO;
   }

   return APC_OK;
}

apc_error_t CSerialPort::closePort()
{
   try
   {
      boost::system::error_code err;
      DUSTLOG_INFO(SERIAL_LOGGER,"Closing Serial Port: " << m_apiPort);
      m_serial.close();
   } catch(boost::exception const&  ex) {
      DUSTLOG_WARN(SERIAL_LOGGER,"Failed to close : "<< m_apiPort <<" Boost error: "<< boost::diagnostic_information(ex));
      return APC_ERR_IO;
   }

   return APC_OK;
}


bool CSerialPort::isReady()
{
   return m_serial.is_open();
}
CSerialPort::~CSerialPort()
{
   stop();
   delete m_encoder;
}

apc_error_t CSerialPort::start(IAPMMsgHandler& inputHandler)
{
   m_inputHandler = &inputHandler;

   // schedule the next read
   m_serial.async_read_some(boost::asio::buffer(m_input),
                            boost::bind(&CSerialPort::handleRead, this,
                                        boost::asio::placeholders::error,
                                        boost::asio::placeholders::bytes_transferred));
   return APC_OK;
}

apc_error_t CSerialPort::restart()
{
   // schedule the next read
   m_serial.async_read_some(boost::asio::buffer(m_input),
                            boost::bind(&CSerialPort::handleRead, this,
                                        boost::asio::placeholders::error,
                                        boost::asio::placeholders::bytes_transferred));
   return APC_OK;
}


void CSerialPort::stop()
{
   // cancel reads and writes
   if(m_serial.is_open())
      m_serial.cancel();
   
}

// this handler is called when data is received from the serial port
void CSerialPort::handleRead(const boost::system::error_code& error,
                             size_t length)
{
   mngr_time_t startTime = TIME_NOW();
   if (!error) {

      if (m_inputHandler != NULL) {
         if (m_encoder) {
            // TODO: replace with buffer xfer
            for (size_t i = 0; i < length; ++i) {
               m_encoder->addByte(m_input[i]);
            }
            // input is written to the other side when a frame is complete
         } else {
            // write input to the other side
            std::vector<uint8_t> xfer(length);
            std::copy(m_input.begin(), m_input.begin()+length, xfer.begin());
            
            apc_error_t res = m_inputHandler->dataReceived(xfer.data(), xfer.size());
            if (res != APC_OK)
               DUSTLOG_ERROR(SERIAL_LOGGER, "Serial port processing data received error" << toString(res));
         }
      }
      
      // schedule the next read
      m_serial.async_read_some(boost::asio::buffer(m_input),
                               boost::bind(&CSerialPort::handleRead, this,
                                           boost::asio::placeholders::error,
                                           boost::asio::placeholders::bytes_transferred));
   }
   m_statFromAP.addEvent(startTime);

}

// this handler is called when a complete HDLC frame is received
void CSerialPort::frameComplete(const std::vector<uint8_t>& data)
{
   if (m_inputHandler) {
      std::ostringstream os;
      os << "HDLC frame " << "[" << data.size() << "]";
      DUSTLOG_TRACEDATA(SERIAL_LOGGER, os.str(), data.data(), data.size());

      m_inputHandler->dataReceived(data.data(), data.size());
   }
}

int CSerialPort::handleData(const std::vector<uint8_t>& data)
{
   write(data);
   return data.size();
}

void CSerialPort::write(const iobuffer_t& data)
{
   mngr_time_t startTime = TIME_NOW();
   CBufferPool::iobufid_t  budId = m_outbufPool.alloc();
   iobuffer_t&             output = m_outbufPool.get(budId);  
   output.reserve(data.size()+4);
   if (m_encoder) {
      output = encodeHDLC(data);
   } else {
      // copy data to our buffer
      std::copy(data.begin(), data.end(), std::back_inserter(output));
   }

   std::ostringstream os;
   os << "HDLC output " << "[" << data.size() << "/" << output.size() << " BufID:" << budId << "]";
   DUSTLOG_TRACEDATA(SERIAL_LOGGER, os.str(), output.data(), output.size());
   boost::asio::async_write(m_serial, boost::asio::buffer(output.data(), output.size()),
                            boost::bind(&CSerialPort::handleWriteComplete, this, budId,
                                        boost::asio::placeholders::error));
   m_statToAP.addEvent(startTime);
}

void CSerialPort::handleWriteComplete(CBufferPool::iobufid_t bufId, const boost::system::error_code& error)
{
   m_outbufPool.free(bufId);
}

void CSerialPort::resetDtrLine()
{
#ifdef WIN32
   HANDLE hComm;
   // open the reset port
   hComm = CreateFileA( m_resetPort.c_str(), GENERIC_READ | GENERIC_WRITE, 
                        0, 
                        0, 
                        OPEN_EXISTING,
                        NULL,
                        0);
   if (hComm == INVALID_HANDLE_VALUE) {
      DUSTLOG_ERROR(SERIAL_LOGGER, "AP reset open failed: " << m_resetPort << ": "
                    << strerror(errno));
      return;
   }
   if (EscapeCommFunction(hComm, CLRDTR) == 0)  // Clear DTR bit
      DUSTLOG_ERROR(SERIAL_LOGGER, "AP reset clear DTR failed: " << strerror(errno));
   CloseHandle(hComm);
#else
   // open the reset port
   int fd = open(m_resetPort.c_str(), O_RDWR);
   if (fd < 0) {
      DUSTLOG_ERROR(SERIAL_LOGGER, "AP reset open failed: " << m_resetPort << ": "
                    << strerror(errno));
      return;
   }
   int linemask = TIOCM_DTR; // DTR bit
   if (ioctl(fd, TIOCMBIC, &linemask) < 0) 
      DUSTLOG_ERROR(SERIAL_LOGGER, "AP reset clear DTR failed: " << strerror(errno));
   close(fd);
#endif
}

void CSerialPort::resetTxLine()
{
   try {
      boost::asio::serial_port resetPort(m_io_service);
      resetPort.open(m_resetPort);
      
      // set parameters: 9600 baud
      boost::system::error_code err;
      resetPort.set_option(serial_port_base::baud_rate(RESET_PORT_BAUD), err);
      boost::asio::write(resetPort, boost::asio::buffer(RESET_PORT_DATA,
                                                        sizeof(RESET_PORT_DATA)));
      resetPort.close();
   } catch (const boost::exception& ex) {
      DUSTLOG_WARN(SERIAL_LOGGER, "Failed to open AP reset port: " << m_resetPort);
   }
}

void CSerialPort::sendResetAP()
{
   DUSTLOG_INFO(SERIAL_LOGGER, "use reset signal " << ((m_resetSignal == AP_RESET_SIGNAL_TX)?RESET_SIGNAL_TX:RESET_SIGNAL_DTR));
   // apc_main assures m_resetSignal can only be "TX" or "DTR"
   if (m_resetSignal == AP_RESET_SIGNAL_TX) {
   	  resetTxLine();
   } else {
      resetDtrLine();
   }
}

statdelays_s CSerialPort::getStatToAP()
{
   statdelays_s res;
   m_statToAP.getStat(&res);
   return res;
}

statdelays_s CSerialPort::getStatFromAP()
{
   statdelays_s res;
   m_statFromAP.getStat(&res);
   return res;
}

//------------------------------------------------------
// CBufferPool
CSerialPort::CBufferPool::iobufid_t CSerialPort::CBufferPool::alloc()
{
   boost::unique_lock<boost::mutex> lock(m_lock);
   iobufid_t bufIdx = 0, ee = (iobufid_t )m_pool.size();
   for(; bufIdx < ee && m_pool[bufIdx].first; bufIdx++);
   if (bufIdx < ee) 
      m_pool[bufIdx].first = true;                          // mark as allocated
   else 
      m_pool.push_back(std::make_pair(true, iobuffer_t())); // create new
   return bufIdx;
}

void CSerialPort::CBufferPool::free(iobufid_t bufId)
{
   boost::unique_lock<boost::mutex> lock(m_lock);
   BOOST_ASSERT(bufId < m_pool.size() && m_pool[bufId].first);
   m_pool[bufId].second.clear();    // Clean buffer
   m_pool[bufId].first = false;     // Mark as free
}

CSerialPort::iobuffer_t& CSerialPort::CBufferPool::get(iobufid_t bufId)
{
   boost::unique_lock<boost::mutex> lock(m_lock);
   BOOST_ASSERT(bufId < m_pool.size() && m_pool[bufId].first);
   return m_pool[bufId].second;
}

 