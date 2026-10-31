#!/usr/bin/env python3
"""Compare a raw preview against the Games reference; no third-party packages."""
import argparse
import json
from pathlib import Path
from reference_io import decode, write_png

ROOT = Path(__file__).resolve().parents[1]

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--reference', type=Path, default=ROOT.parent / 'References/widescreen/games_home.png')
    parser.add_argument('--capture', type=Path, default=ROOT / 'build/games-home.ppm')
    parser.add_argument('--output', type=Path, default=ROOT / 'build/refcheck/current')
    parser.add_argument('--threshold', type=int, default=40)
    args = parser.parse_args()
    w, h, ref = decode(args.reference)
    magic, dimensions, scale, pixels = args.capture.read_bytes().split(b'\n', 3)
    if magic != b'P6' or dimensions != f'{w} {h}'.encode() or scale != b'255' or len(pixels) != w*h*3:
        raise ValueError('Expected raw P6 RGB capture with the same dimensions as the reference')
    args.output.mkdir(parents=True, exist_ok=True)
    capture = [pixels[y*w*3:(y+1)*w*3] for y in range(h)]
    regions = {'whole': (0,0,w,h), 'left_rails': (0,0,220,h),
               'right_rails': (1100,0,w,h), 'profile': (250,90,650,232),
               'menu': (250,240,655,350), 'description': (660,240,1060,485),
               'header': (220,0,1090,88), 'footer': (220,598,1090,718)}
    def metrics(box):
        x0,y0,x1,y1 = box
        total = far = 0
        for y in range(y0,y1):
            for x in range(x0,x1):
                errors = [abs(ref[y][x*3+k] - capture[y][x*3+k]) for k in range(3)]
                total += sum(errors)
                far += max(errors) > args.threshold
        n = (x1-x0)*(y1-y0)
        return {'mean_channel_error': round(total/(n*3),4), 'far_pixels_percent': round(100*far/n,4)}
    result = {'reference': str(args.reference.resolve()), 'capture': str(args.capture.resolve()),
              'definition': 'Mean absolute RGB channel error on 0..255; far means max channel error > threshold.',
              'threshold': args.threshold, 'regions': {k: metrics(v) for k,v in regions.items()}}
    diff = []
    for y in range(h):
        line = bytearray()
        for x in range(w):
            error = max(abs(ref[y][x*3+k] - capture[y][x*3+k]) for k in range(3))
            line.extend((240,45,35) if error > args.threshold else (200,140,30) if error > 12 else (error,)*3)
        diff.append(line)
    write_png(args.output/'render.png', w,h,capture)
    write_png(args.output/'side.png', w*2,h,[bytes(a)+bytes(b) for a,b in zip(ref,capture)])
    write_png(args.output/'diff.png', w,h,diff)
    (args.output/'metrics.json').write_text(json.dumps(result,indent=2)+'\n')
    print(json.dumps(result,indent=2))

if __name__ == '__main__':
    main()
