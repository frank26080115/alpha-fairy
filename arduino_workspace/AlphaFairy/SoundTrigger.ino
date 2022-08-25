#include "AlphaFairy.h"
#include <M5StickCPlus.h>
#include <driver/i2s.h>

#define PIN_I2S_CLK     0
#define PIN_I2S_DATA    34
#define MICTRIG_READ_LEN 256
#define MICTRIG_GAIN 2

#define MICTRIG_LEVEL_BAR_HEIGHT   8
#define MICTRIG_LEVEL_TRIG_HEIGHT 12

uint8_t mictrig_buffer8[MICTRIG_READ_LEN * 2] = {0};

volatile int16_t* mictrig_buffer16;
volatile bool mictrig_active = false;
volatile int32_t mictrig_lastMax = 0;
volatile int32_t mictrig_filteredMax = 0;
volatile int32_t mictrig_decay = 0;
volatile bool mictrig_hasTriggered = false;

bool gui_microphoneActive = false;
uint32_t mictrig_ignoreTime = 0; // ignore the trigger if it's happening too soon after a button click

const configitem_t mictrig_config[] = {
  // item pointer                               ,  max , min , step , text            , flags
  { (int32_t*)&(config_settings.mictrig_level  ),   100,    1,     1, "Trigger Level" , TXTFMT_NONE },
  { (int32_t*)&(config_settings.mictrig_delay  ), 10000,    0,     1, "Start Delay"   , TXTFMT_TIME },
  { NULL, 0, 0, 0, "" }, // end of table
};

#ifdef MICTRIG_NEW_I2S_LIB
i2s_chan_handle_t mictrig_i2shandle;
static IRAM_ATTR bool mictrig_rx_cb(i2s_chan_handle_t handle, i2s_event_data_t *event, void *user_ctx);
#endif

void mictrig_init()
{
    #ifdef MICTRIG_NEW_I2S_LIB
    i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_AUTO, I2S_ROLE_MASTER);
    i2s_new_channel(&chan_cfg, NULL, &mictrig_i2shandle);
    i2s_std_config_t std_cfg = {
        .clk_cfg = I2S_PDM_RX_CLK_DEFAULT_CONFIG(36000),
        .slot_cfg = I2S_PDM_RX_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_MONO),
        .gpio_cfg = {
            .mclk = I2S_GPIO_UNUSED,
            .bclk = I2S_GPIO_UNUSED,
            .ws = GPIO_NUM_0,
            .dout = I2S_GPIO_UNUSED,
            .din = GPIO_NUM_34,
            .invert_flags = {
                .mclk_inv = false,
                .bclk_inv = false,
                .ws_inv = false,
            },
        },
    };

    i2s_channel_init_pdm_rx_mode(mictrig_i2shandle, &pdm_rx_cfg);

    i2s_event_callbacks_t cbs = {
        .on_recv = mictrig_rx_cb,
        .on_recv_q_ovf = NULL,
        .on_sent = NULL,
        .on_send_q_ovf = NULL,
    };
    i2s_channel_register_event_callback(mictrig_i2shandle, &cbs, NULL);

    #else

    // code adapted from https://github.com/m5stack/M5StickC/blob/master/examples/Basics/Micophone/Micophone.ino
    i2s_config_t i2s_config = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX | I2S_MODE_PDM), // Set the I2S operating mode
        .sample_rate     = 38000,                                           // Set the I2S sampling rate
        .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,                       // Fixed 12-bit stereo MSB
        .channel_format  = I2S_CHANNEL_FMT_ALL_RIGHT,                       // Set the channel format
#if ESP_IDF_VERSION > ESP_IDF_VERSION_VAL(4, 1, 0)
        .communication_format = I2S_COMM_FORMAT_STAND_I2S,                  // Set the format of the communication
#else
        .communication_format = I2S_COMM_FORMAT_I2S,
#endif
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,                           // Set the interrupt flag
        .dma_buf_count    = 2,                                              // DMA buffer count
        .dma_buf_len      = 128,                                            // DMA buffer length
    };

    i2s_pin_config_t pin_config;

#if (ESP_IDF_VERSION > ESP_IDF_VERSION_VAL(4, 3, 0))
    pin_config.mck_io_num = I2S_PIN_NO_CHANGE;
#endif

    pin_config.bck_io_num   = I2S_PIN_NO_CHANGE;
    pin_config.ws_io_num    = PIN_I2S_CLK;
    pin_config.data_out_num = I2S_PIN_NO_CHANGE;
    pin_config.data_in_num  = PIN_I2S_DATA;

    i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL);
    i2s_set_pin(I2S_NUM_0, &pin_config);
    i2s_set_clk(I2S_NUM_0, 38000, I2S_BITS_PER_SAMPLE_16BIT, I2S_CHANNEL_MONO);

    mictrig_buffer16 = (int16_t*)mictrig_buffer8;

    #endif
}

#ifdef MICTRIG_NEW_I2S_LIB
static IRAM_ATTR bool mictrig_rx_cb(i2s_chan_handle_t handle, i2s_event_data_t *event, void *user_ctx)
{
    mictrig_buffer16 = (int16_t*)(event->data);
    uint32_t sz      = (uint32_t)(event->size);
    uint32_t i;
    int32_t m = 0;
    for (i = 0; i < sz / 2; i++) {
        int16_t s = mictrig_buffer16[i];
        s = (s < 0) ? -s : s;
        if (s > m) {
            m = s;
        }
    }

    mictrig_decayTask();

    m *= MICTRIG_GAIN;           // apply gain
    m = m > 0x7FFF ? 0x7FFF : m; // limit after gain

    mictrig_lastMax = m; // remember the latest peak

    if (m > mictrig_filteredMax)
    {
        mictrig_decay = 64 * 4;     // slow down the decay
        mictrig_filteredMax = m; // set the new displayed peak
    }

    // check if triggered
    int32_t thresh = config_settings.mictrig_level * 327; // re-map value 0-100 to actual level
    if (m > thresh) {
        mictrig_hasTriggered = true;
    }

    return false;
}

#endif

void mictrig_unpause()
{
    static bool has_init = false;
    if (has_init == false) {
        mictrig_init();
        has_init = true;
    }
    #ifdef MICTRIG_NEW_I2S_LIB
    i2s_channel_enable(mictrig_i2shandle);
    #else
    i2s_start(I2S_NUM_0);
    #endif
}

void mictrig_pause()
{
    #ifdef MICTRIG_NEW_I2S_LIB
    i2s_channel_disable(mictrig_i2shandle);
    #else
    i2s_stop(I2S_NUM_0);
    #endif
}

void mictrig_poll()
{
    #ifndef MICTRIG_NEW_I2S_LIB
    size_t bytesread;
    uint32_t tstart = millis();
    for (uint8_t iter = 0; iter < 100 && (millis() - tstart) < 100; iter++)
    {
        i2s_read(I2S_NUM_0, (uint8_t *)mictrig_buffer8, MICTRIG_READ_LEN * 2, &bytesread, 0);

        uint32_t i;
        int32_t m = 0;
        for (i = 0; i < bytesread / 2; i++) {
            int16_t s = mictrig_buffer16[i];
            s = (s < 0) ? -s : s;
            if (s > m) {
                m = s;
            }
        }

        if (bytesread <= 0) {
            break;
        }

        m *= MICTRIG_GAIN;           // apply gain
        m = m > 0x7FFF ? 0x7FFF : m; // limit after gain

        mictrig_decayTask();

        mictrig_lastMax = m; // remember the latest peak

        if (m > mictrig_filteredMax)
        {
            mictrig_decay = 64 * 4;     // slow down the decay
            mictrig_filteredMax = m; // set the new displayed peak
        }

        // check if triggered
        int32_t thresh = config_settings.mictrig_level * 327; // re-map value 0-100 to actual level
        if (m > thresh) {
            mictrig_hasTriggered = true;
        }
    }

    #endif
}

void mictrig_decayTask()
{
    // decay the displayed peak
    if (mictrig_filteredMax >= mictrig_decay) {
        mictrig_filteredMax -= mictrig_decay;
    }
    // accelerate the decay
    if (mictrig_decay <= 0xFFF) {
        mictrig_decay += 32 * 3;
    }
}

void sound_shutter(void* mip)
{
    dbg_ser.println("sound_shutter");

    mictrig_lastMax = 0;
    mictrig_filteredMax = 0;

    menuitem_t* menuitm = (menuitem_t*)mip;
    menustate_t* m = &menustate_soundshutter;
    configitem_t* ctable = (configitem_t*)mictrig_config;

    m->idx = 0;
    m->last_idx = -1;
    for (m->cnt = 0; ; m->cnt++) {
        configitem_t* itm = &(ctable[m->cnt]);
        if (itm->ptr_val == NULL) {
            break;
        }
    }
    m->cnt++; // one more for the back option

    gui_startAppPrint();
    M5Lcd.fillScreen(TFT_BLACK);
    mictrig_drawIcon();
    app_waitAllRelease(BTN_DEBOUNCE);

    mictrig_unpause();
    gui_microphoneActive = true;
    mictrig_ignoreTime = millis();

    while (true)
    {
        app_poll();
        mictrig_poll();

        if (redraw_flag) {
            redraw_flag = false;
            gui_startAppPrint();
            M5Lcd.fillScreen(TFT_BLACK);
            mictrig_drawIcon();
        }

        if (btnSide_hasPressed())
        {
            m->idx = (m->idx >= m->cnt) ? 0 : (m->idx + 1);
            M5Lcd.fillScreen(TFT_BLACK); // item has changed so clear the screen
            mictrig_drawIcon();
            redraw_flag = false;
            mictrig_hasTriggered = false;
            mictrig_ignoreTime = millis();
            btnSide_clrPressed();
        }
        #if defined(USE_PWR_BTN_AS_BACK) && !defined(USE_PWR_BTN_AS_EXIT)
        if (btnPwr_hasPressed())
        {
            m->idx = (m->idx <= 0) ? m->cnt : (m->idx - 1);
            M5Lcd.fillScreen(TFT_BLACK); // item has changed so clear the screen
            mictrig_drawIcon();
            redraw_flag = false;
            mictrig_hasTriggered = false;
            mictrig_ignoreTime = millis();
            btnPwr_clrPressed();
        }
        #endif

        if (m->idx != m->cnt - 1) // not on the armed trigger screen
        {
            if (mictrig_hasTriggered) {
                // turn on the light if it is loud
                pwr_tick(true);
                // do not reset mictrig_hasTriggered
            }
        }

        configitem_t* cfgitm = (configitem_t*)&(config_items[m->idx]);
        if (m->idx == m->cnt) // last item is the exit item
        {
            M5Lcd.setCursor(SUBMENU_X_OFFSET, MICTRIG_LEVEL_MARGIN);
            M5Lcd.setTextFont(4);
            M5Lcd.print("Exit");
            gui_drawBackIcon();
            if (btnBig_hasPressed())
            {
                settings_save();
                gui_microphoneActive = false;
                btnBig_clrPressed();
                return;
            }
            mictrig_hasTriggered = false;
        }
        else if (m->idx == m->cnt - 1) // 2nd to last item is the start/stop item
        {
            M5Lcd.setCursor(SUBMENU_X_OFFSET, MICTRIG_LEVEL_MARGIN);
            M5Lcd.setTextFont(4);
            M5Lcd.print("ARMED");
            gui_blankRestOfLine();
            M5Lcd.println();
            gui_setCursorNextLine();
            gui_drawGoIcon();
            M5Lcd.setTextFont(4);
            M5Lcd.print("Level: ");
            gui_showVal(config_settings.mictrig_level, TXTFMT_NONE, (Print*)&M5Lcd);
            gui_blankRestOfLine();
            M5Lcd.println();
            gui_setCursorNextLine();
            M5Lcd.print("Delay: ");
            M5Lcd.print(config_settings.mictrig_delay, DEC);
            M5Lcd.print("s");
            gui_blankRestOfLine();

            if (btnBig_hasPressed())
            {
                mictrig_ignoreTime = millis();
                btnBig_clrPressed();
            }

            if (mictrig_hasTriggered)
            {
                // ignore the trigger if it's happening too soon after a button click
                if ((millis() - mictrig_ignoreTime) < 1000) {
                    mictrig_hasTriggered = false;
                }
            }

            if (mictrig_hasTriggered)
            {
                mictrig_hasTriggered = false;
                pwr_tick(true);
                ledblink_setMode(LEDMODE_OFF);
                mictrig_shoot(mip);
                ledblink_setMode(LEDMODE_NORMAL);
                gui_startAppPrint();
                M5Lcd.fillScreen(TFT_BLACK);
                mictrig_drawIcon();
                app_waitAllRelease(BTN_DEBOUNCE);
                pwr_tick(true);
                mictrig_hasTriggered = false;
                mictrig_ignoreTime = millis();
                btnAll_clrPressed();
            }
        }
        else
        {
            configitem_t* cfgitm = (configitem_t*)&(ctable[m->idx]);
            // first line shows name of item, second line shows the value
            M5Lcd.setCursor(SUBMENU_X_OFFSET, MICTRIG_LEVEL_MARGIN);
            M5Lcd.setTextFont(4);
            M5Lcd.print(cfgitm->text);
            M5Lcd.println();
            gui_setCursorNextLine();
            gui_valIncDec(cfgitm);
            gui_blankRestOfLine();
            mictrig_hasTriggered = false;
        }

        #ifdef USE_PWR_BTN_AS_EXIT
        if (btnPwr_hasPressed())
        {
            btnPwr_clrPressed();
            gui_microphoneActive = false;
            return;
        }
        #endif

        mictrig_drawLevel();
        mictrig_drawIcon();
    }
    gui_microphoneActive = false;
    mictrig_pause();
}

void mictrig_shoot(void* mip)
{
    M5Lcd.setCursor(SUBMENU_X_OFFSET, MICTRIG_LEVEL_MARGIN);
    M5Lcd.fillRect(0, MICTRIG_LEVEL_MARGIN, M5Lcd.width() - 60, M5Lcd.height() - MICTRIG_LEVEL_MARGIN - 12, TFT_BLACK);
    mictrig_drawLevel();
    if (config_settings.mictrig_delay > 0)
    {
        uint32_t now = millis();
        int32_t twait = config_settings.mictrig_delay;
        if (intervalometer_wait(twait, now, 0, "MIC TRIG'ed", config_settings.mictrig_delay > 2, 0))
        {
            return;
        }
    }
    else
    {
        M5Lcd.setCursor(SUBMENU_X_OFFSET, MICTRIG_LEVEL_MARGIN);
        M5Lcd.setTextFont(4);
        M5Lcd.print("MIC TRIG'ed");
    }
    remote_shutter(mip);
    mictrig_ignoreTime = millis();
    mictrig_lastMax = 0;
    mictrig_filteredMax = 0;
}

void mictrig_drawIcon()
{
    gui_drawStatusBar(true);
    #ifdef USE_SPRITE_MANAGER
    sprites->draw(
    #else
    M5Lcd.drawPngFile(SPIFFS,
    #endif
        "/mic_icon.png", M5Lcd.width() - 60, M5Lcd.height() - 60
    #ifdef USE_SPRITE_MANAGER
        , 60, 60
    #endif
        );
}

TFT_eSprite* miclevel_canvas = NULL;

void mictrig_drawLevel()
{
    if (miclevel_canvas == NULL) {
        miclevel_canvas = new TFT_eSprite(&M5Lcd);
        miclevel_canvas->createSprite(M5Lcd.width() - 60, MICTRIG_LEVEL_MARGIN);
    }

    uint32_t lvl = (mictrig_filteredMax + 90) / 182;
    lvl = (lvl < MICTRIG_LEVEL_BAR_HEIGHT) ? MICTRIG_LEVEL_BAR_HEIGHT : lvl;
    lvl = (lvl > 180) ? 180 : lvl;
    volatile uint32_t thresh = config_settings.mictrig_level;
    thresh *= 318;
    thresh /= 180;
    thresh = (thresh < MICTRIG_LEVEL_BAR_HEIGHT) ? MICTRIG_LEVEL_BAR_HEIGHT : thresh;

    miclevel_canvas->fillSprite(TFT_BLACK);
    miclevel_canvas->fillRect(0     , 0, lvl, MICTRIG_LEVEL_BAR_HEIGHT , TFT_RED  );
    miclevel_canvas->fillRect(thresh, 0, 3  , MICTRIG_LEVEL_TRIG_HEIGHT, TFT_GREEN);
    miclevel_canvas->pushSprite(0, 0);
}
