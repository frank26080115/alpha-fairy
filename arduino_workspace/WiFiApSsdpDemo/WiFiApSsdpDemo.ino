#include "WiFi.h"
#include "AsyncUDP.h"

const char * ssid     = "afairywifi";
const char * password = "1234567890";

AsyncUDP udp;

void on_udp_packet(AsyncUDPPacket packet)
{
  Serial.print("Got UDP Packet! Type: ");
  Serial.print(packet.isBroadcast()?"Broadcast":packet.isMulticast()?"Multicast":"Unicast");
  Serial.print(", From: ");
  Serial.print(packet.remoteIP());
  Serial.print(":");
  Serial.print(packet.remotePort());
  Serial.print(", To: ");
  Serial.print(packet.localIP());
  Serial.print(":");
  Serial.print(packet.localPort());
  Serial.print(", Length: ");
  Serial.print(packet.length());
  Serial.print(", Data: ");
  Serial.write(packet.data(), packet.length());
  Serial.println();
}

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

  if (udp.listen(1900)) {
    Serial.print("UDP Listening on IP: ");
    Serial.println(WiFi.localIP());
    udp.onPacket(on_udp_packet);
  }
}

void loop() {
  // put your main code here, to run repeatedly:

}
