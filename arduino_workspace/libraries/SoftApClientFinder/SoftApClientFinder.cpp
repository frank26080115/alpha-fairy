#include "SoftApClientFinder.h"

#include <string.h>

#include <WiFi.h>
#include "esp_wifi.h"

static wifi_sta_list_t          wifi_sta_list;
static tcpip_adapter_sta_list_t adapter_sta_list;
static ip4_addr_t               client_ip;

void SoftApClient_Check(void)
{
    memset(&wifi_sta_list   , 0, sizeof(wifi_sta_list   ));
    memset(&adapter_sta_list, 0, sizeof(adapter_sta_list));
    esp_wifi_ap_get_sta_list  (&wifi_sta_list);
    tcpip_adapter_get_sta_list(&wifi_sta_list, &adapter_sta_list);
}

uint8_t SoftApClient_Count()
{
    return adapter_sta_list.num;
}

ip4_addr_t* SoftApClient_First()
{
    if (adapter_sta_list.num <= 0) {
        return NULL;
    }
    memcpy(&client_ip, &(adapter_sta_list.sta[0].ip), sizeof(ip4_addr_t));
    return &client_ip;
}