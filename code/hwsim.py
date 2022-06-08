import threading, time

class FakeLCD:
    PORTRAIT = 1
    LANDSCAPE = 2
    FONT_Default = 3

    def __init__(self):
        pass

    def text(self, text, x=0, y=0, c=0, a1=0):
        print("LCD SIM TXT: \"%s\"" % text)

    def print(self, text, x=0, y=0, c=0, a1=0):
        print("LCD SIM PRINT: \"%s\"" % text)

    def image(self, img_path, x=0, y=0, c=0, a1=0):
        print("LCD SIM IMG: \"%s\"" % img_path)

    def clear(self):
        print("LCD SIM CLEAR")

    def circle(self, x, y, r, c1=0, c2=0):
        print("LCD SIM CIRCLE: %d %d %d" % (x, y, r))

    def orient(self, x):
        print("LCD SIM ORIENT %u" % x)

    def font(self, x):
        print("LCD SIM FONT %u" % x)

lcd = FakeLCD()

class FakeButton:

    def __init__(self, c):
        self.char = c
        self.callback = None
        self.isdown = False
        pass

    def wasPressed(self, callback = 0):
        if callback != 0:
            self.callback = callback

    def wasDoublePress(self, callback = 0):
        if callback != 0:
            self.callback2 = callback

    def isPressed(self):
        return self.isdown

    def sim_press(self):
        if not self.isdown:
            print("BTN SIM %s" % self.char.upper())
        self.isdown = True
        if self.callback is not None:
            self.callback()

    def sim_release(self):
        self.isdown = False

btnA = FakeButton('a')
btnB = FakeButton('b')
signal_quit = False

def btn_checker():
    import keyboard
    global btnA
    global btnB
    global signal_quit
    while signal_quit == False:
        time.sleep(0.1)
        if keyboard.is_pressed(btnA.char):
            btnA.sim_press()
        else:
            btnA.sim_release()
        if keyboard.is_pressed(btnB.char):
            btnB.sim_press()
        else:
            btnB.sim_release()

btn_check_thread = threading.Thread(target=btn_checker)

def start_button_checker():
    global btn_check_thread
    btn_check_thread.start()

def stop_button_checker():
    global signal_quit
    global btn_check_thread
    signal_quit = True
    if btn_check_thread is not None:
        btn_check_thread.join()

class FakeLed:
    def __init__(self):
        self.is_on = False
        pass

    def on(self):
        if not self.is_on:
            print("M5Led SIM ON")
        self.is_on = True

    def off(self):
        if self.is_on:
            print("M5Led SIM OFF")
        self.is_on = False

M5Led = FakeLed()

def const(x):
    return x
