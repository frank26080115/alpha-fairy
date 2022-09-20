#include "AlphaFairy.h"

#include "driver/uart.h"

static uint32_t cpufreq_xtal = 0;

static const uint32_t cpufreq_cpuFreqMax = 240;
static uint32_t cpufreq_cpuFreqMin = 80; // change later based on crystal

static uint32_t cpufreq_timestamp = 0;
static uint32_t cpufreq_cpuFreqLast = 80;

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
            cpufreq_cpuFreqMin = 10;
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

    dbg_ser.printf("CPU freq on boot: %u MHz\r\n", cpufreq_cpuFreqLast);

    if (cpufreq_cpuFreqLast != cpufreq_cpuFreqMax) {
        cpufreq_boost();
    }
}

void cpufreq_change(uint32_t mhz)
{
    #ifdef ENABLE_CPU_FREQ_SCALING
    if (cpufreq_cpuFreqLast != mhz) {
        setCpuFrequencyMhz(mhz);
        cpufreq_cpuFreqLast = mhz;

        uart_config_t uart_config;
        uart_config.baud_rate           = _get_effective_baudrate(115200);
        uart_config.data_bits           = UART_DATA_8_BITS;
        uart_config.parity              = UART_PARITY_DISABLE;
        uart_config.stop_bits           = UART_STOP_BITS_1;
        uart_config.flow_ctrl           = UART_HW_FLOWCTRL_DISABLE;
        uart_config.rx_flow_ctrl_thresh = 122;
        uart_config.source_clk          = UART_SCLK_APB;
        uart_param_config(0, &uart_config);

        dbg_ser.printf("new CPU freq %u MHz\r\n", mhz);
    }
    #endif
}

void cpufreq_boost(void)
{
    #ifdef ENABLE_CPU_FREQ_SCALING
    uint32_t now = millis();
    cpufreq_timestamp = now;
    cpufreq_change(cpufreq_cpuFreqMax);
    #endif
}

void cpufreq_task(void)
{
    #ifdef ENABLE_CPU_FREQ_SCALING
    uint32_t now = millis();
    if ((now - cpufreq_timestamp) < 1000) {
        return;
    }
    cpufreq_change(cpufreq_cpuFreqMin);
    #endif
}
