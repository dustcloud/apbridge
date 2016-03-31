#include <stdint.h>
#include <time.h>

void convertToLocaltime(time_t * pTime, tm * pLocalTime, int32_t * pTZ) 
{
   localtime_r(pTime, pLocalTime);
   if (pTZ) {
      tzset();
      *pTZ = timezone;
   }
}

