"""Poll usbd enumeration contexts and log every change (with timestamps)."""
import os, sys, time, hashlib
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from xbdm import Xbdm, hexdump
from usbenum import ENUM_ARRAY, UNRECOGNIZED
log = open(sys.argv[1], "a", buffering=1)
x = Xbdm()
ctxs = [x.u32(ENUM_ARRAY + 4 * i) for i in range(4)]
last = {}
t0 = time.time()
while True:
    try:
        for i, c in enumerate(ctxs):
            d = x.read(c, 0x1B0) + x.read(c + 0x270, 0x10)
            h = hashlib.md5(bytes(v or 0 for v in d)).hexdigest()
            if last.get(i) != h:
                last[i] = h
                log.write("=== t=%.2f controller %d\n" % (time.time() - t0, i))
                import io, contextlib
                buf = io.StringIO()
                with contextlib.redirect_stdout(buf):
                    hexdump(c, d[:0x1B0]); hexdump(c + 0x270, d[0x1B0:])
                log.write(buf.getvalue())
        u = x.read(UNRECOGNIZED, 0xA0)
        hu = bytes(v or 0 for v in u)
        if last.get("u") != hu:
            last["u"] = hu
            log.write("=== t=%.2f unrecognized %s\n" % (time.time() - t0, hu.hex()))
        time.sleep(0.05)
    except Exception as e:
        log.write("=== t=%.2f error %r\n" % (time.time() - t0, e))
        time.sleep(1)
        try:
            x = Xbdm()
        except Exception:
            pass
