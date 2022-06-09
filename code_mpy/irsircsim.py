def const(x):
    return x

BITS_ADDR = const(13)
BITS_CMD  = const(7)

class InfraredSIRC:

    ADDR_SONYCAMERA = const(0x1E3A)
    CMD_SHUTTER     = const(0x2D)
    CMD_SHUTTER_2S  = const(0x37)
    CMD_MOVIE       = const(0x48)

    def __init__(self):
        pass

    def can_work(self):
        return True

    def tx(self, addr, cmd, wait=True):
        print("IR SIM 0x%04X 0x%02X" % (addr, cmd))
