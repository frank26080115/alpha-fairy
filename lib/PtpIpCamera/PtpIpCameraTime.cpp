#include "PtpIpCameraTime.h"
#include <stdio.h>
#include <string.h>

static unsigned days_in_month(unsigned year, unsigned month)
{
    static const uint8_t days[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    if (month < 1 || month > 12) return 0;
    return days[month - 1] + (month == 2 && year % 4 == 0 && (year % 100 != 0 || year % 400 == 0));
}

bool ptpip_datetime_valid(const ptpip_datetime_t& t)
{
    return t.year >= 1 && t.year <= 9999 && t.month >= 1 && t.month <= 12
        && t.day >= 1 && t.day <= days_in_month(t.year, t.month)
        && t.hour < 24 && t.minute < 60 && t.second < 60 && t.decisecond < 10;
}

bool ptpip_datetime_parse(const char* text, ptpip_datetime_t* value)
{
    if (!text || !value) return false;
    size_t len = strlen(text);
    if (len != 19 && len != 21) return false;
    unsigned digits[21] = {};
    for (size_t i = 0; i < len; ++i) {
        if (i == 4 || i == 7) { if (text[i] != '-') return false; }
        else if (i == 10) { if (text[i] != ' ' && text[i] != 'T') return false; }
        else if (i == 13 || i == 16) { if (text[i] != ':') return false; }
        else if (i == 19) { if (text[i] != '.') return false; }
        else {
            if (text[i] < '0' || text[i] > '9') return false;
            digits[i] = text[i] - '0';
        }
    }
    ptpip_datetime_t t = {
        (uint16_t)(digits[0] * 1000 + digits[1] * 100 + digits[2] * 10 + digits[3]),
        (uint8_t)(digits[5] * 10 + digits[6]), (uint8_t)(digits[8] * 10 + digits[9]),
        (uint8_t)(digits[11] * 10 + digits[12]), (uint8_t)(digits[14] * 10 + digits[15]),
        (uint8_t)(digits[17] * 10 + digits[18]), (uint8_t)(len == 21 ? digits[20] : 0)
    };
    if (!ptpip_datetime_valid(t)) return false;
    *value = t;
    return true;
}

bool ptpip_datetime_format(const ptpip_datetime_t& t, char* text, size_t size, bool fraction)
{
    if (!text || size < (fraction ? 22u : 20u) || !ptpip_datetime_valid(t)) return false;
    int len = snprintf(text, size, "%04u-%02u-%02u %02u:%02u:%02u", t.year, t.month, t.day, t.hour, t.minute, t.second);
    if (fraction) snprintf(text + len, size - len, ".%u", t.decisecond);
    return true;
}

int64_t ptpip_datetime_seconds(const ptpip_datetime_t& t)
{
    // Calendar arithmetic avoids mktime's local timezone/DST and 32-bit time_t.
    int64_t year = t.year - 1;
    int64_t days = year * 365 + year / 4 - year / 100 + year / 400 + t.day - 1;
    for (unsigned month = 1; month < t.month; ++month) days += days_in_month(t.year, month);
    return ((days * 24 + t.hour) * 60 + t.minute) * 60 + t.second;
}

bool ptpip_sony_clock_decode(const uint8_t* data, size_t size, ptpip_datetime_t* value)
{
    // 0x9248: version 100, ASCII YYYYMMDDThhmmss.s NUL, +/-hhmm NUL, DST byte.
    if (!data || size != 29 || data[0] != 100 || data[1] || data[2] || data[3]
        || data[12] != 'T' || data[19] != '.' || data[21] || data[27] || data[28] > 1
        || (data[22] != '+' && data[22] != '-')) return false;
    for (unsigned i = 23; i < 27; ++i) if (data[i] < '0' || data[i] > '9') return false;
    if ((data[23] - '0') * 10 + data[24] - '0' > 23
        || (data[25] - '0') * 10 + data[26] - '0' > 59) return false;
    char text[22];
    snprintf(text, sizeof(text), "%.4s-%.2s-%.2s %.2s:%.2s:%.2s.%.1s",
        (const char*)data + 4, (const char*)data + 8, (const char*)data + 10,
        (const char*)data + 13, (const char*)data + 15, (const char*)data + 17, (const char*)data + 20);
    return ptpip_datetime_parse(text, value);
}

bool ptpip_sony_clock_encode(const ptpip_datetime_t& t, uint8_t payload[37])
{
    if (!payload || !ptpip_datetime_valid(t)) return false;
    char compact[18];
    snprintf(compact, sizeof(compact), "%04u%02u%02uT%02u%02u%02u.%u",
        t.year, t.month, t.day, t.hour, t.minute, t.second, t.decisecond);
    // A1 II silently ignores D223 strings that omit the fractional second.
    memset(payload, 0, 37);
    payload[0] = 18; // UTF-16 code units including the terminating NUL.
    for (unsigned i = 0; i < 17; ++i) payload[1 + i * 2] = compact[i];
    return true;
}
