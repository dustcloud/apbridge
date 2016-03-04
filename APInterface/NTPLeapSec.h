#pragma once
#include "common/common.h"
#include <time.h>

/**
 * Get leap seconds information.
 *
 * \return  number of leap seconds.
 */
uint32_t ntpGetLeapSecs();

/**
 * Check current day
 *
 * \return TRUE if leap second occurs at midnight of the current day.
 */
bool  ntpIsLeapDay();

/**
 * Gets the get current UTC.
 *
 * \return  current UTC time.
 */
tm  ntpGetCurUTC();
