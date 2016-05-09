#include "GPS.h"
#include "Logger.h"

#include "NTPLeapSec.h"
#include <time.h>
#include "6lowpan/public/dn_api_local.h"

const char * LOGGER_NAME = "gps";
#define RETRY_TIME 5000 // Retry interval if connection with GPS daemon is lost (milliseconds)

CGPS::CGPS()
   : m_readThreadRunning(false),
     m_gpsdReadThread(nullptr),
     m_lastRecord(nullptr),
     m_current_gps_status(GPS_ST_NO_DEVICE),
     m_notifHandler(nullptr),
     m_stable_start_time(0),
     m_satellites_used(0),
     m_satellites_visible(0)
{
   ;
}

CGPS::~CGPS(void)
{
   stop();
}
gps_error_t CGPS::start(const start_param_t& config,IGPSNotifHandler *gpsStatusChangeHandler)
{
   m_config_params = config;
   m_notifHandler = gpsStatusChangeHandler;

   std::stringstream ss;
   ss << "Configured no. of min satellitesInUse : " << m_config_params.m_minSatellitesInUse << " min stable Time:"<< m_config_params.m_maxTimeToStable <<" secs \n";
   DUSTLOG_INFO(LOGGER_NAME, ss.str());

   m_readThreadRunning = false;
   //Start Thread to read GPS records from gpsd
   m_gpsdReadThread = new boost::thread(boost::bind(&CGPS::readGPSThreadFun, this));

  for(int i=0; i < 100 && !m_gpsdReadThread; i++)      // Wait thread start 1 sec (100 * 10 sec)
      boost::this_thread::sleep_for(msec_t(10));

  if (!m_gpsdReadThread) 
      return GPS_ERR_STATE;

   return GPS_OK;
}
void CGPS::stop()
{
   m_readThreadRunning = false;

   //Wait stopping GPS read thread
   if (m_gpsdReadThread != nullptr) {
      m_gpsdReadThread->join();
      delete m_gpsdReadThread;
      m_gpsdReadThread = nullptr;
   }
}
void CGPS::readGPSThreadFun()
{
   m_readThreadRunning = true;

   gpsmm *gps_rec = nullptr;

   while(m_readThreadRunning)
   {
      //cleanup
      if(gps_rec != nullptr) //Remove pre-allocated gps object
      {
         delete gps_rec;
         gps_rec = nullptr;
      }

      gps_rec = new gpsmm(m_config_params.m_gpsdAddress.c_str(), m_config_params.m_gpsdPort.c_str());
      
      if (gps_rec->stream(WATCH_ENABLE) == 0) {
        DUSTLOG_WARN(LOGGER_NAME,"No GPSD running.");
         //Causes To be here
         //1. No GPSD Running on the host
         //m_current_gps_status = GPS_ST_NO_DEVICE;
         setStatus(GPS_ST_NO_DEVICE);
         boost::this_thread::sleep_for(msec_t(RETRY_TIME)); //Wait for 5 seconds before retry
         continue;
      }
 
       while(m_readThreadRunning)
       {
         if (!gps_rec->waiting(m_config_params.m_waitTimeout))
         {
            DUSTLOG_INFO(LOGGER_NAME,"GPSD Input not available");
            //Causes To be here
            //1. Antenna Removed
            //2. GPS device Removed
            //3. GPSD unable to receive new sentences.
            if(m_lastRecord != NULL) // No last record available to get the status.
            {
               delete m_lastRecord;
               m_lastRecord = NULL;
               setStatus(GPS_ST_CONN_LOST);
            }
            continue;
         }
   
         m_lastRecord = gps_rec->read();
         
         if (m_lastRecord == NULL) 
         {
            DUSTLOG_ERROR(LOGGER_NAME, "GPSD Read error.");
            //Causes To be here
            //1. GPSD aborted in between. GPSD need to restart and device has to be redetected.
            setStatus(GPS_ST_CONN_LOST);
	         break; 
         }
         else
         {

             dn_utc_time_t   utcTime;
             std::stringstream ss;

             /* When the number of satellites in use is greater than our configurable parameter,
             we record the "stable start time". When the number of satellites in use has been
             stable for long enough, then we declare that the GPS is sync'ed.
             If the number of satellites in use drops below our configurable parameter, then
             we reset the stable start time (as part of setting our status to GPS_ST_NO_SYNC).*/

             // get time from gps
             ss.precision(17);
             ss <<  " receive gps data, timestamp: " << m_lastRecord->online;
             ss << " utc time in gps: " << m_lastRecord->devices.time << std::endl;
             ss << " gps status: " << m_lastRecord->status;
             ss << " skyview time: " << m_lastRecord->skyview_time;
             ss << " update : " << m_lastRecord->fix.time;
             DUSTLOG_INFO(LOGGER_NAME, ss.str());

            //Send SetParameter Command with System time to AP
        	boost::chrono::system_clock::duration mngrUTCDuration = SYSTIME_NOW().time_since_epoch();
           // Seconds since beginning
        	utcTime.seconds = TO_SEC(mngrUTCDuration).count() - ntpGetLeapSecs(); 
        	// Microseconds since last second
           utcTime.useconds = (uint32_t)TO_USEC((mngrUTCDuration - TO_SEC(mngrUTCDuration))).count(); 
        	DUSTLOG_INFO(LOGGER_NAME, "system time: utc="<< utcTime.seconds <<"." 
                    << std::setfill ('0') << std::setw(6) << utcTime.useconds);

         

			if (m_satellites_visible != m_lastRecord->satellites_visible) {
				setSatellitesVisible(m_lastRecord->satellites_visible);
			}
			if (m_satellites_used != m_lastRecord->satellites_used) {
				setSatellitesUsed(m_lastRecord->satellites_used);
			}	

            if(m_lastRecord->satellites_used >= m_config_params.m_minSatellitesInUse)
            {
               if(m_stable_start_time == 0) //if stable start time is not noted before
               {
                  ss.clear();
                  ss <<  "GPS signal start: current satellites in use : " << m_lastRecord->satellites_used <<"\n";
                  DUSTLOG_DEBUG(LOGGER_NAME, ss.str());
                  m_stable_start_time = TO_SEC(TIME_NOW().time_since_epoch()).count(); //Note the time when stable signal starts.
               }
               else 
               { 
                  uint64_t currTime = TO_SEC(TIME_NOW().time_since_epoch()).count();
                  ss.clear();
                  ss <<  "No. of satellites in use:" << m_lastRecord->satellites_used << ",since last " << (currTime - m_stable_start_time) << "secs" ;
                  DUSTLOG_DEBUG(LOGGER_NAME, ss.str());

                  if( (currTime - m_stable_start_time) >= m_config_params.m_maxTimeToStable)
                     setStatus(GPS_ST_SYNC); //safe to declare signal as stable
               }
            }
            else
            {
               ss <<  "GPS signal Lost: current satellites in use : " << m_lastRecord->satellites_used <<"\n";
               DUSTLOG_DEBUG(LOGGER_NAME, ss.str());
               setStatus(GPS_ST_NO_SYNC); 
            }
         }
      }
      m_stable_start_time = 0; //Clear the stable signal timestamp
   }

   //cleanup
   if(gps_rec != nullptr) //Remove pre-allocated gps object
   {
      delete gps_rec;
      gps_rec = nullptr;
   }

   m_readThreadRunning = false;
}
gps_status_t CGPS::getStatus()
{
   return m_current_gps_status;
}
void CGPS::setStatus(const gps_status_t& newstatus)
{
   switch(newstatus)
   {
      case GPS_ST_NO_DEVICE:
      case GPS_ST_CONN_LOST:
      case GPS_ST_NO_SYNC:
           m_stable_start_time = 0; //Clear the stable signal timestamp
           break;
      case GPS_ST_SYNC:
           break;
   }

   if(m_current_gps_status != newstatus)
   {
      //Notify about the GPS status change.
      if(m_notifHandler)
      {     
         m_notifHandler->handleGPSStatusChanged(newstatus);
         std::stringstream ss;
         ss <<  "GPS Stat changed from "  <<  toString(m_current_gps_status) << " to " << toString(newstatus) << "\n";
         DUSTLOG_INFO(LOGGER_NAME, ss.str());
      }
   }

   m_current_gps_status = newstatus;
}


void CGPS::setSatellitesVisible(uint16_t visible)
{
	m_satellites_visible = visible;
	m_notifHandler->handleSatellitesVisibleChanged(visible);

}

void CGPS::setSatellitesUsed(uint16_t used)
{
	m_satellites_used = used;
	m_notifHandler->handleSatellitesUsedChanged(used);

}


