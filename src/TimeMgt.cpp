#include "AlphaFairy.h"
#include <stdio.h>
#include <string.h>

static constexpr int64_t RTC_BUILD_TOLERANCE_SECONDS = 2 * 60 * 60;
static constexpr int64_t CAMERA_SYNC_LAG_SECONDS = 72 * 60 * 60;
static constexpr uint32_t CAMERA_SYNC_INTERVAL_MS = 10 * 60 * 1000;
static constexpr uint32_t CAMERA_CLOCK_TIMEOUT_MS = 1000;

static bool check_on_next_properties = true;
static uint32_t last_check_ms = 0;

static int64_t compile_time_seconds()
{
    // __DATE__/__TIME__ and the RTC are compared as local wall time, without DST conversion.
    char month[4];
    unsigned year, day, hour, minute, second;
    if (sscanf(__DATE__, "%3s %u %u", month, &day, &year) != 3
        || sscanf(__TIME__, "%u:%u:%u", &hour, &minute, &second) != 3) return 0;
    const char* months = "JanFebMarAprMayJunJulAugSepOctNovDec";
    const char* found = strstr(months, month);
    if (!found) return 0;
    ptpip_datetime_t value = {(uint16_t)year, (uint8_t)((found - months) / 3 + 1), (uint8_t)day,
        (uint8_t)hour, (uint8_t)minute, (uint8_t)second, 0};
    return ptpip_datetime_valid(value) ? ptpip_datetime_seconds(value) : 0;
}

bool time_rtc_valid(const ptpip_datetime_t& value)
{
    static const int64_t built_at = compile_time_seconds();
    return built_at != 0 && ptpip_datetime_valid(value) && value.year >= 2000 && value.year <= 2099
        && ptpip_datetime_seconds(value) > built_at - RTC_BUILD_TOLERANCE_SECONDS;
}

static bool read_rtc_calendar(ptpip_datetime_t* value)
{
    if (!value || !M5.Rtc.isEnabled() || M5.Rtc.getVoltLow()) return false;
    m5::rtc_datetime_t rtc;
    if (!M5.Rtc.getDateTime(&rtc)) return false;
    ptpip_datetime_t current = {(uint16_t)rtc.date.year, (uint8_t)rtc.date.month, (uint8_t)rtc.date.date,
        (uint8_t)rtc.time.hours, (uint8_t)rtc.time.minutes, (uint8_t)rtc.time.seconds, 0};
    if (!ptpip_datetime_valid(current) || current.year < 2000 || current.year > 2099) return false;
    *value = current;
    return true;
}

bool time_get_rtc(ptpip_datetime_t* value)
{
    ptpip_datetime_t current;
    if (!value || !read_rtc_calendar(&current) || !time_rtc_valid(current)) return false;
    *value = current;
    return true;
}

bool time_sync_rtc_from_camera(ptpip_datetime_t* synced_time)
{
    if (!ptpcam.isOperating() || !M5.Rtc.isEnabled()) return false;
    char text[22];
    ptpip_datetime_t camera;
    if (!ptpcam.get_time(text, sizeof(text)) || !ptpip_datetime_parse(text, &camera)
        || camera.year < 2000 || camera.year > 2099) return false;

    // Explicitly copy the camera wall time, even if the existing RTC is invalid.
    // RTC hardware has whole-second precision; timezone/DST are not converted.
    m5::rtc_datetime_t rtc;
    rtc.date = m5::rtc_date_t(camera.year, camera.month, camera.day);
    rtc.time = m5::rtc_time_t(camera.hour, camera.minute, camera.second);
    M5.Rtc.setDateTime(rtc);
    ptpip_datetime_t actual;
    if (!read_rtc_calendar(&actual)) return false;
    int64_t delta = ptpip_datetime_seconds(actual) - ptpip_datetime_seconds(camera);
    if (delta < 0 || delta > 1) return false;
    if (synced_time) *synced_time = actual;
    return true;
}

static void camera_time_written(bool success, const char* datetime)
{
    // Best effort: no messages, verification traffic or immediate retries.
    (void)success;
    (void)datetime;
}

static void camera_time_read(bool success, const char* datetime)
{
    if (!success || !datetime || !ptpcam.isOperating()) return;
    ptpip_datetime_t camera, rtc;
    if (!ptpip_datetime_parse(datetime, &camera) || !time_get_rtc(&rtc)) return;
    if (ptpip_datetime_seconds(rtc) - ptpip_datetime_seconds(camera) <= CAMERA_SYNC_LAG_SECONDS) return;
    char text[20];
    if (ptpip_datetime_format(rtc, text, sizeof(text))) {
        ptpcam.request_set_time(text, camera_time_written, CAMERA_CLOCK_TIMEOUT_MS);
    }
}

void time_camera_connected(void)
{
    check_on_next_properties = true;
    // Request an initial snapshot now, rather than waiting for the periodic interval.
    ptpcam.need_check_properties = true;
}

void time_camera_properties_updated(void)
{
    if (!ptpcam.isOperating()) return;
    uint32_t now = millis();
    if (!check_on_next_properties && (uint32_t)(now - last_check_ms) < CAMERA_SYNC_INTERVAL_MS) return;
    check_on_next_properties = false;
    last_check_ms = now; // Throttle failures and invalid RTCs too.
    ptpip_datetime_t rtc;
    if (!time_get_rtc(&rtc)) return;
    ptpcam.request_time(camera_time_read, CAMERA_CLOCK_TIMEOUT_MS);
}
