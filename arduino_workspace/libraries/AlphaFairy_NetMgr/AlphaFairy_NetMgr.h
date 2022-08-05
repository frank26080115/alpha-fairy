#ifndef _ALPHAFAIRY_NETMGR_H_
#define _ALPHAFAIRY_NETMGR_H_

#include <stdint.h>

#include <WiFi.h>
#include "esp_wifi.h"

#ifdef __cplusplus
extern "C" {
#endif

enum
{
    WIFIOPMODE_NONE = 0,
    WIFIOPMODE_AP   = 1,
    WIFIOPMODE_STA  = 2,
};

typedef void (*got_client_t)(uint32_t);

void NetMgr_beginAP(char* ssid, char* password, got_client_t cb);
void NetMgr_beginSTA(char* ssid, char* password, got_client_t cb);
void NetMgr_task(void);
void NetMgr_reset(void);
void NetMgr_reboot(void);
uint8_t NetMgr_getOpMode(void);
char* NetMgr_getSSID(void);
char* NetMgr_getPassword(void);
uint32_t NetMgr_getClient(void);

#ifdef __cplusplus
}
#endif

#endif
