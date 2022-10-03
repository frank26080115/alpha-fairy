#include "FairyEncoder.h"

#define ENCODER_REG  0x10
#define BUTTON_REG   0x20
#define RGB_LED_REG  0x30

FairyEncoder::FairyEncoder(TwoWire* wire, uint8_t i2c_addr, uint32_t chk_intv_fast, uint32_t chk_intv_slow, uint32_t chk_intv_sleep)
{
    _wire = wire;
    _i2c_addr = i2c_addr;
    _last_cnt = 0;
    _last_read = 0;
    _last_check_time = 0;
    _last_move_time = 0;
    _chkintv_fast = chk_intv_fast;
    _chkintv_slow = chk_intv_slow;
    _chkintv_sleep = chk_intv_sleep;
    _check_interval = _chkintv_slow;
    _has_begun = false;
}

void FairyEncoder::begin()
{
    if (_has_begun == false) {
        _wire->begin();
        _has_begun = true;
    }
    _last_cnt = 0;
    _last_read = 0;
    _last_check_time = 0;
    _last_move_time = 0;
}

void FairyEncoder::task()
{
    uint32_t now = millis();

    if ((now - _last_check_time) < _check_interval) {
        return;
    }

    if (_has_begun == false) {
        begin();
    }

    _last_check_time = now;

    bool prev_avail = _avail;
    int16_t x;

    readBytes(ENCODER_REG, (uint8_t*)&x, 2);
    _last_cnt = x;
    if (_last_cnt != _last_read) {
        _last_move_time = now;
        _check_interval = _chkintv_fast;
    }

    if ((now - _last_move_time) >= _chkintv_sleep) {
        _check_interval = _chkintv_slow;
    }

    if (_avail == false || _avail != prev_avail) {
        _last_read = _last_cnt;
    }
}

int16_t FairyEncoder::read(bool to_clr)
{
    int16_t d = _last_cnt - _last_read;
    if (to_clr) {
        _last_read = _last_cnt;
    }
    return d;
}

int16_t FairyEncoder::readRaw()
{
    return _last_cnt;
}

bool FairyEncoder::avail()
{
    return _avail;
}

void FairyEncoder::writeBytes(uint8_t reg, uint8_t* buffer, uint8_t length)
{
    _wire->beginTransmission(_i2c_addr);
    _wire->write(reg);
    for (int i = 0; i < length; i++) {
        _wire->write(*(buffer + i));
    }
    if (_wire->endTransmission() == 0) {
        _avail = false;
    }
}

bool FairyEncoder::readBytes(uint8_t reg, uint8_t* buffer, uint8_t length)
{
    uint8_t index = 0;
    _wire->beginTransmission(_i2c_addr);
    _wire->write(reg);
    if (_wire->endTransmission(false) == 0)
    {
        if (_wire->requestFrom((uint8_t)_i2c_addr, (uint8_t)length, (uint8_t)true) > 0)
        {
            for (int i = 0; i < length; i++) {
                buffer[index++] = _wire->read();
            }
            _avail = true;
            return true;
        }
    }
    _avail = false;
    _check_interval = _chkintv_slow;
    return false;
}

bool FairyEncoder::getButtonStatus()
{
    uint8_t data;
    readBytes(BUTTON_REG, &data, 1);
    return data;
}

void FairyEncoder::setLEDColor(uint8_t index, uint32_t color)
{
    uint8_t data[4];
    data[3] = color & 0xff;
    data[2] = (color >> 8) & 0xff;
    data[1] = (color >> 16) & 0xff;
    data[0] = index;
    writeBytes(RGB_LED_REG, data, 4);
}
