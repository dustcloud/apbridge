/*
 * Copyright (c) 2013, Linear Technology. All rights reserved.
 */

#pragma once

#include <iostream>
#include "logging/Logger.h"

namespace rpclogger {
   // RPC loggers names
   extern const char * RPC_SERVER_NAME;
   extern const char * RPC_CLIENT_NAME; 
   extern const char * RPC_WORKER_NAME; 
   extern const char * RPC_WORKER_TPLGDB_NAME;
   extern const char * RPC_WORKER_MANAGER_NAME;
   extern const char * RPC_WORKER_INPUTEVENT_NAME;
   extern const char * RPC_WORKER_CONFIG_NAME;
   extern const char * RPC_WORKER_STATISTICS_NAME;
   extern const char * RPC_WORKER_AUTH_MNGR_NAME;
   extern const char * RPC_WORKER_AUTHENTICATON_NAME;
   extern const char * RPC_WORKER_NOTIF_GENERATOR_NAME;
   extern const char * RPC_WORKER_INTERNALCMD_NAME;
   extern const char * RPC_PUBLISHER_NAME;
   extern const char * RPC_PUBLISHER_OUTPUTCMD_NAME;
   extern const char * RPC_PUBLISHER_TPLGDB_NAME;
   extern const char * RPC_COLLECTOR_MNGR_NAME;
   extern const char * RPC_SHOWOBJ_NAME;
   extern const char * RPC_DBGOBJ_NAME;

   void              openLogger();
   void              closeLogger();
};
