/*
 * Copyright (c) 2013, Linear Technology. All rights reserved.
 */

#pragma once

#include <zmq.hpp>
#include <boost/thread.hpp>
#include "rpc/public/zmqUtils.h"
#include "rpc/public/RpcClient.h"
#include "rpc/public/IBaseSubscriber.h"
#include "rpc/public/SerializerUtility.h"
#include "logevent.pb.h"

// TODO: namespace Logger

class CLogClient : public RpcClient
{
public:
   CLogClient(zmq::context_t* context, std::string logServerAddr,
              std::string identity, std::string authServerAddr,
              user_creds_t userCred)
      : RpcClient(context, logServerAddr, identity,
                  authServerAddr, userCred),
        m_context(context)
   {
      ;
   }

   virtual ~CLogClient() { ; }

   // TODO: provide methods for remote control log functions
   
private:
   zmq::context_t* m_context;
};


/**
 * The LogReceiver listens to log events from a log publishers.
 */
class CLogReceiver : public IBaseSubscriber
{
public:
   /**
    * Create the LogReceiver
    * \param context ZMQ context
    * \param publisherAddr Endpoint of the log publisher to connect to
    */
   CLogReceiver(zmq::context_t* ctx, const std::string publisherAddr)
      : IBaseSubscriber(ctx, publisherAddr)
   {
      ;
   }

   virtual ~CLogReceiver() {
   }

   /**
    * Handle the log event
    *
    * \param logevent Protobuf message containing the log event
    */
   virtual void handleLogEvent(const logevent::LogEvent& logmsg) = 0;

protected:

   virtual void handleEvent(zmqUtils::zmessage* logNotif)
   {
      std::string logger = logNotif->popstr();
      logevent::LogEvent logmsg;
      deserialize2pb(logmsg, *logNotif);
      handleLogEvent(logmsg);
   }
};

