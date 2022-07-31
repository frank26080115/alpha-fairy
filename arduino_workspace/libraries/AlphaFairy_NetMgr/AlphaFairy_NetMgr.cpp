#include "AlphaFairy_NetMgr.h"

#include <WiFi.h>
#include "esp_wifi.h"

static got_client_t callback;

static uint32_t last_client = 0;

static char* NetMgr_ssid = NULL;
static char* NetMgr_password = NULL;

void NetMgr_begin(char* ssid, char* password, got_client_t cb)
{
    last_client = 0;

    NetMgr_ssid     = (char*)malloc(strlen(ssid    ) + 1);
    NetMgr_password = (char*)malloc(strlen(password) + 1);
    strcpy(NetMgr_ssid    , ssid);
    strcpy(NetMgr_password, password);

    WiFi.softAP(ssid, password);
    callback = cb;
}

void NetMgr_task()
{
    wifi_sta_list_t          wifi_sta_list;
    tcpip_adapter_sta_list_t adapter_sta_list;
    memset(&wifi_sta_list   , 0, sizeof(wifi_sta_list   ));
    memset(&adapter_sta_list, 0, sizeof(adapter_sta_list));
    esp_wifi_ap_get_sta_list  (&wifi_sta_list);
    tcpip_adapter_get_sta_list(&wifi_sta_list, &adapter_sta_list);

    int i;
    for (i = 0; i < adapter_sta_list.num; i++) {
        if (adapter_sta_list.sta[i].ip.addr != 0) {
            if (last_client != adapter_sta_list.sta[i].ip.addr) {
                if (callback != NULL) {
                    callback(adapter_sta_list.sta[i].ip.addr);
                }
            }
            last_client = adapter_sta_list.sta[i].ip.addr;
        }
    }
}

void NetMgr_reset()
{
    last_client = 0;
}

void NetMgr_reboot()
{
    last_client = 0;
    esp_wifi_disconnect();
    esp_wifi_stop();
    esp_wifi_deinit();
    WiFi.softAP(NetMgr_ssid, NetMgr_password);
}

char* NetMgr_getSSID() {
    return NetMgr_ssid;
}

char* NetMgr_getPassword() {
    return NetMgr_password;
}

uint32_t NetMgr_getClient() {
    return last_client;
}
