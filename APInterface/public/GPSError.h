#pragma once
#include "common.h"

/**
 * \file APCGPSError.h
 */

/**
 * Values that represent APC GPS Errors.
 */
enum gps_error_t
{
   GPS_OK,                    ///< GPS Init OK
   GPS_ERR_STATE              ///< GPS Initi in Error
};
ENUM2STR(gps_error_t);

/**
 * Values that represent GPS Status
 */
enum gps_status_t
{
   GPS_ST_NO_DEVICE,          ///<  No GPS Device or Daemon
   GPS_ST_NO_SYNC,            ///<  GPS daemon not in sync with satellite
   GPS_ST_SYNC,               ///<  GPS daemon is in sync with satellite
   GPS_ST_CONN_LOST           ///<  Established GPS daemon connection lost
};
ENUM2STR(gps_status_t);

