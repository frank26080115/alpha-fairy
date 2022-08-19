#include <FairyKeyboard.h>
#include <M5StickCPlus.h>

#define PIN_BTN_SIDE 39
#define PIN_BTN_BIG 37
FairyKeyboard kbd(&(M5.Lcd));

char input_str[64] = {0};

void draw_header()
{
    M5.Lcd.setTextColor(TFT_BLACK, TFT_WHITE);
    M5.Lcd.setTextFont(2);
    M5.Lcd.setCursor(5, 4);
    M5.Lcd.print("SSID: ");
    if (input_str[0] == 0) {
        M5.Lcd.print("[empty]");
    }
    else {
        M5.Lcd.print(input_str);
    }
    M5.Lcd.fillRect(M5.Lcd.getCursorX(), M5.Lcd.getCursorY(), M5.Lcd.width() - M5.Lcd.getCursorX(), M5.Lcd.fontHeight(), TFT_WHITE);
    M5.Lcd.setCursor(5, 18);
    M5.Lcd.print("Password:");
}

void setup()
{
    Serial.begin(115200);
    M5.begin(false); // do not initialize the LCD, we have our own extended M5Lcd class to initialize later
    M5.IMU.Init();
    M5.IMU.SetGyroFsr(M5.IMU.GFS_500DPS);
    M5.IMU.SetAccelFsr(M5.IMU.AFS_4G);
    M5.Axp.begin();
    M5.Axp.ScreenSwitch(false); // turn off the LCD backlight while initializing, avoids junk being shown on the screen
    M5.Lcd.begin(); // our own extended LCD object
    M5.Lcd.fillScreen(TFT_WHITE);
    M5.Axp.ScreenBreath(12);

    pinMode(PIN_BTN_SIDE, INPUT_PULLUP);
    pinMode(PIN_BTN_BIG, INPUT_PULLUP);

    kbd.register_redraw_cb(draw_header);

    kbd.reset();
    kbd.draw_base();
}

void loop()
{
    static bool prev_big_btn = false;
    static bool prev_side_btn = false;
    static uint32_t last_imu_time = 0;
    uint32_t now = millis();

    float roll, pitch, yaw;

    if ((now - last_imu_time) >= 40)
    {
        M5.IMU.getAhrsData(&pitch, &roll, &yaw);
        kbd.update(roll, pitch);
        last_imu_time = now;
    }

    if (digitalRead(PIN_BTN_BIG) == LOW)
    {
        if (prev_big_btn == false)
        {
            if (kbd.click())
            {
                strcpy(input_str, kbd.get_str());
                kbd.clr_str();
                draw_header();
            }
        }
        prev_big_btn = true;
    }
    else
    {
        prev_big_btn = false;
    }

    if (digitalRead(PIN_BTN_SIDE) == LOW)
    {
        if (prev_side_btn == false)
        {
            kbd.toggleLowerCase();
        }
        prev_side_btn = true;
    }
    else
    {
        prev_side_btn = false;
    }
}


