#include "AlphaFairy.h"
#include <SerialCmdLine.h>

void shoot_func     (void* cmd, char* argstr, Stream* stream);
void echo_func      (void* cmd, char* argstr, Stream* stream);
void memcheck_func  (void* cmd, char* argstr, Stream* stream);
void statscheck_func(void* cmd, char* argstr, Stream* stream);
void reboot_func    (void* cmd, char* argstr, Stream* stream);
void imu_func       (void* cmd, char* argstr, Stream* stream);

const cmd_def_t cmds[] = {
  { "shoot" , shoot_func },
  { "echo"  , echo_func },
  { "mem"   , memcheck_func },
  { "imu"   , imu_func },
  { "stats" , statscheck_func },
  { "reboot", statscheck_func },
  { "", NULL }, // end of table
};

SerialCmdLine cmdline(&Serial, (cmd_def_t*)cmds, false, (char*)">>>", (char*)"???", true, 512);

void shoot_func(void* cmd, char* argstr, Stream* stream)
{
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
  stream->println(argstr);
}

void reboot_func(void* cmd, char* argstr, Stream* stream)
{
  stream->println("rebooting...\r\n\r\n");
  ESP.restart();
}

void memcheck_func(void* cmd, char* argstr, Stream* stream)
{
  stream->printf("free heap mem: %u\r\n", ESP.getFreeHeap());
}

void statscheck_func(void* cmd, char* argstr, Stream* stream)
{
  #ifdef PTPIP_KEEP_STATS
  stream->printf("ptpipcam stats: %u  %u  %u\r\n", camera.stats_tx, camera.stats_acks, camera.stats_pkts);
  #else
  stream->printf("ptpipcam stats not available\r\n");
  #endif
}

extern float imu_pitch, imu_roll, imu_yaw;
void imu_func(void* cmd, char* argstr, Stream* stream)
{
  stream->printf("imu:   %0.1f   %0.1f   %0.1f\r\n", imu_pitch, imu_roll, imu_yaw);
}