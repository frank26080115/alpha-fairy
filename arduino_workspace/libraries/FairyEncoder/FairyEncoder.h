#ifndef _FAIRYENCODER_H_
#define _FAIRYENCODER_H_

#include "Arduino.h"
#include "Wire.h"

#include <stdint.h>
#include <stdbool.h>

class FairyEncoder
{
    public:
        FairyEncoder(TwoWire* wire = &Wire, uint8_t i2c_addr = 0x40, uint32_t chk_intv_fast = 1, uint32_t chk_intv_slow = 200, uint32_t chk_intv_sleep = 2000);
        void    task(void);
        int16_t read(bool clear = true);
        int16_t readRaw(void);
        bool    avail(void);
        bool    getButtonStatus(void);
        void    setLEDColor(uint8_t index, uint32_t color);

    protected:
        TwoWire* _wire;
        uint8_t _i2c_addr;
        bool _avail;
        int16_t _last_cnt;
        int16_t _last_read;
        uint32_t _last_check_time, _last_move_time;
        uint32_t _check_interval;
        uint32_t _chkintv_fast, _chkintv_slow, _chkintv_sleep;
        void writeBytes(uint8_t reg, uint8_t* buffer, uint8_t length);
        bool readBytes(uint8_t reg, uint8_t* buffer, uint8_t length);
};

#endif
