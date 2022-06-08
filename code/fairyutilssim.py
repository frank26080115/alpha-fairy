#!/usr/bin/env python3

import sys, os, time

def time_millis():
    now = time.monotonic()
    return int(round(now * 1000))

def time_sleep_ms(x):
    time.sleep(x / 1000)

def guid():
    import uuid
    return uuid.uuid4().bytes

def bytearray_get_uint_n(barr, idx, dsz):
    i = 0
    x = 0
    while i < dsz:
        x += int(barr[idx + i]) << (8 * i)
        i += 1
    return x
