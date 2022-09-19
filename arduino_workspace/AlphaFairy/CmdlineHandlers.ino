#include "AlphaFairy.h"
#include <SerialCmdLine.h>

/*
handles command line commands
this is used only for testing
*/

void factory_reset_func(void* cmd, char* argstr, Stream* stream);
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
#endif

const cmd_def_t cmds[] = {
  { "factoryreset", factory_reset_func},
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
    M5.Axp.GetBatVoltage(),
    M5.Axp.GetBatCurrent(),
    M5.Axp.GetVBusVoltage(),
    M5.Axp.GetVBusCurrent()
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

#endif
