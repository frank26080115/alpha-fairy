DEFAULT_SSID          = "afairywifi"
DEFAULT_WIFI_PASSWORD = "1234567890"

try:
    from m5stack import *
    import fairyutils

    welcome_time = fairyutils.time_millis()
    if lcd.screensize()[1] == 160:
        lcd.image(0, 0, "screens_160/welcome.jpg")
    elif lcd.screensize()[1] == 240:
        lcd.image(0, 0, "screens_240/welcome.jpg")

    import sys

    import gc
    gc.collect()

    from micropython import const
    import netmgr
    import irsirc
    print("\nhello, finished first set of imports")
except Exception as ex:
    print("ERROR during import: %s" % ex)
    import netmgrsim as netmgr
    import fairyutilssim as fairyutils
    import irsircsim as irsirc
    import hwsim
    from hwsim import const, lcd, btnA, btnB, M5Led

import random

import ptpcamera
import ptpsonyalphacamera
import ptpcodes
import ptpsonycodes

print("\nhello, finished second set of imports")

IRBLASTER_ENABLED          = False

MENUSTATE_MAIN             = const(0)
MENUSTATE_CONFIG           = const(1)

MENUITEM_FOCUSSTACK_FAR_1  = const(0)
MENUITEM_FOCUSSTACK_FAR_2  = const(1)
MENUITEM_FOCUSSTACK_FAR_3  = const(2)
MENUITEM_FOCUSSTACK_NEAR_1 = const(3)
MENUITEM_FOCUSSTACK_NEAR_2 = const(4)
MENUITEM_FOCUSSTACK_NEAR_3 = const(5)
MENUITEM_FOCUSSTACK_9POINT = const(6)
MENUITEM_REMOTESHUTTER     = const(7)
MENUITEM_REMOTESHUTTER_2S  = const(8)
MENUITEM_REMOTESHUTTER_5S  = const(9)
MENUITEM_REMOTESHUTTER_10S = const(10)
MENUITEM_MOVIE_RECORD      = const(11)
MENUITEM_CONFIG            = const(12)
MENUITEM_WIFI_INFO         = const(13)
MENUITEM_LAST = MENUITEM_WIFI_INFO

menu = [
    [ MENUITEM_REMOTESHUTTER     , "screens/remoteshutter" ],
    [ MENUITEM_REMOTESHUTTER_2S  , "screens/remoteshutter_2s" ],
    [ MENUITEM_MOVIE_RECORD      , "screens/movierecord" ],
    [ MENUITEM_FOCUSSTACK_FAR_1  , "screens/focusstack_far_1" ],
    [ MENUITEM_FOCUSSTACK_FAR_2  , "screens/focusstack_far_2" ],
    [ MENUITEM_FOCUSSTACK_FAR_3  , "screens/focusstack_far_3" ],
    #[ MENUITEM_FOCUSSTACK_NEAR_1 , "screens/focusstack_near_1" ],
    #[ MENUITEM_FOCUSSTACK_NEAR_2 , "screens/focusstack_near_2" ],
    #[ MENUITEM_FOCUSSTACK_NEAR_3 , "screens/focusstack_near_3" ],
    [ MENUITEM_FOCUSSTACK_9POINT , "screens/focus_9point" ],
    [ MENUITEM_REMOTESHUTTER_5S  , "screens/remoteshutter_5s" ],
    [ MENUITEM_REMOTESHUTTER_10S , "screens/remoteshutter_10" ],
    #[ MENUITEM_CONFIG            , "screens/config" ],
    [ MENUITEM_WIFI_INFO         , "screens/wifiinfo" ],
]

class AlphaFairy:
    def __init__(self):
        btnA.wasPressed(self.handle_btn_A)
        btnB.wasPressed(self.handle_btn_B)
        btnA.wasDoublePress(self.handle_btn_A2)
        btnB.wasDoublePress(self.handle_btn_B2)
        self.btn_pressed_big = False
        self.btn_pressed_side = False
        self.btn_pressed_big2 = False
        self.btn_pressed_side2 = False
        self.screen_size = lcd.screensize()
        self.net_mgr = netmgr.NetManager(self)
        self.camera = None
        self.ir = irsirc.InfraredSIRC()
        self.menu_state = MENUSTATE_MAIN
        self.menu_idx = 0
        self.submenu_idx = 0
        self.last_img_path = ""
        self.load_cfg()
        # continue to show welcome, this masks the network starting and connection
        tnow = fairyutils.time_millis()
        while (tnow - welcome_time) < 1000:
            self.task_poll()
            tnow = fairyutils.time_millis()
        if self.camera is None:
            self.show_full_screen("screens/connecting")
            tstart = fairyutils.time_millis()
            tnow = tstart
            while (tnow - tstart) < 1000 and self.camera is None:
                self.task_poll()
                tnow = fairyutils.time_millis()

    def load_cfg(self):
        self.cfg = {}
        self.cfg["wifi_ssid"] = DEFAULT_SSID
        self.cfg["wifi_password"] = DEFAULT_WIFI_PASSWORD
        self.cfg["focus_delay"] = 100
        self.cfg["shutter_press_time"] = 250
        try:
            with open("cfg.json", "r") as f:
                jcfg = json.load(f)
            for i in self.cfg:
                if i in jcfg:
                    self.cfg[i] = jcfg[i]
        except Exception as ex:
            print("ERROR loading config json: %s" % ex)

    def save_cfg(self):
        try:
            with open("cfg.json", "w") as f:
                json.dump(self.cfg, f)
        except Exception as ex:
            print("ERROR saving config json: %s" % ex)

    def make_ptp(self, sock, sock_evt):
        self.camera = ptpsonyalphacamera.PtpSonyAlphaCamera(sock, sock_evt)

    def handle_btn_A(self):
        self.btn_pressed_big = True

    def handle_btn_B(self):
        self.btn_pressed_side = True

    def handle_btn_A2(self):
        self.btn_pressed_big = True

    def handle_btn_B2(self):
        self.btn_pressed_side = False
        self.btn_pressed_side2 = True

    def show_wifi_info(self):
        lcd.font(lcd.FONT_Default)
        lcd.orient(lcd.LANDSCAPE)
        lcd.clear()
        xy = (0,0) if self.screen_size[1] == 160 else (5,5)
        lcd.print("SSID:\n %s\nPassword:\n %s\n" % (self.cfg["wifi_ssid"], self.cfg["wifi_password"]), xy[0], xy[1])
        if self.camera is not None:
            lcd.print("Client IP:\n %s" % self.net_mgr.ssdp_cli_addr)
        while btnA.isPressed():
            self.task_poll()
        self.show_full_screen("screens/wifiinfo.bmp")

    def focus_stack(self, speed):
        i = 0
        was_mf = self.camera.is_manuallyfocused()
        self.camera.ptpcmd_ManualFocusMode(True)
        while btnA.isPressed() and self.camera is not None:
            M5Led.on()
            self.task_poll(show_led=False)
            self.show_dots(5, i, reverse = speed > 0)
            self.camera.ptpcmd_ManualFocusStep(speed)
            self.camera.wait_while_busy(min_time=self.cfg["focus_delay"])
            self.camera.shoot(t = self.cfg["shutter_press_time"])
            i += 1
        if not was_mf:
            self.camera.ptpcmd_ManualFocusMode(False)
        M5Led.off()
        self.show_full_screen(None)

    def focus_stack_9points(self):
        x_spacing = int(round(ptpsonycodes.FOCUSPOINT_X_MAX / 4))
        y_spacing = int(round(ptpsonycodes.FOCUSPOINT_Y_MAX / 4))
        dot_spacing = 30
        dot_x_center = self.screen_size[0] // 2
        dot_y_center = 97 if self.screen_size[1] == 160 else 147
        cords = [
                [int(round(ptpsonycodes.FOCUSPOINT_X_MID            )), int(round(ptpsonycodes.FOCUSPOINT_Y_MID            )), dot_x_center              , dot_y_center              ],
                [int(round(ptpsonycodes.FOCUSPOINT_X_MID + x_spacing)), int(round(ptpsonycodes.FOCUSPOINT_Y_MID            )), dot_x_center + dot_spacing, dot_y_center              ],
                [int(round(ptpsonycodes.FOCUSPOINT_X_MID - x_spacing)), int(round(ptpsonycodes.FOCUSPOINT_Y_MID            )), dot_x_center - dot_spacing, dot_y_center              ],
                [int(round(ptpsonycodes.FOCUSPOINT_X_MID            )), int(round(ptpsonycodes.FOCUSPOINT_Y_MID + y_spacing)), dot_x_center              , dot_y_center + dot_spacing],
                [int(round(ptpsonycodes.FOCUSPOINT_X_MID            )), int(round(ptpsonycodes.FOCUSPOINT_Y_MID - y_spacing)), dot_x_center              , dot_y_center - dot_spacing],
                [int(round(ptpsonycodes.FOCUSPOINT_X_MID + x_spacing)), int(round(ptpsonycodes.FOCUSPOINT_Y_MID + y_spacing)), dot_x_center + dot_spacing, dot_y_center + dot_spacing],
                [int(round(ptpsonycodes.FOCUSPOINT_X_MID - x_spacing)), int(round(ptpsonycodes.FOCUSPOINT_Y_MID + y_spacing)), dot_x_center - dot_spacing, dot_y_center + dot_spacing],
                [int(round(ptpsonycodes.FOCUSPOINT_X_MID + x_spacing)), int(round(ptpsonycodes.FOCUSPOINT_Y_MID - y_spacing)), dot_x_center + dot_spacing, dot_y_center - dot_spacing],
                [int(round(ptpsonycodes.FOCUSPOINT_X_MID - x_spacing)), int(round(ptpsonycodes.FOCUSPOINT_Y_MID - y_spacing)), dot_x_center - dot_spacing, dot_y_center - dot_spacing]
            ]
        was_mf = self.camera.is_manuallyfocused()
        self.camera.ptpcmd_ManualFocusMode(False)
        i = 0
        while btnA.isPressed() and self.camera is not None:
            M5Led.on()
            self.task_poll(show_led=False)
            if i < len(cords):
                x = cords[i][0]
                y = cords[i][1]
            else:
                x = random.randint(0, ptpsonycodes.FOCUSPOINT_X_MAX)
                y = random.randint(0, ptpsonycodes.FOCUSPOINT_Y_MAX)
            self.camera.ptpcmd_FocusPointSet(x, y)
            self.camera.wait_while_busy(min_time=self.cfg["focus_delay"])
            self.camera.ptpcmd_AutoFocus(True)
            self.camera.wait_while_busy(min_time=2000, check_focus=True, timeout = 2000)
            self.camera.shoot(t = self.cfg["shutter_press_time"])
            self.camera.ptpcmd_AutoFocus(False)
            self.camera.wait_while_busy(min_time=self.cfg["focus_delay"] // 2)
            if i < len(cords):
                lcd.circle(cords[i][1], cords[i][2], 4, (0,255,0), (0,255,0))
            i += 1
        if was_mf:
            self.camera.ptpcmd_ManualFocusMode(True)
        M5Led.off()
        self.show_full_screen(None)

    def remote_shutter(self, dly = 0):
        is_mf = self.camera.is_manuallyfocused()
        if not is_mf:
            self.camera.ptpcmd_AutoFocus(True)
        i = 0
        while i < dly and self.btn_pressed_side == False and self.btn_pressed_side2 == False:
            self.task_poll(show_led=False)
            if dly > 2:
                self.show_dots(dly, i)
            t = fairyutils.time_millis()
            tnow = t
            while (tnow - t) < 1000:
                self.task_poll(show_led=False)
                tnow = fairyutils.time_millis()
                M5Led.on()
                self.camera.wait_while_busy(min_time=50)
                M5Led.off()
                self.camera.wait_while_busy(min_time=50)
            i += 1
        if self.btn_pressed_side == False and self.btn_pressed_side2 == False:
            self.camera.shoot(t = self.cfg["shutter_press_time"])
        self.btn_pressed_side  = False
        self.btn_pressed_side2 = False
        if not is_mf:
            self.camera.ptpcmd_AutoFocus(False)
        M5Led.off()
        self.show_full_screen(None)

    def record_movie(self):
        while btnA.isPressed() == True or btnB.isPressed() == True:
            self.camera.wait_while_busy()
        self.camera.ptpcmd_MovieRecord(True)
        M5Led.on()
        self.btn_pressed_big  = False
        self.btn_pressed_side = False
        i = 0
        while self.btn_pressed_big == False and btnA.isPressed() == False and self.btn_pressed_side == False and btnB.isPressed() == False:
            self.task_poll(show_led=False)
            self.show_dots(5, i)
            self.camera.wait_while_busy(min_time=1000)
            i += 1
        self.camera.ptpcmd_MovieRecord(False)
        self.btn_pressed_big  = False
        self.btn_pressed_side = False
        M5Led.off()
        self.show_full_screen(None)

    def irblast_shutter(self, dly=0):
        M5Led.on()
        if dly == 0:
            self.ir.tx(self.ir.ADDR_SONYCAMERA, self.ir.CMD_SHUTTER)
            self.ir.tx(self.ir.ADDR_SONYCAMERA, self.ir.CMD_SHUTTER)
        elif dly == 2:
            self.ir.tx(self.ir.ADDR_SONYCAMERA, self.ir.CMD_SHUTTER_2S)
            self.ir.tx(self.ir.ADDR_SONYCAMERA, self.ir.CMD_SHUTTER_2S)
        M5Led.off()

    def irblast_movie(self):
        M5Led.on()
        self.ir.tx(self.ir.ADDR_SONYCAMERA, self.ir.CMD_MOVIE)
        self.ir.tx(self.ir.ADDR_SONYCAMERA, self.ir.CMD_MOVIE)
        M5Led.off()

    def show_full_screen(self, img_path):
        lcd.orient(lcd.PORTRAIT)
        if img_path is None:
            img_path = self.last_img_path
        lcd.image(0, 0, img_path.replace("/", "_%u" % self.screen_size[1]) + ".jpg")
        self.last_img_path = img_path

    def show_dots(self, dots_cnt, dot_highlight, dot_size = 6, reverse = False, color_idle=(255,200,200), color_highlighted=(255,0,0), x_offset = 0, y_width = None):
        dot_highlight %= dots_cnt
        if reverse:
            dot_highlight = dots_cnt - dot_highlight - 1
        if y_width is None or y_width == 0:
            y_width = self.screen_size[1]
        spacing = y_width / (dots_cnt + 1)
        y_center = 0
        x_center = int(round((self.screen_size[0] // 2) + x_offset))
        i = 0
        while i < dots_cnt:
            y_center += spacing
            lcd.circle(x_center, int(round(y_center)), dot_size, color_idle, color_highlighted if dot_highlight == i else color_idle)
            i += 1
        self.last_img_path = 0

    def sleep(self, ms):
        t = fairyutils.time_millis()
        now = t
        while (now - t) < ms:
            fairyutils.time_sleep_ms(0)
            self.task_poll()
            now = fairyutils.time_millis()

    def task_poll(self, show_led=True):
        try:
            if self.net_mgr.state != self.net_mgr.NETMGRSTATE_POLLING:
                self.net_mgr.task()
            elif self.camera is not None:
                self.camera.task()
        except ConnectionAbortedError as ex:
            self.net_mgr.state = self.net_mgr.NETMGRSTATE_RESTART
            if self.camera is not None:
                self.camera.close()
                self.camera = None
        gc.collect()
        if show_led:
            t = fairyutils.time_millis()
            t %= 1500
            if t < 200:
                M5Led.on()
            elif self.camera is not None and t > 400 and t < 600:
                M5Led.on()
            else:
                M5Led.off()

    def task(self):
        self.task_poll()

        if self.menu_state == MENUSTATE_MAIN:

            if self.btn_pressed_side2:
                self.btn_pressed_side  = False
                self.btn_pressed_side2 = False
                if self.menu_idx <= 0:
                    self.menu_idx = len(menu) - 1
                else:
                    self.menu_idx -= 1
            elif self.btn_pressed_side:
                self.btn_pressed_side = False
                self.btn_pressed_side2 = False
                self.menu_idx += 1
                if self.menu_idx >= len(menu):
                    self.menu_idx = 0

            mi = menu[self.menu_idx]
            if mi is not None:
                img_path = mi[1]
                if img_path != self.last_img_path:
                    self.show_full_screen(img_path)
                if self.btn_pressed_big:
                    self.btn_pressed_big = False
                    mii = mi[0]
                    cam_ready = not (self.camera is None or (self.camera is not None and self.camera.is_ready() == False))
                    if IRBLASTER_ENABLED and self.ir.can_work() and not cam_ready and mii == MENUITEM_REMOTESHUTTER:
                        self.irblast_shutter(dly=0)
                    elif IRBLASTER_ENABLED and self.ir.can_work() and not cam_ready and mii == MENUITEM_REMOTESHUTTER_2S:
                        self.irblast_shutter(dly=2)
                    elif IRBLASTER_ENABLED and self.ir.can_work() and not cam_ready and mii == MENUITEM_MOVIE_RECORD:
                        self.irblast_movie()
                    elif not cam_ready and (mii >= MENUITEM_FOCUSSTACK_FAR_1 and mii <= MENUITEM_MOVIE_RECORD):
                        self.show_full_screen("screens/connecting")
                        self.sleep(1000)
                    #elif mii == MENUITEM_CONFIG:
                    #    self.menu_state = MENUSTATE_CONFIG
                    elif mii == MENUITEM_WIFI_INFO:
                        self.show_wifi_info()
                    elif mii >= MENUITEM_FOCUSSTACK_FAR_1 and mii <= MENUITEM_FOCUSSTACK_NEAR_3:
                        speed = (mii - MENUITEM_FOCUSSTACK_FAR_1) % 3
                        if speed == 0:
                            speed = ptpsonycodes.FOCUSSTEP_FARTHER_SMALL
                        elif speed == 1:
                            speed = ptpsonycodes.FOCUSSTEP_FARTHER_MEDIUM
                        elif speed == 2:
                            speed = ptpsonycodes.FOCUSSTEP_FARTHER_LARGE
                        if mii > MENUITEM_FOCUSSTACK_FAR_3:
                            speed *= -1
                        self.focus_stack(speed)
                    elif mii == MENUITEM_FOCUSSTACK_9POINT:
                        self.focus_stack_9points()
                    elif mii >= MENUITEM_REMOTESHUTTER and mii <= MENUITEM_REMOTESHUTTER_10S:
                        shutter_delay = 0
                        if mii == MENUITEM_REMOTESHUTTER_2S:
                            shutter_delay = 2
                        elif mii == MENUITEM_REMOTESHUTTER_5S:
                            shutter_delay = 5
                        elif mii == MENUITEM_REMOTESHUTTER_10S:
                            shutter_delay = 10
                        self.remote_shutter(dly = shutter_delay)
                    elif mii == MENUITEM_MOVIE_RECORD:
                        self.record_movie()

def main():
    try:
        hwsim.start_button_checker()
    except:
        pass
    app = AlphaFairy()
    while True:
        app.task()
        fairyutils.time_sleep_ms(0)

if __name__ == "__main__" or True:
    try:
        main()
    except Exception as ex:
        print("FATAL EXCEPTION: %s" % ex)
        sys.traceback.print_exception(ex)
        with open("last_error.txt", "w") as f:
            sys.traceback.print_exception(ex, file=f)
    finally:
        try:
            hwsim.stop_button_checker()
        except:
            pass
