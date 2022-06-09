from micropython import const
from m5stack import ir
import gc

BITS_ADDR = const(13)
BITS_CMD  = const(7)

class InfraredSIRC:

    ADDR_SONYCAMERA = const(0x1E3A)
    CMD_SHUTTER     = const(0x2D)
    CMD_SHUTTER_2S  = const(0x37)
    CMD_MOVIE       = const(0x48)

    def __init__(self):
        # dir(ir.__dict__['_tx_encode']._rmt) = RMT(channel=3, pin=9, source_freq=80000000, clock_div=80, carrier_freq=38000, carrier_duty_percent=33)
        try:
            ir.__dict__['_tx_encode']._rmt.deinit()
            del ir
        except:
            pass
        gc.collect()
        self.rmt = None
        retries = 3
        while retries > 0:
            try:
                self.rmt = esp32.RMT(3, pin=machine.Pin(9), clock_div=80, carrier_freq=40000, carrier_duty_percent=33)
                break
            except:
                pass
            retries -= 1

    def can_work(self):
        return self.rmt is not None

    def tx(self, addr, cmd, need_wait=True):
        x = []
        x += [24000]
        i = 0
        while i < BITS_CMD:
            if (cmd & (1 << i)) == 0:
                x += [6000, 6000]
            else:
                x += [6000, 12000]
            i += 1
        i = 0
        while i < BITS_ADDR:
            if (addr & (1 << i)) == 0:
                x += [6000, 6000]
            else:
                x += [6000, 12000]
            i += 1
        #print("IR pulses %s" % x)
        if self.rmt is None:
            return
        self.rmt.write_pulses(x)
        if need_wait:
            self.rmt.wait_done()
