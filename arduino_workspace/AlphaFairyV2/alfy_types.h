#ifndef _ALFY_TYPES_H_
#define _ALFY_TYPES_H_

#include <stdint.h>
#include <stdbool.h>
#include "alfy_conf.h"
#include "alfy_defs.h"

typedef struct
{
  uint32_t magic;
  uint32_t len;

  int32_t wifi_profile;

  int32_t focus_pause_time_ms;
  int32_t shutter_press_time_ms;
  int32_t manual_focus_return;
  int32_t nine_point_dist;
  int32_t shutter_speed_step_cnt;
  int32_t shutter_step_time_ms;
  int32_t infrared_enabled;
  int32_t gpio_enabled;

  int32_t pwr_save_secs;
  int32_t lcd_dim_secs;
  int32_t wifi_pwr;
  int32_t led_enabled;
  int32_t lcd_brightness;

  int32_t intv_bulb;
  int32_t intv_intval;
  int32_t intv_delay;
  int32_t intv_limit;

  int32_t astro_bulb;
  int32_t astro_pause;
  //int32_t astro_delay;
  //int32_t astro_limit;

  int32_t mictrig_level;
  int32_t mictrig_delay;

  int32_t fenc_multi;
  int32_t fenc_large;

  int32_t tallylite;

  uint32_t crc32;
}
configsettings_t;

enum
{
    SPEEDTYPE_NONE,
    SPEEDTYPE_PTP,
    SPEEDTYPE_HTTP,
};

typedef struct
{
    uint8_t  flags;    // use SPEEDTYPE_XXX
    uint32_t u32;      // use for values from PTP camera
    char     str[16];  // use for values from HTTP camera
}
speed_t;

typedef struct
{
    char ssid    [WIFI_STRING_LEN + 2];
    char password[WIFI_STRING_LEN + 2];
    uint8_t opmode;
    char guid[16 + 1];
}
wifiprofile_t;

#endif
