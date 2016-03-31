#pragma once
#include "StatDelays.h"
#include "common.pb.h"

void convertDelayStat(const statdelays_s& stat, common::DelayStat * pRpcStat);
