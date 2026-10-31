#!/usr/bin/env python3
"""Extract UI cues from the selected reference; audio is not synthesized."""
from pathlib import Path
import subprocess,wave,array,json
ROOT=Path(__file__).resolve().parents[1]
SOURCE=ROOT.parent/'work/reference-audit/widescreen-audio.webm'
clips=[('move',25.98,.22),('blade',24.66,.34),('select',27.48,.46),('guide_open',161.60,.43),('guide_close',165.30,.43)]
parts=['#include "ui_audio.h"']
table=[]
for name,start,duration in clips:
 path=ROOT/'assets/audio'/f'{name}.wav'
 subprocess.run(['/opt/homebrew/bin/ffmpeg','-hide_banner','-loglevel','error','-y','-ss',str(start),'-t',str(duration),'-i',str(SOURCE),'-ac','1','-ar','48000','-c:a','pcm_s16le',str(path)],check=True)
 with wave.open(str(path),'rb') as w:b=w.readframes(w.getnframes())
 samples=array.array('h',b)
 if __import__('sys').byteorder!='little':samples.byteswap()
 parts.append(f'static const short clip_{name}[] = {{')
 for i in range(0,len(samples),24):parts.append(','.join(str(v) for v in samples[i:i+24])+',')
 parts.append('};')
 table.append(f'{{clip_{name},sizeof clip_{name}/sizeof clip_{name}[0]}}')
parts.append('const UiAudioClip ui_audio_clips[5] = {'+','.join(table)+'};')
(ROOT/'src/ui_audio_data.c').write_text('\n'.join(parts)+'\n')
(ROOT/'assets/audio/source.json').write_text(json.dumps({'url':'https://www.youtube.com/watch?v=5JPMeN1SkUE','clips':[{'name':n,'start_seconds':s,'duration_seconds':d} for n,s,d in clips],'note':'Video-derived mono PCM cues. Isolation and exact onset still need listening verification; Back currently reuses move.'},indent=2)+'\n')
