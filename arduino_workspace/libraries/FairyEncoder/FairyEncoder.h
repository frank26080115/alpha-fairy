#ifndef _FAIRYENCODER_H_
#define _FAIRYENCODER_H_

#include "Arduino.h"
#include "Wire.h"

#include <stdint.h>
#include <stdbool.h>

class FairyEncoder
{
    public:
        FairyEncoder(TwoWire* wire = &Wire, uint8_t i2c_addr = 0x40
                        , uint32_t chk_intv_fast = 1        // delay between I2C reads if the encoder has recently been rotated (in milliseconds)
                        , uint32_t chk_intv_slow = 200      // delay between I2C reads if the encoder has not been recently rotated (in milliseconds)
                        , uint32_t chk_intv_sleep = 2000    // the criteria for being "recent" for the above two delay times (in milliseconds)
                        );
        void    begin(void);                                // calls _wire->begin() and resets variables
        void    task(void);                                 // reads the I2C device when it needs to
        int16_t read(bool clear = true);                    // returns relative encoder steps since last clear
        int16_t readRaw(void);                              // returns the absolute encoder steps stored on encoder module
        bool    avail(void);                                // true if encoder is connected, false if I2C communication has failed
        bool    getButtonStatus(void);
        void    setLEDColor(uint8_t index, uint32_t color);

        void    writeBytes(uint8_t reg, uint8_t* buffer, uint8_t length);
        bool    readBytes(uint8_t reg, uint8_t* buffer, uint8_t length);

    protected:
        TwoWire* _wire;
        uint8_t _i2c_addr;
        bool _avail;
        int16_t _last_cnt;
        int16_t _last_read;
        uint32_t _last_check_time, _last_move_time;
        uint32_t _check_interval;
        uint32_t _chkintv_fast, _chkintv_slow, _chkintv_sleep;
};

#endif
