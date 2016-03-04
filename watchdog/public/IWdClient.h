#pragma once
#include "IWdClientCommon.h"
#include "common/common.h"
#include "common/IChangeNodeState.h"
#include <string>
#include <memory>
#include <zmq.hpp>

/**
 * Watchdog Client Notification interface.
 */
class IWdClientNotif : public IWdSrvNotif
{
public:
   virtual ~IWdClientNotif(){;}

   /**
    * Sending Keep Alive unsuccessful 
    *
    * \param   err   Send error.
    */
   virtual void ka_error(wd_clnt_error_t err)  = 0; 

   /**
    * Watch point expired
    *
    * \param   wpName         Name of the watch point.
    * \param   lastFeedTime   Time of last feeding or start time.
    */
   virtual void wp_error(const char * wpName, mngr_time_t lastFeedTime)  = 0; 
};

class IWdClient 
{
public:

   /**
    * Watchdog Client notification callback handlers.
    */
   struct init_param_s {
      zmq::context_t * m_context;   ///< ZMQ context
      std::string      m_epWd;      ///< RPC endpoint of watchdog server
      std::string      m_epWdNotif; ///< RPC endpoint of watchdog  notification
      std::string      m_identity;  ///< ZMQ identify (can be empty)
      std::string      m_appName;   ///< Application name (as in watchdog INI file)
      uint32_t         m_kaTimeout; ///< Keep Alive timeout (sec)
      IWdClientNotif * m_pNotif;    ///< Watchdog Client Notification interface
   };

   /**
    * Create connects object with watchdog.
    *
    * \param         initParam    The initialize parameter.
    * \param [out]   pRes         (Optional) If non-null, the result code.
    * \param         notRealConn  (Optional) Create object with 'fake' connection
    *                             
    * \return  null if it fails, else pointer to connect object.
    */
   static std::unique_ptr<IWdClient>  connect(const init_param_s& initParam, wd_clnt_error_t * pRes = NULL, 
                               bool notRealConn = false);
   
   virtual ~IWdClient () {;}

   /**
    * Disconnects from Watchdog server
    *
    * \param   severity The severity of disconnection.
    */
   virtual void disconnect(wd_clnt_severity_t severity) = 0;
   virtual void disconnect(IChangeNodeState::stopseverity_t stopres) = 0;

   /**
    * Node finishes promotion to master mode
    *
    * \return  WDCLNT_OK if successful otherwise error code.
    */
   virtual wd_clnt_error_t promoted() = 0;

   /**
    * Gets current node state.
    *
    * \return  The node state.
    */
   virtual wd_clnt_nodestate_t getNodeState() const = 0;

   /**
    * Create watch-point.
    *
    * \param        name   The name of watch-point.
    * \param [out]  pIdx    The index of created watch-point.
    *               
    * \return WDCLNT_OK if successful otherwise error code
    */
   virtual wd_clnt_error_t addWP(const char * name, wd_clnt_wpidx_t* pIdx) = 0;

   /**
    * Delete watch-point.
    *
    * \param   idx   The index of watch-point.
    *               
    * \return WDCLNT_OK if successful otherwise error code
    */
   virtual wd_clnt_error_t deleteWP(wd_clnt_wpidx_t idx) = 0;

   /**
    * Start watch-point.
    *
    * \param   idx            The index of watch-point.
    * \param   durationMsec   The duration msec.
    *               
    * \return WDCLNT_OK if successful otherwise error code
    */
   virtual wd_clnt_error_t startWP(wd_clnt_wpidx_t idx, uint32_t durationMsec) = 0;

   /**
    * Stop active watch-point.
    *
    * \param   idx   The index of watch-point.
    *               
    * \return WDCLNT_OK if successful otherwise error code
    */
   virtual wd_clnt_error_t stopWP(wd_clnt_wpidx_t idx) = 0;

   /**
    * Restart the timer of the specified watch-point.
    *
    * \param   idx   The index of watch-point.
    *               
    * \return WDCLNT_OK if successful otherwise error code
    */
   virtual wd_clnt_error_t feedWP(wd_clnt_wpidx_t idx) = 0;


   /**
    * Get Watch Points statistics
    *
    * \return  Watch Points statistics
    */
   virtual watchpoint_stat_list_t   getWPStat() const = 0;

   /**
    * Keep Alive control: enable or disable generation Keep Alive messages
    *
    * \param   isEnable true if generation Keep Alive messages is enable otherwise disabled
    */

   virtual void kaCntrl(bool isEnable) = 0;

   /**
    * Query if generation Keep Alive enabled.
    *
    * \return  true if enabled, false if not.
    */
   virtual bool isKAenabled() const = 0;
};
