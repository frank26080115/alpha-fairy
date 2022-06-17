/*
this is a wrapper around a HardwareSerial object but with an enable flag
so that an application can put print statements everywhere without messy conditional statements before each print call
*/

#ifndef _DEBUGGINGSERIAL_H_
#define _DEBUGGINGSERIAL_H_

#include <Arduino.h>
#include <HardwareSerial.h>

class DebuggingSerial : public HardwareSerial
{
    public:
        DebuggingSerial(HardwareSerial* s);
        bool enabled;
        // override the functions that HardwareSerial have that calls a real UART function
        size_t write(uint8_t c);
        size_t write(const uint8_t *buffer, size_t size);
    protected:
        HardwareSerial* ser_obj;
};

#endif
