"""Follow hiddriver's GIP trace ring live over xbdm. Needs a "build.cmd giptrace" build.

  python tools\\giptrace.py          print new entries as they arrive (Ctrl+C to stop)
  python tools\\giptrace.py once     print what is there and exit
"""
import os
import re
import struct
import sys
import time

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from xbdm import Xbdm

MAGIC = b"GIPTRACE"


def find_trace(x):
    for line in x.cmd("modules")[1:]:
        m = re.search(r'name="([^"]+)" base=0x([0-9a-fA-F]+) size=0x([0-9a-fA-F]+)', line)
        if not m or "hiddriver" not in m.group(1).lower():
            continue
        base, size = int(m.group(2), 16), int(m.group(3), 16)
        for a in range(base, base + size, 0x4000):
            chunk = bytes(v or 0 for v in x.read(a, min(0x4000 + 8, base + size - a)))
            i = chunk.find(MAGIC)
            if i >= 0:
                return a + i
    raise SystemExit("hiddriver trace not found (is a giptrace build loaded?)")


def fmt(tick, code, a, b):
    tag = struct.pack(">I", code).decode("latin-1")
    extra = ""
    if tag in ("RECV", "SEND"):
        extra = "  bytes " + struct.pack(">II", a, b).hex(" ")
    elif tag == "ADD ":
        extra = "  vid:pid %04X:%04X" % (b >> 16, b & 0xFFFF)
    elif tag == "MTCH":
        d = struct.pack(">I", a)
        extra = "  iface %d alt %d eps %d class %02X -> %d" % (d[0], d[1], d[2], d[3], b)
    return "[%8u] %s  %08X %08X%s" % (tick, tag, a, b, extra)


def main():
    x = Xbdm()
    addr = find_trace(x)
    print("trace at %08X" % addr)
    seen = 0
    while True:
        hdr = bytes(x.read(addr + 8, 8))
        count, size = struct.unpack(">iI", hdr)
        if count > seen:
            first = max(seen, count - size)
            raw = bytes(v or 0 for v in x.read(addr + 32, size * 16))
            for i in range(first, count):
                print(fmt(*struct.unpack_from(">IIII", raw, (i % size) * 16)), flush=True)
            seen = count
        if len(sys.argv) > 1 and sys.argv[1] == "once":
            return
        time.sleep(0.02)


if __name__ == "__main__":
    main()
