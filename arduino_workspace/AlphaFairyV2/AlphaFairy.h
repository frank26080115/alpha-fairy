#ifndef _ALPHAFAIRY_H_
#define _ALPHAFAIRY_H_

#include <stdint.h>
#include <stdbool.h>

#include "alfy_conf.h"
#include "alfy_types.h"
#include "alfy_defs.h"

#include <M5StickCPlus.h>
#include <M5DisplayExt.h>
#include <FS.h>
#include <SPIFFS.h>
#include <PtpIpCamera.h>
#include <PtpIpSonyAlphaCamera.h>
#include <SonyHttpCamera.h>
#include <AlphaFairy_NetMgr.h>
#include <AlphaFairyImu.h>
#include <FairyKeyboard.h>
#include <FairyEncoder.h>
#include <SerialCmdLine.h>
#include <SonyCameraInfraredRemote.h>
#include <DebuggingSerial.h>

extern PtpIpSonyAlphaCamera ptpcam;
extern SonyHttpCamera       httpcam;
extern DebuggingSerial      dbg_ser;
extern SerialCmdLine        cmdline;
extern configsettings_t     config_settings;

extern menustate_t* curmenu;

extern bool app_poll(void);

#endif
