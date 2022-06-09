#ifndef _SOFTAPCLIENTFINDER_H_
#define _SOFTAPCLIENTFINDER_H_

#include <stdint.h>

#include <WiFi.h>
#include "esp_wifi.h"

#ifdef __cplusplus
extern "C" {
#endif

void SoftApClient_Check(void);
uint8_t SoftApClient_Count(void);
ip4_addr_t* SoftApClient_First(void);

#ifdef __cplusplus
}
#endif

#endif
