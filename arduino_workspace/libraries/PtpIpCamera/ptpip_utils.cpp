#include "ptpip_utils.h"
#include <Arduino.h>
#include <math.h>

//#define PTPIP_DEBUG_RX

uint32_t copy_bytes_to_utf16(void* dest, void* src) {
    int i, j;
    uint8_t* dp = (uint8_t*)dest;
    uint8_t* sp = (uint8_t*)src;
    i = 0; j = 0;
    while (1) {
        uint8_t c = sp[i];
        dp[j] = c;
        dp[j+1] = 0;
        i += 1;
        j += 2;
        if (c == 0) {
            break;
        }
    }
    return j;
}

uint32_t copy_utf16_to_bytes(void* dest, void* src) {
    int i, j;
    uint8_t* dp = (uint8_t*)dest;
    uint8_t* sp = (uint8_t*)src;
    i = 0; j = 0;
    while (1) {
        uint8_t c = sp[i];
        dp[j] = c;
        i += 2;
        j += 1;
        if (c == 0) {
            break;
        }
    }
    return j;
}

void copyn_utf16_to_bytes(void* dest, void* src, uint32_t n) {
    int i, j;
    uint8_t* dp = (uint8_t*)dest;
    uint8_t* sp = (uint8_t*)src;
    i = 0; j = 0;
    while (j < n) {
        uint8_t c = sp[i];
        dp[j] = c;
        i += 2;
        j += 1;
        if (c == 0) {
            break;
        }
    }
    dp[j] = 0;
}

void buffer_consume(uint8_t buff[], uint32_t* buff_idx, uint32_t read_cnt, uint32_t buff_max)
{
    uint32_t i, j;
    if (read_cnt > (*buff_idx)) {
        read_cnt = (*buff_idx);
    }
    //for (i = 0, j = read_cnt; j < buff_max; i++, j++) {
    //    buff[i] = buff[j];
    //}
    memcpy(buff, &(buff[read_cnt]), buff_max - read_cnt);
    (*buff_idx) -= read_cnt;
    #ifdef PTPIP_DEBUG_RX
    if ((*buff_idx) > 0)
    {
        Serial.printf("BUF CONSUME (%u , %u) ", read_cnt, (*buff_idx));
        for (i = 0; i < (*buff_idx); i++) {
            Serial.printf(" %02X", buff[i]);
        }
        Serial.printf("\r\n");
    }
    #endif
}

void print_buffer_hex(uint8_t* data, uint32_t len)
{
    #if 1
    if (len > 128) {
        Serial.printf(" too long");
        return;
    }
    #endif
    uint32_t i;
    for (i = 0; i < len; i++) {
        if (i != 0 && (i % 32) == 0) {
            Serial.printf("\r\n");
        }
        else {
            Serial.printf(" ");
        }
        Serial.printf("%02X", data[i]);
    }
}

uint32_t decode_chunk_to_uint(uint16_t data_type, uint8_t* data_chunk, uint8_t data_size)
{
    uint8_t dsz = property_data_get_size(data_type);
    uint8_t i;
    uint32_t res = 0;
    for (i = 0; i < dsz; i++) {
        res += data_chunk[i] << (8 * i);
    }
    return res;
}

int32_t decode_chunk_to_int(uint16_t data_type, uint8_t* data_chunk, uint8_t data_length)
{
    int dsz = property_data_get_size(data_type);
    int32_t u = (int32_t)decode_chunk_to_uint(data_type, data_chunk, data_length);
    if ((data_type & 1) != 0)
    {
        // signed
        if (u >= pow(2, ((8 * dsz) - 1))) {
            u -= pow(2, ((8 * dsz)));
        }
    }
    return u;
}

uint8_t property_data_get_size(uint16_t data_type)
{
    uint8_t dsz = 1;
    uint16_t dt4bit = data_type & 0x0F;
    switch (dt4bit) {
        case 1: case    2: dsz =  1; break;
        case 3: case    4: dsz =  2; break;
        case 5: case    6: dsz =  4; break;
        case 7: case    8: dsz =  8; break;
        case 9: case 0x0A: dsz = 16; break;
    }
    return dsz;
}