"""Minimal xbdm client for poking at a live console from the PC.

  python tools\\xbdm.py mem <addr> <len>        hex dump
  python tools\\xbdm.py dis <addr> <count>      disassemble (needs capstone)
  python tools\\xbdm.py cmd <text>              raw xbdm command
"""
import os
import socket
import struct
import sys

# Set XBOX_IP, or XBOX_MAC (aa-bb-cc-dd-ee-ff) to look the console up in the arp table, Windows connection
# sharing hands it a new address on every boot
HOST = os.environ.get("XBOX_IP")
XBOX_MAC = os.environ.get("XBOX_MAC", "").lower().replace(":", "-")


def candidates():
    import subprocess
    arp = subprocess.run(["arp", "-a"], capture_output=True, text=True).stdout
    return [l.split()[0] for l in arp.splitlines() if XBOX_MAC in l.lower()][::-1]


def connect():
    for ip in [HOST] if HOST else candidates() if XBOX_MAC else []:
        try:
            return socket.create_connection((ip, 730), timeout=2)
        except OSError:
            pass
    raise SystemExit("console not found on port 730 (set XBOX_IP or XBOX_MAC)")


class Xbdm:
    def __init__(self):
        self.s = connect()
        self.s.settimeout(5)
        self.f = self.s.makefile("rb")
        self.line()  # 201- connected

    def line(self):
        return self.f.readline().decode("latin-1").rstrip("\r\n")

    def cmd(self, text):
        self.s.sendall(text.encode() + b"\r\n")
        first = self.line()
        lines = [first]
        if first.startswith("202"):
            while True:
                l = self.line()
                if l == ".":
                    break
                lines.append(l)
        return lines

    def read(self, addr, length):
        """Returns bytes, with None for unreadable bytes."""
        out = []
        while length > 0:
            n = min(length, 0x400)
            lines = self.cmd("getmem addr=0x%08X length=0x%X" % (addr, n))
            if not lines[0].startswith("202"):
                raise RuntimeError(lines[0])
            hexs = "".join(lines[1:])
            for i in range(0, len(hexs), 2):
                pair = hexs[i:i + 2]
                out.append(None if pair == "??" else int(pair, 16))
            addr += n
            length -= n
        return out

    def u32(self, addr):
        b = self.read(addr, 4)
        if None in b:
            return None
        return struct.unpack(">I", bytes(b))[0]


def hexdump(addr, data):
    for i in range(0, len(data), 16):
        row = data[i:i + 16]
        print("%08X: %s" % (addr + i, " ".join("??" if b is None else "%02X" % b for b in row)))


def disasm(addr, data):
    import capstone
    md = capstone.Cs(capstone.CS_ARCH_PPC, capstone.CS_MODE_32 | capstone.CS_MODE_BIG_ENDIAN)
    raw = bytes(0 if b is None else b for b in data)
    for i in md.disasm(raw, addr):
        print("%08X: %s %-8s %s" % (i.address, raw[i.address - addr:i.address - addr + 4].hex().upper(), i.mnemonic, i.op_str))


if __name__ == "__main__":
    x = Xbdm()
    op = sys.argv[1]
    if op == "mem":
        a, n = int(sys.argv[2], 16), int(sys.argv[3], 16)
        hexdump(a, x.read(a, n))
    elif op == "dis":
        a, n = int(sys.argv[2], 16), int(sys.argv[3], 0)
        disasm(a, x.read(a, n * 4))
    elif op == "cmd":
        print("\n".join(x.cmd(" ".join(sys.argv[2:]))))
