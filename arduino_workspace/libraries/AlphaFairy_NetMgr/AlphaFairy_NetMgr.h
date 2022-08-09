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

enum
{
    WIFICLIFLAG_NONE     = 0,
    WIFICLIFLAG_IS_CAM   = 0x01,
    WIFICLIFLAG_IS_HTTP  = 0x02,
    WIFICLIFLAG_IS_ERROR = 0x04,
};

void NetMgr_beginAP(char* ssid, char* password);
void NetMgr_beginSTA(char* ssid, char* password);
void NetMgr_task(void);
void NetMgr_reset(void);
void NetMgr_reboot(void);
uint8_t NetMgr_getOpMode(void);
char* NetMgr_getSSID(void);
char* NetMgr_getPassword(void);

void NetMgr_regCallback(void(*cb)(void));

uint32_t NetMgr_getConnectableClient(void);
void NetMgr_markClientCamera(uint32_t ip);
void NetMgr_markClientHttp(uint32_t ip);
void NetMgr_markClientError(uint32_t ip);
void NetMgr_markClientDisconnect(uint32_t ip);
bool NetMgr_shouldReportError(void);

#ifdef __cplusplus
}
#endif

#endif
