import argparse
import sys
import time

import serial


parser = argparse.ArgumentParser()
parser.add_argument("--port", default="COM3")
parser.add_argument("--seconds", type=float, default=60)
parser.add_argument("--reset", action="store_true")
args = parser.parse_args()

deadline = time.monotonic() + args.seconds
with serial.Serial(args.port, 115200, timeout=0.25) as port:
    if args.reset:
        port.dtr = False
        port.rts = True
        time.sleep(0.15)
        port.rts = False

    while time.monotonic() < deadline:
        data = port.readline()
        if data:
            text = data.decode("utf-8", errors="replace").rstrip()
            sys.stdout.buffer.write((text + "\n").encode("utf-8", errors="replace"))
            sys.stdout.buffer.flush()
