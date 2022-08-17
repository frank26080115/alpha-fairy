#include "DebuggingSerial.h"

#ifdef SERDBG_DISABLE
size_t DebuggingSerial::printf (const char * format, ...)                  { return 0; }
size_t DebuggingSerial::print  (const __FlashStringHelper * x)             { return 0; }
size_t DebuggingSerial::print  (const String & x)                          { return 0; }
size_t DebuggingSerial::print  (const char x[])                            { return 0; }
size_t DebuggingSerial::print  (char x)                                    { return 0; }
size_t DebuggingSerial::print  (unsigned char x, int y)                    { return 0; }
size_t DebuggingSerial::print  (int x, int y)                              { return 0; }
size_t DebuggingSerial::print  (unsigned int x, int y)                     { return 0; }
size_t DebuggingSerial::print  (long x, int y)                             { return 0; }
size_t DebuggingSerial::print  (unsigned long x, int y)                    { return 0; }
size_t DebuggingSerial::print  (long long x, int y)                        { return 0; }
size_t DebuggingSerial::print  (unsigned long long x, int y)               { return 0; }
size_t DebuggingSerial::print  (double x, int y)                           { return 0; }
size_t DebuggingSerial::print  (const Printable& x)                        { return 0; }
size_t DebuggingSerial::print  (struct tm * timeinfo, const char * format) { return 0; }
size_t DebuggingSerial::println(const __FlashStringHelper * x)             { return 0; }
size_t DebuggingSerial::println(const String &s)                           { return 0; }
size_t DebuggingSerial::println(const char x[])                            { return 0; }
size_t DebuggingSerial::println(char x)                                    { return 0; }
size_t DebuggingSerial::println(unsigned char x, int y)                    { return 0; }
size_t DebuggingSerial::println(int x, int y)                              { return 0; }
size_t DebuggingSerial::println(unsigned int x, int y)                     { return 0; }
size_t DebuggingSerial::println(long x, int y)                             { return 0; }
size_t DebuggingSerial::println(unsigned long x, int y)                    { return 0; }
size_t DebuggingSerial::println(long long x, int y)                        { return 0; }
size_t DebuggingSerial::println(unsigned long long x, int y)               { return 0; }
size_t DebuggingSerial::println(double x, int y)                           { return 0; }
size_t DebuggingSerial::println(const Printable& x)                        { return 0; }
size_t DebuggingSerial::println(struct tm * timeinfo, const char * format) { return 0; }
size_t DebuggingSerial::println(void)                                      { return 0; }
#endif