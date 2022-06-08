#!/usr/bin/env python3

import sys, os, subprocess, time, datetime
import socket
import ptpcamera, ptpcodes, ptpsonycodes
import ptpsonyalphacamera
import fairyutilssim as fairyutils

DEFAULT_SSID          = "alphafairywifi"
DEFAULT_WIFI_PASSWORD = "1234567890"

def run_cmdline_read(x):
    s = ""
    p = subprocess.Popen(x, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    stdout, stderr = p.communicate()
    s = stdout.decode('utf-8')
    if "is not recognized as an internal or external command" in s:
        raise Exception("ERROR: command \"%\" does not exist" % x)
    return s

def arp():
    res = []
    x = run_cmdline_read("arp -a")
    lines = x.split('\n')
    for line in lines:
        li = line.strip()
        while "  " in li:
            li = li.replace("  ", " ").strip()
        pts = li.split(' ')
        if len(pts) == 3:
            addr = pts[0]
            if "." in addr:
                adrpts = addr.split('.')
                is_addr = True
                if len(adrpts) == 4:
                    for i in adrpts:
                        if not i.isnumeric() or int(i) > 255 or int(i) < 0:
                            is_addr = False
                if is_addr:
                    res.append(addr)
    return res

def nslookup(ip_addr):
    x = run_cmdline_read("nslookup %s" % ip_addr)
    lines = x.split('\n')
    for line in lines:
        li = line.strip()
        while "  " in li:
            li = li.replace("  ", " ").strip()
        pts = li.split(' ')
        if pts[0] == "Name:" and len(pts[1]) > 0:
            name = pts[1]
            if '.' in name:
                namepts = name.split('.')
                if len(namepts[0]) > 0:
                    return namepts[0]
                else:
                    return name
            else:
                return name
    return None

def get_devices():
    res = {}
    x = arp()
    for i in x:
        n = nslookup(i)
        if n is not None and len(n) > 0:
            res[i] = n
    return res

def find_camera_ip():
    d = get_devices()
    for i in d:
        if d[i].lower() == "sonyimagingdevice":
            return (i, d)
    return None

class NetManager:
    NETMGRSTATE_INIT          = (0)
    NETMGRSTATE_WAITFORCLIENT = (1)
    NETMGRSTATE_POLLING       = (2)
    NETMGRSTATE_RESTART       = (3)
    NETMGRSTATE_END           = (4)

    def __init__(self, parent):
        self.state = self.NETMGRSTATE_INIT
        self.ssdp_cli_addr = ""
        self.parent = parent

    def start_ap(self):
        pass

    def start_server(self):
        self.state = self.NETMGRSTATE_WAITFORCLIENT

    def ssdp_server_task(self):
        pass

    def restart_ssdp_server(self):
        self.start_server()

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
        if self.state < self.NETMGRSTATE_POLLING:
            x = find_camera_ip()
            if x is not None:
                if self.init_ptp_conn(x[0]):
                    self.ssdp_cli_addr = x[0]
                    if self.parent is not None:
                        self.parent.make_ptp(self.ptp_sock, self.ptp_sock_evt)
                    self.state = self.NETMGRSTATE_POLLING
        elif self.state == self.NETMGRSTATE_POLLING:
            pass

class FakeApp:
    def __init__(self):
        self.netmgr = NetManager(self)
        self.camera = None

    def make_ptp(self, sock1, sock2):
        self.camera = ptpsonyalphacamera.PtpSonyAlphaCamera(sock1, sock2)

    def task(self):
        self.netmgr.task()
        if self.camera is not None:
            self.camera.task()

def main():
    d = get_devices()
    for i in d:
        print("%24s - %s" % (i, d[i]))
    ptp_cam = None
    app = FakeApp()
    while True:
        app.task()
        time.sleep(0)
    pass

if __name__ == "__main__":
    main()
