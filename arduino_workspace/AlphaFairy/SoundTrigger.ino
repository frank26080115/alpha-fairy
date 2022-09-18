#include "AlphaFairy.h"
#include <M5StickCPlus.h>
#include <driver/i2s.h>

/*
NOTE: the newer I2S code in ESP-IDF has not made it into the Arduino ESP32 core yet
*/

#define PIN_I2S_CLK     0
#define PIN_I2S_DATA    34
#define MICTRIG_READ_LEN 256
#define MICTRIG_GAIN 2

#define MICTRIG_LEVEL_BAR_HEIGHT   8
#define MICTRIG_LEVEL_TRIG_HEIGHT 12

static bool mictrig_hasInit = false;

uint8_t mictrig_buffer8[MICTRIG_READ_LEN * 2] = {0};

volatile int16_t* mictrig_buffer16;
volatile int32_t mictrig_lastMax = 0;
volatile int32_t mictrig_filteredMax = 0;
volatile int32_t mictrig_decay = 0;
volatile bool mictrig_hasTriggered = false;
bool gui_microphoneActive = false;

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
    if (mictrig_hasInit == false) {
        mictrig_init();
        mictrig_hasInit = true;
    }
    #ifdef MICTRIG_NEW_I2S_LIB
    i2s_channel_enable(mictrig_i2shandle);
    #else
    i2s_start(I2S_NUM_0);
    #endif
    gui_microphoneActive = true;
}

void mictrig_pause()
{
    if (mictrig_hasInit == false) {
        return;
    }
    #ifdef MICTRIG_NEW_I2S_LIB
    i2s_channel_disable(mictrig_i2shandle);
    #else
    i2s_stop(I2S_NUM_0);
    #endif
    gui_microphoneActive = false;
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
            mictrig_decay = 64 * 4;  // slow down the decay
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

bool mictrig_shoot()
{
    M5Lcd.setCursor(SUBMENU_X_OFFSET, MICTRIG_LEVEL_MARGIN);
    M5Lcd.fillRect(0, MICTRIG_LEVEL_MARGIN, M5Lcd.width() - 60, M5Lcd.height() - MICTRIG_LEVEL_MARGIN - 12, TFT_BLACK);
    mictrig_drawLevel();
    if (config_settings.mictrig_delay > 0)
    {
        uint32_t now = millis();
        int32_t twait = config_settings.mictrig_delay;
        // wait with a countdown on the GUI
        if (intervalometer_wait(twait, now, 0, "MIC TRIG'ed", config_settings.mictrig_delay > 2, 0))
        {
            return true;
        }
    }
    else
    {
        M5Lcd.setCursor(SUBMENU_X_OFFSET, MICTRIG_LEVEL_MARGIN);
        M5Lcd.setTextFont(4);
        M5Lcd.print("MIC TRIG'ed");
        gui_blankRestOfLine();
    }

    // do the actual shutter command, depending on connectivity method
    cam_shootQuick();

    // reset the mic states
    mictrig_lastMax = 0;
    mictrig_filteredMax = 0;
    return false;
}

TFT_eSprite* miclevel_canvas = NULL;

void mictrig_drawLevel()
{
    if (miclevel_canvas == NULL) {
        miclevel_canvas = new TFT_eSprite(&M5Lcd);
        miclevel_canvas->createSprite(M5Lcd.width() - 60, MICTRIG_LEVEL_MARGIN);
    }

    // calculate the length of the red bar representing the current sound level peak
    uint32_t lvl = (mictrig_filteredMax + 90) / 182;
    lvl = (lvl < MICTRIG_LEVEL_BAR_HEIGHT) ? MICTRIG_LEVEL_BAR_HEIGHT : lvl;
    lvl = (lvl > 180) ? 180 : lvl;

    // calculate the position of the green tick representing the trigger level
    volatile uint32_t thresh = config_settings.mictrig_level;
    thresh *= 318;
    thresh /= 180;
    thresh = (thresh < MICTRIG_LEVEL_BAR_HEIGHT) ? MICTRIG_LEVEL_BAR_HEIGHT : thresh;

    miclevel_canvas->fillSprite(TFT_BLACK);
    miclevel_canvas->fillRect(0     , 0, lvl, MICTRIG_LEVEL_BAR_HEIGHT , TFT_RED  );
    miclevel_canvas->fillRect(thresh, 0, 3  , MICTRIG_LEVEL_TRIG_HEIGHT, TFT_GREEN);
    miclevel_canvas->pushSprite(0, 0);
}

// parent class that handles checking the mic samples and drawing the level bar
class PageSoundTrigger : public FairyCfgItem
{
    public:
        PageSoundTrigger(const char* disp_name, int32_t* linked_var, int32_t val_min, int32_t val_max, int32_t step_size, uint16_t fmt_flags) : FairyCfgItem(disp_name, linked_var, val_min, val_max, step_size, fmt_flags)
        { this->_margin_y = MICTRIG_LEVEL_MARGIN; this->_autosave = true; };
        PageSoundTrigger(const char* disp_name, bool (*cb)(void*), const char* icon) : FairyCfgItem(disp_name, cb, icon)
        { this->_margin_y = MICTRIG_LEVEL_MARGIN; this->_autosave = true; };

        virtual void on_drawLive (void) { mictrig_drawLevel(); };
        virtual void on_extraPoll(void) { mictrig_poll(); };
};

class PageSoundTriggerLevel : public PageSoundTrigger
{
    public:
        PageSoundTriggerLevel() : PageSoundTrigger("Trigger Level", (int32_t*)&(config_settings.mictrig_level), 0, 100, 1, TXTFMT_NONE) {
        };
        virtual void on_redraw (void) { mictrig_unpause(); FairyCfgItem::on_redraw(); };
};

class PageSoundTriggerDelay : public PageSoundTrigger
{
    public:
        PageSoundTriggerDelay() : PageSoundTrigger("Start Delay", (int32_t*)&(config_settings.mictrig_delay), 0, 1000, 1, TXTFMT_TIME) {
        };
};

// called via button press, button should do nothing, triggering is done by sound
bool mictrig_nullfunc(void* ptr)
{
    return false;
}

class PageSoundTriggerArmed : public PageSoundTrigger
{
    public:
        PageSoundTriggerArmed() : PageSoundTrigger("ARMED", mictrig_nullfunc, "/go_icon.png") {
        };

        virtual void on_redraw(void)
        {
            FairyCfgItem::on_redraw();
            M5Lcd.setTextFont(4);
            M5Lcd.setCursor(_margin_x, get_y(1));
            M5Lcd.print("Level: ");
            gui_showVal(config_settings.mictrig_level, TXTFMT_NONE, (Print*)&M5Lcd);
            blank_line();
            M5Lcd.setCursor(_margin_x, get_y(2));
            M5Lcd.print("Delay: ");
            M5Lcd.print(config_settings.mictrig_delay, DEC);
            M5Lcd.print("s");
            blank_line();
        };

        virtual void on_navTo(void)
        {
            _ignore_time = millis(); // since each button press causes the mic to pick up sound, ignore the sound for a time period after button press
            on_redraw();
        };

        virtual void on_eachFrame(void)
        {
            on_extraPoll(); on_drawLive();
            pwr_tick(true); // no sleep while listening
            uint32_t now = millis();
            if ((now - _ignore_time) > 1000) // since each button press causes the mic to pick up sound, ignore the sound for a time period after button press
            {
                if (mictrig_hasTriggered)
                {
                    mictrig_hasTriggered = false;

                    bool ret = mictrig_shoot();

                    app_waitAllRelease();
                    mictrig_hasTriggered = false;
                    _ignore_time = millis();
                    redraw_flag = true;

                    // if we are falsely triggered by the power button
                    // then we redirect to another page so we are not armed
                    if (ret)
                    {
                        FairyCfgApp* parent_app = dynamic_cast<FairyCfgApp*>((FairyCfgApp*)get_parent());
                        if (parent_app != NULL) {
                            parent_app->rewind();
                        }
                    }
                }
            }
            else
            {
                mictrig_hasTriggered = false;
            }
        };

    protected:
        uint32_t _ignore_time = 0;
};

class AppSoundShutter : public FairyCfgApp
{
    public:
        AppSoundShutter() : FairyCfgApp("/soundshutter.png", "/mic_icon.png") {
            this->install(new PageSoundTriggerLevel());
            this->install(new PageSoundTriggerDelay());
            this->install(new PageSoundTriggerArmed());
        };

        virtual void on_navOut(void)
        {
            mictrig_pause();
        };
};

extern FairySubmenu menu_remote;
void setup_soundshutter()
{
    static AppSoundShutter app;
    menu_remote.install(&app);
}
