#include "IOSrvThread.h"
#include "logging/Logger.h"

CIOSrvThread::CIOSrvThread(const char * logName, boost::asio::io_service * pIOSrv) : 
   m_pIOSrv(nullptr), m_liveTimer(nullptr), m_pThread(nullptr), m_pBarrier(nullptr) 
{
   setLogName(logName);
   if (pIOSrv != nullptr) 
      startIOSrvThread(pIOSrv);
}

CIOSrvThread::~CIOSrvThread() 
{ 
   stopIOSrvThread(); 
}

void CIOSrvThread::setLogName(const char * logName) 
{ 
   if (logName != NULL)
      m_logName = logName; 
   else
      m_logName.clear();
}

apc_error_t CIOSrvThread::startIOSrvThread(boost::asio::io_service * pIOSrv) 
{
   if (m_pIOSrv != nullptr || m_liveTimer != nullptr || m_pBarrier != nullptr || m_pThread !=nullptr)
      return APC_ERR_INIT;
   m_pIOSrv = pIOSrv;
   m_liveTimer = new boost::asio::deadline_timer(*m_pIOSrv);
   startLiveTimer_p();
   m_pBarrier = new boost::barrier(2);
   m_pThread  = new boost::thread(boost::bind(&CIOSrvThread::threadFun_p, this));
   m_pBarrier->wait();
   DUSTLOG_TRACE(m_logName, "Boost IO Service Thread starts");
   return APC_OK;
}

void CIOSrvThread::stopIOSrvThread() 
{
   if (m_pBarrier != nullptr) {
      delete m_pBarrier;
      m_pBarrier = nullptr;
   }
   if (m_liveTimer != nullptr) {
      m_liveTimer->cancel();
   }
   if (m_pThread != nullptr) {
      m_pThread->join();
      delete m_pThread;
      m_pThread = nullptr;
   }
   if (m_liveTimer != nullptr) {
      delete m_liveTimer;
      m_liveTimer = nullptr;
   }
   if (m_pIOSrv)
      DUSTLOG_TRACE(m_logName, "Boost IO Service Thread stops");
   m_pIOSrv = nullptr;
}

void   CIOSrvThread::threadFun_p() 
{
   m_pBarrier->wait();
   try {
      m_pIOSrv->run();
   } catch (std::exception& e) {
      if (m_logName.empty())
         DUSTLOG_ERROR(m_logName, "CIOSrvThread error: " << e.what());
   }
}

void CIOSrvThread::timerFun_p(const boost::system::error_code& error)
{
   if (error || m_liveTimer == nullptr)
      return;
   // Refresh timer
   startLiveTimer_p();
}

void CIOSrvThread::startLiveTimer_p()
{
   try {
      m_liveTimer->expires_from_now(boost::posix_time::hours(1));
      m_liveTimer->async_wait(boost::bind(&CIOSrvThread::timerFun_p, this, boost::asio::placeholders::error));
   } catch(std::exception& e) {
      if (m_logName.empty())
         DUSTLOG_ERROR(m_logName, "Live Timer error: " << e.what());
   }
}

