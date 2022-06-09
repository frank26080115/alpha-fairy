#include <WiFi.h>
#include <SoftApClientFinder.h>

const char * ssid     = "afairywifi";
const char * password = "1234567890";

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  while (Serial.available() <= 0) {
    Serial.println("waiting for user input to start");
    delay(500);
  }

  WiFi.softAP(ssid, password);
  IPAddress IP = WiFi.softAPIP();
  Serial.print("Wi-Fi AP IP address: ");
  Serial.println(IP);
}

void loop() {
  // put your main code here, to run repeatedly:
  uint8_t cnt;
  SoftApClient_Check();
  if ((cnt = SoftApClient_Count()) > 0) {
    Serial.print(millis());
    Serial.printf(": %u Clients\r\n", cnt);
    ip4_addr_t* ip = SoftApClient_First();
    Serial.print("IP: ");
    Serial.println(ip4addr_ntoa(ip));
    delay(1000);
  }
  else {
    Serial.print(millis());
    Serial.println(": No Clients");
    delay(1000);
  }
}
