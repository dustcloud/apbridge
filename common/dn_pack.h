/*
Copyright (c) 2010, Dust Networks.  All rights reserved.
*/

#ifndef __DN_PACK_H
#define __DN_PACK_H

#ifdef __GNUC__
   #include "stdint.h"
   #define  PACKED_ATTR  __attribute__((packed))
   #define  PACKED_START _Pragma("pack(1)")
   #define  PACKED_STOP  _Pragma("pack()")

   #if !defined(L_ENDIAN) && !defined(B_ENDIAN)
      #include <endian.h>
      #if __BYTE_ORDER == __LITTLE_ENDIAN
         #define L_ENDIAN
      #elif __BYTE_ORDER == __BIG_ENDIAN
         #define B_ENDIAN
      #else
         #error "Unknown __BYTE_ORDER"
      #endif // __BYTE_ORDER
   #endif // !defined(L_ENDIAN) && !defined(B_ENDIAN)
#endif // __GNUC__

#ifdef __IAR_SYSTEMS_ICC__
   #define  PACKED_ATTR 
   #define  PACKED_START _Pragma("pack(1)")
   #define  PACKED_STOP  _Pragma("pack()")
   #if !defined(L_ENDIAN) && !defined(B_ENDIAN)
      #define L_ENDIAN
   #endif // !defined(L_ENDIAN) && !defined(B_ENDIAN)
#endif // __IAR_SYSTEMS_ICC__

#ifdef ARMCC
   #define  PACKED_ATTR 
   #define  PACKED_START _Pragma("pack(1)")
   #define  PACKED_STOP  _Pragma("pack()")
   #if !defined(L_ENDIAN) && !defined(B_ENDIAN)
      #define L_ENDIAN
   #endif // !defined(L_ENDIAN) && !defined(B_ENDIAN)
#endif // ARMCC

#ifdef WIN32
   #define  PACKED_ATTR
   #define  PACKED_START  __pragma(pack(1))
   #define  PACKED_STOP   __pragma(pack())
   #if !defined(L_ENDIAN) && !defined(B_ENDIAN)
      #define L_ENDIAN
   #endif // !defined(L_ENDIAN) && !defined(B_ENDIAN)
#endif // WIN32


//PACKED_START /* all structures that follow are packed */

//PACKED_STOP /* back to default packing */

#endif

