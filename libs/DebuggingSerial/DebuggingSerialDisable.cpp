#include "DebuggingSerial.h"


DebuggingSerialDisabled::DebuggingSerialDisabled(HardwareSerial* s) : HardwareSerial(0)
{
    this->ser_obj = s;
}

// override the functions that HardwareSerial have that calls a real UART function

size_t DebuggingSerialDisabled::write(uint8_t c)
{
    return 1;
}

size_t DebuggingSerialDisabled::write(const uint8_t *buffer, size_t size)
{
    return 0;
}

size_t DebuggingSerialDisabled::printf (const char * format, ...)                  { return 0; }
size_t DebuggingSerialDisabled::print  (const __FlashStringHelper * x)             { return 0; }
size_t DebuggingSerialDisabled::print  (const String & x)                          { return 0; }
size_t DebuggingSerialDisabled::print  (const char x[])                            { return 0; }
size_t DebuggingSerialDisabled::print  (char x)                                    { return 0; }
size_t DebuggingSerialDisabled::print  (unsigned char x, int y)                    { return 0; }
size_t DebuggingSerialDisabled::print  (int x, int y)                              { return 0; }
size_t DebuggingSerialDisabled::print  (unsigned int x, int y)                     { return 0; }
size_t DebuggingSerialDisabled::print  (long x, int y)                             { return 0; }
size_t DebuggingSerialDisabled::print  (unsigned long x, int y)                    { return 0; }
size_t DebuggingSerialDisabled::print  (long long x, int y)                        { return 0; }
size_t DebuggingSerialDisabled::print  (unsigned long long x, int y)               { return 0; }
size_t DebuggingSerialDisabled::print  (double x, int y)                           { return 0; }
size_t DebuggingSerialDisabled::print  (const Printable& x)                        { return 0; }
size_t DebuggingSerialDisabled::print  (struct tm * timeinfo, const char * format) { return 0; }
size_t DebuggingSerialDisabled::println(const __FlashStringHelper * x)             { return 0; }
size_t DebuggingSerialDisabled::println(const String &s)                           { return 0; }
size_t DebuggingSerialDisabled::println(const char x[])                            { return 0; }
size_t DebuggingSerialDisabled::println(char x)                                    { return 0; }
size_t DebuggingSerialDisabled::println(unsigned char x, int y)                    { return 0; }
size_t DebuggingSerialDisabled::println(int x, int y)                              { return 0; }
size_t DebuggingSerialDisabled::println(unsigned int x, int y)                     { return 0; }
size_t DebuggingSerialDisabled::println(long x, int y)                             { return 0; }
size_t DebuggingSerialDisabled::println(unsigned long x, int y)                    { return 0; }
size_t DebuggingSerialDisabled::println(long long x, int y)                        { return 0; }
size_t DebuggingSerialDisabled::println(unsigned long long x, int y)               { return 0; }
size_t DebuggingSerialDisabled::println(double x, int y)                           { return 0; }
size_t DebuggingSerialDisabled::println(const Printable& x)                        { return 0; }
size_t DebuggingSerialDisabled::println(struct tm * timeinfo, const char * format) { return 0; }
size_t DebuggingSerialDisabled::println(void)                                      { return 0; }