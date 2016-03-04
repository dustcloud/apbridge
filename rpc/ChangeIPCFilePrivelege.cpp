#include <sys/stat.h>
#include <unistd.h>
#include <grp.h>
#include <string>
#include <algorithm>
#include <sstream>
#include <string.h>
#include <errno.h>

using namespace std;

const string GR_NAME    = "dust";
const string IPC_PREFIX = "ipc://";

bool changeIPCFilePrivelege(const string& serverAddr, string * pError)
{
   struct group * pGrStat;
   struct stat   fileStat;
   string        fileName;
   int           res = 0;
   ostringstream os;
   
   fileName = serverAddr.substr(0, IPC_PREFIX.length());
   transform(fileName.begin(), fileName.end(), fileName.begin(), ::tolower);
   if (fileName != IPC_PREFIX)
      return true;
   
   fileName = serverAddr.substr(IPC_PREFIX.length());
   
   pGrStat = getgrnam(GR_NAME.c_str());   
   if (pGrStat == NULL) {
      os << "Can not find group '" << GR_NAME << "'";
      *pError = os.str();
      return false;
   }
   
   res = stat(fileName.c_str(), &fileStat);   
   if (res == 0) {
      fileStat.st_mode |= S_IRGRP | S_IWGRP | S_IXGRP;
      res = chmod (fileName.c_str(), fileStat.st_mode);
   }
   if (res == 0)  {
      res = chown (fileName.c_str(), fileStat.st_uid, pGrStat->gr_gid);
   }
   if (res != 0) {
      os << strerror(errno) << ". File: '" << fileName <<"'";
      *pError = os.str();
      return false;
   }
   
   return true;
}
