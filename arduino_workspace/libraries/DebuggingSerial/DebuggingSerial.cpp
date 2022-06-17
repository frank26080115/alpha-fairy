#include <DebuggingSerial.h>

DebuggingSerial::DebuggingSerial(HardwareSerial* s) : HardwareSerial(0)
{
    this->ser_obj = s;
}

// override the functions that HardwareSerial have that calls a real UART function

size_t DebuggingSerial::write(uint8_t c)
{
    if (this->enabled) {
        ((HardwareSerial*)this->ser_obj)->write(c);
    }
    #if ARDUINO >= 100
    return 1;
    #endif
}

size_t DebuggingSerial::write(const uint8_t *buffer, size_t size)
{
    if (this->enabled) {
        return ((HardwareSerial*)this->ser_obj)->write(buffer, size);
    }
    return 0;
}