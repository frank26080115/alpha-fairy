#include "AlphaFairy_NetMgr.h"

#include <WiFi.h>
#include "esp_wifi.h"

typedef struct
{
    uint32_t ip;
    uint8_t  mac[6];
    uint32_t flags;
}
wificli_classifier_t;

static uint8_t wifi_op_mode = WIFIOPMODE_NONE;

static int last_sta_status = WL_IDLE_STATUS;
static char* NetMgr_ssid = NULL;
static char* NetMgr_password = NULL;

static uint32_t gateway_ip = 0;

static void (*callback)(void) = NULL;
static void (*disconnect_callback)(void) = NULL;

void NetMgr_taskAP(void);
void NetMgr_taskSTA(void);
void NetMgr_eventHandler(WiFiEvent_t event, WiFiEventInfo_t info);

int8_t NetMgr_findInTableSta(tcpip_adapter_sta_info_t* sta);
int8_t NetMgr_insertInTableSta(tcpip_adapter_sta_info_t* sta);
int8_t NetMgr_findInTableIp(uint32_t ip);
void NetMgr_tableSync(tcpip_adapter_sta_list_t* lst);

#define WIFICLI_TABLE_SIZE 4
wificli_classifier_t wificli_table[WIFICLI_TABLE_SIZE];

void NetMgr_beginAP(char* ssid, char* password)
{
    if (wifi_op_mode != WIFIOPMODE_NONE)
    {
        esp_wifi_disconnect();
    }

    wifi_op_mode = WIFIOPMODE_AP;
    NetMgr_reset();

    NetMgr_ssid     = (char*)malloc(strlen(ssid    ) + 1);
    NetMgr_password = (char*)malloc(strlen(password) + 1);
    strcpy(NetMgr_ssid    , ssid);
    strcpy(NetMgr_password, password);

    WiFi.mode(WIFI_AP);
    WiFi.softAP(ssid, password);
}

void NetMgr_beginSTA(char* ssid, char* password)
{
    if (wifi_op_mode != WIFIOPMODE_NONE)
    {
        esp_wifi_disconnect();
    }

    wifi_op_mode = WIFIOPMODE_STA;
    NetMgr_reset();

    NetMgr_ssid     = (char*)malloc(strlen(ssid    ) + 1);
    NetMgr_password = (char*)malloc(strlen(password) + 1);
    strcpy(NetMgr_ssid    , ssid);
    strcpy(NetMgr_password, password);

    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
}

void NetMgr_regCallback(void(*cb_evt)(void), void(*cb_disconn)(void))
{
    WiFi.onEvent(NetMgr_eventHandler, WiFiEvent_t::ARDUINO_EVENT_WIFI_AP_STAIPASSIGNED);
    WiFi.onEvent(NetMgr_eventHandler, WiFiEvent_t::ARDUINO_EVENT_WIFI_AP_STADISCONNECTED);
    WiFi.onEvent(NetMgr_eventHandler, WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_GOT_IP);
    WiFi.onEvent(NetMgr_eventHandler, WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_DISCONNECTED);
    WiFi.onEvent(NetMgr_eventHandler, WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_LOST_IP);
    callback = cb_evt;
    disconnect_callback = cb_disconn;
}

void NetMgr_taskAP()
{
    wifi_sta_list_t          wifi_sta_list;
    tcpip_adapter_sta_list_t adapter_sta_list;
    memset(&wifi_sta_list   , 0, sizeof(wifi_sta_list   ));
    memset(&adapter_sta_list, 0, sizeof(adapter_sta_list));
    esp_wifi_ap_get_sta_list  (&wifi_sta_list);
    tcpip_adapter_get_sta_list(&wifi_sta_list, &adapter_sta_list);

    NetMgr_tableSync(&adapter_sta_list);
    if (callback != NULL) {
        callback();
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
                gateway_ip = gateway;
                if (callback != NULL) {
                    callback();
                }
            }
        }
    }
    else
    {
        if (status != last_sta_status && last_sta_status == WL_CONNECTED && disconnect_callback != NULL) {
            disconnect_callback();
        }
        last_sta_status = status;
    }
}

void NetMgr_eventHandler(WiFiEvent_t event, WiFiEventInfo_t info)
{
    if (wifi_op_mode == WIFIOPMODE_AP) {
        NetMgr_taskAP();
    }
    else if (wifi_op_mode == WIFIOPMODE_STA) {
        if (event == ARDUINO_EVENT_WIFI_STA_DISCONNECTED && last_sta_status == WL_CONNECTED) {
            WiFi.reconnect();
        }
        NetMgr_taskSTA();
    }
}

void NetMgr_task()
{
    if (callback != NULL) {
        return;
    }
    if (wifi_op_mode == WIFIOPMODE_AP) {
        NetMgr_taskAP();
    }
    else if (wifi_op_mode == WIFIOPMODE_STA) {
        NetMgr_taskSTA();
    }
}

int8_t NetMgr_findInTableSta(tcpip_adapter_sta_info_t* sta)
{
    int i;
    for (i = 0; i < WIFICLI_TABLE_SIZE; i++) {
        wificli_classifier_t* t = &(wificli_table[i]);
        if (memcmp(t->mac, sta->mac, 6) == 0) {
            return i;
        }
    }
    return -1;
}

int8_t NetMgr_insertInTableSta(tcpip_adapter_sta_info_t* sta)
{
    int8_t i = NetMgr_findInTableSta(sta);
    wificli_classifier_t* t;
    if (i < 0) {
        for (i = 0; i < WIFICLI_TABLE_SIZE; i++) {
            t = &(wificli_table[i]);
            if (t->ip == 0) {
                t->ip = sta->ip.addr;
                memcpy(t->mac, sta->mac, 6);
                t->flags = 0;
                return i;
            }
        }
    }
    else {
        t = &(wificli_table[i]);
        t->ip = sta->ip.addr;
        memcpy(t->mac, sta->mac, 6);
        return i;
    }
    return -1;
}

int8_t NetMgr_findInTableIp(uint32_t ip)
{
    int i;
    for (i = 0; i < WIFICLI_TABLE_SIZE; i++) {
        wificli_classifier_t* t = &(wificli_table[i]);
        if (t->ip == ip) {
            return i;
        }
    }
    return -1;
}

void NetMgr_tableSync(tcpip_adapter_sta_list_t* lst)
{
    int i, j;
    for (i = 0; i < WIFICLI_TABLE_SIZE; i++) {
        wificli_classifier_t* t = &(wificli_table[i]);
        if (t->ip != 0) {
            bool found = false;
            for (j = 0; j < lst->num; j++) {
                if (lst->sta[j].ip.addr != 0 && memcmp(lst->sta[j].mac, t->mac, 6) == 0) {
                    t->ip = lst->sta[j].ip.addr;
                    found = true;
                }
            }
            if (found == false) {
                t->ip = 0;
            }
        }
    }
    for (j = 0; j < lst->num; j++) {
        if (lst->sta[j].ip.addr != 0) {
            NetMgr_insertInTableSta(&(lst->sta[j]));
        }
    }
}

uint32_t NetMgr_getConnectableClient(void)
{
    if (wifi_op_mode == WIFIOPMODE_STA) {
        return gateway_ip;
    }

    int i;
    for (i = 0; i < WIFICLI_TABLE_SIZE; i++) {
        wificli_classifier_t* t = &(wificli_table[i]);
        if (t->ip != 0 && t->flags == 0) {
            return t->ip;
        }
    }
    return 0;
}

void NetMgr_markClientCameraPtp(uint32_t ip)
{
    int8_t i = NetMgr_findInTableIp(ip);
    if (i < 0) {
        return;
    }
    wificli_classifier_t* t = &(wificli_table[i]);
    t->flags |= WIFICLIFLAG_IS_CAMPTP;
}

void NetMgr_markClientCameraHttp(uint32_t ip)
{
    int8_t i = NetMgr_findInTableIp(ip);
    if (i < 0) {
        return;
    }
    wificli_classifier_t* t = &(wificli_table[i]);
    t->flags |= WIFICLIFLAG_IS_CAMHTTP;
}

void NetMgr_markClientPhoneHttp(uint32_t ip)
{
    int8_t i = NetMgr_findInTableIp(ip);
    if (i < 0) {
        return;
    }
    wificli_classifier_t* t = &(wificli_table[i]);
    t->flags |= WIFICLIFLAG_IS_PHONEHTTP;
}

void NetMgr_markClientError(uint32_t ip)
{
    int8_t i = NetMgr_findInTableIp(ip);
    if (i < 0) {
        return;
    }
    wificli_classifier_t* t = &(wificli_table[i]);
    t->flags |= WIFICLIFLAG_IS_ERROR;
}

void NetMgr_markClientDisconnect(uint32_t ip)
{
    int8_t i = NetMgr_findInTableIp(ip);
    if (i < 0) {
        return;
    }
    wificli_classifier_t* t = &(wificli_table[i]);
    t->ip = 0;
    t->flags = WIFICLIFLAG_NONE;
}

bool NetMgr_shouldReportError(void)
{
    bool has_cli = false, rpt_err = true;
    int i;
    for (i = 0; i < WIFICLI_TABLE_SIZE; i++) {
        wificli_classifier_t* t = &(wificli_table[i]);
        if (t->ip != 0) {
            has_cli = true;
            if ((t->flags & WIFICLIFLAG_IS_ERROR) == 0 || (t->flags & WIFICLIFLAG_IS_PHONEHTTP) != 0 || (t->flags & WIFICLIFLAG_IS_CAMHTTP) != 0) {
                rpt_err = false;
            }
        }
    }
    if (has_cli == false) {
        return false;
    }
    return rpt_err;
}

uint8_t NetMgr_getOpMode()
{
    return wifi_op_mode;
}

void NetMgr_reset()
{
    last_sta_status = WL_IDLE_STATUS;
    gateway_ip = 0;
    memset((void*)wificli_table, 0, sizeof(wificli_classifier_t) * WIFICLI_TABLE_SIZE);
}

void NetMgr_reboot()
{
    NetMgr_reset();
    esp_wifi_disconnect();
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
