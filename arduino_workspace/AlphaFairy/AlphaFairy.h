#ifndef _ALPHAFAIRY_H_
#define _ALPHAFAIRY_H_

#include <stdint.h>
#include <stdbool.h>

#include "alfy_conf.h"
#include "alfy_types.h"
#include "alfy_defs.h"

#include <M5StickC.h>
#include <M5DisplayExt.h>
#include <PtpIpCamera.h>
#include <PtpIpSonyAlphaCamera.h>
#include <AlphaFairy_NetMgr.h>
#include <SerialCmdLine.h>
#include <SonyCameraInfraredRemote.h>

#if 1
#define DEBUG_PRINTF Serial.printf
#else
#define DEBUG_PRINTF(...)
#endif

extern configsettings_t config_settings;
extern bool app_poll(void);
extern SerialCmdLine cmdline;
extern uint8_t imu_angle;
extern bool imu_hasChange;

#endif
