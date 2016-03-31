/*
 * Copyright (c) 2016, Linear Technology. All rights reserved.
 */

#pragma once
#include "common/common.h"

// retry interval for initialization actions, in milliseconds
const uint32_t RETRY_INTERVAL = 1000; 

const size_t INPUT_BUFFER_LEN = 256;

#define RESET_SIGNAL_TX  "TX"        // use "TX" line to reset AP
#define RESET_SIGNAL_DTR "DTR"       // use "DTR" line to reset AP
