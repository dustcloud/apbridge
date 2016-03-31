#pragma once
#include "common/common.h"
#include <vector>
#include <iostream>
#include <string>

enum wd_clnt_error_t
{
   WDCLNT_OK             = 0,    // "OK"
   WDCLNT_ERR_RPC        = 1,    // "General RPC error"
   WDCLNT_ERR_STATE      = 2,    // "Wrong state"
   WDCLNT_ERR_NOT_FOUND  = 3,    // "Not found"
   WDCLNT_ERR_VALIDATION = 4,    // "Wrong parameter value" 
};

enum wd_clnt_nodestate_t
{
   WDCLNT_NS_NA,
   WDCLNT_NS_SLAVE,
   WDCLNT_NS_MASTER,
   WDCLNT_NS_STANDALONE,
   WDCLNT_NS_ERROR,
};

enum wd_clnt_severity_t
{
   WDCLNT_NO_FAILURE,
   WDCLNT_NORMAL_FAILURE,
   WDCLNT_FATAL_FAILURE,
};

typedef uint32_t wd_clnt_wpidx_t;
const wd_clnt_wpidx_t WPIDX_EMPTY = 0;

ENUM2STR(wd_clnt_error_t);
ENUM2STR(wd_clnt_nodestate_t);
ENUM2STR(wd_clnt_severity_t);

struct watchpoint_stat_s
{
   std::string  m_name;
   usec_t       m_maxDuration;
   mngr_time_t  m_startTime;
};
typedef std::vector<watchpoint_stat_s>    watchpoint_stat_list_t;
typedef watchpoint_stat_list_t::iterator  watchpoint_stat_list_i;    
std::ostream& operator << (std::ostream& os, const watchpoint_stat_s& st);

class IWdSrvNotif {
public:
   virtual ~IWdSrvNotif() {;}

   /**
    * Request for promote node state to Master
    * After receiving this notification node must do promotion to Master 
    * mode send  message IWdClien::nodeState with new state to Watchdog server
    */
   virtual void promoteRequest() = 0; 

};