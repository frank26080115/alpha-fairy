#ifndef _ALPHAFAIRY_NETMGR_H_
#define _ALPHAFAIRY_NETMGR_H_

#include <stdint.h>

#include <WiFi.h>
#include "esp_wifi.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*got_client_t)(ip4_addr_t);

void NetMgr_begin(char* ssid, char* password, got_client_t cb);
void NetMgr_task(void);

#ifdef __cplusplus
}
#endif

#endif
