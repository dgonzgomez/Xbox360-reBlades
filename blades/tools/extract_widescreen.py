#!/usr/bin/env python3
"""Extract the user-selected 16:9 reference; does not download network content."""
import json,subprocess
from pathlib import Path
import argparse,shutil
ROOT=Path(__file__).resolve().parents[2]
parser=argparse.ArgumentParser(description="Extract the canonical video frames at 1280x720.")
parser.add_argument('--video',type=Path,default=ROOT/'work/reference-audit/widescreen-5JPMeN1SkUE.mp4')
args=parser.parse_args()
source=args.video
output=ROOT/'References/widescreen'
output.mkdir(parents=True,exist_ok=True)
ffmpeg=shutil.which('ffmpeg') or '/opt/homebrew/bin/ffmpeg' 
frames={
'market_shell':19.85,'market_home':20.5,
'live_shell':24.616667,'live_home':18.6,
'games_shell':24.833333,'games_home':26.7,
'media_shell':138.55,'media_home':139.6,
'system_shell':140.95,'system_home':141.3,
'games_library':31.6,'my_games':36.5,
'console_settings':143.3,'display':144.5,'audio':148.3,
'windows_media_center':137.4,
}
for name,t in frames.items():
 subprocess.run([ffmpeg,'-hide_banner','-loglevel','error','-y','-ss',str(t),'-i',str(source),'-frames:v','1','-vf','scale=1280:720:flags=lanczos','-pix_fmt','rgb24',str(output/f'{name}.png')],check=True)
(output/'source.json').write_text(json.dumps({'url':'https://www.youtube.com/watch?v=5JPMeN1SkUE','title':'Xbox 360 Blades Dashboard Walkthrough','source_width':1920,'source_height':1080,'source_fps':60,'output_width':1280,'output_height':720,'resampling':'Lanczos','frames_seconds':frames,'canonical':True,'note':'User selected this five-blade 16:9 video as the main reference. Earlier 4:3 frames and games tab.png remain historical references.'},indent=2)+'\n')
