#include "AlphaFairy.h"

void wifiprofile_getIdxFname(uint8_t idx, char* tgt)
{
    sprintf(tgt, "/wifipro_%u.txt", idx);
}

int wifiprofile_fileReadLine(File* f, char* tgt, int charlimit)
{
    int i = 0;
    if (f->available() <= 0) {
        return -1;
    }
    while (f->available() > 0 && i < charlimit - 1) {
        char c = f->read();
        if (c != '\r' && c != '\n' && c != '\0') {
            tgt[i] = c;
            i += 1;
            tgt[i] = 0;
        }
        else {
            tgt[i] = 0;
            if (i > 0) { // this trims the start of a line
                i += 1;
                break;
            }
        }
    }
    return i;
}

bool wifiprofile_getProfileRaw(uint8_t idx, char* ssid, char* password, uint8_t* opmode)
{
    if (idx == 0)
    {
        #ifdef WIFI_AP_UNIQUE_NAME
        char wifi_ap_name[64];
        wifi_get_unique_ssid(ssid);
        strcpy(password, (char*)WIFI_DEFAULT_PASS);
        #else
        strcpy(ssid, (char*)WIFI_DEFAULT_SSID);
        strcpy(password, (char*)WIFI_DEFAULT_PASS);
        #endif
        *opmode = WIFIOPMODE_AP;
        return true;
    }

    char fname[32];
    char* tmp = (char*)fname;
    wifiprofile_getIdxFname(idx, fname);
    File f = SPIFFS.open(fname);
    if (!f) {
        return false;
    }

    int r;
    r = wifiprofile_fileReadLine(&f, ssid, 32);
    if (r < 0) {
        f.close();
        return false;
    }
    r = wifiprofile_fileReadLine(&f, password, 32);
    if (r < 0) {
        f.close();
        return false;
    }
    r = wifiprofile_fileReadLine(&f, tmp, 32);
    if (r < 0) {
        f.close();
        return false;
    } else {
        if (tmp[0] == '2') {
            *opmode = WIFIOPMODE_STA;
        }
        else {
            *opmode = WIFIOPMODE_AP;
        }
    }
    f.close();
    return true;
}

bool wifiprofile_writeProfileRaw(uint8_t idx, char* ssid, char* password, uint8_t opmode)
{
    if (idx == 0) {
        return false;
    }

    char fcontents[64];
    char fname[32];
    char* tmp = (char*)fname;
    wifiprofile_getIdxFname(idx, fname);
    File f = SPIFFS.open(fname, FILE_WRITE);
    if (!f) {
        return false;
    }

    f.println(ssid);
    f.println(password);
    f.println(opmode, DEC);
    f.close();
    return true;
}

bool wifiprofile_getProfile(uint8_t idx, wifiprofile_t* p)
{
    return wifiprofile_getProfileRaw(idx, (char*)(p->ssid), (char*)(p->password), (uint8_t*)&(p->opmode));
}

bool wifiprofile_writeProfile(uint8_t idx, wifiprofile_t* p)
{
    return wifiprofile_writeProfileRaw(idx, (char*)(p->ssid), (char*)(p->password), p->opmode);
}

bool wifiprofile_connect(uint8_t idx)
{
    wifiprofile_t profile;
    if (wifiprofile_getProfile(idx, &profile))
    {
        if (idx != 0 && strlen(profile.ssid) > 0) {
            Serial.print("WiFi AP Name: ");
            Serial.println(profile.ssid);
            if (profile.opmode == WIFIOPMODE_STA) {
                NetMgr_beginSTA((char*)profile.ssid, (char*)profile.password);
            }
            else {
                NetMgr_beginAP((char*)profile.ssid, (char*)profile.password);
            }
            return true;
        }
    }

    // fallback to default settings
    #ifdef WIFI_AP_UNIQUE_NAME
        char wifi_ap_name[64];
        wifi_get_unique_ssid(wifi_ap_name);
        Serial.print("WiFi AP Name: ");
        Serial.println(wifi_ap_name);
        NetMgr_beginAP((char*)wifi_ap_name, (char*)WIFI_DEFAULT_PASS);
    #else
        NetMgr_beginAP((char*)WIFI_DEFAULT_SSID, (char*)WIFI_DEFAULT_PASS);
    #endif
    return false;
}

void wifiprofile_deleteAll()
{
    int i;
    for (i = 1; i <= 9; i++)
    {
        wifiprofile_writeProfileRaw(i, (char*)"", (char*)"", 0);
    }
}
