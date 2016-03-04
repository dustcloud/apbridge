#pragma once
#include "common.h"

/**
 * Change state of node to Master or Stop.
 */
class IChangeNodeState 
{
public:
   enum result_t // "#IChangeNodeState"
   {
      NA,
      STOPPED,
      MASTER,
   };

   enum stopseverity_t // "#IChangeNodeState"
   {
      STOP_OK,
      STOP_ERROR,
      STOP_FATAL_ERROR,
   };

   /**
    * Gets object.
    *
    * \param   signum   (Optional) the shutdown signal from external process
    *
    * \return  The shutdown object.
    */
   static IChangeNodeState& getChangeNodeStateObj(int signum = 0);

   virtual ~IChangeNodeState(){;}

   /**
    * Wait of change state signal
    * 
    * \return new state (STOP or MASTER)
    */
   virtual result_t  waitSignal() = 0;

   /**
    * Shutdown process.
    *
    * \param   who           name of who stop process
    * \param   stopSeverity  (Optional) the stop severity.
    * \param   exitCode      (Optional) The exit code.
    */
   virtual void     stop(const char * who, stopseverity_t stopSeverity = STOP_OK, uint32_t exitCode = 0) = 0;

   /**
    * Promotes this node state to master.
    */
   virtual void     promote() = 0;
   
   /**
    * Gets exit code set by 'stop' command
    */
   virtual uint32_t getExitCode() const = 0;

   /**
    * Gets the  severity set by 'stop' command
    */
   virtual stopseverity_t getStopSeverity() const = 0;

   virtual const char * getWho() const = 0;
};
   
