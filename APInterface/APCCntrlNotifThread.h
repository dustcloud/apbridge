#pragma once
#include "APCConnector.h"
#include <boost/bind.hpp>
#include <string>
#include <deque>
#include <boost/thread.hpp>
#include <boost/intrusive/list.hpp>


class CAPCCtrlNotifThread  : public IAPCConnectorNotif
{
public:
   CAPCCtrlNotifThread ();
   virtual ~CAPCCtrlNotifThread ();

   void        init(IAPCConnectorNotif * pExtrnAPCCntrl);
   void        clear();
   apc_error_t start();
   void        stop();
   void        threadFun();

   // IAPCCtrl interface
   virtual void apcStarted(CAPCConnector::ptr pAPC);
   virtual void apcConnected(CAPCConnector::ptr pAPC, param_connected_s& param);
   virtual void apcDisconnected(CAPCConnector::ptr pAPC, param_disconnected_s& param);
   virtual void messageReceived(param_received_s& param, const uint8_t * pPayload, uint16_t size);

protected:
   enum apc_notiftype_t {  // "#IGNORE"
      APC_NA,
      APC_START,
      APC_CONNECT,
      APC_DISCONNECT,
      APC_MSGRCVD,
      APC_TERMINATE,
   };

   class apcnotif_t: public boost::intrusive::list_base_hook<>
   {
   public:
      apcnotif_t() : m_type(APC_NA), m_apc(nullptr), m_payloadSize(0) {;}
      apc_notiftype_t      m_type;
      CAPCConnector::ptr   m_apc;
      std::vector<uint8_t> m_payload;
      uint32_t             m_payloadSize;
      union {
         IAPCConnectorNotif::param_connected_s    m_connect;
         IAPCConnectorNotif::param_disconnected_s m_disconnect;
         IAPCConnectorNotif::param_received_s     m_msg;
      }                                           m_param;

   };

   struct apcnotif_disposer_s {   // Disposer for intrusive lists
      void operator() (apcnotif_t * p) {delete p;}
   };


   typedef boost::intrusive::list<apcnotif_t> apcnotiflist_t;

   apcnotiflist_t             m_apcNotifQ;
   apcnotiflist_t             m_freeApcNotif;
   boost::mutex               m_lock;
   boost::condition_variable  m_signal;
   boost::mutex               m_allocLock;

   boost::thread           *  m_pThread;
   IAPCConnectorNotif      *  m_pExtrnAPCNotif;
   bool                       m_isWork;

   void                       sendStopSignal_p();
   void                       insertNotif_p(apcnotif_t * pNotif);
   apcnotif_t              *  newNotif_p();
   void                       freeNotif_p(apcnotif_t * pNotif);
};

