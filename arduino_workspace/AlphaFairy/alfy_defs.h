#ifndef _ALFY_DEFS_H_
#define _ALFY_DEFS_H_

#include "alfy_conf.h"

#define CONFIGSETTINGS_MAGIC 0xDEADBEEF

enum
{
    MENUITEM_NONE = 0,
    // if a unique identifier is needed for a menu item, add it here
    MENUITEM_INTERVAL,
    MENUITEM_ASTRO,
    MENUITEM_TRIGGER,
    MENUITEM_LEPTON,
};

enum
{
  TXTFMT_NONE           =        0,
  TXTFMT_BASEMASK       =   0x00FF,
  TXTFMT_BOOL           =   0x0001,
  TXTFMT_TIME                     ,
  TXTFMT_BULB                     ,
  TXTFMT_TIMEMS                   ,
  TXTFMT_TIMELONG                 ,
  TXTFMT_SHUTTER                  ,
  TXTFMT_ISO                      ,
  TXTFMT_LCDBRITE                 ,
  TXTFMT_PROTOCOL                 ,
  TXTFMT_TRIGSRC                  ,
  TXTFMT_TRIGACT                  ,
  TXTFMT_PINCFG                   ,
  TXTFMT_TALLEYLITE               ,
  TXTFMT_BYTENS         =   0x0100,
  TXTFMT_AUTOCFG        =   0xFF00,
  TXTFMT_ALLCAPS        = 0x010000,
  TXTFMT_SMALL          = 0x020000,
  TXTFMT_ALLLOWER       = 0x040000,
  TXTFMT_ZEROOFF        = 0x080000,
  TXTFMT_NEGOFF         = 0x100000,
  TXTFMT_ZEROINF        = 0x200000,
  TXTFMT_NEGINF         = 0x400000,
  TXTFMT_DIVHUNDRED     = 0x800000,
};

enum
{
  BATTSTAT_NONE,
  BATTSTAT_LOW,
  BATTSTAT_CHARGING,
  BATTSTAT_CHARGING_LOW,
  BATTSTAT_FULL,
};

enum
{
  SPRITESHOLDER_FOCUSPULL = 0x01,
  SPRITESHOLDER_INTERVAL  = 0x02,
};

enum
{
    AUTOCONNRES_NONE,
    AUTOCONNRES_QUIT,
    AUTOCONNRES_FOUND_EXISTING,
    AUTOCONNRES_FOUND_EXISTING_NEED_PASSWORD,
    AUTOCONNRES_FOUND_NEW,
};

enum
{
    WIFIPROMPTRET_CANCEL,
    WIFIPROMPTRET_DONE,
    WIFIPROMPTRET_EMPTY,
};

enum
{
    AUTOCONNSTS_NONE,
    AUTOCONNSTS_CONNECTED,
    AUTOCONNSTS_FAILED,
};

enum
{
    WAITGFX_NONE,
    WAITGFX_CONNECTING,
    WAITGFX_UNSUPPORTED,
};

enum
{
    ALLOWEDPROTOCOL_ALL   = 0,
    ALLOWEDPROTOCOL_PTP   = 1,
    ALLOWEDPROTOCOL_HTTP  = 2,
};

enum
{
    PINCFG_NONE = 0,
    #ifndef ENABLE_BUILD_LEPTON
    PINCFG_G0,      // front header, shared with Lepton SPI
    PINCFG_G25,     // shared with G36 and Lepton SPI
    #endif
    PINCFG_G26,     // front header
    PINCFG_G32,     // rear I2C
    PINCFG_G33,     // rear I2C
    #ifndef ENABLE_BUILD_LEPTON
    PINCFG_G36,     // front header, shared with G25 (and Lepton SPI)
    #endif
    PINCFG_END,
};

enum
{
    TRIGSRC_ALL = 0,
    TRIGSRC_MIC,
    TRIGSRC_EXINPUT,
    TRIGSRC_IMU,
    TRIGSRC_THERMAL,
};

enum
{
    TRIGACT_PHOTO = 0,
    TRIGACT_VIDEO,
    TRIGACT_INTERVAL,
};

enum
{
    MES_AUTO_MAX = 0,
    MES_AUTO_MIN,
    MES_CENTER,
    DISP_MODE_RGB = 0,
    DISP_MODE_GRAY,
    DISP_MODE_GOLDEN,
    DISP_MODE_RAINBOW,
    DISP_MODE_IRONBLACK,
};

enum
{
    LEPINIT_RST_1 = 0,
    LEPINIT_RST_2,
    LEPINIT_RST_3,
    LEPINIT_RST_4,
    LEPINIT_BEGIN,
    LEPINIT_SYNC,
    LEPINIT_CMD,
    LEPINIT_DONE,
    LEPINIT_FAIL,
};

enum
{
    THERMTRIG_OFF = 0,
    THERMTRIG_HOT,
    THERMTRIG_COLD,
};

enum
{
    INFOSCR_LANDSCAPE_WHITE,
    INFOSCR_LANDSCAPE_BLACK,
    INFOSCR_PORTRAIT_WHITE,
    INFOSCR_PORTRAIT_BLACK,
    INFOSCR_END,
};

enum
{
    QIKRMTBTN_IDLE,
    QIKRMTBTN_PRESSED_LOCKING_WAIT,
    QIKRMTBTN_PRESSED_UNLOCKING_WAIT,
};

enum
{
    QIKRMTIMU_FREE,
    QIKRMTIMU_FREE_TEMP,
    QIKRMTIMU_LOCKED,
};

enum
{
    QIKRMT_ROW_INFOSCR = 0x0F,
};

enum
{
    EDITITEM_SHUTTER,
    EDITITEM_APERTURE,
    EDITITEM_ISO,
    EDITITEM_EXPOCOMP,
    EDITITEM_END,
};

enum
{
    TALLYLITE_OFF,
    TALLYLITE_SCREEN,
    TALLYLITE_BOTH,
    TALLYLITE_LED,
};

#endif
