#ifndef _INCL_FMTOUTPUT_H_
#define _INCL_FMTOUTPUT_H_

#ifdef MEMORY_LEAK
   #include "vld.h"
#endif

/*
 * Copyright (c) 1999-2004 by Eugene B. Surovegin <esl@ebshome.net>
 * Copyright (c) 2007 by Sergey Zolotarev <s.zolotarev@gmail.com>
 *
 * Code ripped from Apache source. Apache extension %pI removed from vformatter.
 * Original esl_vformatter implementation by Eugene B. Surovegin
 * Refactored as C++ base class interface by Sergey Zolotarev
 *
 * $Source:
 * $Id:
 */

#include <stdarg.h>
#include <stdio.h>
#include <string>

class IFmtOutput {
public:
   // Format extensions are registered application-wide
   typedef char* (*FormatExtHandler)(const void* arg, char* buf, int* len);
   // This method is not thread safe, call it as earlier as possible
   // returns 0: OK, -1: error
   static bool registerFormatExt(char ext, FormatExtHandler handler);

   // names are given intentionally to match output to stdout
   // formatted output, variable arguments list. Return number of chars written
   int printf(const char* format, ...);
   int vprintf(const char* format, va_list ap);
   // use to safely output a string (may have % character)
   int puts(const char *str) { return printf("%s", str); }
   // Print dump
   void printDump(const unsigned char* array, int length, const char * separator,
                  const char* hdrStr=NULL, const char* endStr = NULL);
   // Print MAC address (8 bytes) in format XX-XX...-XX
   void printMac(const unsigned char* mac) { return printDump(mac, 8, "-"); }

   IFmtOutput() { }
   virtual ~IFmtOutput() { }
protected:
   /* Must be implemented by each derived class;
    * returns 0: OK, -1: error
    */
   virtual bool putCh(char c) = 0;
   virtual bool flush() = 0;
private:
   int esl_vformatter(const char *fmt, va_list ap);
   int printfNoFlush_p(const char* format, ...);
};

/********** Some derived classes *********
 * CFmtBuffer: safe alternative to sprintf()
 * Usage:
 *    CFmtBuffer<100> b;
 *    b.printf("Line %d", 1);
 *    b.printf("; ");
 *    b.printf("Line %d\n", 2);
 *    puts(b.str());
 * Output: 
 *    Line 1; Line 2
 */
class CFmtBufferWrap : public IFmtOutput
{
public:
   CFmtBufferWrap(char *buffer, int size) : bp(buffer), sz(size) { clear(); }
   virtual ~CFmtBufferWrap() { }
   const char *str() const { return bp; }
   operator const char *() const { return bp; }
   int length() const { return static_cast<int>(cp - bp); }
   // make sure it's always 0-terminated
   void clear() { cp = bp; ep = &bp[sz-1]; *bp = *ep = '\0'; }
protected:
   virtual bool putCh(char c)
   {
      if (cp >= ep)
         return false;
      *cp++ = c;
      return true;
   }
   virtual bool flush()
   {
      // terminate with 0 and stay in the same pos
      // for next printf
      if (cp < ep)
         *cp = '\0';
      return true;
   }
private:
   char *bp;
   int  sz;
   char *cp;
   char *ep;
};

template<int BUFSZ>
class CFmtBuffer : public CFmtBufferWrap
{
public:
   CFmtBuffer() : CFmtBufferWrap(buf, BUFSZ) { }
   virtual ~CFmtBuffer() { }
private:
   char buf[BUFSZ];
};

/* CFmtString: std::string with printf()
 * Usage:
 *    CFmtString s(fd);
 *    s.printf("Line %d; ", 1);
 *    s.printf("Line %d\n", 2);
 *    puts(s.str());
 * Output: 
 *    Line 1; Line 2
 */
class CFmtString : public IFmtOutput
{
public:
   CFmtString() { }
   virtual ~CFmtString() { }
   const std::string &str() const { return s; }
   operator const char *() const { return s.c_str(); }
   int length() const { return static_cast<int>(s.size()); }
   void clear() { s.clear(); }
protected:
   virtual bool putCh(char c) { s += c; return true; }
   virtual bool flush() { return true; }
private:
   std::string s;
};

/* CFmtFile: buffered fprintf() analog
 * Usage:
 *    FILE *fd = fopen("example.txt", "w");
 *    CFmtFile ff(fd);
 *    ff.printf("Line %d\n", 1);
 *    ff.printf("Line %d\n", 2);
 *    fclose(fd); 
 * Output: 
 *    Line 1
 *    Line 2
 */
class CFmtFile : public IFmtOutput
{
public:
   CFmtFile(FILE *_fd) : fd(_fd), cur(buf), end(&buf[BUFSZ-1]) { }
   virtual ~CFmtFile() { }
protected:
   virtual bool putCh(char c)
   {
      if (cur > end && !flush())
         return false;
      *cur++ = c;
      return true;
   }
   virtual bool flush()
   {
      unsigned int len = static_cast<unsigned int>(cur - buf);
      cur = buf;
      if (len > 0 && fwrite(buf, 1, len, fd) != len)
         return false;
      return true;
   }
private:
   enum { BUFSZ = 512 };
   FILE *fd;
   char buf[BUFSZ];
   char *cur;
   char *end;
};

#endif // _INCL_FMTOUTPUT_H_
