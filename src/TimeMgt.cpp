#include "AlphaFairy.h"
#include <stdio.h>
#include <string.h>

static constexpr int64_t RTC_BUILD_TOLERANCE_SECONDS = 2 * 60 * 60;
static constexpr int64_t CAMERA_SYNC_DIFFERENCE_DS = 30 * 10;
static constexpr uint32_t CAMERA_SYNC_INTERVAL_MS = 10 * 60 * 1000;
static constexpr uint32_t CAMERA_CLOCK_TIMEOUT_MS = 1000;

enum class ClockOperation { None, Offset, Periodic, ToRtc, AutoWrite, ManualWrite };
enum class ManualSync { None, ToCamera, ToRtc };
static ClockOperation operation = ClockOperation::None;
static ManualSync manual_sync = ManualSync::None;
static bool first_properties = true;
static bool initial_sync_failed = false;
static bool auto_write_pending = false, periodic_pending = false;
static bool view_active = false, refresh_pending = false;
static bool background_refresh_pending = false;
static bool offset_valid = false, camera_needs_sync = false;
static int64_t offset_ds = 0;
static uint32_t session = 0, operation_session = 0;
static bool operation_auto_sync = false;
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
    if (!value || !M5.Rtc.isEnabled()) return false;
    m5::rtc_datetime_t rtc;
    if (!M5.Rtc.getDateTime(&rtc)) return false;
    ptpip_datetime_t current = {(uint16_t)rtc.date.year, (uint8_t)rtc.date.month, (uint8_t)rtc.date.date,
        (uint8_t)rtc.time.hours, (uint8_t)rtc.time.minutes, (uint8_t)rtc.time.seconds, 0};
    if (!ptpip_datetime_valid(current) || current.year < 2000 || current.year > 2099) return false;
    *value = current;
    return true;
}

bool time_get_rtc(ptpip_datetime_t* value, bool require_valid)
{
    ptpip_datetime_t current;
    if (!value || !read_rtc_calendar(&current)) {
        if (require_valid) camera_needs_sync = false;
        return false;
    }
    if (require_valid && (M5.Rtc.getVoltLow() || !time_rtc_valid(current))) {
        camera_needs_sync = false;
        return false;
    }
    *value = current;
    return true;
}

static bool write_rtc(const ptpip_datetime_t& camera, ptpip_datetime_t* synced_time)
{
    if (!M5.Rtc.isEnabled() || !ptpip_datetime_valid(camera)
        || camera.year < 2000 || camera.year > 2099) return false;

    // Explicitly copy the camera wall time, even if the existing RTC is invalid.
    // RTC hardware has whole-second precision; timezone/DST are not converted.
    m5::rtc_datetime_t rtc;
    rtc.date = m5::rtc_date_t(camera.year, camera.month, camera.day);
    rtc.time = m5::rtc_time_t(camera.hour, camera.minute, camera.second);
    offset_valid = camera_needs_sync = false;
    M5.Rtc.setDateTime(rtc);
    ptpip_datetime_t actual;
    if (!read_rtc_calendar(&actual) || M5.Rtc.getVoltLow()) return false;
    int64_t delta = ptpip_datetime_seconds(actual) - ptpip_datetime_seconds(camera);
    if (delta < 0 || delta > 1) return false;
    if (synced_time) *synced_time = actual;
    return true;
}

static void cache_offset(const ptpip_datetime_t& camera)
{
    ptpip_datetime_t rtc;
    offset_valid = time_get_rtc(&rtc, false);
    if (offset_valid) offset_ds = (ptpip_datetime_seconds(camera) - ptpip_datetime_seconds(rtc)) * 10 + camera.decisecond;
    camera_needs_sync = offset_valid && !M5.Rtc.getVoltLow() && time_rtc_valid(rtc)
        && (offset_ds <= -CAMERA_SYNC_DIFFERENCE_DS || offset_ds >= CAMERA_SYNC_DIFFERENCE_DS);
}

bool time_get_camera_offset(int64_t* deciseconds)
{
    if (!deciseconds || !offset_valid || !ptpcam.isOperating()) return false;
    *deciseconds = offset_ds;
    return true;
}

bool time_camera_needs_sync(void)
{
    return camera_needs_sync && ptpcam.isOperating();
}

void time_invalidate_clock_comparison(void)
{
    offset_valid = camera_needs_sync = false;
    background_refresh_pending = ptpcam.isOperating();
}

void time_note_camera_time(const char* datetime)
{
    offset_valid = camera_needs_sync = false;
    background_refresh_pending = refresh_pending = false; // This read supplies the requested comparison.
    ptpip_datetime_t camera;
    if (ptpcam.isOperating() && ptpip_datetime_parse(datetime, &camera)) cache_offset(camera);
}

bool time_sync_rtc_from_camera(ptpip_datetime_t* synced_time)
{
    if (!ptpcam.isOperating() || !M5.Rtc.isEnabled()) return false;
    char text[22];
    ptpip_datetime_t camera;
    if (!ptpcam.get_time(text, sizeof(text)) || !ptpip_datetime_parse(text, &camera)) {
        time_note_camera_time(NULL);
        return false;
    }
    bool success = write_rtc(camera, synced_time);
    time_note_camera_time(text);
    return success;
}

static void camera_time_written(bool success, const char* datetime)
{
    (void)datetime;
    if (operation != ClockOperation::AutoWrite && operation != ClockOperation::ManualWrite) return;
    operation = ClockOperation::None;
    if (operation_session != session || !ptpcam.isOperating()) return;
    if (success) {
        initial_sync_failed = false;
        auto_write_pending = periodic_pending = false;
        offset_valid = camera_needs_sync = false;
    }
    background_refresh_pending = true;
}

static void camera_time_read(bool success, const char* datetime)
{
    ClockOperation completed = operation;
    if (completed != ClockOperation::Offset && completed != ClockOperation::Periodic && completed != ClockOperation::ToRtc) return;
    operation = ClockOperation::None;
    if (operation_session != session || !ptpcam.isOperating()) return;
    offset_valid = camera_needs_sync = false;
    ptpip_datetime_t camera, rtc;
    if (!success || !datetime || !ptpip_datetime_parse(datetime, &camera)) return;
    if (completed == ClockOperation::ToRtc) write_rtc(camera, NULL);
    cache_offset(camera);
    if (completed == ClockOperation::Periodic && operation_auto_sync && config_settings.time_auto_sync
        && manual_sync == ManualSync::None && time_get_rtc(&rtc)) {
        int64_t difference = (ptpip_datetime_seconds(camera) - ptpip_datetime_seconds(rtc)) * 10 + camera.decisecond;
        if (difference <= -CAMERA_SYNC_DIFFERENCE_DS || difference >= CAMERA_SYNC_DIFFERENCE_DS) auto_write_pending = true;
    }
}

void time_camera_connected(void)
{
    ++session;
    operation = ClockOperation::None;
    manual_sync = ManualSync::None;
    first_properties = true;
    initial_sync_failed = false;
    auto_write_pending = periodic_pending = offset_valid = camera_needs_sync = false;
    background_refresh_pending = refresh_pending = false;
    last_check_ms = millis();
    // Request an initial snapshot now, rather than waiting for the periodic interval.
    ptpcam.need_check_properties = true;
}

void time_camera_properties_updated(void)
{
    if (!ptpcam.isOperating()) return;
    uint32_t now = millis();
    bool initial = first_properties;
    if (!initial && (uint32_t)(now - last_check_ms) < CAMERA_SYNC_INTERVAL_MS) return;
    first_properties = false;
    last_check_ms = now; // Throttle failures and invalid RTCs too.
    ptpip_datetime_t rtc;
    bool rtc_valid = time_get_rtc(&rtc);
    if (!rtc_valid) camera_needs_sync = false;
    if (config_settings.time_auto_sync && rtc_valid && (initial || initial_sync_failed)) {
        if (initial) initial_sync_failed = true; // Remains set if starting or completing the write fails.
        auto_write_pending = true;
        background_refresh_pending = true; // Read after the attempt, without scheduling another write.
    } else if (rtc_valid) {
        periodic_pending = true; // The warning needs a comparison even with auto-sync disabled.
    }
    if (view_active) refresh_pending = true;
}

void time_clock_view_active(bool active)
{
    if (active && !view_active) refresh_pending = true;
    if (!active) refresh_pending = false;
    view_active = active;
}

bool time_request_sync_camera_from_rtc(void)
{
    ptpip_datetime_t rtc;
    if (!ptpcam.isOperating() || manual_sync != ManualSync::None
        || operation == ClockOperation::ToRtc || operation == ClockOperation::ManualWrite || !time_get_rtc(&rtc)) return false;
    manual_sync = ManualSync::ToCamera;
    return true;
}

bool time_request_sync_rtc_from_camera(void)
{
    if (!ptpcam.isOperating() || !M5.Rtc.isEnabled() || manual_sync != ManualSync::None
        || operation == ClockOperation::ToRtc || operation == ClockOperation::ManualWrite) return false;
    manual_sync = ManualSync::ToRtc;
    return true;
}

void time_task(void)
{
    if (!ptpcam.isOperating()) {
        operation = ClockOperation::None;
        manual_sync = ManualSync::None;
        auto_write_pending = periodic_pending = refresh_pending = background_refresh_pending = offset_valid = camera_needs_sync = false;
        return;
    }
    if (!config_settings.time_auto_sync) auto_write_pending = false;
    if (operation != ClockOperation::None || !ptpcam.canSend() || ptpcam.properties_pending || ptpcam.objinfo_pending) return;
    if (first_properties && manual_sync == ManualSync::None) return;

    ClockOperation next = ClockOperation::None;
    if (manual_sync != ManualSync::None) {
        next = manual_sync == ManualSync::ToCamera ? ClockOperation::ManualWrite : ClockOperation::ToRtc;
        manual_sync = ManualSync::None;
    } else if (auto_write_pending) {
        auto_write_pending = false;
        next = ClockOperation::AutoWrite;
    } else if (periodic_pending) {
        periodic_pending = refresh_pending = background_refresh_pending = false; // One read serves both comparison and display.
        next = ClockOperation::Periodic;
    } else if (refresh_pending || background_refresh_pending) {
        refresh_pending = background_refresh_pending = false;
        next = ClockOperation::Offset;
    }
    if (next == ClockOperation::None) return;
    if (next == ClockOperation::ToRtc) refresh_pending = background_refresh_pending = false;

    bool started = false;
    operation = next;
    operation_session = session;
    operation_auto_sync = config_settings.time_auto_sync != 0;
    if (next == ClockOperation::AutoWrite || next == ClockOperation::ManualWrite) {
        ptpip_datetime_t rtc;
        char text[20];
        if (time_get_rtc(&rtc) && ptpip_datetime_format(rtc, text, sizeof(text))) {
            started = ptpcam.request_set_time(text, camera_time_written, CAMERA_CLOCK_TIMEOUT_MS);
        }
    } else if (next != ClockOperation::ToRtc || M5.Rtc.isEnabled()) {
        started = ptpcam.request_time(camera_time_read, CAMERA_CLOCK_TIMEOUT_MS);
    }
    if (!started) {
        operation = ClockOperation::None;
        if (next == ClockOperation::Offset || next == ClockOperation::Periodic || next == ClockOperation::ToRtc) offset_valid = camera_needs_sync = false;
    }
}
