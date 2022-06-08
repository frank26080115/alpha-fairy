#!/usr/bin/env python3

import usys, uos, utime
import machine

def time_millis():
    return utime.ticks_ms()

def time_sleep_ms(x):
    utime.sleep_ms(x)

def guid():
    x = bytearray()
    x.extend(machine.unique_id())
    while len(x) < 16:
        x.append(0)
    return x

def bytearray_get_uint_n(barr, idx, dsz):
    i = 0
    x = 0
    while i < dsz:
        x += int(barr[idx + i]) << (8 * i)
        i += 1
    return x
