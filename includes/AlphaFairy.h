#ifndef _ALPHAFAIRY_H_
#define _ALPHAFAIRY_H_

#include <stdint.h>
#include <stdbool.h>

#include "alfy_conf.h"
#include "alfy_types.h"
#include "alfy_defs.h"
#include "sprites.h"

#include <M5Unified.h>
#include <M5DisplayExt.h>
#include <SpriteMgr.h>
#include <FS.h>
#include <LittleFS.h>
#include <PtpIpCamera.h>
#include <PtpIpSonyAlphaCamera.h>
#include <SonyHttpCamera.h>
#include "AlphaFairyCamera.h"
#include <AlphaFairy_NetMgr.h>
#include <AlphaFairyImu.h>
#include <FairyKeyboard.h>
#include <FairyEncoder.h>
#include <SerialCmdLine.h>
#include <SonyCameraInfraredRemote.h>
#include <DebuggingSerial.h>

#ifdef ENABLE_BUILD_LEPTON
#include <Lepton.h>
#endif

extern PtpIpSonyAlphaCamera ptpcam;
extern SonyHttpCamera       httpcam;
extern AlphaFairyCamera     fairycam;
extern SerialCmdLine        cmdline;
extern configsettings_t     config_settings;
extern AlphaFairyImu        imu;
extern SpriteMgr* sprites;

extern
#ifdef DISABLE_ALL_MSG
    DebuggingSerialDisabled
#else
    DebuggingSerial
#endif
                            dbg_ser;

extern bool app_poll(void);

#define ALFY_FS LittleFS
static constexpr const char* ALFY_FS_LABEL = "littlefs";
static constexpr const char* ALFY_FS_BASE_PATH = "/littlefs";
static constexpr uint8_t ALFY_FS_MAX_OPEN_FILES = 5;

static inline uint8_t m5gfx_fromLegacyBrightness(int32_t brightness)
{
    if (brightness <= 0) {
        return 0;
    }
    if (brightness <= 12) {
        brightness = brightness < 5 ? 5 : brightness;
        return 16 + ((brightness - 5) * (255 - 16)) / (12 - 5);
    }
    return brightness > 255 ? 255 : brightness;
}

static inline void m5gfx_setBrightness(int32_t brightness)
{
    uint8_t mapped = m5gfx_fromLegacyBrightness(brightness);
    M5.Display.setBrightness(mapped);
    M5Lcd.setBrightness(mapped);
}

static inline void m5gfx_screenSwitch(bool state)
{
    m5gfx_setBrightness(state ? config_settings.lcd_brightness : 0);
}

static inline float m5power_getBatteryVoltage(void)
{
    int16_t mv = M5.Power.getBatteryVoltage();
    return mv < 0 ? -1.0f : (float)mv / 1000.0f;
}

static inline float m5power_getBatteryCurrent(void)
{
    return (float)M5.Power.getBatteryCurrent();
}

static inline float m5power_getVBusVoltage(void)
{
    int16_t mv = M5.Power.getVBUSVoltage();
    return mv < 0 ? -1.0f : (float)mv / 1000.0f;
}

static inline float m5power_getVBusCurrent(void)
{
    return -1.0f;
}

static inline uint8_t m5power_getButtonPress(void)
{
    return M5.Power.getKeyState();
}

static inline void m5power_powerOff(void)
{
    M5.Power.powerOff();
}

extern void app_waitAllRelease(void);
extern void app_waitAllReleaseConnecting(void);
extern void app_waitAllReleaseUnsupported(void);
extern void app_sleep(uint32_t x, bool forget_btns);

extern void settings_save(void);

extern void pwr_lcdUndim(void);
extern void pwr_sleepCheck(void);
extern void pwr_tick(bool);

extern bool btnSide_hasPressed(void);
extern bool btnBig_hasPressed(void);
extern bool btnPwr_hasPressed(void);
extern bool btnBoth_hasPressed(void);
extern bool btnAny_hasPressed(void);
extern bool btnSide_isPressed(void);
extern bool btnBig_isPressed(void);
extern bool btnPwr_isPressed(void);
extern bool btnBoth_isPressed(void);
extern bool btnAll_isPressed(void);
extern void btnSide_clrPressed(void);
extern void btnBig_clrPressed(void);
extern void btnPwr_clrPressed(void);
extern void btnBoth_clrPressed(void);
extern void btnAny_clrPressed(void);

#endif
