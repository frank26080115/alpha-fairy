#include <M5StickC.h>
#include <M5DisplayExt.h>
#include <FS.h>
#include <SPIFFS.h>

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  M5.begin(false);
  M5Lcd.begin();
  if(!SPIFFS.begin(true)){
      Serial.println("SPIFFS Mount Failed");
      return;
  }
}

void loop() {
  // put your main code here, to run repeatedly:
  M5Lcd.drawJpgFile(SPIFFS, "/welcome.jpg", 0, 0);
  delay(2000);
  M5Lcd.drawBmpFile(SPIFFS, "/wifiinfo.bmp", 0, 0);
  delay(2000);
  M5Lcd.drawPngFile(SPIFFS, "/connecting.png", 0, 0);
  delay(2000);
}
