#include "AlphaFairy.h"
#include <SerialCmdLine.h>

/*
handles command line commands
this is used only for testing
*/

void factory_reset_func(void* cmd, char* argstr, Stream* stream);
void rtcset_func    (void* cmd, char* argstr, Stream* stream);
void rtcget_func    (void* cmd, char* argstr, Stream* stream);
void camtimeget_func(void* cmd, char* argstr, Stream* stream);
void camtimeset_func(void* cmd, char* argstr, Stream* stream);
void rtcfromcam_func(void* cmd, char* argstr, Stream* stream);
#ifndef DISABLE_CMD_LINE
void focuscalib_func(void* cmd, char* argstr, Stream* stream);
void shoot_func     (void* cmd, char* argstr, Stream* stream);
void echo_func      (void* cmd, char* argstr, Stream* stream);
void memcheck_func  (void* cmd, char* argstr, Stream* stream);
void statscheck_func(void* cmd, char* argstr, Stream* stream);
void reboot_func    (void* cmd, char* argstr, Stream* stream);
void imu_func       (void* cmd, char* argstr, Stream* stream);
void imushow_func   (void* cmd, char* argstr, Stream* stream);
void mic_func       (void* cmd, char* argstr, Stream* stream);
void pwr_func       (void* cmd, char* argstr, Stream* stream);
void btncnt_func    (void* cmd, char* argstr, Stream* stream);
void debug_func     (void* cmd, char* argstr, Stream* stream);
void camdebug_func  (void* cmd, char* argstr, Stream* stream);
void infrared_func  (void* cmd, char* argstr, Stream* stream);
void savewifi_func  (void* cmd, char* argstr, Stream* stream);
void dumpwifi_func  (void* cmd, char* argstr, Stream* stream);
void wifipwr_func   (void* cmd, char* argstr, Stream* stream);
void pmiclog_func   (void* cmd, char* argstr, Stream* stream);
void listlog_func   (void* cmd, char* argstr, Stream* stream);
void readlog_func   (void* cmd, char* argstr, Stream* stream);
void readhex_func   (void* cmd, char* argstr, Stream* stream);
#endif

const cmd_def_t cmds[] = {
  { "factoryreset", factory_reset_func},
  { "rtcset"   , rtcset_func },
  { "rtcget"   , rtcget_func },
  { "camtimeget", camtimeget_func },
  { "camtimeset", camtimeset_func },
  { "rtcfromcam", rtcfromcam_func },
  #ifndef DISABLE_CMD_LINE
  { "focuscalib", focuscalib_func },
  { "shoot"    , shoot_func },
  { "echo"     , echo_func },
  { "mem"      , memcheck_func },
  { "imu"      , imu_func },
  { "imushow"  , imushow_func },
  { "mic"      , mic_func },
  { "pwr"      , pwr_func },
  { "btncnt"   , btncnt_func },
  { "stats"    , statscheck_func },
  { "reboot"   , reboot_func },
  { "debug"    , debug_func },
  { "camdebug" , camdebug_func },
  { "ir"       , infrared_func },
  { "savewifi" , savewifi_func },
  { "dumpwifi" , dumpwifi_func },
  { "wifipwr"  , wifipwr_func },
  { "pmiclog"  , pmiclog_func },
  { "listlog"  , listlog_func },
  { "readlog"  , readlog_func },
  { "readhex"  , readhex_func },
  #endif
  { "", NULL }, // end of table
};

SerialCmdLine cmdline(&Serial, (cmd_def_t*)cmds, false, (char*)">>>", (char*)"???", true, 512);

extern bool redraw_flag;

void factory_reset_func(void* cmd, char* argstr, Stream* stream)
{
  pwr_tick(true);
  settings_default();
  settings_save();
  stream->println("factory reset performed");
}

// Clock commands intentionally remain available with DISABLE_CMD_LINE defined.
static char* clock_trim(char* text)
{
    while (*text == ' ' || *text == '\t') ++text;
    size_t len = strlen(text);
    while (len && (text[len - 1] == ' ' || text[len - 1] == '\t')) text[--len] = '\0';
    return text;
}

static bool clock_no_args(char* args, Stream* stream, const char* name)
{
    if (*clock_trim(args) == '\0') return true;
    stream->printf("ERROR %s: no arguments expected\r\n", name);
    return false;
}

static bool clock_read_rtc(ptpip_datetime_t* value, Stream* stream, const char* name)
{
    if (!M5.Rtc.isEnabled()) {
        stream->printf("ERROR %s: RTC unavailable on this device\r\n", name);
        return false;
    }
    m5::rtc_datetime_t rtc;
    if (!M5.Rtc.getDateTime(&rtc)) {
        stream->printf("ERROR %s: RTC read failed\r\n", name);
        return false;
    }
    ptpip_datetime_t t = {(uint16_t)rtc.date.year, (uint8_t)rtc.date.month, (uint8_t)rtc.date.date,
        (uint8_t)rtc.time.hours, (uint8_t)rtc.time.minutes, (uint8_t)rtc.time.seconds, 0};
    if (M5.Rtc.getVoltLow() || !ptpip_datetime_valid(t) || t.year < 2000 || t.year > 2099) {
        stream->printf("ERROR %s: RTC time invalid; use rtcset YYYY-MM-DD HH:MM:SS\r\n", name);
        return false;
    }
    *value = t;
    return true;
}

static bool clock_camera_connected(Stream* stream, const char* name)
{
    if (ptpcam.isOperating()) return true;
    stream->printf("ERROR %s: no PTP/IP camera connected\r\n", name);
    return false;
}

void rtcset_func(void* cmd, char* argstr, Stream* stream)
{
    pwr_tick(true);
    char* text = clock_trim(argstr);
    ptpip_datetime_t value;
    if (strlen(text) != 19 || !ptpip_datetime_parse(text, &value) || value.year < 2000 || value.year > 2099) {
        stream->println("ERROR rtcset: use YYYY-MM-DD HH:MM:SS; year 2000-2099, valid calendar date required");
        return;
    }
    if (!M5.Rtc.isEnabled()) {
        stream->println("ERROR rtcset: RTC unavailable on this device");
        return;
    }
    m5::rtc_datetime_t rtc;
    rtc.date = m5::rtc_date_t(value.year, value.month, value.day); // M5Unified computes weekday.
    rtc.time = m5::rtc_time_t(value.hour, value.minute, value.second);
    time_invalidate_clock_comparison();
    M5.Rtc.setDateTime(rtc);
    ptpip_datetime_t actual;
    if (!clock_read_rtc(&actual, stream, "rtcset")) return;
    int64_t delta = ptpip_datetime_seconds(actual) - ptpip_datetime_seconds(value);
    if (delta < 0 || delta > 1) {
        stream->println("ERROR rtcset: RTC write did not match readback");
        return;
    }
    char formatted[20];
    ptpip_datetime_format(actual, formatted, sizeof(formatted));
    stream->printf("OK rtcset %s\r\n", formatted);
}

void rtcget_func(void* cmd, char* argstr, Stream* stream)
{
    pwr_tick(true);
    if (!clock_no_args(argstr, stream, "rtcget")) return;
    ptpip_datetime_t value;
    if (!clock_read_rtc(&value, stream, "rtcget")) return;
    char formatted[20];
    ptpip_datetime_format(value, formatted, sizeof(formatted));
    stream->printf("RTC %s\r\n", formatted);
}

void camtimeget_func(void* cmd, char* argstr, Stream* stream)
{
    pwr_tick(true);
    if (!clock_no_args(argstr, stream, "camtimeget") || !clock_camera_connected(stream, "camtimeget")) return;
    char formatted[22];
    if (!ptpcam.get_time(formatted, sizeof(formatted))) {
        time_note_camera_time(NULL);
        stream->println("ERROR camtimeget: camera clock read failed (unsupported, busy, disconnected or timed out)");
        return;
    }
    time_note_camera_time(formatted);
    stream->printf("CAMERA %s\r\n", formatted);
}

void camtimeset_func(void* cmd, char* argstr, Stream* stream)
{
    pwr_tick(true);
    if (!clock_no_args(argstr, stream, "camtimeset") || !clock_camera_connected(stream, "camtimeset")) return;
    ptpcam.wait_while_busy(0, DEFAULT_SAVE_TIMEOUT);
    ptpip_datetime_t value;
    if (!clock_read_rtc(&value, stream, "camtimeset")) return;
    if (!time_rtc_valid(value)) {
        stream->println("ERROR camtimeset: RTC is at least 2 hours older than firmware build time; use rtcset");
        return;
    }
    char formatted[22];
    ptpip_datetime_format(value, formatted, sizeof(formatted));
    if (!ptpcam.set_time(formatted)) {
        stream->println("ERROR camtimeset: camera clock write failed (unsupported, busy, disconnected or timed out)");
        return;
    }
    time_invalidate_clock_comparison();
    ptpip_datetime_t actual, rtc_now;
    if (!ptpcam.get_time(formatted, sizeof(formatted)) || !ptpip_datetime_parse(formatted, &actual)) {
        time_note_camera_time(NULL);
        stream->println("ERROR camtimeset: write acknowledged, but camera clock readback failed");
        return;
    }
    time_note_camera_time(formatted);
    if (!clock_read_rtc(&rtc_now, stream, "camtimeset")) return;
    int64_t delta = ptpip_datetime_seconds(actual) - ptpip_datetime_seconds(rtc_now);
    if (delta < -2 || delta > 2) {
        stream->printf("ERROR camtimeset: camera readback %s differs from RTC\r\n", formatted);
        return;
    }
    stream->printf("OK camtimeset %s (from RTC; camera timezone/DST unchanged)\r\n", formatted);
}

void rtcfromcam_func(void* cmd, char* argstr, Stream* stream)
{
    pwr_tick(true);
    if (!clock_no_args(argstr, stream, "rtcfromcam") || !clock_camera_connected(stream, "rtcfromcam")) return;
    if (!M5.Rtc.isEnabled()) {
        stream->println("ERROR rtcfromcam: RTC unavailable on this device");
        return;
    }
    ptpip_datetime_t actual;
    if (!time_sync_rtc_from_camera(&actual)) {
        stream->println("ERROR rtcfromcam: camera clock read, date validation or RTC write/readback failed");
        return;
    }
    char formatted[20];
    ptpip_datetime_format(actual, formatted, sizeof(formatted));
    stream->printf("OK rtcfromcam %s (from camera)\r\n", formatted);
}

#ifndef DISABLE_CMD_LINE

void focuscalib_func(void* cmd, char* argstr, Stream* stream)
{
    fenc_calibrate();
}

void shoot_func(void* cmd, char* argstr, Stream* stream)
{
  pwr_tick(true);
  if (fairycam.isOperating())
  {
    stream->println("shoot");
    fairycam.cmd_Shoot(250);
  }
  else
  {
    stream->println("camera not connected");
  }
}

void echo_func(void* cmd, char* argstr, Stream* stream)
{
  pwr_tick(true);
  stream->println(argstr);
}

void reboot_func(void* cmd, char* argstr, Stream* stream)
{
  stream->println("rebooting...\r\n\r\n");
  ESP.restart();
}

void memcheck_func(void* cmd, char* argstr, Stream* stream)
{
  pwr_tick(true);
  stream->printf("free heap mem: %u\r\n", ESP.getFreeHeap());
}

void statscheck_func(void* cmd, char* argstr, Stream* stream)
{
  pwr_tick(true);
  #ifdef PTPIP_KEEP_STATS
  stream->printf("ptpipcam stats: %u  %u  %u\r\n", ptpcam.stats_tx, ptpcam.stats_acks, ptpcam.stats_pkts);
  #else
  stream->printf("ptpipcam stats not available\r\n");
  #endif
}

void imu_func(void* cmd, char* argstr, Stream* stream)
{
  pwr_tick(true);
  stream->printf("imu:   %0.1f   %0.1f   %0.1f\r\n", imu.pitch, imu.roll, imu.yaw);
}

void imushow_func(void* cmd, char* argstr, Stream* stream)
{
    gui_startAppPrint();
    M5Lcd.setTextFont(4);
    int spin_cnt = 0;
    while (true)
    {
        app_poll();
        pwr_tick(true);
        stream->printf("imu:   %0.1f   %0.1f   %0.1f\r\n", imu.pitch, imu.roll, imu.yaw);
        M5Lcd.setCursor(SUBMENU_X_OFFSET, SUBMENU_Y_OFFSET);
        M5Lcd.printf("%0.1f , %0.1f    ", imu.roll, imu.pitch); gui_blankRestOfLine(); M5Lcd.println(); gui_setCursorNextLine();
        M5Lcd.printf("%0.1f , %0.1f    ", imu.roll_adj, imu.pitch_adj); gui_blankRestOfLine(); M5Lcd.println(); gui_setCursorNextLine();
        M5Lcd.printf("%d", imu.pitch_accum); gui_blankRestOfLine(); M5Lcd.println(); gui_setCursorNextLine();
        int spin = imu.getSpin();
        if (spin > 0) {
            spin_cnt++;
        }
        else if (spin < 0) {
            spin_cnt--;
        }
        M5Lcd.printf("spin  %d", spin_cnt); gui_blankRestOfLine();
        if (spin != 0) {
            imu.resetSpin();
        }
        if (btnAny_hasPressed()) {
            stream->printf("user exit\r\n");
            delay(100);
            ESP.restart();
        }
    }
}

extern volatile int32_t mictrig_lastMax, mictrig_filteredMax;

void mic_func(void* cmd, char* argstr, Stream* stream)
{
    #if 0
    gui_startAppPrint();
    mictrig_drawIcon();
    while (true)
    {
        pwr_tick(true);
        app_poll();
        mictrig_poll();
        stream->printf("mic:    %d   %d\r\n", mictrig_lastMax, mictrig_filteredMax);
        mictrig_drawLevel();
        if (btnAny_hasPressed()) {
            stream->printf("user exit\r\n");
            delay(100);
            ESP.restart();
        }
    }
    #endif
}

void pwr_func(void* cmd, char* argstr, Stream* stream)
{
  pwr_tick(true);
  stream->printf("pwr:   %0.3f   %0.3f   %0.3f   %0.3f\r\n",
    m5power_getBatteryVoltage(),
    m5power_getBatteryCurrent(),
    m5power_getVBusVoltage(),
    m5power_getVBusCurrent()
    );
}

extern volatile uint32_t btnSide_cnt, btnBig_cnt, btnPwr_cnt;
void btncnt_func(void* cmd, char* argstr, Stream* stream)
{
    pwr_tick(true);
    stream->printf("btn cnt:   %u    %u    %u\r\n", btnSide_cnt, btnBig_cnt, btnPwr_cnt);
}


extern DebuggingSerial dbg_ser;
void debug_func(void* cmd, char* argstr, Stream* stream)
{
    pwr_tick(true);
    dbg_ser.enabled = atoi(argstr) != 0;
    stream->printf("debugging output = %u\r\n", dbg_ser.enabled);
    dbg_ser.println("test output from debugging serial port");
}

void camdebug_func(void* cmd, char* argstr, Stream* stream)
{
    pwr_tick(true);
    int x = atoi(argstr);
    ptpcam.set_debugflags(x);
    httpcam.set_debugflags(x);
    stream->printf("camera debugging output = %u\r\n", x);
    ptpcam.test_debug_msg("test from ptpcam debug serport\r\n");
    httpcam.test_debug_msg("test from httpcam debug serport\r\n");
}

void infrared_func(void* cmd, char* argstr, Stream* stream)
{
    pwr_tick(true);
    stream->printf("infrared test fire\r\n");
    SonyCamIr_Shoot();
}

void savewifi_func(void* cmd, char* argstr, Stream* stream)
{
    pwr_tick(true);
    char delim[] = ",";
    char *ptr = strtok(argstr, delim);
    int i = 0;
    int profile_num;
    wifiprofile_t profile;
    memset(&profile, 0, sizeof(wifiprofile_t));
    while (ptr != NULL)
    {
        switch (i)
        {
            case 0:
                profile_num = atoi(ptr);
                break;
            case 1:
                strncpy(profile.ssid, ptr, WIFI_STRING_LEN);
                break;
            case 2:
                strncpy(profile.password, ptr, WIFI_STRING_LEN);
                break;
            case 3:
                if (memcmp("sta", ptr, 3) == 0 || memcmp("STA", ptr, 3) == 0) {
                    profile.opmode = WIFIOPMODE_STA;
                }
            case 4:
                strncpy(profile.guid, ptr, 17);
                break;
            default:
                break;
        }
        ptr = strtok(NULL, delim);
        i++;
    }
    if (profile_num > 0 && profile_num <= WIFIPROFILE_LIMIT) {
        if (profile.ssid[0] != 0) {
            wifiprofile_writeProfile(profile_num, &profile);
            stream->printf("WiFi profile %d written\r\n", profile_num);
        }
        else {
            wifiprofile_deleteProfile(profile_num);
            stream->printf("WiFi profile %d deleted\r\n", profile_num);
        }
    }
    else {
        stream->printf("ERROR: WiFi profile %d write disallowed\r\n", profile_num);
    }
    redraw_flag = true;
}

void dumpwifi_func(void* cmd, char* argstr, Stream* stream)
{
    pwr_tick(true);
    int i;
    stream->printf("WiFi Profile Dump:\r\n");
    for (i = 0; i <= WIFIPROFILE_LIMIT; i++)
    {
        wifiprofile_t profile;
        memset(&profile, 0, sizeof(wifiprofile_t));
        if (wifiprofile_getProfile(i, &profile)) {
            stream->printf("%d,%s,%s,%s,%s\r\n", i, profile.ssid, profile.password, profile.opmode == WIFIOPMODE_STA ? "sta" : "ap", profile.guid);
        }
    }
    stream->printf("\r\n");
}

void wifipwr_func(void* cmd, char* argstr, Stream* stream)
{
    pwr_tick(true);
    int8_t x;
    int ret;
    if (strlen(argstr) == 0) {
        ret = (int)esp_wifi_get_max_tx_power(&x);
        stream->printf("WiFi get pwr %d , return code %d\r\n", x, ret);
    }
    else {
        x = atoi(argstr);
        ret = (int)esp_wifi_set_max_tx_power(x);
        stream->printf("WiFi set pwr %d , return code %d\r\n", x, ret);
    }
}

void listlog_func(void* cmd, char* argstr, Stream* stream)
{
    File root = ALFY_FS.open("/");
    File file = root.openNextFile();
    stream->println("listing log files");
    while(file)
    {
        if (memcmp(file.name(), "pwrlog_", 7) == 0)
        {
            stream->print("file: ");
            stream->println(file.name());
        }
        file = root.openNextFile();
    }
    stream->println("end of file listing");
}

void readlog_func(void* cmd, char* argstr, Stream* stream)
{
    File f = ALFY_FS.open(argstr);
    stream->println();
    while (f.available() > 0) {
        stream->write((uint8_t)(f.read()));
    }
    f.close();
    stream->println();
}

void readhex_func(void* cmd, char* argstr, Stream* stream)
{
    int i = 0;
    File f = ALFY_FS.open(argstr);
    stream->println();
    while (f.available() > 0) {
        stream->printf("0x%02X, ", (uint8_t)(f.read()));
        i++;
        if ((i % 16) == 0) {
            stream->printf("\r\n");
        }
    }
    f.close();
    stream->println();
}

void pmiclog_func(void* cmd, char* argstr, Stream* stream)
{
    pmic_startCoulombCount();
    stream->println("coulomb count started");
}

#endif
