#include <FairyKeyboard.h>
#include <M5Unified.h>
#include <M5DisplayExt.h>
#include <math.h>

#define PIN_BTN_SIDE 39
#define PIN_BTN_BIG 37
FairyKeyboard kbd(&M5Lcd);

char input_str[64] = {0};

void draw_header()
{
    M5Lcd.setTextColor(TFT_BLACK, TFT_WHITE);
    M5Lcd.setTextFont(2);
    M5Lcd.setCursor(5, 4);
    M5Lcd.print("SSID: ");
    if (input_str[0] == 0) {
        M5Lcd.print("[empty]");
    }
    else {
        M5Lcd.print(input_str);
    }
    M5Lcd.fillRect(M5Lcd.getCursorX(), M5Lcd.getCursorY(), M5Lcd.width() - M5Lcd.getCursorX(), M5Lcd.fontHeight(), TFT_WHITE);
    M5Lcd.setCursor(5, 18);
    M5Lcd.print("Password:");
}

void setup()
{
    Serial.begin(115200);
    auto cfg = M5.config();
    cfg.clear_display = false;
    cfg.internal_imu = true;
    cfg.internal_mic = false;
    cfg.internal_spk = false;
#if defined(M5GFX_BOARD)
    cfg.fallback_board = static_cast<m5::board_t>(m5gfx::M5GFX_BOARD);
#endif
    M5.begin(cfg);

    M5.Display.setBrightness(0);
    M5Lcd.begin();
    M5Lcd.setBrightness(M5LCD_BRIGHTNESS_MAX);
    M5Lcd.fillScreen(TFT_WHITE);

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

    float roll = 0, pitch = 0;

    if ((now - last_imu_time) >= 40)
    {
        float ax, ay, az;
        if (M5.Imu.getAccelData(&ax, &ay, &az)) {
            roll = atan2f(ay, az) * RAD_TO_DEG;
            pitch = atan2f(-ax, sqrtf((ay * ay) + (az * az))) * RAD_TO_DEG;
            kbd.update(roll, pitch);
        }
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


