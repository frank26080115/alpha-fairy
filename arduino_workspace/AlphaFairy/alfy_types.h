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
  uint32_t magic;

  int32_t focus_pause_time_ms;
  int32_t shutter_press_time_ms;
  int32_t manual_focus_return;
  int32_t infrared_enabled;
  int32_t nine_point_dist;

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
}
configitem_t;

#endif
