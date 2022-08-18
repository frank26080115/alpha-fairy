/*
this is a wrapper around a HardwareSerial object but with an enable flag
so that an application can put print statements everywhere without messy conditional statements before each print call
*/

#ifndef _DEBUGGINGSERIAL_H_
#define _DEBUGGINGSERIAL_H_

#include <Arduino.h>
#include <HardwareSerial.h>

#define SERDBG_DISABLE

class DebuggingSerial : public HardwareSerial
{
    public:
        DebuggingSerial(HardwareSerial* s);
        bool enabled;
        // override the functions that HardwareSerial have that calls a real UART function
        size_t write(uint8_t c);
        size_t write(const uint8_t *buffer, size_t size);
        #ifdef SERDBG_DISABLE
        size_t printf(const char * format, ...)  __attribute__ ((format (printf, 2, 3)));
        size_t print(const __FlashStringHelper *);
        size_t print(const String &);
        size_t print(const char[]);
        size_t print(char);
        size_t print(unsigned char, int = DEC);
        size_t print(int, int = DEC);
        size_t print(unsigned int, int = DEC);
        size_t print(long, int = DEC);
        size_t print(unsigned long, int = DEC);
        size_t print(long long, int = DEC);
        size_t print(unsigned long long, int = DEC);
        size_t print(double, int = 2);
        size_t print(const Printable&);
        size_t print(struct tm * timeinfo, const char * format = NULL);
        size_t println(const __FlashStringHelper *);
        size_t println(const String &s);
        size_t println(const char[]);
        size_t println(char);
        size_t println(unsigned char, int = DEC);
        size_t println(int, int = DEC);
        size_t println(unsigned int, int = DEC);
        size_t println(long, int = DEC);
        size_t println(unsigned long, int = DEC);
        size_t println(long long, int = DEC);
        size_t println(unsigned long long, int = DEC);
        size_t println(double, int = 2);
        size_t println(const Printable&);
        size_t println(struct tm * timeinfo, const char * format = NULL);
        size_t println(void);
        #endif
    protected:
        HardwareSerial* ser_obj;
};

#endif
