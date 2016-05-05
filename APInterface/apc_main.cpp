/*
 * Copyright (c) 2014, Linear Technology. All rights reserved.          
 */

#include <iostream>
#include <iomanip>
#include <iterator>
#include <algorithm>
#include <vector>
#include <string>
#include <sstream>
#include <fstream>
#include <signal.h>
#include <memory>

#include "common.h"
#include "Logger.h"

#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/assign/list_of.hpp> 

#include "SerialPort.h"
#include "APMTransport.h"
#include "GPS.h"
#include "APCoupler.h"
#include "APCClient.h"

#include "common/ProcessInputArguments.h"
#include "watchdog/public/IWdClntWrapper.h"
#include "6lowpan/public/dn_api_common.h"
#include "6lowpan/public/dn_api_local.h"
#include <boost/program_options.hpp>

#include "rpc/public/RpcCommon.h"
#include "rpc/APCRpcWorker.h"
#include "rpc/public/RpcServer.h"

#include "apc_main.h"
#include "apc_common.h"
#include "Version.h"

//#define INTERRACT 1  // If define then manager wait Press key for stopping otherwise signal

#ifndef WIN32
  namespace posix = boost::asio::posix;
#endif

typedef std::vector<uint8_t> bytes_t;

//========================== Namespaces ==============================
using namespace std;
namespace po = boost::program_options;

//========================== Defines =================================
const char WELCOME_MSG[] = "APC";
const char* APC_LOG_NAME = "apc.main";
static const uint32_t WD_APC_KA_TIMEOUT_SEC = 2;
EAPResetSignal APResetSignal = AP_RESET_SIGNAL_TX;

boost::asio::io_service g_svc;

typedef std::pair<const char *, const char *> ArgsAttr_t;

class CApcProcessInputArguments : public CProcessInputArguments
{
public:
   std::string clientId;
   std::string sHostName;
   uint16_t    port;
   std::string sApiPortName;
   std::string sResetPortName;
   uint32_t    baudRate;

   //Config params coming only from INI
   uint16_t maxQueueSize;   // Maximum number of messages can be queued to AP
   uint16_t highQueueWatermark;
   uint16_t lowQueueWatermark;
   uint32_t pingTimeout;
   uint32_t retryDelay;
   uint32_t retryTimeout; // In milliseconds
   uint16_t maxRetries;
   uint16_t maxMsgSize;	  // Maximum buffer size of one message to AP
   uint32_t maxPacketAge; // Maximum packet age allowed before triggering TX_PAUSE to manager

   std::string sGpsdHost;
   std::string sGpsdPort;
   uint32_t gpsdTimeout;
   uint16_t gpsdMinSatsInUse;
   uint32_t gpsMaxStableTime;

   uint32_t apcMaxQueueSize;
   uint32_t apcKaTimeout;
   uint32_t apcfreeBufferTimeout;
   uint32_t apcReconnectDelay;
   uint32_t apcDisconnectTimeout;

   uint32_t resetBootTimeout;
   uint32_t disconnectShortBootTimeoutMsec;
   uint32_t disconnectLongBootTimeoutMsec;

   std::string sResetSignal;
   bool bReconnectSerial;
   bool bGpsdConn;

   CApcProcessInputArguments(): CProcessInputArguments(DEFAULT_FILE_NAME, "APC") {
      sHostName = DEFAULT_MNGR_HOST;
      port = DEFAULT_MNGR_PORT;
      sApiPortName = DEFAULT_DEV_API_PORT;
      sResetPortName = DEFAULT_DEV_RESET_PORT;
      baudRate = DEFAULT_BAUD_RATE;

      maxQueueSize = APM_DEFAULT_MAX_QUEUE_SIZE; 
      highQueueWatermark = APM_DEFAULT_HIGH_QUEUE_WATER_MARK;
      lowQueueWatermark = APM_DEFAULT_LOW_QUEUE_WATER_MARK;
      pingTimeout = APM_DEFAULT_PING_TIMEOUT;
      retryDelay = APM_DEFAULT_RETRY_DELAY;
      maxRetries = APM_DEFAULT_MAX_RETRIES;
      retryTimeout = APM_DEFAULT_RETRY_TIMEOUT;
      maxMsgSize = APM_DEFAULT_MAX_MSG_SIZE;
	  maxPacketAge = APM_DEFAULT_MAX_PACKET_AGE;

      sGpsdHost = GPSD_DEFAULT_HOST;
      sGpsdPort = GPSD_DEFAULT_PORT;
      gpsdTimeout = GPSD_DEFAULT_READ_TIMEOUT;
      gpsdMinSatsInUse = GPSD_DEFAULT_MIN_SATS_INUSE;
      gpsMaxStableTime = GPS_DEFAULT_MAX_STABLE_TIME;

      apcMaxQueueSize = APC_DEFAULT_MAX_QUEUE_SIZE;
      apcKaTimeout = APC_DEFAULT_KATIMEOUT;
      apcfreeBufferTimeout = APC_DEFAULT_FREEBUFFER_TIMEOUT;
      apcReconnectDelay = APC_DEFAULT_RECONNECT_DELAY;
      apcDisconnectTimeout = APC_DEFAULT_DISCONNECT_TIMEOUT;

      resetBootTimeout                = RESET_BOOT_TIMEOUT;
      disconnectShortBootTimeoutMsec  = DISCONNECT_BOOT_TIMEOUT_SHORT;
      disconnectLongBootTimeoutMsec   = DISCONNECT_BOOT_TIMEOUT_LONG;

	  sResetSignal = RESET_SIGNAL_TX;
	  bReconnectSerial = true;
	  bGpsdConn = false;
   }
protected:
   virtual void add_options(boost::program_options::options_description& desc) {
      desc.add_options()
      ("client-id", boost::program_options::value<string>(&clientId), "<name> APC Client Name")
      ("apc-disconnect-timeout", boost::program_options::value<uint32_t>(&apcDisconnectTimeout), "APC Client disconnect timeout, in milliseconds")
      ("apc-free-buffer-timeout", boost::program_options::value<uint32_t>(&apcfreeBufferTimeout), "APC Client free buffer timeout")
      ("apc-ka-timeout", boost::program_options::value<uint32_t>(&apcKaTimeout), "APC Client keep-alive timeout")
      ("apc-max-queue-size", boost::program_options::value<uint32_t>(&apcMaxQueueSize), "APC Client queue size")
      ("apc-reconnect-delay", boost::program_options::value<uint32_t>(&apcReconnectDelay), "APC Client reconnection delay, in milliseconds")
      ("api-device", boost::program_options::value<string>(&sApiPortName), "Serial device for AP Serial API")
      ("apm-max-msg-size", boost::program_options::value<uint16_t>(&maxMsgSize), "Maximum message size to AP")
      ("baud", boost::program_options::value<uint32_t>(&baudRate), "Baud rate")
      ("gps-max-stable-time", boost::program_options::value<uint32_t>(&gpsMaxStableTime), "Max time for GPS signal to be stable (In Seconds)")
      ("gpsd-host", boost::program_options::value<string>(&sGpsdHost), "GPSD host")
      ("gpsd-min-sats-in-use", boost::program_options::value<uint16_t>(&gpsdMinSatsInUse), "Threshold for minimum satellites used to trust / sync the GPS daemon output")
      ("gpsd-port", boost::program_options::value<string>(&sGpsdPort), "GPSD port")
      ("gpsd-timeout", boost::program_options::value<uint32_t>(&gpsdTimeout), "Read timeout from gpsd, in microseconds")
      ("gpsd-conn", boost::program_options::value<bool>(&bGpsdConn), "Whether APC connects to gpsd or not")
      ("high-queue-watermark", boost::program_options::value<uint16_t>(&highQueueWatermark), "High watermark for NACKing incoming messages")
      ("host", boost::program_options::value<string>(&sHostName), "Manager APC host")
      ("low-queue-watermark", boost::program_options::value<uint16_t>(&lowQueueWatermark), "Low watermark for allowing incoming messages")
      ("max-queue-size", boost::program_options::value<uint16_t>(&maxQueueSize), "Maximum queue size for messages to AP")
      ("max-retries", boost::program_options::value<uint16_t>(&maxRetries), "APC retry count to AP")
      ("ping-timeout", boost::program_options::value<uint32_t>(&pingTimeout), "Ping to AP if no activity recorded within this time, in milliseconds")
      ("port", boost::program_options::value<uint16_t>(&port), "Manager APC port")
      ("reset-device", boost::program_options::value<string>(&sResetPortName), "Serial device for AP reset control")
      ("retry-delay", boost::program_options::value<uint32_t>(&retryDelay), "Delay on receiving a NACK before resending the command, in milliseconds")
      ("retry-timeout", boost::program_options::value<uint32_t>(&retryTimeout), "APC retry timeout to AP, in milliseconds")
      ("reset-boot-timeout", boost::program_options::value<uint32_t>(&resetBootTimeout), "Boot-timeout after hardware AP reset")
      ("disconnect-boot-timeout-short", boost::program_options::value<uint32_t>(&disconnectShortBootTimeoutMsec), "Short boot-timeout after send disconnect command to AP")
      ("disconnect-boot-timeout-long", boost::program_options::value<uint32_t>(&disconnectLongBootTimeoutMsec), "Long boot-timeout after send disconnect command to AP")
      ("reset-signal", boost::program_options::value<string>(&sResetSignal), "Signal used to reset AP")
      ("reconnect-serial", boost::program_options::value<bool>(&bReconnectSerial), "Reconnect serial port on errors")
      ("max-packet-age", boost::program_options::value<uint32_t>(&maxPacketAge), "Maximim age allowed for packets wait in queue before triggering PAUSE to manager, in milliseconds")
      ;
   }
   virtual void process(boost::program_options::variables_map &vm) { 
      if (clientId.empty())
         throw boost::program_options::error("Missing ClientID");

      if (sResetSignal != RESET_SIGNAL_TX && 
          sResetSignal != RESET_SIGNAL_DTR) {
         throw boost::program_options::error("Invalid reset-signal value, must be TX or DTR.");
      }
   }
};

//TODO: If we want to Log the configuration and tunning parameters.
void printConfiguration(CApcProcessInputArguments& inputArgs)
{
   // print the command-line and config file params 
   DUSTLOG_INFO(APC_LOG_NAME,"APC Configuration: "<<"\n"<<
                "ClientID : "<<inputArgs.clientId<<"\n"<<
                "Config file : "<<inputArgs.getVal().confName <<"\n"<<
                "Host : " <<inputArgs.sHostName<<"\n"<<
                "Port : "<<inputArgs.port<<"\n"<<
                "API Port : "<<inputArgs.sApiPortName<<"\n"<<
                "Reset Port : "<<inputArgs.sResetPortName<<"\n"<<
                "Baud : "<<inputArgs.baudRate<<"\n"<<
                "Log Name : "  << inputArgs.getVal().logName<<"\n"<<
                "Max Queue Size : "<<inputArgs.maxQueueSize<<"\n"<<
                "High Queue Watermark : "<<inputArgs.highQueueWatermark<<"\n"<<
                "Low Queue Watermark : "<<inputArgs.lowQueueWatermark<<"\n"<<
                "Ping Timeout : "<<inputArgs.pingTimeout<<"\n"<<
                "NACK Retry Delay : "<<inputArgs.retryDelay<<"\n"<<
                "Retry Timeout : "<<inputArgs.retryTimeout<<"\n"<<
                "Max Retries : "<<inputArgs.maxRetries<<"\n"<<
                "Max Message Size : "<<inputArgs.maxMsgSize<<"\n"<<
                "GPSD Host : "<<inputArgs.sGpsdHost<<"\n"<<
                "GPSD Port : "<<inputArgs.sGpsdPort<<"\n"<<
                "GPSD Read Timeout : "<<inputArgs.gpsdTimeout<<"\n"<<
                "GPSD Min Sats in use : "<<inputArgs.gpsdMinSatsInUse<<"\n"<<
                "GPS Max Time to be Stable : "<<inputArgs.gpsMaxStableTime<<"\n"<<
                "GPSD Conn : " <<inputArgs.bGpsdConn<<"\n"<<
                "APC Client Max Queue Size      : "<<inputArgs.apcMaxQueueSize<<"\n"<<
                "APC Client KA Timeout          : "<<inputArgs.apcKaTimeout<<"\n"<<
                "APC Client Free Buffer Timeout : "<<inputArgs.apcfreeBufferTimeout<<"\n"<<
                "APC Client Reconnect Delay     : "<<inputArgs.apcReconnectDelay<<"\n"<<
                "APC Client Disconnect Timeout  : "<<inputArgs.apcDisconnectTimeout<<"\n"<<
                "Reset Signal : "<<inputArgs.sResetSignal<<"\n"<<
                "Reconnect Serial : "<<inputArgs.bReconnectSerial<<"\n"
                "Max Packet Age : "<<inputArgs.maxPacketAge<<"\n"
                );
}

/*
 * Welcome menu
 */
void welcomeMenu(const CApcProcessInputArguments& args)
{
   cout << "== " << toString(SYSTIME_NOW()) << endl;
   cout << "== " << WELCOME_MSG << " " << getVersionLabel() << " == " << endl;
   cout << args.toString() << endl;
}

int main(int argc, char* argv[])
{

try
{
   CApcProcessInputArguments  inputArgs;

   // parse command line options
   if (!inputArgs.parse(argc, argv)) {
      Logger::closeLogging();
      return 1;
   }

   //printConfiguration(inputArgs);
   
   // setup logging and log level
   Logger::openLogging(inputArgs.getVal().logName);
   DUSTLOG_SETLEVEL(APC_LOG_NAME, inputArgs.getVal().logLevel);
   DUSTLOG_SETLEVEL("apm", inputArgs.getVal().logLevel);
   DUSTLOG_SETLEVEL("apc", inputArgs.getVal().logLevel);

   DUSTLOG_INFO("apc", "**** APC START " << getVersionLabel() << " ****");
   welcomeMenu(inputArgs);

   
   std::unique_ptr<zmq::context_t> ctx(new zmq::context_t(1));

   CApiEndpointBuilder epGetter(inputArgs);
   string epRpc     = epGetter.getServer(APC_ENDPOINT_PATH, inputArgs.clientId),
          epLogNotif = epGetter.getServer(APC_LOG_ENDPOINT_PATH, inputArgs.clientId),
          epWd      = epGetter.getClient(WATCHDOG_ENDPOINT_PATH),
          epWdNotif = epGetter.getClient(WATCHDOG_NOTIFY_ENDPOINT_PATH);
  
   Logger::CScopedPublisher pub(ctx.get(), epLogNotif, log4cxx::Level::getTrace());
   
   CAPMTransport::init_param_t transportCfg(
                                            inputArgs.maxQueueSize,
                                            inputArgs.highQueueWatermark,
                                            inputArgs.lowQueueWatermark,
                                            inputArgs.pingTimeout,
                                            inputArgs.retryDelay,
                                            inputArgs.retryTimeout,
                                            inputArgs.maxRetries,
                                            inputArgs.maxMsgSize,
                                            inputArgs.maxPacketAge
                                          );

   IGPS::start_param_t gpsCfg = {
      inputArgs.sGpsdHost,
      inputArgs.sGpsdPort,
      inputArgs.gpsdTimeout,
      inputArgs.gpsdMinSatsInUse,
      inputArgs.gpsMaxStableTime,
      inputArgs.bGpsdConn
   };

   IAPCClient::start_param_t apcStartCfg = {
      inputArgs.sHostName,
      inputArgs.port,
      inputArgs.apcKaTimeout, // KA timeout
      inputArgs.apcfreeBufferTimeout, // free buffer waiting time
      inputArgs.apcReconnectDelay,  // reconnectionDelayMsec 
      inputArgs.apcDisconnectTimeout, // offlineMsec,             
   };

   if (inputArgs.sResetSignal == RESET_SIGNAL_TX) {
      APResetSignal = AP_RESET_SIGNAL_TX;
   } else {
      APResetSignal = AP_RESET_SIGNAL_DTR;
   }
   CSerialPort   port(g_svc, inputArgs.sApiPortName, inputArgs.sResetPortName, APResetSignal,
   	                  inputArgs.baudRate, true);
   CGPS          gps;
   CAPCoupler    coupler(g_svc);
   CAPCClient    client(inputArgs.apcMaxQueueSize);  
   CAPMTransport transport(INPUT_BUFFER_LEN, g_svc, &port, &coupler, 
   	                       inputArgs.bReconnectSerial);
   apc_error_t result;
   
   IAPCClient::open_param_t apcOpenCfg = {
      &coupler,           // IAPCClientNotif callback
      inputArgs.clientId, //
      "apc.client",       // TODO: log name
    };

   // Open Manger Client
   result = client.open(apcOpenCfg);
   if (result != APC_OK) {
      DUSTLOG_FATAL(APC_LOG_NAME, "APC Client open failed, " << toString(result));
      throw runtime_error("APC Client open failed");
   }

   // Open AP serial port
   while ((result = port.openPort()) != APC_OK) {
      DUSTLOG_WARN(APC_LOG_NAME, "Serial port open failed, " << toString(result) << ". Retrying...");
      boost::this_thread::sleep(boost::posix_time::milliseconds(RETRY_INTERVAL));
   }  

   // Open transport
   transport.open(transportCfg);

   // Start coupler
   CAPCoupler::init_param_t apm_init_params = {
      &transport,
		&client,
      apcStartCfg,
      &gps,
      gpsCfg,
      inputArgs.resetBootTimeout,
      inputArgs.disconnectShortBootTimeoutMsec,
      inputArgs.disconnectLongBootTimeoutMsec
   };
  
   result = coupler.open(apm_init_params);
   if (result != APC_OK) {
      DUSTLOG_FATAL(APC_LOG_NAME, "APCoupler start failed, " << toString(result));
      throw runtime_error("APCoupler start failed");
   }

   // Start Serial Port
   while ((result = port.start(transport)) != APC_OK) {
      DUSTLOG_WARN(APC_LOG_NAME, "Serial port start failed, " << toString(result) << ". Retrying...");
      boost::this_thread::sleep(boost::posix_time::milliseconds(RETRY_INTERVAL));
   }
   
   // Create and initialize Watchdog Notification object
   std::unique_ptr<IWdClntWrapper> pWdClnt(IWdClntWrapper::createWdClntWrapper(
      inputArgs.getVal().wdName, APC_LOG_NAME, epGetter, ctx.get()));

   // Create RPC workers
   std::unique_ptr<CAPCRpcWorker> apcwrk(new CAPCRpcWorker(ctx.get(), epRpc, inputArgs.clientId, 
                                                           &coupler, &client, &port));
   
   // Create RPC sever
   std::unique_ptr<RpcServer> svr(new RpcServer(ctx.get(), epRpc));
   // Add workers
   svr->addWorker(apcwrk.get());
   svr->start();
   boost::thread srvThread(boost::bind(&boost::asio::io_service::run, &g_svc));
   
   coupler.start();

   #ifndef INTERRACT
   pWdClnt->wait();
   #else
   cout << "Press any key for stop "; char c; cin >> c;
   #endif

   coupler.stop(); // calls client.stop()

   g_svc.stop();
   srvThread.join();
   
   pWdClnt->finish();

   port.stop();    // closes serial port
   
   svr->stop();


   DUSTLOG_INFO(APC_LOG_NAME, "Closing apc");

   //Logger::closePublisher();
   Logger::closeLogging();


} catch(zmq::error_t& ex) {
      cout << "ZMQ error: "<< ex.what() << endl;
      return 1;
}catch(boost::exception const&  ex) {
      cout << "Boost error: "<< boost::diagnostic_information(ex) << endl;
      return 1;
}catch (runtime_error& re) {
      cout << re.what() << endl;
      return 1;
} 
 
   
   return 0;
}



