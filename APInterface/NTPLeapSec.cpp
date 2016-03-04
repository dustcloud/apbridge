#include "APInterface/NTPLeapSec.h"
#include <sys/timex.h>

// Get leap seconds information.
uint32_t ntpGetLeapSecs()
{
   struct ntptimeval tmval;
   if (ntp_gettime(&tmval) != TIME_ERROR)
      return tmval.tai;
   return 0;
}

// Check current day
// TRUE if next midnights is time of change number of leap seconds.
bool  ntpIsLeapDay()
{
   struct ntptimeval tmval;
   int res = ntp_gettime(&tmval);
   if ( res == TIME_ERROR || res == TIME_OK)
      return false;
   return true;
}

// Gets the get current UTC.
tm  ntpGetCurUTC()
{
   time_t t = time(NULL);
   tm     res;
   gmtime_r(&t, &res);
   return res;
}
