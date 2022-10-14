#include "AlphaFairyCamera.h"

extern uint32_t shutter_to_millis(uint32_t x);
extern uint32_t parse_shutter_speed_str(char* s);
static int get_idx_in_str_tbl(char* tbl_prt, uint32_t x, uint32_t cvt_mode);
static bool get_str_at_tbl_idx(char* tbl, int idx, char* dst);
static bool get_val_at_tbl_idx(char* tbl, int idx, void* dst, uint32_t cvt_mode);

uint32_t AlphaFairyCamera::getIp(void)
{
    if (cam_ptp->isOperating()) {
        return cam_ptp->getIp();
    }
    if (cam_http->isOperating()) {
        return cam_http->getIp();
    }
    return 0;
}

char* AlphaFairyCamera::getCameraName(void)
{
    if (cam_ptp->isOperating()) {
        return cam_ptp->getCameraName();
    }
    if (cam_http->isOperating()) {
        return cam_http->getCameraName();
    }
    return NULL;
}

void AlphaFairyCamera::wait_while_busy(uint32_t min_wait, uint32_t max_wait, volatile bool* exit_signal)
{
    if (cam_ptp->isOperating()) {
        return cam_ptp->wait_while_busy(min_wait, max_wait, exit_signal);
    }
    if (cam_http->isOperating()) {
        return cam_http->wait_while_busy(min_wait, max_wait, exit_signal);
    }
}

bool AlphaFairyCamera::is_movierecording(void)
{
    if (cam_ptp->isOperating()) {
        return cam_ptp->is_movierecording();
    }
    if (cam_http->isOperating()) {
        return cam_http->is_movierecording();
    }
    return false;
}

bool AlphaFairyCamera::is_manuallyfocused(void)
{
    if (cam_ptp->isOperating()) {
        return cam_ptp->is_manuallyfocused();
    }
    if (cam_http->isOperating()) {
        return cam_http->is_manuallyfocused() == SHCAM_FOCUSMODE_MF;
    }
    return false;
}

bool AlphaFairyCamera::is_focused(void)
{
    if (cam_ptp->isOperating()) {
        return cam_ptp->is_focused;
    }
    if (cam_http->isOperating()) {
        return cam_http->is_focused;
    }
    return false;
}

uint32_t AlphaFairyCamera::get_exposureMode(void)
{
    int32_t x = 0;
    if (cam_ptp->isOperating()) {
        if (cam_ptp->has_property(PTP_PROPCODE_ExposureProgramMode)) {
            x = cam_ptp->get_property(PTP_PROPCODE_ExposureProgramMode);
            if ((x & 0x8000) != 0) {
                return x; // bit 15  is set so just return it since it's a part of the enums
            }
            if ((x & 0xF0000) != 0) {
                return x & 0xFFFF; // mask off to match the enums
            }

            // handle classic PTP codes
            if (x > 0) {
                // convert it to the Sony codes
                switch (x)
                {
                    case PTP_EXPOPROGMODE_MANUAL:
                        return SONYALPHA_EXPOMODE_M;
                    case PTP_EXPOPROGMODE_AUTO:
                        return SONYALPHA_EXPOMODE_P;
                    case PTP_EXPOPROGMODE_A:
                        return SONYALPHA_EXPOMODE_A;
                    case PTP_EXPOPROGMODE_S:
                        return SONYALPHA_EXPOMODE_S;
                }
            }
        }
        if (cam_ptp->has_property(SONYALPHA_PROPCODE_ExposeIndex)) {
            x = cam_ptp->get_property(SONYALPHA_PROPCODE_ExposeIndex);
            if (x >= SONYALPHA_EXPOMODE_IntelligentAuto && x <= 0x9000) {
                return x;
            }
        }
    }
    if (cam_http->isOperating())
    {
        // convert string to enumeration
        char* str_em = cam_http->get_str_expomode();
        if (memcmp("Manual", str_em, 5) == 0) {
            return SONYALPHA_EXPOMODE_M;
        }
        if (memcmp("Program Auto", str_em, 5) == 0) {
            return SONYALPHA_EXPOMODE_P;
        }
        if (memcmp("Aperture", str_em, 5) == 0) {
            return SONYALPHA_EXPOMODE_A;
        }
        if (memcmp("Shutter", str_em, 5) == 0) {
            return SONYALPHA_EXPOMODE_S;
        }
        if (memcmp("Intelligent Auto", str_em, 5) == 0) {
            return SONYALPHA_EXPOMODE_IntelligentAuto;
        }
        if (memcmp("Superior Auto", str_em, 5) == 0) {
            return SONYALPHA_EXPOMODE_SuperiorAuto;
        }
    }
    return x;
}

bool AlphaFairyCamera::need_wait_af(void)
{
    if (cam_ptp->isOperating()) {
        return cam_ptp->need_wait_af();
    }
    if (cam_http->isOperating()) {
        return cam_http->need_wait_af();
    }
    return false;
}

bool AlphaFairyCamera::cmd_AutoFocus(bool onoff)
{
    if (cam_ptp->isOperating()) {
        return cam_ptp->cmd_AutoFocus(onoff);
    }
    if (cam_http->isOperating()) {
        cam_http->cmd_AutoFocus(onoff);
        return true;
    }
    return false;
}

bool AlphaFairyCamera::cmd_Shutter(bool openclose)
{
    if (cam_ptp->isOperating()) {
        return cam_ptp->cmd_Shutter(openclose);
    }
    if (openclose && cam_http->isOperating()) {
        cam_http->cmd_Shoot();
        return true;
    }
    return false;
}

bool AlphaFairyCamera::cmd_FocusPointSet(int16_t x, int16_t y)
{
    if (cam_ptp->isOperating()) {
        return cam_ptp->cmd_FocusPointSet(x, y);
    }
    if (cam_http->isOperating()) {
        cam_http->cmd_FocusPointSet16(x, y);
        return true;
    }
    return false;
}

bool AlphaFairyCamera::cmd_Shoot(int t)
{
    if (cam_ptp->isOperating()) {
        return cam_ptp->cmd_Shoot(t);
    }
    if (cam_http->isOperating()) {
        cam_http->cmd_Shoot();
        return true;
    }
    return false;
}

bool AlphaFairyCamera::cmd_MovieRecord(bool onoff)
{
    if (cam_ptp->isOperating()) {
        return cam_ptp->cmd_MovieRecord(onoff);
    }
    if (cam_http->isOperating()) {
        cam_http->cmd_MovieRecord(onoff);
        return true;
    }
    return false;
}

bool AlphaFairyCamera::cmd_MovieRecordToggle()
{
    if (cam_ptp->isOperating()) {
        return cam_ptp->cmd_MovieRecordToggle();
    }
    if (cam_http->isOperating()) {
        cam_http->cmd_MovieRecordToggle();
        return true;
    }
    return false;
}

bool AlphaFairyCamera::cmd_ManualFocusMode(bool onoff, bool precheck)
{
    if (cam_ptp->isOperating()) {
        return cam_ptp->cmd_ManualFocusMode(onoff, precheck);
    }
    if (cam_http->isOperating()) {
        cam_http->cmd_ManualFocusMode(onoff, precheck);
        return true;
    }
    return false;
}

bool AlphaFairyCamera::cmd_ManualFocusToggle(bool onoff)
{
    if (cam_ptp->isOperating()) {
        return cam_ptp->cmd_ManualFocusToggle(onoff);
    }
    if (cam_http->isOperating()) {
        cam_http->cmd_ManualFocusToggle(onoff);
        return true;
    }
    return false;
}

bool AlphaFairyCamera::cmd_IsoSet(uint32_t x)
{
    if (cam_ptp->isOperating()) {
        return cam_ptp->cmd_IsoSet(x);
    }
    if (cam_http->isOperating()) {
        cam_http->cmd_IsoSet(x);
        return true;
    }
    return false;
}

bool AlphaFairyCamera::cmd_ShutterSpeedSet(uint32_t x)
{
    if (cam_ptp->isOperating()) {
        return cam_ptp->cmd_ShutterSpeedSet32(x);
    }
    if (cam_http->isOperating() && cam_http->tbl_shutterspd != NULL) {
        int idx = getIdx_shutter(x);
        if (idx >= 0)
        {
            char sbuff[32];
            bool r = get_str_at_tbl_idx(cam_http->tbl_shutterspd, idx, sbuff);
            if (r)
            {
                int slen = strlen(sbuff);
                if (sbuff[0] == '1' && sbuff[1] == '/') {
                    // do nothing
                }
                else if (sbuff[0] >= '0' && sbuff[0] <= '9') {
                    sbuff[slen] = '\\';
                    sbuff[slen + 1] = '"';
                    sbuff[slen + 2] = 0;
                }
                cam_http->cmd_ShutterSpeedSetStr(sbuff);
                return true;
            }
        }
        return true;
    }
    return false;
}

bool AlphaFairyCamera::cmd_ApertureSet(uint32_t x)
{
    if (cam_ptp->isOperating()) {
        return cam_ptp->cmd_ApertureSet((uint16_t)x);
    }
    if (cam_http->isOperating()) {
        cam_http->cmd_ApertureSet32(x);
        return true;
    }
    return false;
}

bool AlphaFairyCamera::cmd_ExpoCompSet(int32_t x)
{
    if (cam_ptp->isOperating()) {
        return cam_ptp->cmd_ExpoCompSet(x);
    }
    if (cam_http->isOperating()) {
        cam_http->cmd_ExpoCompSet32(x);
        return true;
    }
    return false;
}

void AlphaFairyCamera::set_debugflags(uint32_t x)
{
    if (cam_ptp != NULL) {
        cam_ptp->set_debugflags(x);
    }
    if (cam_http != NULL) {
        cam_http->set_debugflags(x);
    }
}

void AlphaFairyCamera::force_disconnect(void)
{
    if (cam_ptp != NULL) {
        cam_ptp->force_disconnect();
    }
    if (cam_http != NULL) {
        cam_http->force_disconnect();
    }
}

static int get_idx_in_str_tbl(char* tbl_ptr, uint32_t x, uint32_t cvt_mode)
{
    uint32_t oricomp = x;
    if (cvt_mode == SONYALPHA_PROPCODE_ShutterSpeed) {
        oricomp = shutter_to_millis(x);
    }
    else if (cvt_mode == SONYALPHA_PROPCODE_ISO) {
        oricomp &= 0xFFFFFF;
    }

    int commas = 0;

    int has_min = -1;
    uint32_t min_dist;

    char dst[32];
    int i, j = strlen(tbl_ptr);
    for (i = 0; i < j; i++) {
        char c = tbl_ptr[i];
        if (i == 0 || c == ',') // first entry or found comma
        {
            i++;
            int k;
            for (k = 0, dst[0] = 0; i < j && k < 30; i++)
            {
                c = tbl_ptr[i];
                if ((c >= '0' && c <= '9') || (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '+' || c == '-' || c == '.' || c == '/') // look for valid numeric characters
                {
                    // record valid character into string
                    dst[k] = c;
                    dst[k+1] = 0;
                    k++;
                }
                else if (c == ',') // found another comma, end of entry, go back to outer loop
                {
                    i--;
                    break;
                }
            }

            if (k > 0) // if has result
            {
                if (cvt_mode == SONYALPHA_PROPCODE_ShutterSpeed)
                {
                    uint32_t y = parse_shutter_speed_str(dst);
                    uint32_t yms = shutter_to_millis(y);
                    if (oricomp == yms) {
                        return commas;
                    }
                    else {
                        uint32_t dist = (oricomp > yms) ? (oricomp - yms) : (yms - oricomp);
                        if (has_min < 0 || dist < min_dist) {
                            has_min = commas;
                            min_dist = dist;
                        }
                    }
                }
                else if (cvt_mode == SONYALPHA_PROPCODE_ISO)
                {
                    if (dst[0] >= '0' && dst[0] <= '9') {
                        uint32_t y = atoi(dst);
                        if (oricomp == y) {
                            return commas;
                        }
                        uint32_t dist = (oricomp > y) ? (oricomp - y) : (y - oricomp);
                        if (has_min < 0 || dist < min_dist) {
                            has_min = commas;
                            min_dist = dist;
                        }
                    }
                    else {
                        if (oricomp == 0 || oricomp == 0xFFFFFF) {
                            return commas;
                        }
                    }
                }
                else if (cvt_mode == SONYALPHA_PROPCODE_Aperture)
                {
                    uint32_t y = (uint32_t)lround(100.0f * atof(dst));
                    if (y >= (oricomp - 6) && y <= (oricomp + 6)) {
                        return commas;
                    }
                    uint32_t dist = (oricomp > y) ? (oricomp - y) : (y - oricomp);
                    if (has_min < 0 || (dist < min_dist && min_dist < 200)) {
                        has_min = commas;
                        min_dist = dist;
                    }
                }
            }

            commas++;
        }
    }

    if (has_min >= 0) {
        return has_min;
    }

    return -1;
}

static bool get_str_at_tbl_idx(char* tbl, int idx, char* dst)
{
    int commas = 0;
    int i, j = strlen(tbl);
    for (i = 0; i < j; i++)
    {
        char c = tbl[i];
        if (c == ',')
        {
            commas++;
        }
        if (commas == idx)
        {
            i++;
            int k;
            for (k = 0; k < 30 && i < j; i++)
            {
                c = tbl[i];
                if ((c >= '0' && c <= '9') || (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '+' || c == '-' || c == '.' || c == '/') {
                    dst[k] = c;
                    dst[k + 1] = 0;
                    k++;
                }
                else if (c == ',')
                {
                    break;
                }
            }
            if (k > 0)
            {
                return true;
            }
        }
    }
    return false;
}

static bool get_val_at_tbl_idx(char* tbl, int idx, void* dst, uint32_t cvt_mode)
{
    char sbuff[32];
    if (idx < 0) {
        idx = 0;
    }
    while (idx >= 0)
    {
        bool r = get_str_at_tbl_idx(tbl, idx, (char*) sbuff);
        if (r)
        {
            if (cvt_mode == SONYALPHA_PROPCODE_ShutterSpeed)
            {
                uint32_t* p = (uint32_t*)dst;
                (*p) = parse_shutter_speed_str((char*)sbuff);
                return true;
            }
            else if (cvt_mode == SONYALPHA_PROPCODE_Aperture)
            {
                uint32_t* p = (uint32_t*)dst;
                (*p) = (uint32_t)lround(100.0f * atof(sbuff));
                return true;
            }
            else if (cvt_mode == SONYALPHA_PROPCODE_ISO)
            {
                uint32_t* p = (uint32_t*)dst;
                if (sbuff[0] >= '0' && sbuff[0] <= '9') {
                    (*p) = (uint32_t)atoi(sbuff);
                }
                else {
                    (*p) = 0;
                }
                return true;
            }
        }
        idx--;
    }
    return false;
}

int AlphaFairyCamera::getIdx_shutter(uint32_t x)
{
    if (cam_ptp->isOperating() && cam_ptp->table_shutter_speed != NULL) {
        uint32_t* tbl = (uint32_t*)&(cam_ptp->table_shutter_speed[1]);
        uint32_t i, j = cam_ptp->table_shutter_speed[0];
        for (i = 0; i < j; i++) {
            if (tbl[i] == x) {
                return i;
            }
        }
    }
    if (cam_http->isOperating() && cam_http->tbl_shutterspd != NULL) {
        return get_idx_in_str_tbl(cam_http->tbl_shutterspd, x, SONYALPHA_PROPCODE_ShutterSpeed);
    }
    return -1;
}

int AlphaFairyCamera::getIdx_aperture(uint32_t x)
{
    if (cam_ptp->isOperating() && cam_ptp->table_aperture != NULL) {
        uint16_t* tbl = (uint16_t*)&(cam_ptp->table_aperture[1]);
        uint32_t i, j = cam_ptp->table_aperture[0];
        for (i = 0; i < j; i++) {
            if (tbl[i] <= (x + 6) && tbl[i] >= (x - 6)) {
                return i;
            }
        }
    }
    if (cam_http->isOperating() && cam_http->tbl_aperture != NULL) {
        return get_idx_in_str_tbl(cam_http->tbl_aperture, x, SONYALPHA_PROPCODE_Aperture);
    }
    return -1;
}

int AlphaFairyCamera::getIdx_iso(uint32_t x)
{
    if (cam_ptp->isOperating() && cam_ptp->table_iso != NULL) {
        uint32_t* tbl = (uint32_t*)&(cam_ptp->table_iso[1]);
        uint32_t i, j = cam_ptp->table_iso[0];
        for (i = 0; i < j; i++) {
            if (tbl[i] == x) {
                return i;
            }
        }
        if (x == 0) {
            x = 0xFFFFFF;
        }
        for (i = 0; i < j; i++) {
            if (tbl[i] == x) {
                return i;
            }
        }
    }
    if (cam_http->isOperating() && cam_http->tbl_iso != NULL) {
        return get_idx_in_str_tbl(cam_http->tbl_iso, x, SONYALPHA_PROPCODE_ISO);
    }
    return -1;
}

int AlphaFairyCamera::getIdx_expoComp(int32_t x)
{
    float fx = x;
    return (int)lround(fx / 333.3f);
}

uint32_t AlphaFairyCamera::getVal_shutter(int idx)
{
    idx = idx < 0 ? 0 : idx;
    if (cam_ptp->isOperating() && cam_ptp->table_shutter_speed != NULL) {
        uint32_t* tbl = (uint32_t*)&(cam_ptp->table_shutter_speed[1]);
        if (idx >= cam_ptp->table_shutter_speed[0]) {
            return tbl[cam_ptp->table_shutter_speed[0] - 1];
        }
        else if (idx < 0) {
            return tbl[0];
        }
        return tbl[idx];
    }
    if (cam_http->isOperating() && cam_http->tbl_shutterspd != NULL) {
        uint32_t v;
        bool r = get_val_at_tbl_idx(cam_http->tbl_shutterspd, idx, &v, SONYALPHA_PROPCODE_ShutterSpeed);
        if (r) {
            return v;
        }
    }

    return 0;
}

uint32_t AlphaFairyCamera::getVal_aperture(int idx)
{
    idx = idx < 0 ? 0 : idx;
    if (cam_ptp->isOperating() && cam_ptp->table_aperture != NULL) {
        uint16_t* tbl = (uint16_t*)&(cam_ptp->table_aperture[1]);
        if (idx >= cam_ptp->table_aperture[0]) {
            return tbl[cam_ptp->table_aperture[0] - 1];
        }
        else if (idx < 0) {
            return tbl[0];
        }
        return tbl[idx];
    }
    if (cam_http->isOperating() && cam_http->tbl_aperture != NULL) {
        uint32_t v;
        bool r = get_val_at_tbl_idx(cam_http->tbl_aperture, idx, &v, SONYALPHA_PROPCODE_Aperture);
        if (r) {
            return v;
        }
    }

    return 0;
}

uint32_t AlphaFairyCamera::getVal_iso(int idx)
{
    idx = idx < 0 ? 0 : idx;
    if (cam_ptp->isOperating() && cam_ptp->table_iso != NULL) {
        uint32_t* tbl = (uint32_t*)&(cam_ptp->table_iso[1]);
        if (idx >= cam_ptp->table_iso[0]) {
            return tbl[cam_ptp->table_iso[0] - 1];
        }
        else if (idx < 0) {
            return tbl[0];
        }
        return tbl[idx];
    }
    if (cam_http->isOperating() && cam_http->tbl_iso != NULL) {
        uint32_t v;
        bool r = get_val_at_tbl_idx(cam_http->tbl_iso, idx, &v, SONYALPHA_PROPCODE_ISO);
        if (r) {
            return v;
        }
    }

    return 0;
}

int32_t AlphaFairyCamera::getVal_expoComp(int idx)
{
    return idx * 333;
}
