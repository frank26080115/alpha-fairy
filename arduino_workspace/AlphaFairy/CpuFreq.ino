#include "AlphaFairy.h"

#include "driver/uart.h"

static uint32_t cpufreq_xtal = 0;

static const uint32_t cpufreq_cpuFreqMax = 240;
static uint32_t cpufreq_cpuFreqMin = 80; // change later based on crystal

static uint32_t cpufreq_timestamp = 0;
static uint32_t cpufreq_cpuFreqLast = 80;
static uint32_t cpufreq_tgtFreq = 240;

extern "C" {
extern uint32_t _get_effective_baudrate(uint32_t baudrate);
};

void cpufreq_init(void)
{
    cpufreq_xtal = getXtalFrequencyMhz();
    dbg_ser.printf("XTAL freq: %u MHz\r\n", cpufreq_xtal);

    #if 0
    // these frequencies are technically possible but breaks WiFi
    switch (cpufreq_xtal)
    {
        case 40:
            cpufreq_cpuFreqMin = 10; // this is the one for M5StickC-Plus (ESP32-PICO)
            break;
        case 26:
            cpufreq_cpuFreqMin = 13;
            break;
        case 24:
            cpufreq_cpuFreqMin = 12;
            break;
    }
    #else
        #ifdef ENABLE_CPU_FREQ_SCALING
        cpufreq_cpuFreqMin = 80; // this seems to work
        #else
        cpufreq_cpuFreqMin = 240;
        #endif
    #endif
    dbg_ser.printf("CPU freq range: %u - %u MHz\r\n", cpufreq_cpuFreqMax, cpufreq_cpuFreqMin);

    cpufreq_cpuFreqLast = getCpuFrequencyMhz();
    cpufreq_tgtFreq = cpufreq_cpuFreqLast;

    dbg_ser.printf("CPU freq on boot: %u MHz\r\n", cpufreq_cpuFreqLast);

    if (cpufreq_cpuFreqLast != cpufreq_cpuFreqMax) {
        cpufreq_boost();
    }
}

static volatile bool cpufreq_mtx = false;

void cpufreq_setTarget(uint32_t mhz)
{
    #ifdef ENABLE_CPU_FREQ_SCALING
    cpufreq_tgtFreq = mhz;
    if (mhz == cpufreq_cpuFreqMax) {
        cpufreq_change(mhz);
    }
    #endif
}

void cpufreq_change(uint32_t mhz)
{
    #ifdef ENABLE_CPU_FREQ_SCALING
    if (cpufreq_mtx) {
        return;
    }
    cpufreq_mtx = true;
    if (cpufreq_cpuFreqLast != mhz)
    {
        setCpuFrequencyMhz(mhz);
        cpufreq_cpuFreqLast = mhz;

        if (mhz == cpufreq_tgtFreq)
        {
            uart_config_t uart_config;
            uart_config.baud_rate           = _get_effective_baudrate(SERIAL_PORT_BAUDRATE);
            uart_config.data_bits           = UART_DATA_8_BITS;
            uart_config.parity              = UART_PARITY_DISABLE;
            uart_config.stop_bits           = UART_STOP_BITS_2;
            uart_config.flow_ctrl           = UART_HW_FLOWCTRL_DISABLE;
            uart_config.rx_flow_ctrl_thresh = 122;
            uart_config.source_clk          = UART_SCLK_APB;
            uart_param_config(0, &uart_config);

            dbg_ser.printf("new CPU freq %u MHz\r\n", mhz);
        }
    }
    cpufreq_mtx = false;
    #endif
}

void cpufreq_boost(void)
{
    #ifdef ENABLE_CPU_FREQ_SCALING
    uint32_t now = millis();
    cpufreq_timestamp = now;
    cpufreq_setTarget(cpufreq_cpuFreqMax);
    #endif
}

void cpufreq_task(void)
{
    #ifdef ENABLE_CPU_FREQ_SCALING
    uint32_t now = millis();
    if (cpufreq_tgtFreq != cpufreq_cpuFreqLast)
    {
        if (cpufreq_tgtFreq < cpufreq_cpuFreqLast)
        {
            // there is a errata that says to not change directly from 240 MHz to 80 or 40 MHz immediately, instead, step down
            if (cpufreq_cpuFreqLast == 240) {
                cpufreq_change(160);
            }
            else if (cpufreq_cpuFreqLast == 160) {
                cpufreq_change(80);
            }
        }
        else if (cpufreq_tgtFreq > cpufreq_cpuFreqLast)
        {
            /*
            if (cpufreq_cpuFreqLast <= 40) {
                cpufreq_change(80);
            }
            else if (cpufreq_cpuFreqLast == 80) {
                cpufreq_change(160);
            }
            else if (cpufreq_cpuFreqLast == 160) {
                cpufreq_change(240);
            }
            */
            cpufreq_change(cpufreq_tgtFreq);
        }
    }
    if ((now - cpufreq_timestamp) < 1000) {
        return;
    }
    cpufreq_setTarget(cpufreq_cpuFreqMin);
    #endif
}
