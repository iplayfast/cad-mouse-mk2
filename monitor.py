#!/usr/bin/env python3
"""
Monitor serial telemetry from the CAD Mouse and display live + aggregate values.

Usage:
  python3 monitor.py [PORT] [BAUD]

Defaults: PORT=/dev/ttyACM0, BAUD=115200

Keys:
  r  - reset aggregate accumulator
  q  - quit
"""

import os
import sys
import select
import termios
import tty
import re
import serial
import time

PORT = sys.argv[1] if len(sys.argv) > 1 else "/dev/ttyACM0"
BAUD = int(sys.argv[2]) if len(sys.argv) > 2 else 115200
LOG_FILE = "mouse_monitor.log"


def open_log():
    return open(LOG_FILE, "w", buffering=1)


def reset_log():
    try:
        os.remove(LOG_FILE)
    except FileNotFoundError:
        pass
    return open_log()

AXES = ["X", "Y", "Z", "Rx", "Ry", "Rz"]
PATTERN = re.compile(r"^>([A-Za-z]+):(-?[\d.]+(?:e[+-]?\d+)?)$")

BAR_HALF = 20          # chars on each side of the centre pip
BAR_COLS = BAR_HALF * 2 + 3  # [ + left + | + right + ]


def make_bar(val, limit=350):
    frac = max(-1.0, min(1.0, val / limit))
    filled = int(abs(frac) * BAR_HALF)
    if frac >= 0:
        left  = " " * BAR_HALF
        right = "#" * filled + "." * (BAR_HALF - filled)
    else:
        left  = "." * (BAR_HALF - filled) + "#" * filled
        right = " " * BAR_HALF
    return f"[{left}|{right}]"


def render(live, aggregate, sample_count):
    lines = []
    lines.append("\033[H\033[2J")
    lines.append(f"  CAD Mouse Monitor  (samples: {sample_count})  [r=reset  q=quit]")
    lines.append("")
    lines.append(f"  {'Axis':<4}  {'Live':>8}  {'Bar (live)':{BAR_COLS}}  {'Aggregate':>12}")
    lines.append(f"  {'-'*4}  {'-'*8}  {'-'*BAR_COLS}  {'-'*12}")
    for ax in AXES:
        lv = live.get(ax, 0.0)
        ag = aggregate.get(ax, 0.0)
        bar = make_bar(lv)
        lines.append(f"  {ax:<4}  {lv:>8.2f}  {bar}  {ag:>12.1f}")
    lines.append("")
    return "\r\n".join(lines)


def main():
    try:
        ser = serial.Serial(PORT, BAUD, timeout=0.1)
    except serial.SerialException as e:
        print(f"Error opening {PORT}: {e}")
        sys.exit(1)

    live = {ax: 0.0 for ax in AXES}
    aggregate = {ax: 0.0 for ax in AXES}
    sample_count = 0
    log = open_log()

    # Put stdin in raw mode so we can read single keypresses
    fd = sys.stdin.fileno()
    old_settings = termios.tcgetattr(fd)
    tty.setraw(fd)

    try:
        print(render(live, aggregate, sample_count), end="", flush=True)
        buf = b""
        while True:
            # Check for keypress
            if select.select([sys.stdin], [], [], 0)[0]:
                ch = sys.stdin.read(1)
                if ch in ("q", "Q", "\x03"):
                    break
                if ch in ("r", "R"):
                    aggregate = {ax: 0.0 for ax in AXES}
                    sample_count = 0
                    log.close()
                    log = reset_log()

            # Read serial data
            chunk = ser.read(256)
            if chunk:
                buf += chunk
                while b"\n" in buf:
                    line, buf = buf.split(b"\n", 1)
                    text = line.decode("utf-8", errors="ignore").strip()
                    m = PATTERN.match(text)
                    if m:
                        key, val_str = m.group(1), m.group(2)
                        if key in live:
                            val = float(val_str)
                            live[key] = val
                            aggregate[key] += val
                            if key == "Rz":
                                sample_count += 1
                                log.write(
                                    f"{sample_count}\t"
                                    + "\t".join(f"{live[ax]:.4f}" for ax in AXES)
                                    + "\n"
                                )
                            print(render(live, aggregate, sample_count), end="", flush=True)
            else:
                time.sleep(0.01)

    finally:
        log.close()
        termios.tcsetattr(fd, termios.TCSADRAIN, old_settings)
        ser.close()
        print("\033[H\033[2J\033[?25h", end="")  # clear + restore cursor


if __name__ == "__main__":
    main()
