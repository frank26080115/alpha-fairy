#!/usr/bin/env python3
"""Set AlphaFairy's RTC over serial, using local wall time unless --utc is chosen.

Requires pyserial: python -m pip install pyserial
Examples:
  python tools/set_rtc_time.py
  python tools/set_rtc_time.py --port COM7
  python tools/set_rtc_time.py --port COM7 --time "2026-11-05 12:34:56"
  python tools/set_rtc_time.py --utc

An explicit time without an offset is written literally. An ISO time with Z or
+/-HH:MM is converted to local time (or UTC with --utc). RTC stores no timezone.
"""
from __future__ import annotations

import argparse
from datetime import datetime, timedelta, timezone
import re
import sys
import time

try:
    import serial
    from serial.tools import list_ports
except ImportError:
    serial = None
    list_ports = None

DATE_FORMAT = "%Y-%m-%d %H:%M:%S"
ISO_TIME = re.compile(r"\d{4}-\d{2}-\d{2}[ T]\d{2}:\d{2}:\d{2}(?:Z|[+-]\d{2}:\d{2})?\Z", re.ASCII)
RTC_LINE = re.compile(r"RTC (\d{4}-\d{2}-\d{2} \d{2}:\d{2}:\d{2})\Z", re.ASCII)


def parse_time(text: str) -> datetime:
    if not ISO_TIME.fullmatch(text):
        raise argparse.ArgumentTypeError("use YYYY-MM-DD HH:MM:SS, optionally with Z or +/-HH:MM")
    try:
        return datetime.fromisoformat(text.replace("Z", "+00:00"))
    except ValueError as exc:
        raise argparse.ArgumentTypeError(str(exc)) from exc


def choose_time(explicit: datetime | None, utc: bool) -> datetime:
    if explicit is None:
        value = datetime.now(timezone.utc) if utc else datetime.now().astimezone()
    elif explicit.tzinfo is not None:
        value = explicit.astimezone(timezone.utc) if utc else explicit.astimezone()
    else:
        value = explicit
    if not 2000 <= value.year <= 2099:
        raise ValueError("RTC year must be between 2000 and 2099")
    return value.replace(tzinfo=None, microsecond=0)


def open_port(port: str, baud: int, settle: float):
    # Set line states before opening, to avoid the common DTR/RTS reset pulse.
    connection = serial.Serial(port=None, baudrate=baud, timeout=0.1, write_timeout=2)
    connection.dtr = False
    connection.rts = False
    connection.port = port
    try:
        connection.open()
        time.sleep(settle)
        return connection
    except Exception:
        connection.close()
        raise


def exchange(connection, command: str, timeout: float) -> str:
    connection.reset_input_buffer()
    connection.write((command + "\n").encode("ascii"))
    connection.flush()
    name = command.split()[0]
    deadline = time.monotonic() + timeout
    pending = b""
    while time.monotonic() < deadline:
        # readline can return a partial line when its serial timeout expires.
        pending = (pending + connection.readline())[-8192:]
        while b"\n" in pending:
            raw, pending = pending.split(b"\n", 1)
            line = raw.decode("ascii", errors="replace").strip().lstrip("> \t")
            if line.startswith(f"ERROR {name}:"):
                raise RuntimeError(line)
            if name == "rtcget" and RTC_LINE.fullmatch(line):
                return line
            if name == "rtcset" and re.fullmatch(r"OK rtcset \d{4}-\d{2}-\d{2} \d{2}:\d{2}:\d{2}", line, re.ASCII):
                return line
    raise TimeoutError(f"No {name} response on {connection.port}; check the port, baud rate and firmware")


def detect_port(baud: int, settle: float, timeout: float) -> str:
    # Never probe Bluetooth or virtual serial ports. USB candidates are identified
    # with a read-only command before any clock write is sent.
    candidates = [p for p in list_ports.comports() if p.vid is not None]
    matches = []
    failures = []
    for port in candidates:
        try:
            with open_port(port.device, baud, settle) as connection:
                try:
                    exchange(connection, "rtcget", timeout)
                except RuntimeError as exc:
                    # An uninitialized/missing RTC still identifies this firmware.
                    if not str(exc).startswith("ERROR rtcget:"):
                        raise
                matches.append(port.device)
        except (OSError, RuntimeError, TimeoutError, serial.SerialException) as exc:
            failures.append(f"{port.device}: {exc}")
    if len(matches) == 1:
        return matches[0]
    if len(matches) > 1:
        raise RuntimeError("Multiple AlphaFairy devices found: " + ", ".join(matches) + ". Select one with --port.")
    detail = "; ".join(failures) if failures else "No USB serial ports found"
    raise RuntimeError(f"Could not detect AlphaFairy. {detail}. Use --list-ports or specify --port.")


def set_rtc(connection, explicit: datetime | None, utc: bool, timeout: float) -> datetime:
    # Choose 'now' after discovery, opening and startup delays, just before sending.
    target = choose_time(explicit, utc)
    started = time.monotonic()
    print(f"Sending to {connection.port}: rtcset {target.strftime(DATE_FORMAT)}")
    reply = exchange(connection, "rtcset " + target.strftime(DATE_FORMAT), timeout)
    acknowledged = datetime.strptime(reply[len("OK rtcset "):], DATE_FORMAT)
    elapsed = time.monotonic() - started
    if not target <= acknowledged <= target + timedelta(seconds=elapsed + 2):
        raise RuntimeError(f"RTC acknowledgement mismatch: requested {target}, received {acknowledged}")
    print(reply)
    reply = exchange(connection, "rtcget", timeout)
    actual = datetime.strptime(RTC_LINE.fullmatch(reply).group(1), DATE_FORMAT)
    elapsed = time.monotonic() - started
    if not target <= actual <= target + timedelta(seconds=elapsed + 2):
        raise RuntimeError(f"RTC readback mismatch: requested {target}, received {actual}")
    print(reply)
    return actual


def main(argv=None) -> int:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("-p", "--port", help="COM port, e.g. COM7; otherwise detect USB AlphaFairy devices")
    parser.add_argument("-t", "--time", type=parse_time, help="explicit date/time; default: current computer time")
    parser.add_argument("--utc", action="store_true", help="use UTC for current or offset-qualified input time")
    parser.add_argument("--baud", type=int, default=500000, help="serial baud rate (default: 500000)")
    parser.add_argument("--timeout", type=float, default=5, help="response timeout in seconds (default: 5)")
    parser.add_argument("--settle", type=float, default=1, help="wait after opening serial, in seconds (default: 1)")
    parser.add_argument("--list-ports", action="store_true", help="list serial ports and exit")
    parser.add_argument("--dry-run", action="store_true", help="print the intended RTC command without opening serial")
    args = parser.parse_args(argv)
    if args.baud <= 0 or args.timeout <= 0 or args.settle < 0:
        parser.error("baud and timeout must be positive; settle must be nonnegative")
    try:
        choose_time(args.time, args.utc) # Validate range before accessing a device.
        if args.dry_run:
            target = choose_time(args.time, args.utc)
            print("rtcset " + target.strftime(DATE_FORMAT))
            return 0
        if serial is None:
            raise RuntimeError("pyserial is required: python -m pip install pyserial")
        if args.list_ports:
            for port in list_ports.comports():
                print(f"{port.device}: {port.description}")
            return 0
        port = args.port or detect_port(args.baud, args.settle, args.timeout)
        with open_port(port, args.baud, args.settle) as connection:
            set_rtc(connection, args.time, args.utc, args.timeout)
        return 0
    except (OSError, RuntimeError, TimeoutError, ValueError) as exc:
        print(f"ERROR: {exc}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
