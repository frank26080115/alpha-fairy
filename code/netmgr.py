#!/usr/bin/env python3

import gc
gc.collect()

import micropython
import esp
esp.osdebug(None)
import usocket as socket
import uasyncio as asyncio
import utime as time
import uselect as select
import uerrno as errno
import network
import fairyutils

DEFAULT_SSID          = "alphafairywifi"
DEFAULT_WIFI_PASSWORD = "1234567890"
PORT_SSDP     = micropython.const(1900)
TIM_TINY      = micropython.const(0.05)

class NetManager:
    NETMGRSTATE_INIT          = micropython.const(0)
    NETMGRSTATE_WAITFORCLIENT = micropython.const(1)
    NETMGRSTATE_POLLING       = micropython.const(2)
    NETMGRSTATE_RESTART       = micropython.const(3)

    def __init__(self, parent = None):
        self.ap = None
        self.udp_srv_sock = None
        self.udp_srv_poller = None
        self.ssdp_cli_sock = None
        self.ssdp_cli_addr = None
        self.parent = parent

    def start_ap(self):
        self.ap = network.WLAN(network.AP_IF)
        self.ap.active(True)
        if self.parent is not None:
            self.ap.config(essid=self.parent.cfg["wifi_ssid"], password=self.parent.cfg["wifi_password"])
        else:
            self.ap.config(essid=DEFAULT_SSID, password=DEFAULT_WIFI_PASSWORD)
        while ap.active() == False:
            pass
        print("AP started")

    def start_server(self):
        addr = socket.getaddrinfo('0.0.0.0', PORT_SSDP, 0, socket.SOCK_STREAM)[0][-1]
        self.udp_srv_sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)  # server socket
        self.udp_srv_sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        self.udp_srv_sock.bind(addr)
        self.udp_srv_sock.listen(3)
        print('awaiting connection %s %d' % (addr, port))
        self.udp_srv_poller = select.poll()
        self.udp_srv_poller.register(self.udp_srv_sock, select.POLLIN)

    def ssdp_server_task(self):
        res = self.udp_srv_poller.poll(1)
        if res:
            c_sock, c_addr = self.udp_srv_sock.accept()
            c_sock.setblocking(False)
            self.ssdp_cli_sock = c_sock
            self.ssdp_cli_addr = c_addr
            print("SSDP client connected %s" % c_addr)
            start_time = fairyutils.time_millis()
            while True:
                try:
                    d = self.ssdp_cli_sock.recv(4096)
                    if len(d) > 0:
                        print("SSDP client recv data %d" % len(d))
                        return True
                    else:
                        if (fairyutils.time_millis() - start_time) > 5000:
                            return False
                        else:
                            # Waiting for data from client. Limit CPU overhead. 
                            await asyncio.sleep(TIM_TINY)
                except OSError as ex:
                    err = ex.args[0]
                    if err == errno.EAGAIN:
                        if (fairyutils.time_millis() - start_time) > 5000:
                            return False
                        else:
                            # Waiting for data from client. Limit CPU overhead. 
                            await asyncio.sleep(TIM_TINY)
                    else:
                        return False
        return False

    def restart_ssdp_server(self):
        if self.udp_srv_sock is not None:
            try:
                self.udp_srv_sock.close()
            except:
                pass
            self.udp_srv_sock = None
        if self.ssdp_cli_sock is not None:
            try:
                self.ssdp_cli_sock.close()
            except:
                pass
            self.ssdp_cli_sock = None
        gc.collect()
        self.start_server()
        self.state = self.NETMGRSTATE_WAITFORCLIENT

    def init_ptp_conn(self, ip):
        try:
            self.ptp_sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            self.ptp_sock.setsockopt(socket.SOL_SOCKET, socket.SO_KEEPALIVE, 1)
            self.ptp_sock.settimeout(0.1)
            self.ptp_sock.connect((ip, 15740))
            self.ptp_sock_evt = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            self.ptp_sock_evt.setsockopt(socket.SOL_SOCKET, socket.SO_KEEPALIVE, 1)
            self.ptp_sock_evt.settimeout(0.1)
            self.ptp_sock_evt.connect((ip, 15740))
            return True
        except Exception as ex:
            print("ERROR exception while connecting PTP socket: %s" % ex)
            return False

    def task(self):
        if self.state == self.NETMGRSTATE_INIT:
            self.start_ap()
            self.restart_ssdp_server()
        elif self.state == self.NETMGRSTATE_RESTART:
            self.restart_ssdp_server()
        if self.state == self.NETMGRSTATE_WAITFORCLIENT:
            if self.ssdp_server_task():
                if self.init_ptp_conn(self.ssdp_cli_addr):
                    if self.parent is not None:
                        self.parent.make_ptp(self.ptp_sock, self.ptp_sock_evt)
                    self.udp_srv_sock.close()
                    self.state = self.NETMGRSTATE_POLLING
                else:
                    self.restart_ssdp_server()
            else:
                self.restart_ssdp_server()
        if self.state == self.NETMGRSTATE_POLLING:
            pass

def main():
    netmgr = NetManager()
    while True:
        netmgr.task()
    pass

if __name__ == "__main__":
    main()
