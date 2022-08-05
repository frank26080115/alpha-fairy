#include "AlphaFairy_NetMgr.h"

#include <WiFi.h>
#include "esp_wifi.h"

static uint8_t wifi_op_mode = WIFIOPMODE_NONE;

static got_client_t callback;

static uint32_t last_client = 0;
static int last_sta_status = WL_IDLE_STATUS;
static char* NetMgr_ssid = NULL;
static char* NetMgr_password = NULL;

void NetMgr_taskAP(void);
void NetMgr_taskSTA(void);

void NetMgr_beginAP(char* ssid, char* password, got_client_t cb)
{
    if (wifi_op_mode != WIFIOPMODE_NONE)
    {
        esp_wifi_disconnect();
        esp_wifi_stop();
        esp_wifi_deinit();
    }

    wifi_op_mode = WIFIOPMODE_AP;
    NetMgr_reset();

    NetMgr_ssid     = (char*)malloc(strlen(ssid    ) + 1);
    NetMgr_password = (char*)malloc(strlen(password) + 1);
    strcpy(NetMgr_ssid    , ssid);
    strcpy(NetMgr_password, password);

    WiFi.mode(WIFI_AP);
    WiFi.softAP(ssid, password);
    callback = cb;
}

void NetMgr_beginSTA(char* ssid, char* password, got_client_t cb)
{
    if (wifi_op_mode != WIFIOPMODE_NONE)
    {
        esp_wifi_disconnect();
        esp_wifi_stop();
        esp_wifi_deinit();
    }

    wifi_op_mode = WIFIOPMODE_STA;
    NetMgr_reset();

    NetMgr_ssid     = (char*)malloc(strlen(ssid    ) + 1);
    NetMgr_password = (char*)malloc(strlen(password) + 1);
    strcpy(NetMgr_ssid    , ssid);
    strcpy(NetMgr_password, password);

    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    callback = cb;
}

void NetMgr_taskAP()
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

void NetMgr_taskSTA()
{
    int status = WiFi.status();
    if (status == WL_CONNECTED)
    {
        IPAddress gateway = WiFi.gatewayIP();
        if (gateway != 0)
        {
            if (status != last_sta_status)
            {
                last_sta_status = status;
                if (callback != NULL) {
                    callback((uint32_t)gateway);
                }
            }
        }
    }
    else
    {
        last_sta_status = status;
    }
}

void NetMgr_task()
{
    if (wifi_op_mode == WIFIOPMODE_AP) {
        NetMgr_taskAP();
    }
    else if (wifi_op_mode == WIFIOPMODE_STA) {
        NetMgr_taskSTA();
    }
}

uint8_t NetMgr_getOpMode()
{
    return wifi_op_mode;
}

void NetMgr_reset()
{
    last_client = 0;
    last_sta_status = WL_IDLE_STATUS;
}

void NetMgr_reboot()
{
    NetMgr_reset();
    esp_wifi_disconnect();
    esp_wifi_stop();
    esp_wifi_deinit();
    if (wifi_op_mode == WIFIOPMODE_AP) {
        WiFi.mode(WIFI_AP);
        WiFi.softAP(NetMgr_ssid, NetMgr_password);
    }
    else if (wifi_op_mode == WIFIOPMODE_STA) {
        WiFi.mode(WIFI_STA);
        WiFi.begin(NetMgr_ssid, NetMgr_password);
    }
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
