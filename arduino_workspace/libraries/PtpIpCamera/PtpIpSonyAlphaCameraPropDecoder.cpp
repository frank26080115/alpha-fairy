#include "PtpIpSonyAlphaCamera.h"
#include <Arduino.h>
#include "ptpip_utils.h"

void PtpIpSonyAlphaCamera::decode_properties()
{
    prop_rx_cnt++;
    critical_error_cnt = 0;

    properties_pending = false;
    need_check_properties = false;
    if (databuff_idx <= 0) {
        return;
    }

    uint8_t* p = (uint8_t*)databuff;
    int i, j, totalprops = 0, len = databuff_idx;

    #if 0
    Serial.println("DevProps Dump:");
    print_buffer_hex(databuff, len);
    Serial.println();
    #endif

    for (i = 0, j = 0; i < len && (j <= totalprops || i == 0); )
    {
        if (i == 0)
        {
            // the first couple of bytes is supposed to be the total number of properties
            totalprops = *((int*)p);
            dbgser_devprop_dump->printf("\r\nDevProps [%d, %d]:", totalprops, len);
            i += 8;
            continue;
        }

        j += 1;

        dbgser_devprop_dump->printf("\r\n");
        uint16_t propcode = *(uint16_t*)(&(p[i]));
        dbgser_devprop_dump->printf("PROP[%u][PC 0x%04X]", i, propcode);
        i += 2;

        uint16_t datatype = *(uint16_t*)(&(p[i]));
        i += 2;
        if (datatype == 0x0000)
        {
            i += 4;
            dbgser_devprop_dump->printf(" unknown data type 0");
            continue;
        }
        if (propcode == 0x0000)
        {
            i += 4;
            dbgser_devprop_dump->printf(" unknown prop code 0");
            continue;
        }

        i += 2; // skips the get/set visibility byte

        uint8_t* dptr = nullptr;
        int      dsz  = 1;

        if (datatype <= 0x0A)
        {
            dbgser_devprop_dump->printf("[DT 0x%X]", datatype);
        }

        switch (datatype & 0x0F)
        {
            case 0x0001:
            case 0x0002:
                dsz = 1;
                break;
            case 0x0003:
            case 0x0004:
                dsz = 2;
                break;
            case 0x0005:
            case 0x0006:
                dsz = 4;
                break;
            case 0x0007:
            case 0x0008:
                dsz = 8;
                break;
            case 0x0009:
            case 0x000A:
                dsz = 16;
                break;
        }

        if (datatype <= 0x000A) // simple integers
        {
            i += dsz; // skips the default value
            dptr = (uint8_t*)(&(p[i]));
            i += dsz; // skips the data value
            if (dbgser_devprop_dump->enabled) {
                propdecoder_print_hex(datatype, dptr, 1);
            }
        }
        else if (datatype == 0xFFFF)
        {
            // string
            if (propdecode_weird_string == false)
            {
                uint32_t elecnt = p[i];
                i += 1;
                dbgser_devprop_dump->printf("[STR %u]: ", elecnt);
                uint32_t j;
                for (j = 0; j < (elecnt * 2); j+= 2)
                {
                    char uc = p[i + j];
                    if (uc != 0)
                    {
                        dbgser_devprop_dump->printf("%c", uc);
                    }
                }
                i += elecnt * 2;
            }
            else
            {
                uint32_t elecnt = p[i];
                i -= 2;
                if (p[i] == 0x01 && p[i + 1] == 0x01) // check the get/set
                {
                    // I don't understand this but it works
                    i += 2;
                    elecnt = (*((uint32_t*)&p[i])) & 0xFFFF;
                    i += 2;
                }
                else if (p[i] == 0x00 && p[i + 1] == 0x02 && p[i + 2] == 0x00)
                {
                    // I don't understand this but it works
                    i += 3;
                    elecnt = (*((uint32_t*)&p[i])) & 0xFF;
                    i += 1;
                }
                dbgser_devprop_dump->printf("[STR %u]: ", elecnt);
                if (elecnt > 0)
                {
                    // warning: in-place copy
                    copyn_utf16_to_bytes(&(p[i]), &(p[i]), elecnt);
                    dbgser_devprop_dump->printf("\"%s\"", (char*)(&(p[i])));
                    i += elecnt * 2;
                }
                else
                {
                    dbgser_devprop_dump->printf(" weird empty string");
                }
            }
        }
        else if ((datatype & 0x4000) != 0)
        {
            // array
            uint32_t elecnt = *(uint32_t*)(&(p[i]));
            i += 4;
            dbgser_devprop_dump->printf("[ARR %u]", elecnt);
            if (elecnt > 0)
            {
                dptr = (uint8_t*)(&(p[i])); // first element as default
                if (dbgser_devprop_dump->enabled) {
                    propdecoder_print_hex(datatype, dptr, elecnt);
                }
            }
            i += dsz * elecnt;
        }

        bool interesting = update_property(propcode, datatype, dptr, dsz);
        if (interesting) {
            dbgser_devprop_change->printf("***DevProp[0x%04X]: 0x%08X\r\n", propcode, get_property(propcode));
        }

        uint8_t formflag = p[i];
        i += 1;
        if (formflag == 0)
        {
            continue;
        }
        else if (formflag == 0x01)
        {
            // range
            i += 3 * dsz;
            dbgser_devprop_dump->printf(" [FRM RNG %d]", dsz);
            continue;
        }
        else if (formflag == 0x02)
        {
            // enumeration
            if (propdecode_weird_form == false)
            {
                uint16_t enumcnt = *(uint16_t*)(&(p[i]));
                i += 2;

#define PROP_TRYPOPULATETABLE(_tbl, _propcode, _dsz, _enumcnt, _txt) do \
                {\
                    if (propcode == (_propcode) && dsz == (_dsz) && (_enumcnt) != 0) {\
                        if ((_tbl) != NULL) {\
                            if ((_enumcnt) > (_tbl)[0]) {\
                                free((_tbl)); \
                                (_tbl) = NULL; \
                                dbgser_events->printf(_txt " free'd\r\n");\
                            }\
                        }\
                        if ((_tbl) == NULL) {\
                            (_tbl) = (uint32_t*)malloc(((_enumcnt) * dsz) + sizeof(uint32_t));\
                            memcpy((void*)&((_tbl)[1]), (void*)&(p[i]), (_enumcnt) * dsz);\
                            (_tbl)[0] = (_enumcnt);\
                            dbgser_events->printf(_txt " created %u\r\n", (_enumcnt));\
                        }\
                    }\
                } while (0)\

                PROP_TRYPOPULATETABLE(table_shutter_speed, SONYALPHA_PROPCODE_ShutterSpeed, 4, enumcnt, "table_shutter_speed");
                PROP_TRYPOPULATETABLE(table_iso          , SONYALPHA_PROPCODE_ISO         , 4, enumcnt, "table_iso");
                PROP_TRYPOPULATETABLE(table_aperture     , SONYALPHA_PROPCODE_Aperture    , 2, enumcnt, "table_aperture");
                PROP_TRYPOPULATETABLE(table_aperture     , SONYALPHA_PROPCODE_Aperture    , 4, enumcnt, "table_aperture");

                i += enumcnt * dsz;
                dbgser_devprop_dump->printf(" [FRM ENUM %d]", enumcnt);
                continue;
            }
            else
            {
                // weird form, seems to have two forms
                uint16_t enumcnt = *(uint16_t*)(&(p[i]));
                i += 2;

                PROP_TRYPOPULATETABLE(table_shutter_speed, SONYALPHA_PROPCODE_ShutterSpeed, 4, enumcnt, "table_shutter_speed");
                PROP_TRYPOPULATETABLE(table_iso          , SONYALPHA_PROPCODE_ISO         , 4, enumcnt, "table_iso");
                PROP_TRYPOPULATETABLE(table_aperture     , SONYALPHA_PROPCODE_Aperture    , 2, enumcnt, "table_aperture");
                PROP_TRYPOPULATETABLE(table_aperture     , SONYALPHA_PROPCODE_Aperture    , 4, enumcnt, "table_aperture");

                i += enumcnt * dsz;
                uint16_t enumcnt2 = *(uint16_t*)(&(p[i]));
                i += 2;
                PROP_TRYPOPULATETABLE(table_shutter_speed, SONYALPHA_PROPCODE_ShutterSpeed, 4, enumcnt2, "table_shutter_speed");
                PROP_TRYPOPULATETABLE(table_iso          , SONYALPHA_PROPCODE_ISO         , 4, enumcnt2, "table_iso");
                PROP_TRYPOPULATETABLE(table_aperture     , SONYALPHA_PROPCODE_Aperture    , 2, enumcnt2, "table_aperture");
                PROP_TRYPOPULATETABLE(table_aperture     , SONYALPHA_PROPCODE_Aperture    , 4, enumcnt2, "table_aperture");
                i += enumcnt2 * dsz;
                dbgser_devprop_dump->printf(" [FRM ENUMx2 %d]", (enumcnt + enumcnt2));
            }
        }
    }
    dbgser_devprop_dump->printf("\r\n");

    // check if lens has disconnected, in which case, forget the aperture list
    if (table_aperture != NULL && ((has_property(SONYALPHA_PROPCODE_Aperture) && (get_property(SONYALPHA_PROPCODE_Aperture) == 0 || get_property(SONYALPHA_PROPCODE_Aperture) >= 0x7FFF)) || (has_property(SONYALPHA_PROPCODE_Aperture) == false))) {
        dbgser_devprop_change->printf("lens disconnected\r\n");
        free(table_aperture);
        table_aperture = NULL;
    }

    databuff_idx = 0;
    check_props_time = millis();
}

bool PtpIpSonyAlphaCamera::update_property(uint16_t prop_code, uint16_t data_type, uint8_t* data_chunk, uint8_t data_size)
{
    if (data_type > 0x0A) {
        return false;
    }
    int32_t x = decode_chunk_to_int(data_type, data_chunk, data_size);
    int i;

    // the wait routine can check this variable to exit quickly
    if (prop_code == SONYALPHA_PROPCODE_FocusFound) {
        is_focused = (x == SONYALPHA_FOCUSSTATUS_FOCUSED);
        // check if AF-S mode and focus is actually obtained
        if (is_focused == false && has_property(SONYALPHA_PROPCODE_FocusMode)) {
            if (get_property(SONYALPHA_PROPCODE_FocusMode) == SONYALPHA_AFMODE_AFS && x == SONYALPHA_FOCUSSTATUS_AFS_FOCUSED) {
                is_focused |= true;
            }
        }
    }

    bool is_interested = false;
    for (i = 0; i < 255; i++) {
        uint16_t intprop = p_interested_properties[i];
        if (intprop != 0) // not end of table
        {
            if (intprop == prop_code) {
                is_interested = true;
                break;
            }
        }
        else // end of table
        {
            if (i == 0) { // empty table
                is_interested = true;
            }
            break;
        }
    }

    if (is_interested)
    {
        for (i = 0; i < properties_cnt; i++) {
            ptpipcam_prop_t* pp = &(properties[i]);
            if (pp->prop_code == 0) {
                // fill first empty slot
                pp->prop_code = prop_code;
                pp->data_type = data_type;
                pp->value = x;
                return true;
            }
            else if (pp->prop_code == prop_code) {
                // found existing slot
                pp->data_type = data_type;
                if (pp->value != x) {
                    pp->value = x;
                    return true;
                }
            }
        }
    }
    return false;
}

int32_t PtpIpSonyAlphaCamera::get_property(uint16_t prop_code)
{
    int i;
    for (i = 0; i < properties_cnt; i++) {
        ptpipcam_prop_t* pp = &(properties[i]);
        if (pp->prop_code == prop_code) {
            return pp->value;
        }
    }
    return 0;
}

bool PtpIpSonyAlphaCamera::has_property(uint16_t prop_code)
{
    int i;
    for (i = 0; i < properties_cnt; i++) {
        ptpipcam_prop_t* pp = &(properties[i]);
        if (pp->prop_code == prop_code) {
            return true;
        }
    }
    return false;
}

uint32_t PtpIpSonyAlphaCamera::get_property_enum(uint16_t prop_code, uint32_t cur_val, int32_t step)
{
    uint32_t* tbl = NULL;
    if (prop_code == SONYALPHA_PROPCODE_ShutterSpeed) {
        tbl = this->table_shutter_speed;
    }
    else if (prop_code == SONYALPHA_PROPCODE_ISO) {
        tbl = this->table_iso;
    }
    if (tbl == NULL) {
        return 0xFFFFFFFF;
    }
    uint32_t cnt = tbl[0];
    int32_t i, j;
    for (i = 1; i <= cnt; i++)
    {
        uint32_t x = tbl[i], y;
        if (x == cur_val)
        {
            j = i + step;
            y = tbl[j];
            if (j >= cnt || j <= 1)
            {
                j = (j >= cnt) ? cnt : j;
                j = (j <=   1) ?   1 : j;
                y = tbl[j];
                if (y == 0 || y == 0xFFFFFFFF) {
                    j += (j >= cnt) ? (-1) : j;
                    j += (j <=   1) ? ( 1) : j;
                    y = tbl[j];
                }
            }
            return y;
        }
    }
    return 0xFFFFFFFF;
}

void PtpIpSonyAlphaCamera::test_prop_decode(uint8_t* data, uint32_t len)
{
    memcpy(databuff, data, len);
    databuff_idx = len;
    decode_properties();
}

void propdecoder_print_hex(uint16_t datatype, uint8_t* dptr, int cnt)
{
    uint16_t*  ptr16 = (uint16_t*)dptr;
    uint32_t*  ptr32 = (uint32_t*)dptr;
    (void)ptr16;
    (void)ptr32;
    int j, k;
    for (j = 0, k = 0; j < cnt; j++)
    {
        switch (datatype & 0x0F)
        {
            case 1: case 2:  Serial.printf(" 0x%02X", dptr[k] ); k += 1; break;
            case 3: case 4:  Serial.printf(" 0x%04X", ptr16[j]); k += 2; break;
            case 5: case 6:  Serial.printf(" 0x%08X", ptr32[j]); k += 4; break;

            // case 7 and beyond are 64+ bits, not tested as my camera doesn't support
            case 7: case 8:  Serial.printf(" 0x%08X%08X", ptr32[j + 1], ptr32[j]); k += 8; break;
            case 9: case 10:
                Serial.printf(" 0x%08X%08X", ptr32[j + 3], ptr32[j + 2]);
                Serial.printf(   "%08X%08X", ptr32[j + 1], ptr32[j    ]);
                k += 16;
                break;
        }
    }
}

void PtpIpSonyAlphaCamera::decode_objinfo()
{
    objinfo_pending = false;
    need_check_object = false;
    uint32_t* p = (uint32_t*)databuff;
    uint32_t photo_size = p[2];

    dbgser_events->printf("decode_objinfo size %u\r\n", photo_size);

    uint32_t req_params[3];
    req_params[0] = need_check_object_id;
    req_params[1] = 0;
    req_params[2] = photo_size;
    bool success = send_oper_req(PTP_OPCODE_GetPartialObject, req_params, 3, NULL, 0);
    if (success != false) {
        reset_buffers();
        start_stream(NULL, NULL);
    }

    // sample:
    // 0x00 0x00 0x01 0x00 0x01 0x38 0x00 0x00 0x34 0xF5 0x05 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x0D 0x44 0x00 0x53 0x00 0x43 0x00 0x30 0x00 0x39 0x00 0x38 0x00 0x37 0x00 0x31 0x00 0x2E 0x00 0x4A 0x00 0x50 0x00 0x47 0x00 0x00 0x00 0x00 0x00 0x00 

}
