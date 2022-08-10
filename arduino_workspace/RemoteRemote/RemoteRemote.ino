#include <WiFi.h>
#include <HTTPClient.h>
#include <M5StickCPlus.h>
#include <M5DisplayExt.h>

char root_url[256];
bool connected = false;
HTTPClient http;
char url[256];

void setup()
{
    btns_init();
    Serial.begin(115200);
    Serial.println("Hello World RemoteRemote");
    M5.begin(false);
    M5.Axp.begin();
    M5.Axp.ScreenSwitch(false); // turn off the LCD backlight while initializing, avoids junk being shown on the screen
    M5Lcd.begin(); // our own extended LCD object
    M5Lcd.fillScreen(TFT_BLACK);
    while (!SPIFFS.begin(true)){
        Serial.println("SPIFFS Mount Failed");
        delay(500);
    }
    M5.Axp.ScreenBreath(11);
    M5Lcd.setRotation(0);
    M5Lcd.drawPngFile(SPIFFS, "/connecting.png", 0, 0);

    WiFi.mode(WIFI_STA);
    WiFi.begin("fairywifi", "1234567890");
    Serial.println("WIFI initialized and attempting to connect");
    btnAll_clrPressed();
}

void loop()
{
    if (WiFi.status() == WL_CONNECTED)
    {
        if (WiFi.gatewayIP() != IPAddress((uint32_t)0))
        {
            if (connected == false)
            {
                Serial.println("WIFI connected");
                M5Lcd.drawPngFile(SPIFFS, "/remote.png", 0, 0);
                sprintf(root_url, "http://%s/", WiFi.gatewayIP().toString().c_str());
                Serial.print("root URL: ");
                Serial.println(root_url);
                connected = true;
            }
        }
        else {
            connected = false;
        }
    }
    else
    {
        connected = false;
    }

    btnPwr_poll();

    if (btnSide_hasPressed())
    {
        send_press("side");
        btnSide_clrPressed();
    }
    if (btnBig_hasPressed())
    {
        send_press("big");
        btnBig_clrPressed();
    }
    if (btnPwr_hasPressed())
    {
        send_press("pwr");
        btnPwr_clrPressed();
    }
}

void send_press(const char* btn)
{
    Serial.print("press ");
    Serial.print(btn);
    sprintf(url, "%sbtn%s", root_url, btn);
    http.begin(url);
    http.GET();
    http.end();
    Serial.print(" ");
    Serial.println(url);
}
