#include "SonyCameraInfraredRemote.h"
#include <stdint.h>

//#define USE_IRREMOTE
#define USE_ESP32_RMT

#ifdef USE_IRREMOTE

#define USE_OPEN_DRAIN_OUTPUT_FOR_SEND_PIN
#define SEND_PWM_BY_TIMER
#define NO_DECODER

#include <IRremote.h>
#include <ir_Sony.hpp>

#define IR_TX_PIN 9
//#define IR_TX_PIN 26

#endif

#ifdef USE_ESP32_RMT
#include "driver/rmt.h"
#include "soc/rmt_reg.h"

#define IR_TX_PIN   GPIO_NUM_9
//#define IR_TX_PIN   GPIO_NUM_26
#define RMT_CHANNEL RMT_CHANNEL_3

#define ACTIVE_VAL 0

#endif

void SonyCamIr_Init()
{
    #ifdef USE_IRREMOTE
    IrSender.begin(9);
    #endif
    #ifdef USE_ESP32_RMT
    rmt_config_t rmt_tx;
    rmt_tx.channel       = RMT_CHANNEL;
    rmt_tx.gpio_num      = IR_TX_PIN;
    rmt_tx.clk_div       = 80; // 1 MHz, 1 us - we set up sampling to every 1 microseconds
    rmt_tx.mem_block_num = 2;
    rmt_tx.rmt_mode      = RMT_MODE_TX;
    rmt_tx.tx_config.loop_en = false;
    rmt_tx.tx_config.carrier_duty_percent = 33;
    rmt_tx.tx_config.carrier_freq_hz = 40000;
    rmt_tx.tx_config.carrier_level   = (ACTIVE_VAL == 0) ? RMT_CARRIER_LEVEL_LOW : RMT_CARRIER_LEVEL_HIGH;
    rmt_tx.tx_config.carrier_en      = true;
    rmt_tx.tx_config.idle_level      = (ACTIVE_VAL == 0) ? RMT_IDLE_LEVEL_HIGH : RMT_IDLE_LEVEL_LOW;
    rmt_tx.tx_config.idle_output_en  = true;
    rmt_config(&rmt_tx);
    rmt_driver_install(rmt_tx.channel, 0, 0);
    rmt_tx_start(rmt_tx.channel, 1);
    SonyCamIr_SendRaw(0, 0);
    #endif
}

void SonyCamIr_SendRaw(uint16_t addr, uint8_t cmd)
{
    #ifdef USE_IRREMOTE
    IrSender.sendSony(addr, cmd, 1, SONY_BITS_MAX);
    #endif
    #ifdef USE_ESP32_RMT
    uint32_t data;
    static rmt_item32_t rmtObjects[32];
    uint8_t i, j;
    data = addr; data <<= 7; data &= 0xFFFF80; data |= cmd & 0x7F;
    rmtObjects[0].duration0 = 2400;
    rmtObjects[0].level0 = ACTIVE_VAL == 0 ? 0 : 1;
    rmtObjects[0].duration1 = 600;
    rmtObjects[0].level1 = ACTIVE_VAL == 0 ? 1 : 0;
    for (i = 1, j = 0; j < 20; i += 1, j += 1)
    {
        rmtObjects[i].duration0 = (((data & (1UL << j)) != 0) ? 2 : 1) * 600;
        rmtObjects[i].level0 = ACTIVE_VAL == 0 ? 0 : 1;
        rmtObjects[i].duration1 = 600;
        rmtObjects[i].level1 = ACTIVE_VAL == 0 ? 1 : 0;
    }
    rmt_write_items(RMT_CHANNEL, rmtObjects, i, true);
    #endif
}

void SonyCamIr_SendRawX(uint16_t addr, uint8_t cmd, uint8_t xtimes)
{
    uint8_t i;
    for (i = 0; i < xtimes; i++)
    {
        SonyCamIr_SendRaw(addr, cmd);
    }
}

void SonyCamIr_SendRawBits(uint32_t data, uint8_t numbits, uint8_t xtimes)
{
    uint8_t k;
    for (k = 0; k < xtimes; k++)
    {
        #ifdef USE_ESP32_RMT
        static rmt_item32_t rmtObjects[32];
        uint8_t i, j;
        rmtObjects[0].duration0 = 2400;
        rmtObjects[0].level0 = ACTIVE_VAL == 0 ? 0 : 1;
        rmtObjects[0].duration1 = 600;
        rmtObjects[0].level1 = ACTIVE_VAL == 0 ? 1 : 0;
        for (i = 1, j = 0; j < numbits; i += 1, j += 1)
        {
            rmtObjects[i].duration0 = (((data & (1UL << j)) != 0) ? 2 : 1) * 600;
            rmtObjects[i].level0 = ACTIVE_VAL == 0 ? 0 : 1;
            rmtObjects[i].duration1 = 600;
            rmtObjects[i].level1 = ACTIVE_VAL == 0 ? 1 : 0;
        }
        rmt_write_items(RMT_CHANNEL, rmtObjects, i, true);
        #endif
    }
}

void SonyCamIr_Shoot()
{
    SonyCamIr_SendRawX(IR_ADDR_SONYCAM, IR_CMD_SHOOT, 3);
}

void SonyCamIr_Shoot2S()
{
    SonyCamIr_SendRawX(IR_ADDR_SONYCAM, IR_CMD_SHOOT_2S, 3);
}

void SonyCamIr_Movie()
{
    SonyCamIr_SendRawX(IR_ADDR_SONYCAM, IR_CMD_MOVIE, 3);
}
