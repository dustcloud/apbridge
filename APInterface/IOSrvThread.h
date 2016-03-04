#pragma once
#include "common.h"
#include "APInterface/public/APCError.h"

#include <boost/asio.hpp>
#include <boost/thread.hpp>

/**
 * Run IO service thread
 */
class CIOSrvThread
{
public:
   CIOSrvThread(const char * logName = NULL, boost::asio::io_service * pIOSrv = nullptr);
   ~CIOSrvThread();
   void        setLogName(const char * logName);
   apc_error_t startIOSrvThread(boost::asio::io_service * pIOSrv);
   void        stopIOSrvThread();
   bool        isRunning() { return (m_pIOSrv != nullptr); }
private:
   boost::asio::io_service       * m_pIOSrv;
   boost::asio::deadline_timer   * m_liveTimer;
   //boost::asio::io_service::work * m_pWrk;
   boost::thread                 * m_pThread;
   boost::barrier                * m_pBarrier;
   std::string                     m_logName;
   void   threadFun_p();
   void   timerFun_p(const boost::system::error_code& error);
   void   startLiveTimer_p();
};
