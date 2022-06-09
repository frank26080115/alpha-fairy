#include <WiFi.h>
#include "esp_wifi.h"

static got_client_t callback;

static int last_client_cnt = 0;

void NetMgr_begin(char* ssid, char* password, got_client_t cb)
{
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

    int client_cnt = adapter_sta_list.num;
    if (client_cnt == 1 && last_client_cnt <= 0) {
        if (callback != NULL) {
            callback(adapter_sta_list.sta[0].ip);
        }
    }
    last_client_cnt = client_cnt;
}
