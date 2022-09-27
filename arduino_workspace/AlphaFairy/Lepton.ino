#include "AlphaFairy.h"
#include "FairyMenu.h"

#ifdef ENABLE_BUILD_LEPTON

#include <Lepton.h>
#include <FairyEncoder.h>

enum
{
    MES_AUTO_MAX = 0,
    MES_AUTO_MIN,
    MES_CENTER,
    DISP_MODE_CAM = 0,
    DISP_MODE_GRAY,
    DISP_MODE_GOLDEN,
    DISP_MODE_RAINBOW,
    DISP_MODE_IRONBLACK,
};

typedef struct
{
    float data;
    int16_t increment;
    bool sw;
}
lepton_encoder_t;

#define MAX_FLIR_RAW_BUFFER (FLIR_X * FLIR_Y - 1)

#define FLIR_WINDOW_X1 5
#define FLIR_WINDOW_Y1 4
#define FLIR_WINDOW_X2 (FLIR_WINDOW_X1 + FLIR_X)
#define FLIR_WINDOW_Y2 (FLIR_WINDOW_Y1 + FLIR_Y)
#define FLIR_WINDOW_CENTER_X (FLIR_WINDOW_X1 + FLIR_X / 2)
#define FLIR_WINDOW_CENTER_Y (FLIR_WINDOW_Y1 + FLIR_Y / 2)

#define HIST_HEIGHT 70
#define HIST_WINDOWS_X1 171
#define HIST_WINDOWS_Y1 34
#define HIST_WINDOWS_X2 (HIST_WINDOWS_X1 + 64)
#define HIST_WINDOWS_Y2 (HIST_WINDOWS_Y1 + HIST_HEIGHT)

#define MES_MODE_X 171
#define MES_MODE_Y 8

#define DISP_MODE_X 171
#define DISP_MODE_Y 25

Lepton lepton(21, 22, 0, 38);
extern uint16_t fpa_temp, aux_temp;
extern const uint16_t camColors[];
extern const uint16_t GrayLevel[];
extern uint16_t smallBuffer[FLIR_X * FLIR_Y];
extern uint16_t raw_max, raw_min;
extern uint16_t max_x, max_y, min_x, min_y;
TFT_eSprite* lepton_imgBuff = NULL;
bool smallBuffer_Lock = false;
bool img_buffer_Lock = false;
lepton_encoder_t lepton_enc_data = {0};
const float kEncoderTempStep = 0.05f;
uint16_t lepton_histoBuff[64] = {0};
//extern bool fenc_enabled;

/** @brief  Draws temperature focus cursor
  * @param  coordinates
  */
void lepton_dispCursor(uint16_t x, uint16_t y)
{
    lepton_imgBuff->drawCircle(x, y, 6, TFT_WHITE);
    lepton_imgBuff->drawLine(x, y - 10, x, y + 10, TFT_WHITE);
    lepton_imgBuff->drawLine(x - 10, y, x + 10, y, TFT_WHITE);
}

/** @brief  Draws a pseudo-color image & Creat data pack to RemoteImgTransfer
  * @param  raw_diff Quantization step
  * @param  raw_cursor Temperature range
  * @param  palette False color palette
  * @param  dir_flag Temperature range direction selection, H->L or L->H
  */
void lepton_dispImg(float raw_diff, uint16_t raw_cursor, const uint16_t *palette, bool dir_flag)
{
    uint16_t x, y, i = 0, b;
    uint16_t index = 0;

    if (dir_flag)
    {
        for (y = FLIR_WINDOW_Y1; y < FLIR_WINDOW_Y2; y++)
        {
            for (x = FLIR_WINDOW_X1; x < FLIR_WINDOW_X2; x++)
            {
                b = smallBuffer[i];
                if (b < raw_cursor)
                {
                    index = 0;
                }
                else
                {
                    index = (b - raw_cursor) * raw_diff;
                }
                if (index > 255)
                    index = 255;
                lepton_histoBuff[(index >> 2)]++;
                //if (palette != NULL)
                {
                    lepton_imgBuff->drawPixel(x, y, *(palette + index));
                }
                i++;
            }
        }
    }
    else
    {
        for (y = FLIR_WINDOW_Y1; y < FLIR_WINDOW_Y2; y++)
        {
            for (x = FLIR_WINDOW_X1; x < FLIR_WINDOW_X2; x++)
            {
                b = smallBuffer[i];
                if (b > raw_cursor)
                {
                    index = 255;
                }
                else
                {
                    index = (b - raw_min) * raw_diff;
                }
                if (index > 255)
                    index = 255;
                lepton_histoBuff[(index >> 2)]++;
                //if (palette != NULL)
                {
                    lepton_imgBuff->drawPixel(x, y, *(palette + index));
                }
                i++;
            }
        }
    }
}

/** @brief  Draw battery icon
  * @param  coordinates
  * @param  vol battery voltage
  */
void lepton_dispBatt(uint16_t x, uint16_t y, float vol)
{
    const uint8_t w = 18;
    const uint8_t h = 7;
    lepton_imgBuff->drawLine(x + 1, y, x + w, y, TFT_WHITE);                 // -
    lepton_imgBuff->drawLine(x, y + 1, x, y + h, TFT_WHITE);                 // |
    lepton_imgBuff->drawLine(x + 1, y + h + 1, x + w, y + h + 1, TFT_WHITE); // _
    lepton_imgBuff->drawLine(x + w + 1, y + 1, x + w + 1, y + h, TFT_WHITE); // |
    lepton_imgBuff->drawLine(x + w + 3, y + 4, x + w + 3, y + h - 3, TFT_WHITE);
    lepton_imgBuff->drawPixel(x + w + 2, y + 3, TFT_WHITE);
    lepton_imgBuff->drawPixel(x + w + 2, y + h - 2, TFT_WHITE);

    float rate = (vol - 3.4) / (4.1 - 3.4);
    if (rate > 1.0)
    {
        lepton_imgBuff->fillRect(x + 2, y + 2, w - 2, h - 2, TFT_GREEN);
    }
    else if (rate <= 0.05)
    {
        lepton_imgBuff->drawLine(x + 2, y + 2, x + 2, y + h - 1, TFT_GREEN);
    }
    else
    {
        lepton_imgBuff->fillRect(x + 2, y + 2, uint16_t(rate * (w - 2)), h - 2, TFT_GREEN);
    }

    // draw the connection status too
    int16_t x2 = FLIR_X + 10;
    int16_t w2 = 12, w3 = 10, spc = 5;
    if (fairycam.isOperating() == false) {
        lepton_imgBuff->drawLine(x2         , y, x2 + w2    , y         , TFT_RED);
        lepton_imgBuff->drawLine(x2 + (w2/2), y, x2 + (w2/2), y + w3    , TFT_RED);
        lepton_imgBuff->drawLine(x2         , y, x2 + (w2/2), y + (w2/2), TFT_RED);
        lepton_imgBuff->drawLine(x2 + w2    , y, x2 + (w2/2), y + (w2/2), TFT_RED);

        lepton_imgBuff->drawLine(x2 + w2 + spc, y     , x2 + w2 + spc + w3, y + w3, TFT_RED);
        lepton_imgBuff->drawLine(x2 + w2 + spc, y + w3, x2 + w2 + spc + w3, y + 0 , TFT_RED);
    }
}

/** @brief  Update encoder data
  * @param  pointer to lepton_encoder_t
  */
void lepton_updateEncoder()
{
    int16_t d;
    bool b = false;

    if (fencoder.avail())
    {
        fencoder.task();
        d = fencoder.read(true);
        lepton_enc_data.sw = fencoder.getButtonStatus();
    }
    else
    {
        // checks for reconnection
        fencoder.read(true);
        return;
    }

    if (lepton_enc_data.sw) // button press resets the data
    {
        lepton_enc_data.data = 0;
    }
    else
    {
        lepton_enc_data.increment = d;

        if (lepton_enc_data.increment != 0)
        {
            lepton_enc_data.data += ((lepton_enc_data.increment) * kEncoderTempStep);
        }
    }
}

#define GET_PIXEL_TEMPERATURE(_pixidx)     (0.0217f * smallBuffer[(_pixidx)] + ((fpa_temp / 100.0f) - 273.15f) - 177.77f)

/** @brief  Update frame
  */
void lepton_updateFlir(bool gui)
{
    pwr_tick(true);
    lepton.getRawValues();
    lepton_updateEncoder();
    uint16_t i = 0, raw_cursor = raw_max;
    int32_t x, y;
    uint8_t index;
    float raw_diff = 0;

    if (gui && lepton_imgBuff == NULL) {
        gui = false;
    }

    if (gui) {
        lepton_imgBuff->fillRect(0, 0, M5Lcd.width(), M5Lcd.height(), TFT_BLACK);
    }

    //convert temp
    float fpa_temp_f = fpa_temp / 100.0f - 273.15;
    float max_temp = 0.0217f * raw_max + fpa_temp_f - 177.77f;
    float min_temp = 0.0217f * raw_min + fpa_temp_f - 177.77f;
    float center_temp = GET_PIXEL_TEMPERATURE(9519);

    //The quantized step was calculated using the temperature range cursor
    float cursor_temp = max_temp;
    bool dir_flag = lepton_enc_data.data >= 0;
    if (dir_flag)
    {
        if (lepton_enc_data.data > 0.95)
        {
            lepton_enc_data.data = 0.95;
        }

        cursor_temp = min_temp + (max_temp - min_temp) * lepton_enc_data.data;
        raw_cursor = (cursor_temp + 177.77 - fpa_temp_f) / 0.0217f;
        raw_diff = 256.0f / (raw_max - raw_cursor);
    }
    else
    {
        if (lepton_enc_data.data < -0.95)
        {
            lepton_enc_data.data = -0.95;
        }

        cursor_temp = max_temp - ((max_temp - min_temp) * (-lepton_enc_data.data));
        raw_cursor = (cursor_temp + 177.77 - fpa_temp_f) / 0.0217f;
        raw_diff = 256.0f / (raw_cursor - raw_min);
    }

    max_x += FLIR_WINDOW_X1;
    max_y += FLIR_WINDOW_Y1;
    min_x += FLIR_WINDOW_X1;
    min_y += FLIR_WINDOW_Y1;

    if (gui)
    {
        //display mode switch
        switch (config_settings.lepton_dispmode)
        {
            case DISP_MODE_CAM:
                lepton_dispImg(raw_diff, raw_cursor, colormap_cam, dir_flag);
                break;

            case DISP_MODE_GRAY:
                lepton_dispImg(raw_diff, raw_cursor, colormap_grayscale, dir_flag);
                break;

            case DISP_MODE_GOLDEN:
                lepton_dispImg(raw_diff, raw_cursor, colormap_golden, dir_flag);
                break;

            case DISP_MODE_RAINBOW:
                lepton_dispImg(raw_diff, raw_cursor, colormap_rainbow, dir_flag);
                break;

            case DISP_MODE_IRONBLACK:
                lepton_dispImg(raw_diff, raw_cursor, colormap_ironblack, dir_flag);
                break;
        }
    }
    #if 0
    else
    {
        lepton_dispImg(raw_diff, raw_cursor, NULL, dir_flag);
    }
    #endif

    //measure mode switch
    switch (config_settings.lepton_measmode)
    {
        case MES_AUTO_MAX:
            if (gui) {
                lepton_dispCursor(max_x, max_y);
            }
            x = max_x + 5;
            y = max_y + 5;
            if (max_x > FLIR_WINDOW_X2 - 35)
                x = max_x - 35;
            if (max_y > FLIR_WINDOW_Y2 - 15)
                y = max_y - 15;
            if (gui) {
                lepton_imgBuff->setCursor(x, y);
                lepton_imgBuff->printf("%0.2f", max_temp);
            }
            break;

        case MES_AUTO_MIN:
            if (gui) {
                lepton_dispCursor(min_x, min_y);
            }
            x = min_x + 5;
            y = min_y + 5;
            if (min_x > FLIR_WINDOW_X2 - 35)
                x = min_x - 35;
            if (min_y > FLIR_WINDOW_Y2 - 15)
                y = min_y - 15;
            if (gui) {
                lepton_imgBuff->setCursor(x, y);
                lepton_imgBuff->printf("%.02f", min_temp);
            }
            break;

        case MES_CENTER:
            if (gui) {
                lepton_dispCursor(FLIR_WINDOW_CENTER_X, FLIR_WINDOW_CENTER_Y);
                lepton_imgBuff->setCursor(FLIR_WINDOW_CENTER_X + 5, FLIR_WINDOW_CENTER_Y + 5);
                lepton_imgBuff->printf("%0.2f", center_temp);
            }
            break;
    }

    //Histogram
    uint16_t max_hist = 0;
    for (i = 0; i < 64; i++)
    {
        if (lepton_histoBuff[i] > max_hist)
        {
            max_hist = lepton_histoBuff[i];
        }
    }

    uint16_t hist_div = max_hist / HIST_HEIGHT;

    i = 0;
    switch (config_settings.lepton_dispmode)
    {
        case DISP_MODE_CAM:
            for (x = HIST_WINDOWS_X1; x < HIST_WINDOWS_X2; x++)
            {
                if (gui) {
                    lepton_imgBuff->drawLine(x, HIST_WINDOWS_Y2, x, HIST_WINDOWS_Y2 - lepton_histoBuff[i] / hist_div, colormap_cam[i * 4]);
                }
                lepton_histoBuff[i] = 0;
                i++;
            }
            break;

        case DISP_MODE_GRAY:
            for (x = HIST_WINDOWS_X1; x < HIST_WINDOWS_X2; x++)
            {
                if (gui) {
                    lepton_imgBuff->drawLine(x, HIST_WINDOWS_Y2, x, HIST_WINDOWS_Y2 - lepton_histoBuff[i] / hist_div, colormap_grayscale[i * 4]);
                }
                lepton_histoBuff[i] = 0;
                i++;
            }
            break;

        case DISP_MODE_GOLDEN:
            for (x = HIST_WINDOWS_X1; x < HIST_WINDOWS_X2; x++)
            {
                if (gui) {
                    lepton_imgBuff->drawLine(x, HIST_WINDOWS_Y2, x, HIST_WINDOWS_Y2 - lepton_histoBuff[i] / hist_div, colormap_golden[i * 4]);
                }
                lepton_histoBuff[i] = 0;
                i++;
            }
            break;

        case DISP_MODE_RAINBOW:
            for (x = HIST_WINDOWS_X1; x < HIST_WINDOWS_X2; x++)
            {
                if (gui) {
                    lepton_imgBuff->drawLine(x, HIST_WINDOWS_Y2, x, HIST_WINDOWS_Y2 - lepton_histoBuff[i] / hist_div, colormap_rainbow[i * 4]);
                }
                lepton_histoBuff[i] = 0;
                i++;
            }
            break;

        case DISP_MODE_IRONBLACK:
            for (x = HIST_WINDOWS_X1; x < HIST_WINDOWS_X2; x++)
            {
                if (gui) {
                    lepton_imgBuff->drawLine(x, HIST_WINDOWS_Y2, x, HIST_WINDOWS_Y2 - lepton_histoBuff[i] / hist_div, colormap_ironblack[i * 4]);
                }
                lepton_histoBuff[i] = 0;
                i++;
            }
            break;
    }

    double bar_percentage = (double)(raw_cursor - raw_min) / (double)(raw_max - raw_min);
    uint8_t bar_len = bar_percentage * 64;
    if (gui)
    {
        lepton_imgBuff->drawRect(HIST_WINDOWS_X1, HIST_WINDOWS_Y2 + 5, 64, 4, TFT_WHITE);
        if (dir_flag)
        {
            lepton_imgBuff->fillRect(HIST_WINDOWS_X1 + bar_len, HIST_WINDOWS_Y2 + 5, 64 - bar_len, 4, TFT_WHITE);
        }
        else
        {
            lepton_imgBuff->fillRect(HIST_WINDOWS_X1, HIST_WINDOWS_Y2 + 5, bar_len, 4, TFT_WHITE);
        }

        lepton_imgBuff->setCursor(HIST_WINDOWS_X1, HIST_WINDOWS_Y2 + 12);
        lepton_imgBuff->printf("%.0f", min_temp);
        lepton_imgBuff->setCursor(HIST_WINDOWS_X2 - 12, HIST_WINDOWS_Y2 + 12);
        lepton_imgBuff->printf("%.0f", max_temp);

        //Setting info
        lepton_imgBuff->setTextDatum(TC_DATUM);
        float bat_voltage = M5.Axp.GetBatVoltage();
        lepton_dispBatt(214, 4, bat_voltage);
    }

    if (gui)
    {
        #if 0
        switch (config_settings.lepton_dispmode)
        {
            case DISP_MODE_CAM:
                lepton_imgBuff->drawString("RGB", HIST_WINDOWS_X1 + 20, 5);
                break;

            case DISP_MODE_GRAY:
                lepton_imgBuff->drawString("GRAY", HIST_WINDOWS_X1 + 20, 5);
                break;

            case DISP_MODE_GOLDEN:
                lepton_imgBuff->drawString("GOLDEN", HIST_WINDOWS_X1 + 20, 5);
                break;

            case DISP_MODE_RAINBOW:
                lepton_imgBuff->drawString("RAINBOW", HIST_WINDOWS_X1 + 20, 5);
                break;

            case DISP_MODE_IRONBLACK:
                lepton_imgBuff->drawString("IRON", HIST_WINDOWS_X1 + 20, 5);
                break;
        }
        #endif
    }

    if (gui) {
        lepton_imgBuff->pushSprite(0, 0);
    }
}

enum
{
    LEPINIT_BEGIN = 0,
    LEPINIT_SYNC,
    LEPINIT_CMD,
    LEPINIT_DONE,
    LEPINIT_FAIL,
};

bool lepton_success = false;
uint8_t lepton_initStage = 0;
uint32_t lepton_initMidTime = 0;
bool lepton_init()
{
    if (lepton_initStage == LEPINIT_BEGIN)
    {
        if (lepton.begin())
        {
            lepton_success = true;
            lepton_initStage = LEPINIT_SYNC;
        }
        else
        {
            Serial.println("lepton begin failed");
            lepton_initStage = LEPINIT_FAIL;
        }
    }
    else if (lepton_initStage == LEPINIT_SYNC)
    {
        lepton.syncFrame();
        lepton_initMidTime = millis();
        lepton_initStage = LEPINIT_CMD;
    }
    else if (lepton_initStage == LEPINIT_CMD)
    {
        if ((millis() - lepton_initMidTime) >= 1000)
        {
            uint16_t SYNC = 5, DELAY = 3;
            lepton.doSetCommand(0x4854, &SYNC, 1);
            lepton.doSetCommand(0x4858, &DELAY, 1);
            lepton.end();
            lepton_initStage = LEPINIT_DONE;
        }
    }
    return lepton_success;
}

bool lepton_nullFunc(void* x)
{
    return false;
}

class PageLeptonImage : public FairyCfgItem
{
    public:
        PageLeptonImage() : FairyCfgItem("", lepton_nullFunc)
        {
        };

        virtual void on_navTo(void)
        {
            if (lepton_imgBuff == NULL) {
                int16_t w = M5Lcd.width(), h = M5Lcd.height();
                lepton_imgBuff = new TFT_eSprite(&M5Lcd);
                lepton_imgBuff->createSprite(w > h ? w : h, w > h ? h : w);
                lepton_imgBuff->setTextFont(2);
                lepton_imgBuff->setTextColor(TFT_WHITE);
            }
            FairyCfgItem::on_navTo();
        };

        virtual void on_navOut(void)
        {
            if (lepton_imgBuff != NULL) {
                lepton_imgBuff->deleteSprite();
                delete lepton_imgBuff;
                lepton_imgBuff = NULL;
            }
        };

        virtual void on_eachFrame(void)
        {
            lepton_updateFlir(true);
        };

        virtual void on_redraw(void)
        {
            lepton_updateFlir(true);
        };

        virtual void draw_statusBar(void)
        {
            // do nothing, do not draw
        };

        virtual bool on_execute(void)
        {
            return false;
        };
};

class PageLeptonMesMode : public FairyCfgItem
{
    public:
        PageLeptonMesMode() : FairyCfgItem("Meas. Mode", (int32_t*)(&config_settings.lepton_measmode), 0, 2, 1, TXTFMT_NONE)
        {
        };

        virtual void on_redraw(void)
        {
            M5Lcd.fillScreen(TFT_BLACK);
            FairyCfgItem::on_redraw();
            draw_info();
        };

        virtual void on_readjust(void)
        {
            FairyCfgItem::on_readjust();
            draw_info();
        };

    protected:
        void draw_info(void)
        {
            M5Lcd.setTextFont(4);
            M5Lcd.setCursor(_margin_x, get_y(2));
            switch (config_settings.lepton_measmode)
            {
                case MES_AUTO_MAX:
                    M5Lcd.print("Max Temp");
                    break;
                case MES_AUTO_MIN:
                    M5Lcd.print("Min Temp");
                    break;
                case MES_CENTER:
                    M5Lcd.print("Center");
                    break;
                default:
                    M5Lcd.setTextFont(2);
                    M5Lcd.print("Meas Mode Unknown");
                    M5Lcd.setTextFont(4);
                    break;
            }
            blank_line();
        };
};

class PageLeptonDispMode : public FairyCfgItem
{
    public:
        PageLeptonDispMode() : FairyCfgItem("Disp. Mode", (int32_t*)(&config_settings.lepton_dispmode), 0, 4, 1, TXTFMT_NONE)
        {
        };

        virtual void on_redraw(void)
        {
            M5Lcd.fillScreen(TFT_BLACK);
            FairyCfgItem::on_redraw();
            draw_info();
        };

        virtual void on_readjust(void)
        {
            FairyCfgItem::on_readjust();
            draw_info();
        };

    protected:
        void draw_info(void)
        {
            M5Lcd.setTextFont(4);
            M5Lcd.setCursor(_margin_x, get_y(2));
            switch (config_settings.lepton_dispmode)
            {
                case DISP_MODE_CAM:
                    M5Lcd.print("RGB");
                    break;
                case DISP_MODE_GOLDEN:
                    M5Lcd.print("Golden");
                    break;
                case DISP_MODE_GRAY:
                    M5Lcd.print("Gray");
                    break;
                case DISP_MODE_IRONBLACK:
                    M5Lcd.print("Iron Black");
                    break;
                case DISP_MODE_RAINBOW:
                    M5Lcd.print("Rainbow");
                    break;
                default:
                    M5Lcd.setTextFont(2);
                    M5Lcd.print("Disp Mode Unknown");
                    M5Lcd.setTextFont(4);
                    break;
            }
            blank_line();
        };
};

class PageLeptonExit : public FairyCfgItem
{
    public:
        PageLeptonExit() : FairyCfgItem("Exit", lepton_nullFunc, "/back_icon.png")
        {
        };

        virtual bool on_execute(void)
        {
            return true; // causes exit
        };
};

class AppLepton : public FairyCfgApp
{
    public:
        AppLepton() : FairyCfgApp("/main_lepton.png", "/lepton_icon.png", 0)
        {
            install(new PageLeptonImage());
            install(new PageLeptonDispMode());
            install(new PageLeptonMesMode());
            install(new PageLeptonExit());
        };

        virtual bool can_navTo(void)
        {
            if (lepton_initStage == 0) {
                lepton_init();
            }
            return lepton_success;
        };

        virtual void on_eachFrame(void)
        {
            lepton_init();
        };

        virtual bool on_execute(void)
        {
            while (lepton_initStage < LEPINIT_DONE)
            {
                lepton_init();
                app_poll();
            }
            if (lepton_initStage == LEPINIT_DONE) {
                return FairyCfgApp::on_execute();
            }
            else {
                return false;
            }
        };
};

extern FairySubmenu main_menu;
void setup_leptonflir()
{
    static AppLepton app;
    main_menu.install(&app);
}

#endif
