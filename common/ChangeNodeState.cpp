/*
 * Copyright (c) 2013, Linear Technology. All rights reserved.
 */
 

#include <signal.h>
#include <stdexcept>
#include <errno.h>
#include <vector>
#include <string>

#include "IChangeNodeState.h"

#include <boost/thread.hpp>

using namespace std;

static void handle_signal(int signum);

class CChangeNodeState : public IChangeNodeState 
{
public:
   static CChangeNodeState * m_changeNodeState;
   CChangeNodeState(int signum);
   virtual ~CChangeNodeState();
   virtual result_t waitSignal();
   virtual void     stop(const char * who, stopseverity_t stopSeverity = STOP_OK, uint32_t exitCode = 0);
   virtual void     promote();
   virtual uint32_t getExitCode() const { return m_exitCode; }
   virtual stopseverity_t getStopSeverity() const { return m_stopSeverity; }
   virtual const char * getWho() const { return m_who.c_str(); }
private:
   boost::mutex              m_mutex;
   boost::condition_variable m_signal;
   string                    m_who;
   bool                      m_isStop;
   bool                      m_isPromote;
   uint32_t                  m_exitCode;
   stopseverity_t            m_stopSeverity;
};

CChangeNodeState * CChangeNodeState::m_changeNodeState = NULL;

IChangeNodeState& IChangeNodeState::getChangeNodeStateObj(int signum)
{
   static CChangeNodeState shutdown(signum);
   return shutdown;
}

CChangeNodeState::CChangeNodeState(int signum) : 
   m_who("Watchdog Server"), m_isStop(false), m_isPromote(false), m_exitCode(0), m_stopSeverity(STOP_OK) 
{
   m_changeNodeState = this;
   if (signum == 0) 
      signum = SIGTERM;
   signal(signum, &handle_signal);
}

CChangeNodeState::~CChangeNodeState() { m_changeNodeState = NULL; }

/**
 * Wait until the condition variable signals that we're done
 */
CChangeNodeState::result_t CChangeNodeState::waitSignal()
{
   boost::unique_lock<boost::mutex> lock(m_mutex);
   while (!m_isStop && !m_isPromote) {
      m_signal.wait(lock);
      // TODO: timeout instead of notify from signal handler?
   }
   if (m_isStop) {
      m_isStop = false;
      return STOPPED;
   } else {
      m_isPromote = false;
      return MASTER;
   }
}

void CChangeNodeState::stop(const char * who, stopseverity_t stopSeverity, uint32_t exitCode)
{
   boost::unique_lock<boost::mutex> lock(m_mutex);
   m_stopSeverity = stopSeverity;
   m_exitCode = exitCode;
   m_who = who;
   m_isStop = true;
   m_signal.notify_one();
}

void CChangeNodeState::promote()
{
   boost::unique_lock<boost::mutex> lock(m_mutex);
   m_isPromote = true;
   m_signal.notify_one();
}

/**
 * Handle the signal and declare that the process should be stopped.
 */
static void handle_signal(int signum)
{
   // LATER: dispatch based on which signal we're handling

   if (CChangeNodeState::m_changeNodeState) {
      CChangeNodeState::m_changeNodeState->stop("Watchdog Server");
   }
   // TODO: is it safe to do condition variable ops in signal handler?
}

