#pragma once

#include <iostream>
#include <boost/atomic.hpp>
#include <boost/thread.hpp>
#include <boost/asio.hpp>
#include "public/IGPS.h"

#ifdef GPSTEST
   #include "unit_tests/Testlibgpsmm.h"
#else
   #include "libgpsmm.h"
#endif

class CGPS : public IGPS
{
public:
   //ctor & dtor
   CGPS();
   ~CGPS(void);

   virtual gps_error_t start(const start_param_t& config,IGPSNotifHandler *gpsStatusChangeHandler);
   virtual void stop();
   virtual gps_status_t getStatus();

private:
   start_param_t m_config_params; //Initialization parameters

   boost::atomic<bool> m_readThreadRunning;  //True till the GPS Thread is running
   boost::thread *m_gpsdReadThread;
   gps_data_t* m_lastRecord;
  
   boost::atomic<gps_status_t> m_current_gps_status; 
   void setStatus(const gps_status_t& newstatus); //Strictly use this method to set the new status.

   IGPSNotifHandler* m_notifHandler;   // interface for sending GPS status notification
      
   //Thread function to read records from GPS daemon
   void readGPSThreadFun();

   //TimeStamp when stable signal starts in seconds
   uint64_t m_stable_start_time;

   uint16_t m_satellites_used;
   uint16_t m_satellites_visible;

   void setGpsFixMode (uint16_t);
   void setSatellitesVisible(uint16_t);
   void setSatellitesUsed(uint16_t);
   

};

