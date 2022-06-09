#include <SonyCameraInfraredRemote.h>

bool continuous = false;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  SonyCamIr_Init();
}

void loop() {
  // put your main code here, to run repeatedly:
  static uint32_t last_heartbeat = 0;
  uint32_t now;
  if (((now = millis()) - last_heartbeat) >= 1000) {
    Serial.println("hi");
    last_heartbeat = now;
  }
  while (Serial.available() > 0)
  {
    char c = Serial.read();
    if (c == 's') {
      Serial.println("shoot now");
      SonyCamIr_Shoot();
      continuous = false;
    }
    else if (c == 'S') {
      Serial.println("shoot continuous");
      continuous = true;
    }
    else if (c == '2') {
      Serial.println("shutter 2s");
      SonyCamIr_Shoot2S();
      continuous = false;
    }
    else if (c == 'm') {
      Serial.println("movie");
      SonyCamIr_Movie();
      continuous = false;
    }
    else if (c == '\r' || c == '\n') {
      // ignore
    }
    else {
      Serial.println("unknown command");
    }
  }
  if (continuous) {
    SonyCamIr_Shoot();
  }
}
