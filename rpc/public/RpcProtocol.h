/*
 * Copyright (c) 2013, Linear Technology. All rights reserved.
 */
 
#pragma once

/**
 * \file RpcProtocol.h
 * RPC protocol declarations
 */

#include <stdint.h>

#include "RpcCommon.h"
#include "common.pb.h"

// This is the version of RpcClient we implement
#define RPC_CLIENT "RPCC01"

// This is the version of RpcWorker we implement
#define RPC_WORKER "RPCW01"

#define TIMESTAMP_WINDOW_SIZE 60 // In seconds


// RpcServer commands
enum RpcCommands
{
   RPC_REGISTER   = 1, /* Register */
   RPC_REQUEST    = 2, /* Request */
   RPC_REPLY      = 3, /* Reply */
   RPC_HEARTBEAT  = 4, /* Heartbeat */
   RPC_DISCONNECT = 5, /* Disconnect */
   RPC_NOTIFY     = 6, /* Notify */
};
ENUM2STR(RpcCommands);

/**
 * User credentials
 *
 * The user credentials structure is used by the RpcClient(s) to pass
 * authentication information when a command is signed.
 *
 * For the RpcClient, the userId and either password or sessionId must be
 * set. The RpcClient obtains the user's privilege from the AuthManager.
 * 
 * For the InternalRpcClient, the user credentials structure is just a
 * convenient way to store some information that's passed in a signed command.
 * 
 */
struct user_creds_t {
   std::string userId;               ///< User Id
   std::string password;             ///< Password
   std::string sessionId;            ///< Session Id
   common::UserPrivilege privilege;  ///< User privilege

   // TODO: the privilege does not need to be part of this structure, but if
   // it's removed the InternalRpcClient needs to store a privilege value to
   // send with its signature.
   
   user_creds_t(std::string aUser, common::UserPrivilege aPriv = common::NONE)
      : userId(aUser), privilege(aPriv)
   { ; }
};

/*
 * Client-Server protocol
 *
 * The Client and Server communicate with the Request, Reply and Notify commands.
 * A Client connects to the Server and sends a Request message containing:
 * - client identifier
 * - RPC_CLIENT
 * - service identifier
 * - command identifier
 * - serialized parameter structure
 */

typedef uint8_t rpc_cmdid_t; ///< RPC command identifier

struct timestampNonce_t {
   int64_t     timeS;
   std::string nonce;
   timestampNonce_t(int64_t newTimeS, std::string newNonce)
   : timeS(newTimeS), nonce(newNonce) {}
};

typedef std::vector<timestampNonce_t>  timestampNonceLog_t;
typedef timestampNonceLog_t::iterator  timestampNonceLog_i;

/**
 * Get the current timestamp with the resolution used in RPC calls
 * (seconds since the epoch)
 */
int64_t getRpcTimestamp();

/**
 * Return whether or not the  timestamp is within the window relative to refTime
 * (or current time in seconds since the epoch)
 */
bool inTimestampWindow(int64_t timestamp, int64_t refTime = 0);

