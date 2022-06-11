#include "PtpIpSonyAlphaCamera.h"
#include <Arduino.h>
#include "ptpip_utils.h"

#if 0
    #define PROPDECODER_VERBOSE_PRINTF Serial.printf
    #if 1
        #define PROPDECODER_INTERESTED_PRINTF(...)
    #else
        #define PROPDECODER_INTERESTED_PRINTF Serial.printf
    #endif
#else
    #define PROPDECODER_VERBOSE_PRINTF(...)
    #if 1
        #define PROPDECODER_INTERESTED_PRINTF Serial.printf
    #else
        #define PROPDECODER_INTERESTED_PRINTF(...)
    #endif
#endif

void PtpIpSonyAlphaCamera::decode_properties()
{
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
            PROPDECODER_VERBOSE_PRINTF("\r\nDevProps [%d, %d]:", totalprops, len);
            i += 8;
            continue;
        }

        j += 1;

        PROPDECODER_VERBOSE_PRINTF("\r\n");
        uint16_t propcode = *(uint16_t*)(&(p[i]));
        PROPDECODER_VERBOSE_PRINTF("PROP[%u][PC 0x%04X]", i, propcode);
        i += 2;

        uint16_t datatype = *(uint16_t*)(&(p[i]));
        i += 2;
        if (datatype == 0x0000)
        {
            i += 4;
            PROPDECODER_VERBOSE_PRINTF(" unknown data type 0");
            continue;
        }
        if (propcode == 0x0000)
        {
            i += 4;
            PROPDECODER_VERBOSE_PRINTF(" unknown prop code 0");
            continue;
        }

        i += 2; // skips the get/set visibility byte

        uint8_t* dptr = nullptr;
        int      dsz  = 1;

        if (datatype <= 0x0A)
        {
            PROPDECODER_VERBOSE_PRINTF("[DT 0x%X]", datatype);
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
            propdecoder_print_hex(datatype, dptr, 1);
        }
        else if (datatype == 0xFFFF)
        {
            // string
            if (propdecode_weird_string == false)
            {
                uint32_t elecnt = p[i];
                i += 1;
                PROPDECODER_VERBOSE_PRINTF("[STR %u]: ", elecnt);
                uint32_t j;
                for (j = 0; j < (elecnt * 2); j+= 2)
                {
                    char uc = p[i + j];
                    if (uc != 0)
                    {
                        PROPDECODER_VERBOSE_PRINTF("%c", uc);
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
                PROPDECODER_VERBOSE_PRINTF("[STR %u]: ", elecnt);
                if (elecnt > 0)
                {
                    // warning: in-place copy
                    copyn_utf16_to_bytes(&(p[i]), &(p[i]), elecnt);
                    PROPDECODER_VERBOSE_PRINTF("\"%s\"", (char*)(&(p[i])));
                    i += elecnt * 2;
                }
                else
                {
                    PROPDECODER_VERBOSE_PRINTF(" weird empty string");
                }
            }
        }
        else if ((datatype & 0x4000) != 0)
        {
            // array
            uint32_t elecnt = *(uint32_t*)(&(p[i]));
            i += 4;
            PROPDECODER_VERBOSE_PRINTF("[ARR %u]", elecnt);
            if (elecnt > 0)
            {
                dptr = (uint8_t*)(&(p[i])); // first element as default
                propdecoder_print_hex(datatype, dptr, elecnt);
            }
            i += dsz * elecnt;
        }

        bool interesting = update_property(propcode, datatype, dptr, dsz);
        if (interesting) {
            PROPDECODER_INTERESTED_PRINTF(">>>DevProp[0x%04X]: 0x%08X\r\n", propcode, get_property(propcode));
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
            PROPDECODER_VERBOSE_PRINTF(" [FRM RNG %d]", dsz);
            continue;
        }
        else if (formflag == 0x02)
        {
            // enumeration
            if (propdecode_weird_form == false)
            {
                uint16_t enumcnt = *(uint16_t*)(&(p[i]));
                i += 2;
                i += enumcnt * dsz;
                PROPDECODER_VERBOSE_PRINTF(" [FRM ENUM %d]", enumcnt);
                continue;
            }
            else
            {
                // weird form, seems to have two forms
                uint16_t enumcnt = *(uint16_t*)(&(p[i]));
                i += 2;
                i += enumcnt * dsz;
                uint16_t enumcnt2 = *(uint16_t*)(&(p[i]));
                i += 2;
                i += enumcnt2 * dsz;
                PROPDECODER_VERBOSE_PRINTF(" [FRM ENUMx2 %d]", (enumcnt + enumcnt2));
            }
        }
    }
    PROPDECODER_VERBOSE_PRINTF("\r\n");

    databuff_idx = 0;
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
    }

    bool is_interested = false;
    for (i = 0; ; i++) {
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
            case 1: case 2:  PROPDECODER_VERBOSE_PRINTF(" 0x%02X", dptr[k]); k += 1; break;
            case 3: case 4:  PROPDECODER_VERBOSE_PRINTF(" 0x%04X", ptr16[j]); k += 2; break;
            case 5: case 6:  PROPDECODER_VERBOSE_PRINTF(" 0x%08X", ptr32[j]); k += 4; break;

            // case 7 and beyond are 64+ bits, not tested as my camera doesn't support
            case 7: case 8:  PROPDECODER_VERBOSE_PRINTF(" 0x%08X%08X", ptr32[j + 1], ptr32[j]); k += 8; break;
            case 9: case 10:
                PROPDECODER_VERBOSE_PRINTF(" 0x%08X%08X", ptr32[j + 3], ptr32[j + 2]);
                PROPDECODER_VERBOSE_PRINTF(   "%08X%08X", ptr32[j + 1], ptr32[j    ]);
                k += 16;
                break;
        }
    }
}