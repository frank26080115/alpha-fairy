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

#define MICTRIG_I2S_SAMPLERATE 36000

static bool mictrig_hasInit = false;

uint8_t mictrig_buffer8[MICTRIG_READ_LEN * 2] = {0};

volatile int16_t* mictrig_buffer16;
volatile int32_t mictrig_lastMax = 0;
volatile int32_t mictrig_filteredMax = 0;
volatile int32_t mictrig_decay = 0;
volatile bool mictrig_hasTriggered = false;
bool gui_microphoneActive = false;

//#define MICTRIG_STATS
#ifdef MICTRIG_STATS
volatile uint32_t mictrig_stats_sampleCnt = 0;
volatile uint32_t mictrig_stats_startTime = 0;
#endif

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
        .clk_cfg = I2S_PDM_RX_CLK_DEFAULT_CONFIG(MICTRIG_I2S_SAMPLERATE),
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
        .sample_rate     = MICTRIG_I2S_SAMPLERATE,                                           // Set the I2S sampling rate
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
    i2s_set_clk(I2S_NUM_0, MICTRIG_I2S_SAMPLERATE, I2S_BITS_PER_SAMPLE_16BIT, I2S_CHANNEL_MONO);

    mictrig_buffer16 = (int16_t*)mictrig_buffer8;

    #endif
}

#ifdef MICTRIG_NEW_I2S_LIB
static IRAM_ATTR bool mictrig_rx_cb(i2s_chan_handle_t handle, i2s_event_data_t *event, void *user_ctx)
{
    mictrig_buffer16 = (int16_t*)(event->data);
    uint32_t sz      = (uint32_t)(event->size);

    #ifdef MICTRIG_STATS
    if (sz > 0) mictrig_stats_task(sz, millis());
    #endif

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
        mictrig_decay = 16;      // slow down the decay
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
    cpufreq_boost();

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

        #ifdef MICTRIG_STATS
        mictrig_stats_task(bytesread, tstart);
        #endif

        m *= MICTRIG_GAIN;           // apply gain
        m = m > 0x7FFF ? 0x7FFF : m; // limit after gain

        mictrig_decayTask();

        mictrig_lastMax = m; // remember the latest peak

        if (m > mictrig_filteredMax)
        {
            mictrig_decay = 16;      // slow down the decay
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

#ifdef MICTRIG_STATS
void mictrig_stats_task(uint32_t add_bytes, uint32_t now_time)
{
    if (mictrig_stats_sampleCnt <= 0) {
        mictrig_stats_startTime = now_time;
    }
    mictrig_stats_sampleCnt += add_bytes / 2;
    if (mictrig_stats_sampleCnt >= (MICTRIG_I2S_SAMPLERATE * 10)) {
        uint32_t t = millis();
        uint32_t dt = t - mictrig_stats_startTime;
        uint32_t samps = mictrig_stats_sampleCnt * 1000;
        dbg_ser.printf("mic sample rate %u\r\n", (samps / dt));
        mictrig_stats_sampleCnt = 0;
    }
}
#endif

void mictrig_decayTask()
{
    // decay the displayed peak
    if (mictrig_filteredMax >= mictrig_decay) {
        mictrig_filteredMax -= mictrig_decay;
    }
    // accelerate the decay
    if (mictrig_decay <= 0xFFF) {
        mictrig_decay += 8;
    }
}

bool mictrig_shoot()
{
    M5Lcd.setCursor(SUBMENU_X_OFFSET, MICTRIG_LEVEL_MARGIN);
    M5Lcd.fillRect(0, MICTRIG_LEVEL_MARGIN, M5Lcd.width() - GENERAL_ICON_WIDTH, M5Lcd.height() - MICTRIG_LEVEL_MARGIN - 12, TFT_BLACK);
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

void mictrig_drawLevel()
{
    // calculate the length of the red bar representing the current sound level peak
    uint32_t lvl = (mictrig_filteredMax + 90) / 182;
    lvl = (lvl < MICTRIG_LEVEL_BAR_HEIGHT) ? MICTRIG_LEVEL_BAR_HEIGHT : lvl;
    lvl = (lvl > 180) ? 180 : lvl;

    // calculate the position of the green tick representing the trigger level
    volatile uint32_t thresh = config_settings.mictrig_level;
    thresh *= 318;
    thresh /= 180;
    thresh = (thresh < MICTRIG_LEVEL_BAR_HEIGHT) ? MICTRIG_LEVEL_BAR_HEIGHT : thresh;

    gui_drawLevelBar(lvl, -1, thresh, -1);
}
