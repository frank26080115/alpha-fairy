#include "AlphaFairy_NetMgr.h"

#include <WiFi.h>
#include "esp_wifi.h"

static got_client_t callback;

static uint32_t last_client = 0;

void NetMgr_begin(char* ssid, char* password, got_client_t cb)
{
    last_client = 0;
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
