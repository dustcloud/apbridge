/*
 * Copyright (c) 2014, Linear Technology. All rights reserved.          
 */

#pragma once

#include "common.h"
#include "apc_common.h"
#ifdef GPSTEST
   #include "unit_tests/Testlibgpsmm.h"
#else
   #include "libgpsmm.h"
#endif
#include "common/ProcessInputArguments.h"

const char DEFAULT_FILE_NAME[]        = "apc";

#ifdef WIN32
	const char DEFAULT_DEV_API_PORT[] = "COM12"; // Can be changed to appropriate
	const char DEFAULT_DEV_RESET_PORT[] = "COM7"; // Can be changed to appropriate
#else
	const char DEFAULT_DEV_API_PORT[] = "/dev/ttyUSB4";
	const char DEFAULT_DEV_RESET_PORT[] = "/dev/ttyUSB2";
#endif

const uint32_t DEFAULT_BAUD_RATE = 921600;

const char APC_WORKER_ID[] = "apc-worker";
const char INTERNALCMD_WORKER_ID[] = "internal-worker";

const char DEFAULT_MNGR_HOST[] = "127.0.0.1";
const uint16_t DEFAULT_MNGR_PORT = 9100;

const uint16_t APM_DEFAULT_MAX_MSG_SIZE = 256; // Maximum buffer size of one message to AP
const uint16_t APM_DEFAULT_MAX_QUEUE_SIZE = 16; // Maximum number of messages can be queued to AP
const uint16_t APM_DEFAULT_HIGH_QUEUE_WATER_MARK = (uint16_t)(APM_DEFAULT_MAX_QUEUE_SIZE * 0.75);
const uint16_t APM_DEFAULT_LOW_QUEUE_WATER_MARK  = (uint16_t)(APM_DEFAULT_MAX_QUEUE_SIZE * 0.10);
const uint32_t APM_DEFAULT_PING_TIMEOUT = 1000; //in Milliseconds
const uint32_t APM_DEFAULT_RETRY_DELAY = 100; //in milliseconds, retry timer when receiving NACK from AP
const uint16_t APM_DEFAULT_MAX_RETRIES = 3;
const uint32_t APM_DEFAULT_RETRY_TIMEOUT = 800; // milliseconds
const uint32_t APM_DEFAULT_MAX_PACKET_AGE = 1500; // milliseconds, time to trigger TX_PAUSE to manager

const char GPSD_DEFAULT_HOST[] = "localhost";
const char GPSD_DEFAULT_PORT[] = DEFAULT_GPSD_PORT;
const uint32_t GPSD_DEFAULT_READ_TIMEOUT = 5000000; // microseconds
const uint16_t GPSD_DEFAULT_MIN_SATS_INUSE = 1;
const uint32_t GPS_DEFAULT_MAX_STABLE_TIME = 5; // seconds

const uint32_t APC_DEFAULT_MAX_QUEUE_SIZE = 20; // Default client queue size
const uint32_t APC_DEFAULT_KATIMEOUT = 2000; // Default APC Keep-Alive timeout, in milliseconds
const uint32_t APC_DEFAULT_FREEBUFFER_TIMEOUT = 2000; // Default APC time to wait for a free buffer, in milliseconds
const uint32_t APC_DEFAULT_RECONNECT_DELAY = 1000; // Default interval for APC to attempt reconnection, in milliseconds
const uint32_t APC_DEFAULT_DISCONNECT_TIMEOUT = 30000; // Default time to declare the connection is dead, in milliseconds

// Boot timeout (msec)
const uint32_t  RESET_BOOT_TIMEOUT             = 30000;
const uint32_t  DISCONNECT_BOOT_TIMEOUT_SHORT  = 4000;       
const uint32_t  DISCONNECT_BOOT_TIMEOUT_LONG   = 30000;


