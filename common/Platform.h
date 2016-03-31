/*
 *
 */

#ifndef Platform_H_
#define Platform_H_
#include <stdint.h>

#ifdef WIN32
   #include <windows.h>
   #include <sys/types.h> 
   #include <sys/timeb.h>
#else
   #include <sys/time.h>
#endif

namespace Platform {   

}  // end Platform namespace
 

#endif /* ! Platform_H_ */
