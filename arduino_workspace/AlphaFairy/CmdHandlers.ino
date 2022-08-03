#include "AlphaFairy.h"
#include <SerialCmdLine.h>

/*
handles command line commands
this is used only for testing
*/

#ifndef DISABLE_CMD_LINE
void shoot_func     (void* cmd, char* argstr, Stream* stream);
void echo_func      (void* cmd, char* argstr, Stream* stream);
void memcheck_func  (void* cmd, char* argstr, Stream* stream);
void statscheck_func(void* cmd, char* argstr, Stream* stream);
void reboot_func    (void* cmd, char* argstr, Stream* stream);
void imu_func       (void* cmd, char* argstr, Stream* stream);
void imushow_func   (void* cmd, char* argstr, Stream* stream);
void pwr_func       (void* cmd, char* argstr, Stream* stream);
void debug_func     (void* cmd, char* argstr, Stream* stream);
void camdebug_func  (void* cmd, char* argstr, Stream* stream);
void infrared_func  (void* cmd, char* argstr, Stream* stream);
#endif

const cmd_def_t cmds[] = {
  #ifndef DISABLE_CMD_LINE
  { "shoot"    , shoot_func },
  { "echo"     , echo_func },
  { "mem"      , memcheck_func },
  { "imu"      , imu_func },
  { "imushow"  , imushow_func },
  { "pwr"      , pwr_func },
  { "stats"    , statscheck_func },
  { "reboot"   , reboot_func },
  { "debug"    , debug_func },
  { "camdebug" , camdebug_func },
  { "ir"       , infrared_func },
  #endif
  { "", NULL }, // end of table
};

SerialCmdLine cmdline(&Serial, (cmd_def_t*)cmds, false, (char*)">>>", (char*)"???", true, 512);

#ifndef DISABLE_CMD_LINE

void shoot_func(void* cmd, char* argstr, Stream* stream)
{
  pwr_tick();
  if (camera.getState() == PTPSTATE_POLLING)
  {
    stream->println("shoot");
    camera.cmd_Shoot(250);
  }
  else
  {
    stream->println("camera not connected");
  }
}

void echo_func(void* cmd, char* argstr, Stream* stream)
{
  pwr_tick();
  stream->println(argstr);
}

void reboot_func(void* cmd, char* argstr, Stream* stream)
{
  stream->println("rebooting...\r\n\r\n");
  ESP.restart();
}

void memcheck_func(void* cmd, char* argstr, Stream* stream)
{
  pwr_tick();
  stream->printf("free heap mem: %u\r\n", ESP.getFreeHeap());
}

void statscheck_func(void* cmd, char* argstr, Stream* stream)
{
  pwr_tick();
  #ifdef PTPIP_KEEP_STATS
  stream->printf("ptpipcam stats: %u  %u  %u\r\n", camera.stats_tx, camera.stats_acks, camera.stats_pkts);
  #else
  stream->printf("ptpipcam stats not available\r\n");
  #endif
}

void imu_func(void* cmd, char* argstr, Stream* stream)
{
  pwr_tick();
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
        pwr_tick();
        stream->printf("imu:   %0.1f   %0.1f   %0.1f\r\n", imu.pitch, imu.roll, imu.yaw);
        M5Lcd.setCursor(SUBMENU_X_OFFSET, SUBMENU_Y_OFFSET);
        M5Lcd.printf("%0.1f  ,  %0.1f", imu.roll, imu.pitch); gui_blankRestOfLine(); M5Lcd.println(); gui_setCursorNextLine();
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
        if (btnBig_hasPressed(true) || btnSide_hasPressed(true) || M5.Axp.GetBtnPress() != 0) {
            ESP.restart();
        }
    }
}

void pwr_func(void* cmd, char* argstr, Stream* stream)
{
  pwr_tick();
  stream->printf("pwr:   %0.3f   %0.3f   %0.3f   %0.3f\r\n",
    M5.Axp.GetBatVoltage(),
    M5.Axp.GetBatCurrent(),
    M5.Axp.GetVBusVoltage(),
    M5.Axp.GetVBusCurrent()
    );
}

extern DebuggingSerial dbg_ser;
void debug_func(void* cmd, char* argstr, Stream* stream)
{
    pwr_tick();
    dbg_ser.enabled = atoi(argstr) != 0;
    stream->printf("debugging output = %u\r\n", dbg_ser.enabled);
    dbg_ser.println("test output from debugging serial port");
}

void camdebug_func(void* cmd, char* argstr, Stream* stream)
{
    pwr_tick();
    int x = atoi(argstr);
    camera.set_debugflags(x);
    stream->printf("camera debugging output = %u\r\n", x);
    camera.test_debug_msg("test from cam debug serport\r\n");
}

void infrared_func(void* cmd, char* argstr, Stream* stream)
{
    pwr_tick();
    stream->printf("infrared test fire\r\n");
    SonyCamIr_Shoot();
}

#endif
