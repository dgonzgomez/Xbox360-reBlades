"""Small dependency-free RGB PNG helpers for the project reference assets."""
import struct
import zlib
from pathlib import Path

def decode(path):
    data = Path(path).read_bytes()
    i = 8
    idat = []
    w = h = 0
    while i < len(data):
        ln = struct.unpack(">I", data[i : i + 4])[0]
        typ = data[i + 4 : i + 8]
        chunk = data[i + 8 : i + 8 + ln]
        if typ == b"IHDR":
            w, h = struct.unpack(">II", chunk[:8])
            if chunk[8:] != bytes((8, 2, 0, 0, 0)):
                raise ValueError("Expected non-interlaced 8-bit RGB PNG")
        elif typ == b"IDAT":
            idat.append(chunk)
        elif typ == b"IEND":
            break
        i += 12 + ln
    raw = zlib.decompress(b"".join(idat))
    stride = w * 3
    pos = 0
    prev = bytearray(stride)
    rows = []
    for _y in range(h):
        f = raw[pos]
        pos += 1
        row = bytearray(raw[pos : pos + stride])
        pos += stride
        if f == 1:
            for x in range(stride):
                a = row[x - 3] if x >= 3 else 0
                row[x] = (row[x] + a) & 255
        elif f == 2:
            for x in range(stride):
                row[x] = (row[x] + prev[x]) & 255
        elif f == 3:
            for x in range(stride):
                a = row[x - 3] if x >= 3 else 0
                row[x] = (row[x] + ((a + prev[x]) // 2)) & 255
        elif f == 4:
            for x in range(stride):
                a = row[x - 3] if x >= 3 else 0
                b = prev[x]
                c = prev[x - 3] if x >= 3 else 0
                p = a + b - c
                pa, pb, pc = abs(p - a), abs(p - b), abs(p - c)
                pr = a if pa <= pb and pa <= pc else (b if pb <= pc else c)
                row[x] = (row[x] + pr) & 255
        prev = row
        rows.append(row)
    return w, h, rows

def write_png(path, w, h, rgb_rows):
    def chunk(tag, payload):
        return struct.pack(">I", len(payload)) + tag + payload + struct.pack(">I", zlib.crc32(tag + payload) & 0xFFFFFFFF)

    raw = b"".join(b"\x00" + bytes(row) for row in rgb_rows)
    png = b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", struct.pack(">IIBBBBB", w, h, 8, 2, 0, 0, 0))
    png += chunk(b"IDAT", zlib.compress(raw, 6)) + chunk(b"IEND", b"")
    Path(path).write_bytes(png)

