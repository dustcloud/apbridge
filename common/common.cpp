#include "common.h"
#include "FmtOutput.h"
#include <string.h>
#include <cstdlib>
#include <sstream>
#include <iomanip>
#include <string>
#include <boost/filesystem.hpp>
#include <boost/archive/iterators/base64_from_binary.hpp>
#include <boost/archive/iterators/transform_width.hpp>

using namespace std;

#ifndef _WIN32
   #include <netdb.h>
#endif

extern void convertToLocaltime(time_t * pTime, tm * pLocalTime, int32_t * pTZ);

// Convert mngr_time_t to mngr_utc
sys_time_t time_mngr2sys(mngr_time_t t)
{
   return SYSTIME_NOW() - TO_USEC(TIME_NOW() - t);
}

mngr_time_t time_sys2mngr(sys_time_t t)
{
   return TIME_NOW() - TO_USEC(SYSTIME_NOW() - t);
}

// Print current time IFmtOutput
void printCurTime(IFmtOutput& fo)
{
   printTime(fo, TIME_NOW());
}

void printTime(IFmtOutput& fo, const mngr_time_t& time)
{
   msec_t msec = TO_MSEC(time.time_since_epoch());
   time_t sec = static_cast<time_t>(boost::chrono::duration_cast<boost::chrono::seconds>(msec).count());
   tm     locTime;
   convertToLocaltime(&sec, &locTime, NULL);
   fo.printf("%2d:%02d:%02d.%03d", locTime.tm_hour, locTime.tm_min, locTime.tm_sec, msec.count() % 1000);
}

// Print buffer as HEX to IFmtOutput
void printHex(IFmtOutput& fo, const uint8_t * ar, uint32_t len, char div)
{
   for(uint32_t i=0; i<len; i++) {
      if (i)
         fo.printf("%c", div);
      fo.printf("%02x", ar[i]);
   }
}

ostream& operator << (ostream& os, const mngr_time_t& time)
{
   CFmtBuffer<15> buf;
   printTime(buf, time);
   os << (const char *)buf;
   return os;
}

std::string toString(const mngr_time_t& time)
{
   CFmtString buf;
   printTime(buf, time);
   return buf.str();
}

std::ostream& operator << (std::ostream& os, const sys_time_t& utc)
{
   os << toString(utc);
   return os;
}

std::string toString(const sys_time_t& utc)
{
   time_t    t = boost::chrono::system_clock::to_time_t(utc);
   tm        locTime;

   convertToLocaltime(&t, &locTime, NULL);

   ostringstream os;
   os.fill('0');
   os << (locTime.tm_year+1900) << "-" << setw(2) << (locTime.tm_mon + 1) << "-" << setw(2) << locTime.tm_mday << " "
      << setw(2) << locTime.tm_hour << ":" << setw(2) << locTime.tm_min << ":" << setw(2) << locTime.tm_sec << "." 
      << setw(3) << (TO_MSEC(utc.time_since_epoch()).count() % 1000);
   return os.str();
}

std::string toStringISO(const sys_time_t& utc)
{
   time_t    t = boost::chrono::system_clock::to_time_t(utc);
   tm        locTime;
   int32_t   tz;
   string    sig = "+";

   convertToLocaltime(&t, &locTime, &tz);

   if (tz < 0) {
      sig = "-";
      tz = -tz;
   }
   
   ostringstream os;
   os.fill('0');
   os << (locTime.tm_year+1900) << "-" << setw(2) << (locTime.tm_mon + 1) << "-" << setw(2) << locTime.tm_mday << "T"
      << setw(2) << locTime.tm_hour << ":" << setw(2) << locTime.tm_min << ":" << setw(2) << locTime.tm_sec << "." 
      << setw(3) << (TO_MSEC(utc.time_since_epoch()).count() % 1000)
      << sig << setw(2) << (tz /3600) << ":" << setw(2) << ((tz%3600)/60);

   return os.str();
}

// Convert HEX char to binary
uint8_t hexCh2bin(const char ch)
{
   if (ch >= '0' && ch <= '9')
      return ch - '0';
   if (ch >= 'a' && ch <= 'f')
      return ch - 'a' + 10;
   if (ch >= 'A' && ch <= 'F')
      return ch - 'A' + 10;
   return 0xFF;
}

// Convert HEX string to binary
bool hexStr2bin(const char * str, uint8_t * pRes, uint32_t size)
{
   const char * ch;
   uint32_t     idx = size-1;
   uint8_t      subIdx = 0, nn;
   bool         isDelimiter = false;
   
   memset(pRes, 0 , size);
   for (ch = str + strlen(str) - 1; ch >= str; ch--) {
      if ((*ch == '-' || *ch == ':')  && !isDelimiter) {
         isDelimiter = true;
         subIdx = 2;
      } else if ((nn = hexCh2bin(*ch)) != 0xFF) {
         isDelimiter = false;
         if (subIdx == 2) {
            if (idx == 0)
               return false;
            subIdx = 0;
            --idx;
         }
         if (subIdx == 1)
            nn <<= 4;
         pRes[idx] |= nn;
         subIdx++;
      } else {
         return false;
      }
   }
   return true;
}

// Convert hex string (two ASCII characters per byte) into binary data
int hexToBin(const char * str, uint8_t * pRes, size_t len)
{
   size_t idx    = 0;
   int   nn      = 0; 
   bool isOk     = true,
        wasDigit = false;
   memset(pRes, 0, len);
   while (isspace(*str)) str++;
   while (isOk) {
      char c       = *str++;
      bool isDigit = isxdigit(c)!=0;
      isOk = (wasDigit && (c==0 || isspace(c) || c=='-' || c==':')) || isDigit;
      wasDigit =  isDigit;
      if (isOk) {
         if (!isDigit || nn==2) {
            idx++; 
            nn = 0;
         }
         if (isDigit) {
            isOk = idx < len;
            if (isOk) {
               int numb = isdigit(c) ? c-'0' : tolower(c)-'a'+10;
               pRes[idx] = (pRes[idx] << 4) + numb;
               nn++;
            } 
         }
         else if (c!='-' && c!=':') {
            break;
         }
      } 
   }
   if (!isOk) {
      idx = 0;
   }
   return idx;
}

// Print buffer as HEX to ostream
std::ostream& operator << (std::ostream& os, const show_hex& p) {
   char * buf = new char[p.m_len*3];
   CFmtBufferWrap wb(buf, p.m_len*3);
   wb.printDump(p.m_ar, p.m_len, p.m_div);
   os << buf;
   delete[] buf;
   return os;
}

std::string dumpToString(const uint8_t * data, uint32_t len, char d)
{
   CFmtString wb;
   char       div[2] = {d, 0};
   wb.printDump(data, len, div);
   return wb.str();
}

uint32_t getNext(uint32_t val, int32_t delta, uint32_t maxVal)  
{
   bool isNegative = delta < 0;
   if (isNegative)
      delta = -delta;
   if ((uint32_t)delta > maxVal)
      delta %= maxVal;
   if (isNegative) {
      if ((uint32_t)delta > val)
         val += maxVal;
      val -= delta;
   } else {
      val += delta;
      if (val >= maxVal)
         val -= maxVal;
   }
   if (val > maxVal)
      val %= maxVal;

   return val;
}

bool    isMacBroadcast(const macaddr_t& mac) {
   static const macaddr_t bcastMac = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
   return isMacEqu(mac, bcastMac);
}

bool    isMacEmpty(const macaddr_t& mac) {
   static const macaddr_t emptyMac = {0};
   return isMacEqu(mac, emptyMac);
}

void    setMacBroadcast(macaddr_t& mac) {
   memset(&mac, 0xFF, sizeof(macaddr_t));
}

void    setMacEmpty(macaddr_t& mac) {
   memset(&mac, 0, sizeof(macaddr_t));
}

void    copyMac(macaddr_t& dst, const macaddr_t& src) {
   memcpy(&dst, &src, sizeof(macaddr_t));
}

bool    isMacEqu(const macaddr_t& mac1, const macaddr_t& mac2) {
   return memcmp(&mac1, &mac2, sizeof(macaddr_t)) == 0;
}


asn_t ntoh_asn(const uint8_t * netAsn, uint32_t netAsnSize)
{
   size_t asnSize = min(netAsnSize, sizeof(asn_t));
   asn_t result = 0;
   for (size_t i = 0; i < asnSize; i++) {
      result <<= 8; result += netAsn[i];
   }
   return result;
}

uint8_t * hton_asn(const asn_t hostAsn, uint8_t * netAsn, uint32_t netAsnSize)
{
   size_t asnSize = min(netAsnSize, sizeof(asn_t));
   asn_t netAsnVal = hostAsn;
   for (int i = asnSize-1; i >= 0; i--) {
      netAsn[i] = netAsnVal & 0xFF; netAsnVal >>= 8;
   }
   return netAsn;
}

void printAsn(IFmtOutput& fo, const asn_t asn)
{
   // asn is defined as int64_t
   fo.printf("%lld", asn);
}

// Get public IP Address from host.
int getIPAddress(string* sAddress, string* sHostName)
{
   char hostname[512];
   struct hostent* he;

   gethostname(hostname, sizeof(hostname));
   he = gethostbyname(hostname);

   if (he == NULL) {
      cerr << "Can not receive IP address" << endl;
      return -1;
   }

   *sAddress = inet_ntoa(*(struct in_addr*)he->h_addr);
   *sHostName = hostname;
   return 0;
}

// Get random [0..maxVal)
uint32_t getRand(uint32_t maxVal)
{
   if (maxVal < 2)
      return 0;
//TODO change to OpenSSL random generation 
#ifdef WIN32   
   return rand() % maxVal;   
#else
   return rand() / (RAND_MAX / maxVal + 1);
#endif
}

std::string getFullFileName(const std::string& strSubDir, const std::string& strFileName)
{
   const char * strRoot = std::getenv(EV_VOYAGER_HOME);
   boost::filesystem::path  rootPath = strRoot ? strRoot  : "", 
                            subPath = strSubDir, 
                            filePath= strFileName;

   if (boost::filesystem::absolute(filePath).string() == filePath.string()) {
      return filePath.string();
   }
   if (!strSubDir.empty() && boost::filesystem::absolute(subPath).string() == subPath.string()) {
      subPath /= filePath;
      return subPath.string();
   }
   rootPath /= subPath;
   rootPath /= filePath;
   return rootPath.string();
}


/**
 * Base64 encoder
 */
std::string base64Encoder(std::string data)
{
   using namespace boost::archive::iterators;

   typedef base64_from_binary<transform_width<std::string::const_iterator,6,8> >
      it_base64_t;
   const char BASE64_PAD = '=';
   
   // Encode
   unsigned int numPaddingChars = (3-data.length()%3)%3;
   std::string base64(it_base64_t(data.begin()), it_base64_t(data.end()));
   base64.append(numPaddingChars, BASE64_PAD);
   return base64;
}
