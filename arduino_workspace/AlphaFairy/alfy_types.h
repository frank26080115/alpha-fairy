#ifndef _ALFY_TYPES_H_
#define _ALFY_TYPES_H_

#include <stdint.h>
#include <stdbool.h>
#include "alfy_conf.h"
#include "alfy_defs.h"

typedef struct
{
  const uint8_t id;
  const char fname[32];
  void (*func)(void*);
}
menuitem_t;

typedef struct
{
    menuitem_t* items;
    int8_t      id;
    int8_t      idx;
    int8_t      last_idx;
    uint8_t     cnt;
    uint8_t     flags;
}
menustate_t;

typedef struct
{
  uint32_t magic;
  uint32_t len;

  int32_t focus_pause_time_ms;
  int32_t shutter_press_time_ms;
  int32_t manual_focus_return;
  int32_t nine_point_dist;
  int32_t shutter_speed_step_cnt;
  int32_t shutter_step_time_ms;
  int32_t infrared_enabled;
  int32_t gpio_enabled;

  int32_t pwr_save_secs;
  int32_t led_enabled;

  int32_t intv_bulb;
  int32_t intv_intval;
  int32_t intv_delay;
  int32_t intv_limit;

  int32_t astro_bulb;
  int32_t astro_pause;
  //int32_t astro_delay;
  //int32_t astro_limit;

  uint32_t crc32;
}
configsettings_t;

typedef struct
{
  int32_t*      ptr_val;
  const int32_t val_max;
  const int32_t val_min;
  const int32_t step_size;
  const char    text[64];
  const uint8_t flags;
}
configitem_t;

#endif
