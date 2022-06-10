#include <PtpIpCamera.h>
#include <PtpIpSonyAlphaCamera.h>
#include <AlphaFairy_NetMgr.h>

PtpIpSonyAlphaCamera camera((char*)"Alpha-Fairy");

void on_got_client(uint32_t ip);

void setup() {
  // put your setup code here, to run once:
  NetMgr_begin((char*)"afairywifi", (char*)"1234567890", on_got_client);
}

void loop() {
  // put your main code here, to run repeatedly:
  NetMgr_task();
  camera.task();
}

void on_got_client(uint32_t ip)
{
  if (camera.getState() == PTPSTATE_INIT || camera.getState() == PTPSTATE_DISCONNECTED)
  {
    camera.begin(ip);
  }
}
