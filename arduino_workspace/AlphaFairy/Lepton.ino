#include "AlphaFairy.h"
#include "FairyMenu.h"

#ifdef ENABLE_BUILD_LEPTON

#include <Lepton.h>
#include <FairyEncoder.h>

typedef struct
{
    float data;
    int16_t increment;
    bool sw;
}
lepton_encoder_t;

//#define ENABLE_LEPTON_HISTOGRAM
//#define ENABLE_LEPTON_SCALING
//#define ENABLE_LEPTON_COLOURS

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
const uint8_t lepenc_addr = 0x30;
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
#ifdef ENABLE_LEPTON_HISTOGRAM
uint16_t lepton_histoBuff[64] = {0};
#endif
uint8_t lepenc_errCnt = 0;
bool lepton_success = false;
bool lepton_enable_poll = true;
volatile uint8_t lepton_initStage = 0;
uint32_t lepton_initMidTime = 0;
uint32_t lepton_lastPollTime = 0;
uint32_t lepton_startTime = 0;
int lepton_trigBar    = 0;
int lepton_trigThresh = 0;

#ifdef ENABLE_BUILD_LEPTON_THREAD
volatile bool lepton_isPolling = false;
volatile bool lepton_isShowing = false;
#endif

int lepton_saveNum = 0;

extern int32_t trigger_source;

/** @brief  Read the encoder wheel
  */
void lepton_encRead(bool* sw, int16_t* inc, int16_t* rem)
{
    (*sw) = false;
    (*inc) = 0;
    if (lepenc_errCnt > 3) {
        return;
    }
    uint8_t err;
    Wire1.beginTransmission(lepenc_addr); Wire1.write(0x10); err = Wire1.endTransmission();
    if (err != 0) {
        lepenc_errCnt++;
        return;
    }
    Wire1.requestFrom(lepenc_addr, (uint8_t)1);
    while (Wire1.available() < 1) {
        app_poll();
    }
    (*sw) = Wire1.read() != 0;
    if ((*sw))
    {
        pwr_tick(true);
        lepton_encClear();
    }
    else
    {
        Wire1.beginTransmission(lepenc_addr); Wire1.write(0x00); Wire1.endTransmission();
        Wire1.requestFrom(lepenc_addr, (uint8_t)2);
        while (Wire1.available() < 2) {
            app_poll();
        }
        uint8_t* ptr = (uint8_t*)inc;
        ptr[1] = Wire1.read();
        ptr[0] = Wire1.read();
        if ((*inc) != 0)
        {
            pwr_tick(true);

            int16_t x = (*inc);
            int16_t absx = x < 0 ? -x : x;

            if ((absx % 2) == 0) {
                lepton_encClear();
            }
            else {
                (*inc) = 0;
            }
            if (rem != NULL) {
                (*rem) = x < 0 ? (-absx % 2) : (absx % 2);
            }
        }
    }
}

void lepton_encClear()
{
    Wire1.beginTransmission(lepenc_addr); Wire1.write(0x20); Wire1.write(0xFF); Wire1.endTransmission();
}

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
                if (index > 255) {
                    index = 255;
                }
                #ifdef ENABLE_LEPTON_HISTOGRAM
                lepton_histoBuff[(index >> 2)]++;
                #endif
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
                if (index > 255) {
                    index = 255;
                }
                #ifdef ENABLE_LEPTON_HISTOGRAM
                lepton_histoBuff[(index >> 2)]++;
                #endif
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

    #ifdef ENABLE_LEPTON_HISTOGRAM
    TFT_eSprite* ib = lepton_imgBuff;
    #else
    M5DisplayExt* ib = &M5Lcd;
    #endif

    ib->drawLine(x + 1, y, x + w, y, TFT_WHITE);                 // -
    ib->drawLine(x, y + 1, x, y + h, TFT_WHITE);                 // |
    ib->drawLine(x + 1, y + h + 1, x + w, y + h + 1, TFT_WHITE); // _
    ib->drawLine(x + w + 1, y + 1, x + w + 1, y + h, TFT_WHITE); // |
    ib->drawLine(x + w + 3, y + 4, x + w + 3, y + h - 3, TFT_WHITE);
    ib->drawPixel(x + w + 2, y + 3, TFT_WHITE);
    ib->drawPixel(x + w + 2, y + h - 2, TFT_WHITE);

    float rate = (vol - 3.4) / (4.1 - 3.4);
    if (rate > 1.0)
    {
        ib->fillRect(x + 2, y + 2, w - 2, h - 2, TFT_GREEN);
    }
    else if (rate <= 0.05)
    {
        ib->drawLine(x + 2, y + 2, x + 2, y + h - 1, TFT_GREEN);
    }
    else
    {
        uint16_t bw = uint16_t(rate * (w - 2));
        ib->fillRect(x + 2, y + 2, bw, h - 2, TFT_GREEN);
        ib->fillRect(x + 2 + bw, y + 2, w - 2 - bw, h - 2, TFT_BLACK);
    }

    // draw the connection status too
    int16_t x2 = FLIR_X + 10;
    int16_t w2 = 12, w3 = 10, spc = 5;
    uint16_t antc = fairycam.isOperating() ? TFT_BLACK : TFT_RED;
    ib->drawLine(x2         , y, x2 + w2    , y         , antc);
    ib->drawLine(x2 + (w2/2), y, x2 + (w2/2), y + w3    , antc);
    ib->drawLine(x2         , y, x2 + (w2/2), y + (w2/2), antc);
    ib->drawLine(x2 + w2    , y, x2 + (w2/2), y + (w2/2), antc);
    ib->drawLine(x2 + w2 + spc, y     , x2 + w2 + spc + w3, y + w3, antc);
    ib->drawLine(x2 + w2 + spc, y + w3, x2 + w2 + spc + w3, y + 0 , antc);

    #ifndef ENABLE_LEPTON_SCALING
    if (lepton_saveNum > 0)
    {
        ib->setCursor(x2 - 5, 21);
        ib->printf("SAVED: %u", lepton_saveNum);
    }
    #endif
}

#ifndef ENABLE_LEPTON_SCALING
void lepton_saveImg()
{
    char fname[32] = {0};
    while (true)
    {
        if (lepton_saveNum == 0) {
            lepton_saveNum++;
        }
        sprintf(fname, "/flir_%u.bin", lepton_saveNum);
        if (SPIFFS.exists(fname) == false) {
            break;
        }
        else {
            lepton_saveNum++;
            continue;
        }
    }
    File f = SPIFFS.open(fname, FILE_WRITE);
    if (!f) {
        return;
    }
    dbg_ser.printf("saving FLIR img to %s\r\n", fname);
    f.write((uint8_t*)&fpa_temp, 2);
    f.write((uint8_t*)&raw_max, 2);
    f.write((uint8_t*)&raw_min, 2);
    uint8_t* ptr = (uint8_t*)smallBuffer;
    f.write(ptr, FLIR_X * FLIR_Y * 2);
    f.close();
    M5Lcd.fillRect(FLIR_X + 5, 20, 70, 60, TFT_BLACK);
}
#endif

/** @brief  Update encoder data
  */
void lepton_updateEncoder()
{
    #ifdef ENABLE_LEPTON_SCALING
    lepton_encRead(&lepton_enc_data.sw, &lepton_enc_data.increment, &NULL);

    if (lepton_enc_data.sw) // button press resets the data
    {
        lepton_enc_data.data = 0;
    }
    else
    {
        if (lepton_enc_data.increment != 0)
        {
            lepton_enc_data.data += ((lepton_enc_data.increment) * kEncoderTempStep);
        }
    }
    #endif
}

void lepton_makeFrameBuff()
{
    #ifdef ENABLE_LEPTON_HISTOGRAM
    int16_t w = M5Lcd.width(), h = M5Lcd.height();
    #else
    int16_t w = FLIR_X, h = FLIR_Y;
    #endif
    lepton_imgBuff = new TFT_eSprite(&M5Lcd);
    lepton_imgBuff->createSprite(w > h ? w : h, w > h ? h : w);
    lepton_imgBuff->setTextFont(0);
    //lepton_imgBuff->highlight(true);
    lepton_imgBuff->setTextWrap(true);
    //lepton_imgBuff->setHighlightColor(TFT_BLACK);
    lepton_imgBuff->setTextColor(TFT_WHITE);
}

#define GET_PIXEL_TEMPERATURE(_pixidx)     (0.0217f * smallBuffer[(_pixidx)] + ((fpa_temp / 100.0f) - 273.15f) - 177.77f)
#define GET_PIXEL_INDEX(_x, _y)            (((_y) * FLIR_X) + (_x))

/** @brief  Update frame
  */
void lepton_updateFlir(bool gui)
{
    pwr_tick(true);
    #ifndef ENABLE_BUILD_LEPTON_THREAD
    lepton.getRawValues();
    #endif
    #ifdef ENABLE_BUILD_LEPTON_THREAD
    if (lepton_isPolling) {
        return;
    }
    lepton_isShowing = true;
    #endif
    lepton_lastPollTime = millis();
    #ifdef ENABLE_LEPTON_SCALING
    lepton_updateEncoder();
    #else
    bool enc_sw; int16_t enc_inc;
    lepton_encRead(&enc_sw, &enc_inc, NULL);
    if (enc_sw) {
        lepton_saveImg();
    }
    #endif

    uint16_t i = 0, raw_cursor = raw_max;
    int32_t x, y;
    uint8_t index;
    float raw_diff = 0;

    if (gui && lepton_imgBuff == NULL) {
        gui = false;
    }

    if (gui)
    {
        if (lepton_imgBuff == NULL) {
            lepton_makeFrameBuff();
        }
        lepton_imgBuff->fillScreen(TFT_BLACK);
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
        #ifdef ENABLE_LEPTON_COLOURS
        //display mode switch
        switch (config_settings.lepton_dispmode)
        {
            case DISP_MODE_RGB:
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
        #else
        lepton_dispImg(raw_diff, raw_cursor, colormap_cam, dir_flag);
        #endif
    }

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

    #ifdef ENABLE_LEPTON_HISTOGRAM
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
        case DISP_MODE_RGB:
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
    }
    #endif
    if (gui) {
        //Setting info
        lepton_imgBuff->setTextDatum(TC_DATUM);
        float bat_voltage = M5.Axp.GetBatVoltage();
        lepton_dispBatt(214, 4, bat_voltage);
    }

    if (gui) {
        lepton_imgBuff->pushSprite(0, 0);
    }

    #ifdef ENABLE_BUILD_LEPTON_THREAD
    lepton_isShowing = false;
    #endif
}

bool lepton_checkTrigger()
{
    if (config_settings.lepton_trigmode == THERMTRIG_OFF) {
        return false;
    }
    if (lepton_initStage != LEPINIT_DONE) {
        return false;
    }
    #ifdef ENABLE_BUILD_LEPTON_THREAD
    if (lepton_isPolling) {
        return false;
    }
    lepton_isShowing = true;
    #endif
    int16_t x, y;
    int16_t cx = FLIR_X / 2;
    int16_t cy = FLIR_Y / 2;
    int16_t sx = cx - ((config_settings.lepton_trigzone + 1) / 2);
    int16_t sy = cy - ((config_settings.lepton_trigzone + 1) / 2);
    int16_t ex = cx + ((config_settings.lepton_trigzone + 1) / 2);
    int16_t ey = cy + ((config_settings.lepton_trigzone + 1) / 2);
    sx = sx < 0 ? 0 : sx;
    sy = sy < 0 ? 0 : sy;
    ex = ex >= FLIR_X ? FLIR_X : ex;
    ey = ey >= FLIR_Y ? FLIR_Y : ey;
    bool found = false;
    bool foundlim = false;
    int limtemp = 0;
    for (x = sx; x < ex; x++)
    {
        for (y = sy; y < ey; y++)
        {
            float t = GET_PIXEL_TEMPERATURE(GET_PIXEL_INDEX(x, y));
            int tt = lround(t);
            if ((foundlim == false) || (config_settings.lepton_trigmode == THERMTRIG_HOT && t > limtemp) || (config_settings.lepton_trigmode == THERMTRIG_COLD && t < limtemp)) {
                limtemp = t;
                foundlim = true;
            }
            if ((config_settings.lepton_trigmode == THERMTRIG_HOT && tt >= config_settings.lepton_trigtemp)
                || (config_settings.lepton_trigmode == THERMTRIG_COLD && tt <= config_settings.lepton_trigtemp)
            ) {
                found |= true;
            }
        }
    }

    #ifdef ENABLE_BUILD_LEPTON_THREAD
    lepton_isShowing = false;
    #endif

    if (foundlim)
    {
        lepton_trigBar    = map(lround(limtemp * 100.0)              , 0,  6000, 10, 170);
        lepton_trigThresh = map(config_settings.lepton_trigtemp * 100, 0,  6000, 10, 170);
    }

    return found;
}

#ifdef ENABLE_BUILD_LEPTON_THREAD
void lepton_task(void* obj_ptr)
{
    while (true)
    {
        if (lepton_isShowing) {
            delay(1);
            continue;
        }
        lepton_isPolling = true;
        lepton_poll(true);
        lepton_isPolling = false;
        delay(40);
    }
}

void lepton_threadStart()
{
    static TaskHandle_t task;
    static bool task_started = false;
    if (task_started) {
        return;
    }
    xTaskCreatePinnedToCore(
                            lepton_task,
                            "lepton_task",
                            5000,
                            NULL,
                            2,
                            &task,
                            xPortGetCoreID()
                        );
}
#endif

bool lepton_init()
{
    if (lepton_initStage == LEPINIT_RST_1)
    {
        pinMode(LEPTON_RESET_PIN, OUTPUT);
        digitalWrite(LEPTON_RESET_PIN, HIGH);
        lepton_initMidTime = millis();
        lepton_initStage = LEPINIT_RST_2;
    }
    else if (lepton_initStage == LEPINIT_RST_2)
    {
        if ((millis() - lepton_initMidTime) >= 100)
        {
            digitalWrite(LEPTON_RESET_PIN, LOW);
            lepton_initMidTime = millis();
            lepton_initStage = LEPINIT_RST_3;
        }
    }
    else if (lepton_initStage == LEPINIT_RST_3)
    {
        if ((millis() - lepton_initMidTime) >= 300)
        {
            digitalWrite(LEPTON_RESET_PIN, HIGH);
            lepton_initMidTime = millis();
            lepton_initStage = LEPINIT_RST_4;
        }
    }
    else if (lepton_initStage == LEPINIT_RST_4)
    {
        if ((millis() - lepton_initMidTime) >= 50)
        {
            lepton_initStage = LEPINIT_BEGIN;
        }
    }
    else if (lepton_initStage == LEPINIT_BEGIN)
    {
        if (lepton.begin())
        {
            dbg_ser.println("lepton begin");
            lepton_success = true;
            lepton_initStage = LEPINIT_SYNC;
        }
        else
        {
            Serial.println("lepton begin failed");
            lepton_success = false;
            lepton_initStage = LEPINIT_FAIL;
        }
    }
    else if (lepton_initStage == LEPINIT_SYNC)
    {
        lepton.syncFrame();
        dbg_ser.println("lepton syncFrame");
        #if 0
        lepton_initMidTime = millis();
        lepton_initStage = LEPINIT_CMD;
        #else
        delay(1000); // using millis() and scheduling doesn't seem to work as well as using delay()
        uint16_t SYNC = 5, DELAY = 3;
        lepton.doSetCommand(lepton.CMD_OEM_SYNC_SET , &SYNC, 1);
        lepton.doSetCommand(lepton.CMD_OEM_DELAY_SET, &DELAY, 1);
        lepton.end();
        lepton_initStage = LEPINIT_DONE;
        dbg_ser.println("lepton done");
        #endif
    }
    else if (lepton_initStage == LEPINIT_CMD)
    {
        if ((millis() - lepton_initMidTime) >= 1000)
        {
            uint16_t SYNC = 5, DELAY = 3;
            lepton.doSetCommand(lepton.CMD_OEM_SYNC_SET , &SYNC , 1);
            lepton.doSetCommand(lepton.CMD_OEM_DELAY_SET, &DELAY, 1);
            lepton.end();
            uint32_t t = millis();
            if ((t - lepton_initMidTime) > 1500)
            {
                lepton_initStage = LEPINIT_RST_1;
                dbg_ser.println("lepton reattempt init");
            }
            else
            {
                lepton_initStage = LEPINIT_DONE;
                dbg_ser.println("lepton doSetCommand");
            }
        }
    }
    else if (lepton_initStage == LEPINIT_DONE)
    {
        lepton_poll(false);
    }
    return lepton_success;
}

void lepton_poll(bool init)
{
    if (lepton_enable_poll == false) {
        return;
    }

    if (lepton_initStage != LEPINIT_DONE) {
        if (init) {
            lepton_init();
        }
        return;
    }
    lepton.getRawValues();
    lepton_lastPollTime = millis();
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
                lepton_makeFrameBuff();
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
            lepton_encClear();
        };

        virtual void on_eachFrame(void)
        {
            #ifdef ENABLE_BUILD_LEPTON_THREAD
            if (lepton_isPolling) {
                return;
            }
            #endif

            lepton_updateFlir(true);

            #ifdef ENABLE_BUILD_LEPTON_TRIGGER_SIMPLE
            if ((millis() - lepton_startTime) < 5000) {
                return;
            }

            bool trig = lepton_checkTrigger();

            if (trig) {
                draw_borderRect(3, TFT_RED);
                pwr_tick(true);
                cam_shootQuick();
            }
            #endif
        };

        virtual void on_redraw(void)
        {
            M5Lcd.fillScreen(TFT_BLACK);
            #ifndef ENABLE_BUILD_LEPTON_THREAD
            lepton_updateFlir(true);
            #endif
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
        PageLeptonMesMode() : FairyCfgItem("Meas. Mode", (int32_t*)&(config_settings.lepton_measmode), 0, 2, 1, TXTFMT_NONE)
        {
            this->_margin_y = MICTRIG_LEVEL_MARGIN; this->_autosave = false;
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

#ifdef ENABLE_LEPTON_COLOURS
class PageLeptonDispMode : public FairyCfgItem
{
    public:
        PageLeptonDispMode() : FairyCfgItem("Disp. Mode", (int32_t*)&(config_settings.lepton_dispmode), 0, 4, 1, TXTFMT_NONE)
        {
            this->_margin_y = MICTRIG_LEVEL_MARGIN; this->_autosave = false;
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
                case DISP_MODE_RGB:
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
#endif

class PageLeptonTrigger : public FairyCfgItem
{
    public:
        PageLeptonTrigger(const char* disp_name, int32_t* linked_var, int32_t val_min, int32_t val_max, int32_t step_size, uint32_t fmt_flags) : FairyCfgItem(disp_name, linked_var, val_min, val_max, step_size, fmt_flags)
        { this->_margin_y = MICTRIG_LEVEL_MARGIN; this->_autosave = true; };
        PageLeptonTrigger(const char* disp_name, bool (*cb)(void*), const char* icon) : FairyCfgItem(disp_name, cb, icon)
        { this->_margin_y = MICTRIG_LEVEL_MARGIN; this->_autosave = false; };

        virtual void on_drawLive (void)
        {
            if (this->get_parentId() == MENUITEM_TRIGGER) {
                trigger_drawLevel();
            }
            else {
                lepton_drawLevel();
            }
        };

        virtual void on_extraPoll(void) {
            if (this->get_parentId() == MENUITEM_TRIGGER) {
                trigger_all_poll();
            }
            else {
                lepton_checkTrigger();
            }
        };
};

class PageLeptonTriggerMode : public PageLeptonTrigger
{
    public:
        PageLeptonTriggerMode() : PageLeptonTrigger("Trigger Mode", (int32_t*)&(config_settings.lepton_trigmode), 0, 2, 1, TXTFMT_NONE)
        {
        };

        virtual bool can_navTo(void)
        {
            if (this->get_parentId() == MENUITEM_TRIGGER) {
                if (trigger_source != TRIGSRC_ALL && trigger_source != TRIGSRC_THERMAL) {
                    return false;
                }
            }
            return true;
        };

        virtual void on_redraw(void)
        {
            if (this->get_parentId() == MENUITEM_TRIGGER) {
                if (config_settings.lepton_trigmode == THERMTRIG_OFF) {
                    config_settings.lepton_trigmode = THERMTRIG_HOT;
                }
            }
            M5Lcd.fillScreen(TFT_BLACK);
            FairyCfgItem::on_redraw();
            draw_info();
        };

        virtual void on_readjust(void)
        {
            if (config_settings.lepton_trigmode < THERMTRIG_OFF) {
                config_settings.lepton_trigmode = THERMTRIG_OFF;
            }
            FairyCfgItem::on_readjust();
            draw_info();
        };

    protected:
        void draw_info(void)
        {
            M5Lcd.setTextFont(4);
            M5Lcd.setCursor(_margin_x, get_y(2));
            if (config_settings.lepton_trigmode < THERMTRIG_OFF) {
                config_settings.lepton_trigmode = THERMTRIG_OFF;
            }
            switch (config_settings.lepton_trigmode)
            {
                case THERMTRIG_OFF:
                    M5Lcd.print("Off");
                    break;
                case THERMTRIG_HOT:
                    M5Lcd.print("Hot Obj");
                    break;
                case THERMTRIG_COLD:
                    M5Lcd.print("Cold Obj");
                    break;
                default:
                    M5Lcd.setTextFont(2);
                    M5Lcd.print("Trig Mode Unknown");
                    M5Lcd.setTextFont(4);
                    break;
            }
            blank_line();
        };
};

class PageLeptonTriggerTemp : public PageLeptonTrigger
{
    public:
        PageLeptonTriggerTemp() : PageLeptonTrigger("Trig Temp.", (int32_t*)&(config_settings.lepton_trigtemp), 0, 200, 1, TXTFMT_BYTENS)
        {
        };

        virtual bool can_navTo(void)
        {
            if (config_settings.lepton_trigmode == THERMTRIG_OFF) {
                return false;
            }
            if (this->get_parentId() == MENUITEM_TRIGGER) {
                if (trigger_source != TRIGSRC_ALL && trigger_source != TRIGSRC_THERMAL) {
                    return false;
                }
            }
            return true;
        };

        virtual void on_redraw(void)
        {
            M5Lcd.fillScreen(TFT_BLACK);
            FairyCfgItem::on_redraw();
        };
};

class PageLeptonTriggerZone : public PageLeptonTrigger
{
    public:
        PageLeptonTriggerZone() : PageLeptonTrigger("Trig Zone", (int32_t*)&(config_settings.lepton_trigzone), 2, FLIR_X > FLIR_Y ? FLIR_X : FLIR_Y, 1, TXTFMT_BYTENS)
        {
        };

        virtual bool can_navTo(void)
        {
            if (config_settings.lepton_trigmode == THERMTRIG_OFF) {
                return false;
            }
            if (this->get_parentId() == MENUITEM_TRIGGER) {
                if (trigger_source != TRIGSRC_ALL && trigger_source != TRIGSRC_THERMAL) {
                    return false;
                }
            }
            return true;
        };

        virtual void on_redraw(void)
        {
            M5Lcd.fillScreen(TFT_BLACK);
            FairyCfgItem::on_redraw();
        };
};

class PageLeptonExit : public FairyCfgItem
{
    public:
        PageLeptonExit() : FairyCfgItem("Exit", lepton_nullFunc, "/back_icon.png")
        {
            this->_margin_y = MICTRIG_LEVEL_MARGIN; this->_autosave = false;
        };

        virtual void on_redraw(void)
        {
            M5Lcd.fillScreen(TFT_BLACK);
            FairyCfgItem::on_redraw();
        };

        virtual bool on_execute(void)
        {
            return true; // causes exit
        };
};

class AppLepton : public FairyCfgApp
{
    public:
        AppLepton() : FairyCfgApp("/main_lepton.png", "/lepton_icon.png", MENUITEM_LEPTON)
        {
            install(new PageLeptonImage());
            #ifdef ENABLE_LEPTON_COLOURS
            install(new PageLeptonDispMode());
            #endif
            install(new PageLeptonMesMode());
            #ifdef ENABLE_BUILD_LEPTON_TRIGGER_SIMPLE
            install(new PageLeptonTriggerMode());
            install(new PageLeptonTriggerTemp());
            install(new PageLeptonTriggerZone());
            #endif
            install(new PageLeptonExit());
        };

        virtual bool on_execute(void)
        {
            M5Lcd.fillScreen(TFT_DARKGREY);
            while (lepton_initStage < LEPINIT_DONE)
            {
                #ifndef ENABLE_BUILD_LEPTON_THREAD
                lepton_init();
                #endif
                app_poll();
            }
            if (lepton_initStage == LEPINIT_DONE) {
                lepton_startTime = millis();
                bool r = FairyCfgApp::on_execute();
                return r;
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

void install_lepton_trigger(FairyCfgApp* parent)
{
    #ifdef ENABLE_BUILD_LEPTON_TRIGGER_COMPLEX
    parent->install(new PageLeptonTriggerMode());
    parent->install(new PageLeptonTriggerTemp());
    parent->install(new PageLeptonTriggerZone());
    #endif
}

#endif
