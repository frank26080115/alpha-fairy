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

bool wifiprofile_getProfileRaw(uint8_t idx, char* ssid, char* password, uint8_t* opmode, char* guid)
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
        if (guid != NULL) {
            ptpcam.generate_guid(guid);
        }
        return true;
    }

    char fname[32];
    char* tmp = (char*)fname;
    wifiprofile_getIdxFname(idx, fname);
    File f = SPIFFS.open(fname);
    if (!f) {
        ssid[0] = 0;
        password[0] = 0;
        *opmode = 0;
        if (guid != NULL) {
            guid[0] = 0;
        }
        return false;
    }

    int r;
    r = wifiprofile_fileReadLine(&f, ssid, 32);
    if (r < 0) {
        ssid[0] = 0;
        password[0] = 0;
        *opmode = 0;
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
    if (guid != NULL) {
        r = wifiprofile_fileReadLine(&f, guid, 16 + 1);
        if (r < 0) {
            guid[0] = 0;
        }
    }
    f.close();
    return true;
}

bool wifiprofile_writeProfileRaw(uint8_t idx, char* ssid, char* password, uint8_t opmode, char* guid)
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
    if (guid != NULL) {
        if (guid[0] != 0) {
            f.println(guid);
        }
    }
    f.close();
    return true;
}

bool wifiprofile_getProfile(uint8_t idx, wifiprofile_t* p)
{
    return wifiprofile_getProfileRaw(idx, (char*)(p->ssid), (char*)(p->password), (uint8_t*)&(p->opmode), (char*)(p->guid));
}

bool wifiprofile_writeProfile(uint8_t idx, wifiprofile_t* p)
{
    return wifiprofile_writeProfileRaw(idx, (char*)(p->ssid), (char*)(p->password), p->opmode, (char*)(p->guid));
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
    for (i = 1; i <= WIFIPROFILE_LIMIT; i++)
    {
        wifiprofile_writeProfileRaw(i, (char*)"", (char*)"", 0, NULL);
    }
}

void wifiprofile_deleteProfile(uint8_t idx)
{
    if (idx <= 0 || idx > WIFIPROFILE_LIMIT) {
        return;
    }
    char fname[16];
    char fname2[16];
    wifiprofile_getIdxFname(idx, (char*)fname);
    SPIFFS.remove(fname);
    for (; idx < WIFIPROFILE_LIMIT; idx++)
    {
        wifiprofile_getIdxFname(idx, (char*)fname);
        wifiprofile_getIdxFname(idx + 1, (char*)fname2);
        if (SPIFFS.exists(fname2)) {
            SPIFFS.rename(fname, fname2);
        }
    }
}

bool wifiprofile_isBlank(uint8_t idx)
{
    wifiprofile_t p;
    if (wifiprofile_getProfile(idx, &p) == false) {
        return true;
    }
    if (p.ssid[0] == 0) {
        return true;
    }
    return false;
}

void wifiprofile_scanFill()
{
    uint32_t t = millis();
    Serial.printf("WiFi camera scan, start time %u\r\n", t);
    wifiprofile_t pscan, pfile;
    int n = WiFi.scanNetworks(false,false,false,100U);
    int i, j;
    for (i = 0; i < n; i++)
    {
        String ssid = WiFi.SSID(i);
        strcpy(pscan.ssid, ssid.c_str());
        if (memcmp("DIRECT-", pscan.ssid, 7) == 0)
        {
            Serial.print("WiFi scan found camera: ");
            Serial.print(pscan.ssid);
            bool found = false;
            for (j = 1; j <= WIFIPROFILE_LIMIT && found == false; j++)
            {
                bool hasfile = wifiprofile_getProfile(j, &pfile);
                if (hasfile)
                {
                    if (strcmp(pfile.ssid, pscan.ssid) == 0) {
                        found = true;
                        break;
                    }
                }
            }
            if (found == false)
            {
                for (j = 1; j <= WIFIPROFILE_LIMIT && found == false; j++)
                {
                    bool hasfile = wifiprofile_getProfile(j, &pfile);
                    if (hasfile == false || pfile.ssid[0] == 0)
                    {
                        wifiprofile_writeProfileRaw(j, pscan.ssid, (char*)"", WIFIOPMODE_STA, NULL);
                        Serial.printf("\t; wrote to profile #%u", j);
                        found = true;
                        break;
                    }
                }
            }
            Serial.println();
        }
    }
    uint32_t t2 = millis(), t3 = t2 - t;
    Serial.printf("Scan time %u\r\n", t3); // this includes all of the flash file reads
}

int wifiprofile_autoFind(wifiprofile_t* ptgt)
{
    uint32_t t = millis();
    Serial.printf("WiFi camera scan, start time %u\r\n", t);
    wifiprofile_t pscan, pfile;
    int n = WiFi.scanNetworks(false,false,false,100U);
    int i, j;
    for (i = 0; i < n; i++)
    {
        String ssid = WiFi.SSID(i);
        strcpy(pscan.ssid, ssid.c_str());
        if (memcmp("DIRECT-", pscan.ssid, 7) == 0)
        {
            Serial.print("WiFi scan found camera: ");
            Serial.print(pscan.ssid);
            for (j = 1; j <= WIFIPROFILE_LIMIT; j++)
            {
                bool hasfile = wifiprofile_getProfile(j, &pfile);
                if (hasfile)
                {
                    if (strcmp(pfile.ssid, pscan.ssid) == 0)
                    {
                        Serial.printf("\t; matches profile #%d\r\n", i);
                        if (ptgt != NULL) {
                            memcpy(ptgt, &pfile, sizeof(wifiprofile_t));
                        }
                        return i;
                    }
                }
            }
            Serial.println();
        }
    }
    uint32_t t2 = millis(), t3 = t2 - t;
    Serial.printf("Scan time %u\r\n", t3); // this includes all of the flash file reads
    return -1;
}
