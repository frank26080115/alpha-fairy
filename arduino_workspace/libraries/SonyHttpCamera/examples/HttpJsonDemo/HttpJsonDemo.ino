#include <SonyHttpCamera.h>

SonyHttpCamera httpcam;
int wifi_last_status;

void setup()
{
    Serial.begin(115200);
    WiFi.mode(WIFI_STA);
    wifi_last_status = WiFi.status(); // remember pre-init status
    WiFi.begin("DIRECT-W9C2:DSC-RX100M4", "UsmyDXRY"); // connect to camera
    Serial.println("Hello World. HTTP JSON Demo. WiFi initialized, waiting for IP...");
}

void loop()
{
    httpcam.task();

    int status = WiFi.status();
    if (status == WL_CONNECTED)
    {
        IPAddress gateway = WiFi.gatewayIP();
        IPAddress localIp = WiFi.localIP();
        if (gateway != 0 && localIp != 0 && wifi_last_status != status && httpcam.canNewConnect()) // new connect
        {
            wifi_last_status = status;
            Serial.print("WiFi Connected: ");
            Serial.println(gateway);
            httpcam.begin((uint32_t)gateway); // start the handshake
        }
    }
    else
    {
        wifi_last_status = status;
    }

    if (Serial.available()) // check if user has input command from serial port
    {
        char c = Serial.read();
        if (c == 'x') // this is a command from serial port
        {
            Serial.println("Command: Shoot Photo");
            if (httpcam.isOperating()) // check connection
            {
                httpcam.cmd_Shoot(); // execute the command
            }
            else
            {
                Serial.println("Error: Camera Not Connected");
            }
        }
        else if (c != '\r' && c != '\n')
        {
            Serial.printf("Unknown Command: %c\r\n", c);
        }
    }
}
