#include <PtpIpCamera.h>
#include <PtpIpSonyAlphaCamera.h>
#include <AlphaFairy_NetMgr.h>
#include <SerialCmdLine.h>
#include <M5StickC.h>

PtpIpSonyAlphaCamera camera((char*)"Alpha-Fairy");

void on_got_client(uint32_t ip);

void shoot_func     (void* cmd, char* argstr, Stream* stream);
void echo_func      (void* cmd, char* argstr, Stream* stream);
void memcheck_func  (void* cmd, char* argstr, Stream* stream);
void statscheck_func(void* cmd, char* argstr, Stream* stream);

cmd_def_t cmds[] = {
  { "shoot", shoot_func },
  { "echo" , echo_func },
  { "mem"  , memcheck_func },
  { "stats", statscheck_func },
  { NULL, NULL }, // end of table
};

SerialCmdLine cmdline(&Serial, (cmd_def_t*)cmds, false, (char*)">>>", (char*)"???", true, 512);

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  M5.begin(false);
  pinMode(10, OUTPUT);
  digitalWrite(10, HIGH);
  cmdline.print_prompt();
  NetMgr_begin((char*)"afairywifi", (char*)"1234567890", on_got_client);
}

void loop() {
  // put your main code here, to run repeatedly:
  NetMgr_task();
  camera.task();
  cmdline.task();
  if (camera.getState() >= PTPSTATE_DISCONNECTED) {
    NetMgr_reset();
  }
}

void on_got_client(uint32_t ip)
{
  if (camera.canNewConnect())
  {
    camera.begin(ip);
  }
}

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
