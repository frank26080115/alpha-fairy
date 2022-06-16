#ifndef _ALPHAFAIRY_NETMGR_H_
#define _ALPHAFAIRY_NETMGR_H_

#include <stdint.h>

#include <WiFi.h>
#include "esp_wifi.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*got_client_t)(uint32_t);

void NetMgr_begin(char* ssid, char* password, got_client_t cb);
void NetMgr_task(void);
void NetMgr_reset(void);
char* NetMgr_getSSID(void);
char* NetMgr_getPassword(void);
uint32_t NetMgr_getClient(void);

#ifdef __cplusplus
}
#endif

#endif
