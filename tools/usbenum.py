"""Dump usbd's per-controller enumeration contexts (retail 17559)."""
import os, struct, sys, time
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from xbdm import Xbdm, hexdump

ENUM_ARRAY = 0x801A8220
UNRECOGNIZED = 0x801A7FA0

def show(x):
    for i in range(4):
        ctx = x.u32(ENUM_ARRAY + 4 * i)
        print("controller %d context %08X" % (i, ctx or 0))
        if not ctx:
            continue
        d = bytes(v or 0 for v in x.read(ctx, 0x290))
        dev = d[0x4C:0x4C + 18]
        vid, pid = struct.unpack("<HH", dev[8:12])
        print("  device desc: class %02X/%02X/%02X ep0 %d vid:pid %04X:%04X configs %d" % (dev[4], dev[5], dev[6], dev[7], vid, pid, dev[17]))
        print("  bufsize %X  +27A %02X +27B %02X +27C %02X +27D %02X +27E %02X  ifaceofs %X" % (struct.unpack(">I", d[0x28:0x2C])[0], d[0x27A], d[0x27B], d[0x27C], d[0x27D], d[0x27E], struct.unpack(">H", d[0x5E:0x60])[0]))
        print("  header:")
        hexdump(ctx, list(d[:0x60]))
        tl = struct.unpack("<H", d[0x62:0x64])[0]
        print("  config (wTotalLength %d):" % tl)
        hexdump(ctx + 0x60, list(d[0x60:0x60 + min(max(tl, 9), 0x150)]))
    print("unrecognized ports:", " ".join("%08X/%02X" % (x.u32(UNRECOGNIZED + 8 * i) or 0, x.read(UNRECOGNIZED + 8 * i + 4, 1)[0] or 0) for i in range(20) if x.u32(UNRECOGNIZED + 8 * i)))

if __name__ == "__main__":
    show(Xbdm())
