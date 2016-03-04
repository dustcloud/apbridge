#pragma once

#include <string>
#include "common.h"
#include <boost/function.hpp>
#include "public/GPSError.h"


/**
 * \file IAPCGPS.h
 */

/**
 * GPS Notif Handler
 *
 * Sends GPS Status change notifications to APC
 */
class IGPSNotifHandler
{
public:
   IGPSNotifHandler(void){ ; };
   virtual ~IGPSNotifHandler(void){ ; };

   /**
    * handles the GPS Status change.
    *
    */
   virtual void handleGPSStatusChanged(const gps_status_t& newstatus) = 0;

   virtual void handleSatellitesVisibleChanged(uint16_t) = 0;
   virtual void handleSatellitesUsedChanged(uint16_t) = 0;
   
};

/**
 * APC GPS interface
 */

class IGPS
{
public:
  
   struct start_param_t
   {
      std::string m_gpsdAddress; //Address where gps daemon is running
      std::string m_gpsdPort; //DEFAULT_GPSD_PORT is 2947
      uint32_t m_waitTimeout; //Wait Timeout to see if any input is waiting from gpsd (MicroSeconds)
      uint16_t m_minSatellitesInUse; //Minimum required satellites in the Fix.
      uint32_t m_maxTimeToStable; //Time taken to stabilize the signal (Milliseconds)
      bool m_gpsd_conn;       // Control whether APC connects to gpsd or not
   };


   virtual ~IGPS() {;}

   /**
    * Starts the GPS thread. 
    * This method connects to the  GPS daemon and 
    * starts reading the GPS sentences in a separate thread.
    *
    * \param param The initialize parameter of GPS daemon.
    *
    * \return result code.
    */
   virtual gps_error_t start(const start_param_t& config,IGPSNotifHandler *gpsStatusChangeHandler) = 0;

   /**
    * Stops the GPS thread. 
    */
   virtual void stop() = 0;

  /**
    * Gets the status.
    *
    * \return  The GPS status.
    */
   virtual  gps_status_t getStatus() = 0;
};

