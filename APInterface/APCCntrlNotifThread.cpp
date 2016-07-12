#include "APCCntrlNotifThread.h"
using namespace std;

#ifdef WIN32
   #pragma warning( disable : 4996 )
#endif

CAPCCtrlNotifThread ::CAPCCtrlNotifThread() : m_pThread(NULL), m_pExtrnAPCNotif(NULL), m_isWork(false) {;}

CAPCCtrlNotifThread ::~CAPCCtrlNotifThread ()
{
}

void    CAPCCtrlNotifThread::init(IAPCConnectorNotif * pExtrnAPCNotif)
{
   BOOST_ASSERT(m_pThread == NULL);
   m_pExtrnAPCNotif = pExtrnAPCNotif;
   m_isWork = false;
}

void  CAPCCtrlNotifThread::clear()
{
}

apc_error_t  CAPCCtrlNotifThread::start()
{
   BOOST_ASSERT(m_pThread == NULL);
   m_pThread = new boost::thread(boost::bind(&CAPCCtrlNotifThread::threadFun, this));
   for(int i=1; !m_isWork && i < 1000; i++)  // Wait up to 10 sec (1000 * 10 msec)
      boost::this_thread::sleep_for(msec_t(10));
   if (!m_isWork)
      return APC_ERR_STATE;
   return APC_OK;
}

void CAPCCtrlNotifThread::stop()
{
   if (m_pThread != NULL) {
      sendStopSignal_p();
      m_pThread->join();
      delete m_pThread;
      m_pThread = NULL;
   }
   m_apcNotifQ.clear_and_dispose(apcnotif_disposer_s());
   m_freeApcNotif.clear_and_dispose(apcnotif_disposer_s());
}

CAPCCtrlNotifThread::apcnotif_t *  CAPCCtrlNotifThread::newNotif_p()
{
   boost::unique_lock<boost::mutex> lock(m_allocLock);
   apcnotif_t * res;
   if (m_freeApcNotif.empty()) {
      res = new apcnotif_t;
   } else {
      res = &m_freeApcNotif.back();
      m_freeApcNotif.pop_back();
   }
   return res;
}

void CAPCCtrlNotifThread::freeNotif_p(apcnotif_t * pNotif)
{
   boost::unique_lock<boost::mutex> lock(m_allocLock);
   pNotif->m_apc = nullptr;
   pNotif->m_type = APC_NA;
   m_freeApcNotif.push_back(*pNotif);
}

void    CAPCCtrlNotifThread::threadFun()
{
   apcnotif_t * pNotif;
   m_isWork = true;

   for(;;) {
      for(;;) {
         boost::unique_lock<boost::mutex> lock(m_lock);
         if (m_apcNotifQ.empty())
            m_signal.wait(lock);
         if (!m_apcNotifQ.empty()) {
            pNotif = &m_apcNotifQ.back();
            m_apcNotifQ.pop_back();

            if (pNotif->m_type == APC_TERMINATE) {
               m_isWork = false;       // Stop 
               freeNotif_p(pNotif);
               pNotif = nullptr;
            }
            break;
         }
      }

      if (!m_isWork)
         break;

      if (m_pExtrnAPCNotif != nullptr && pNotif != nullptr) {

         switch(pNotif->m_type) {
         case APC_START:
            m_pExtrnAPCNotif->apcStarted(pNotif->m_apc); 
            break;
         case APC_CONNECT:
            m_pExtrnAPCNotif->apcConnected(pNotif->m_apc, pNotif->m_param.m_connect);
            break;
         case APC_DISCONNECT:
            m_pExtrnAPCNotif->apcDisconnected(pNotif->m_apc, pNotif->m_param.m_disconnect);
            break;
         case APC_MSGRCVD:
            m_pExtrnAPCNotif->messageReceived(pNotif->m_param.m_msg, pNotif->m_payload.data(), pNotif->m_payloadSize);
            break;

         default:
            break;
         }
      }

      freeNotif_p(pNotif);
   }
}

void CAPCCtrlNotifThread::insertNotif_p(apcnotif_t * pNotif)
{
   bool isEmpty = m_apcNotifQ.empty();
   m_apcNotifQ.push_front(*pNotif);
   if (isEmpty)
      m_signal.notify_all();
}

void CAPCCtrlNotifThread::apcStarted(CAPCConnector::ptr pAPC)
{
   boost::unique_lock<boost::mutex> lock(m_lock);
   
   if (!m_isWork)
      return;
   
   apcnotif_t * pNotif = newNotif_p();
   pNotif->m_type = APC_START;
   pNotif->m_apc  = pAPC;
   insertNotif_p(pNotif);
}

void CAPCCtrlNotifThread::apcConnected(CAPCConnector::ptr pAPC, param_connected_s& param)
{
   boost::unique_lock<boost::mutex> lock(m_lock);
   if (!m_isWork)
      return;

   apcnotif_t * pNotif = newNotif_p();
   pNotif->m_type = APC_CONNECT;
   pNotif->m_apc  = pAPC;
   pNotif->m_param.m_connect = param;
   insertNotif_p(pNotif);
}

void CAPCCtrlNotifThread::apcDisconnected(CAPCConnector::ptr pAPC, param_disconnected_s& param)
{
   boost::unique_lock<boost::mutex> lock(m_lock);
   if (!m_isWork)
      return;
   apcnotif_t * pNotif = newNotif_p();
   pNotif->m_type = APC_DISCONNECT;
   pNotif->m_apc  = pAPC;
   pNotif->m_param.m_disconnect = param;
   insertNotif_p(pNotif);
}

void CAPCCtrlNotifThread::messageReceived(param_received_s& param, const uint8_t * pPayload, uint16_t size)
{
   boost::unique_lock<boost::mutex> lock(m_lock);
   if (!m_isWork)
      return;
   apcnotif_t * pNotif = newNotif_p();
   pNotif->m_type = APC_MSGRCVD;
   pNotif->m_param.m_msg = param;
   if (size > 0) {
      if (size > pNotif->m_payload.size())
         pNotif->m_payload.resize((size & ~0x1F) + 0x20);   // multiple 32
      memcpy(pNotif->m_payload.data(), pPayload, size);
   }
   pNotif->m_payloadSize = size;
   insertNotif_p(pNotif);
}

void CAPCCtrlNotifThread::sendStopSignal_p()
{
   boost::unique_lock<boost::mutex> lock(m_lock);
   apcnotif_t * pNotif = newNotif_p();
   pNotif->m_type = APC_TERMINATE;
   m_apcNotifQ.push_back(*pNotif);
   m_signal.notify_all();
}


