"""Read and synchronize a Sony A1 II over the installed Windows USB/WPD driver.

Build wpd_probe.exe with build.cmd first. Python uses only the standard library.
All traffic and host timestamps are saved as JSONL alongside this script.
"""
from __future__ import annotations

import argparse
from datetime import datetime, timedelta, timezone
import json
from pathlib import Path
import queue
import struct
import subprocess
import threading
import time

ROOT = Path(__file__).resolve().parent


class Wpd:
    def __init__(self, log_path: Path):
        self.log = log_path.open("w", encoding="utf-8")
        self.proc = subprocess.Popen(
            [str(ROOT / "wpd_probe.exe")], stdin=subprocess.PIPE,
            stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
            text=True, encoding="utf-8", bufsize=1,
        )
        self.lines = queue.Queue()

        def reader():
            for line in self.proc.stdout:
                self.lines.put(line)
            self.lines.put(None)

        threading.Thread(target=reader, daemon=True).start()
        while True:
            row = self.receive()
            if row.get("fatal"):
                self.close()
                raise RuntimeError(row["fatal"])
            if row.get("ready"):
                break

    def receive(self):
        try:
            line = self.lines.get(timeout=35)
        except queue.Empty:
            self.proc.kill()
            raise TimeoutError("Windows PTP command did not complete in 35 seconds")
        if line is None:
            raise RuntimeError("WPD bridge exited unexpectedly")
        row = json.loads(line)
        row["host_received"] = datetime.now().astimezone().isoformat(timespec="milliseconds")
        self.log.write(json.dumps(row) + "\n")
        self.log.flush()
        return row

    def transact(self, direction, opcode, *params, payload=b""):
        request = f"{direction} {opcode:04x}" + "".join(f" {p:x}" for p in params)
        if direction == "write":
            request += " | " + payload.hex()
        start = datetime.now().astimezone()
        self.proc.stdin.write(request + "\n")
        self.proc.stdin.flush()
        row = self.receive()
        stop = datetime.now().astimezone()
        if "error" in row:
            print(f"{request.split('|')[0].strip()}: {row['error']}")
        else:
            print(f"{request.split('|')[0].strip()}: response=0x{row['response']:04x}, {row['size']} bytes")
        row["midpoint"] = start + (stop - start) / 2
        return row

    def initialize(self):
        # Match lib/PtpIpCamera/PtpIpSonyAlphaCamera.cpp's current mode-3 init.
        for opcode, params in [
            (0x9201, (1, 0, 0)), (0x9201, (2, 0, 0)),
            (0x9202, (300, 0, 0)), (0x9201, (3, 0, 0)),
            (0x9202, (300, 0, 0)),
        ]:
            require_ok(self.transact("read", opcode, *params))

    def close(self):
        if self.proc.poll() is None:
            self.proc.stdin.write("quit\n")
            self.proc.stdin.flush()
            try:
                self.proc.wait(timeout=5)
            except subprocess.TimeoutExpired:
                self.proc.kill()
                self.proc.wait(timeout=5)
        self.log.close()


def require_ok(row):
    if row.get("response") != 0x2001:
        raise RuntimeError(f"Camera transaction failed: {row.get('error', row.get('response'))}")
    return bytes.fromhex(row["data"])


def parse_clock(data):
    """0x9248: u32 version, ASCII datetime NUL, ASCII base offset NUL, u8 DST."""
    version, = struct.unpack_from("<I", data)
    date, area, rest = data[4:].split(b"\0", 2)
    if version != 100 or len(rest) != 1 or rest[0] not in (0, 1):
        raise ValueError(f"Unexpected clock format: {data.hex()}")
    date, area, dst = date.decode("ascii"), area.decode("ascii"), bool(rest[0])
    sign = -1 if area[0] == "-" else 1
    minutes = sign * (int(area[1:3]) * 60 + int(area[3:5]))
    offset = timezone(timedelta(minutes=minutes, hours=int(dst)))
    dt = datetime.strptime(date, "%Y%m%dT%H%M%S.%f").replace(tzinfo=offset)
    return {"version": version, "date_time": date, "area": area, "dst": dst, "datetime": dt}


def get_clock(cam):
    row = cam.transact("read", 0x9248)
    clock = parse_clock(require_ok(row))
    clock["offset_from_host_seconds"] = round((clock["datetime"] - row["midpoint"]).total_seconds(), 3)
    print(f"Camera: {clock['datetime'].isoformat()} area={clock['area']} DST={clock['dst']} "
          f"offset from host={clock['offset_from_host_seconds']:+.3f}s")
    record = {k: v.isoformat() if isinstance(v, datetime) else v for k, v in clock.items()}
    cam.log.write(json.dumps({"decoded_clock": record}) + "\n")
    cam.log.flush()
    return clock


def ptp_string(text):
    # PTP STR: u8 number of UTF-16 code units including the trailing NUL.
    encoded = text.encode("utf-16le") + b"\0\0"
    if len(encoded) // 2 > 255:
        raise ValueError("PTP string too long")
    return bytes([len(encoded) // 2]) + encoded


def local_now():
    now = datetime.now().astimezone()
    # time.localtime supplies the actual local DST flag on Windows.
    dst = time.localtime(now.timestamp()).tm_isdst > 0
    base = now.utcoffset() - timedelta(hours=int(dst))
    minutes = int(base.total_seconds() / 60)
    area = f"{'-' if minutes < 0 else '+'}{abs(minutes) // 60:02d}{abs(minutes) % 60:02d}"
    date = now.strftime("%Y%m%dT%H%M%S") + f".{now.microsecond // 100000}"
    return now, date, area, dst


def timezone_payload(date, area, dst, *, set_date=True, set_area=True, set_dst=True):
    # 0x9249 uses ASCII C strings and a separate enable byte before each field.
    # This is different from both PTP STR and the 0x9248 response layout.
    return (struct.pack("<I", 100) + bytes([set_date]) + date.encode("ascii") + b"\0"
            + bytes([set_area]) + area.encode("ascii") + b"\0" + bytes([set_dst, dst]))


def sync_timezone(cam):
    now, date, area, dst = local_now()
    print(f"Setting current computer time: {now.isoformat()}, base area={area}, DST={dst}")
    require_ok(cam.transact("write", 0x9249, payload=timezone_payload(date, area, dst)))
    time.sleep(0.3)
    clock = get_clock(cam)
    if abs(clock["offset_from_host_seconds"]) > 2:
        raise RuntimeError("Clock set returned OK but readback differs from the computer by more than 2s")
    return clock


def sync_d223(cam):
    now, date, _, _ = local_now()
    print(f"Setting D223 to current computer wall time: {date}")
    require_ok(cam.transact("write", 0x9205, 0xD223, payload=ptp_string(date)))
    time.sleep(0.3)
    clock = get_clock(cam)
    # D223 preserves the camera's area and DST. Compare wall times independently
    # of those settings; use sync (0x9249) to synchronize area/DST as well.
    delta = (clock["datetime"].replace(tzinfo=None) - datetime.now()).total_seconds()
    if abs(delta) > 2:
        raise RuntimeError("D223 returned OK but wall-clock readback differs by more than 2s")
    return clock


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("action", choices=["get", "probe", "sync", "sync-d223", "watch", "test-d223", "test-5011"])
    parser.add_argument("--no-init", action="store_true", help="Skip Sony SDIO handshake")
    parser.add_argument("--log", type=Path)
    parser.add_argument("--test-offset-seconds", type=int, default=30,
                        help="Temporary offset for test-d223; restored via 0x9249 immediately")
    parser.add_argument("--d223-format", choices=["seconds", "fraction"], default="fraction")
    args = parser.parse_args()
    if not -60 <= args.test_offset_seconds <= 60:
        parser.error("Test offset must be within +/-60 seconds")
    log = args.log or ROOT / (datetime.now().strftime("%Y%m%d_%H%M%S_") + args.action + ".jsonl")
    print(f"Transcript: {log}")
    cam = Wpd(log)
    try:
        if not args.no_init:
            cam.initialize()
        get_clock(cam)
        if args.action == "probe":
            cam.transact("read", 0x1001)
            cam.transact("read", 0x1014, 0x5011)
            cam.transact("read", 0x1015, 0x5011)
            # Sony properties/controls list plus all current descriptors.
            cam.transact("read", 0x9209)
        elif args.action == "sync":
            sync_timezone(cam)
            time.sleep(2)
            get_clock(cam)
        elif args.action == "sync-d223":
            sync_d223(cam)
            time.sleep(2)
            get_clock(cam)
        elif args.action == "watch":
            for _ in range(5):
                time.sleep(2)
                get_clock(cam)
        elif args.action == "test-d223":
            # Write only the clock property using Sony's immediate property setter.
            # Always finish at correct computer time via 0x9249, even if D223 fails.
            try:
                now, _, _, _ = local_now()
                target = now + timedelta(seconds=args.test_offset_seconds)
                value = target.strftime("%Y%m%dT%H%M%S")
                if args.d223_format == "fraction":
                    value += f".{target.microsecond // 100000}"
                print(f"Temporary D223 test time: {value}; will restore current computer time")
                cam.transact("write", 0x9205, 0xD223, payload=ptp_string(value))
                time.sleep(1)
                after = get_clock(cam)
                observed = after["offset_from_host_seconds"]
                applied = abs(observed - args.test_offset_seconds) < 2
                verdict = {"d223_format": args.d223_format, "requested_offset": args.test_offset_seconds,
                           "observed_offset": observed, "applied": applied}
                print(json.dumps(verdict))
                cam.log.write(json.dumps({"d223_test": verdict}) + "\n")
            finally:
                sync_timezone(cam)
        elif args.action == "test-5011":
            try:
                now, _, _, _ = local_now()
                cam.transact("write", 0x1016, 0x5011, payload=ptp_string(now.strftime("%Y%m%dT%H%M%S")))
                get_clock(cam)
            finally:
                sync_timezone(cam)
        return 0
    finally:
        cam.close()


if __name__ == "__main__":
    raise SystemExit(main())
