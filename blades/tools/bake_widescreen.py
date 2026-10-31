#!/usr/bin/env python3
"""Bake layered native UI assets from the user-selected 2008 widescreen footage.
Static shell, profile, branding and menu icons are textures. Menu text, focus,
input, page hierarchy and accordion movement remain live renderer state.
"""
from pathlib import Path
from reference_io import decode
ROOT=Path(__file__).resolve().parents[1]
REF=ROOT.parent/'References/widescreen'
NAMES=['market','live','games','media','system']
# Dynamic menu and description areas; original header/footer/card/logo stay intact.
ERASE=[[(560,434,400,150)],[(208,244,396,153),(620,245,400,220)],
       [(260,244,390,102),(666,244,390,236)],
       [(304,244,390,200),(708,244,390,236)],
       [(340,104,404,350),(764,104,380,400)]]
ICONS=[[(568,440,40,40),(568,492,40,38),(568,542,40,38)],
       [(230,248,70,62),(230,322,70,66)],
       [(272,250,38,40),(272,298,38,40)],
       [(310,248,42,40),(310,298,42,40),(310,348,42,40),(310,398,42,40)],
       [(346,108,42,40),(346,158,42,40),(346,208,42,40),(346,258,42,40),(346,308,42,40),(346,358,42,40),(346,408,42,40)]]

def crop(rows,x,y,w,h):
 return [row[x*3:(x+w)*3] for row in rows[y:y+h]]

def encode(rows):
 # Lossless RGB delta/run stream. 00rrrrggbb: +/-2 deltas; 0x40 RGB; 0x80..ff repeat.
 out=bytearray();prev=(0,0,0);run=0
 for row in rows:
  for i in range(0,len(row),3):
   c=tuple(row[i:i+3])
   if c==prev:
    run+=1
    if run==128:out.append(255);run=0
    continue
   if run:out.append(128+run-1);run=0
   d=tuple(c[k]-prev[k] for k in range(3))
   if all(-2<=v<=1 for v in d):out.append(((d[0]+2)<<4)|((d[1]+2)<<2)|(d[2]+2))
   else:out.extend((64,*c))
   prev=c
 if run:out.append(128+run-1)
 return out

assets=[]
def add(name,rows):
 assets.append((name,len(rows[0])//3,len(rows),encode(rows)))

for i,name in enumerate(NAMES):
 _,_,home=decode(REF/f'{name}_home.png');_,_,shell=decode(REF/f'{name}_shell.png')
 # Only replace content rectangles, retaining the settled home screen rail geometry.
 base=[bytearray(row) for row in home]
 for x,y,w,h in ERASE[i]:
  for yy in range(y,y+h):base[yy][x*3:(x+w)*3]=shell[yy][x*3:(x+w)*3]
 add(name+'_base',base)
 # Transition shell must have no visible menu content.
 blank=[bytearray(row) for row in home]
 left=132+i*46;right=996+i*46
 for y in range(0,720):
  for x in range(left+10,min(right,1279)):
   blank[y][x*3:x*3+3]=shell[y][x*3:x*3+3]
 add(name+'_shell',blank)
 for j,box in enumerate(ICONS[i]):add(f'{name}_icon{j}',crop(home,*box))
# Fullscreen child pages use the same source's green/purple surfaces, not invented layouts.
for name in ['games_library','console_settings','my_games']:
 _,_,rows=decode(REF/f'{name}.png')
 base=[bytearray(r) for r in rows]
 # Erase only live lists and descriptions using a column gradient from the same pane.
 for y in range(94,586):
  l=tuple(rows[y][120*3:120*3+3]);r=tuple(rows[y][1160*3:1160*3+3])
  for x in range(160,1140):
   u=(x-120)/1040
   base[y][x*3:x*3+3]=bytes(round(l[k]+(r[k]-l[k])*u) for k in range(3))
 add(name+'_base',base)
# Game icons remain individual assets and grow/shrink with the live focused row.
_,_,game_rows=decode(REF/'my_games.png')
for i,y in enumerate([166,214,290,336,382,428,474,520]):
 size=64 if i==1 else 36
 add(f'game_icon{i}',crop(game_rows,178,y,size,size))
# The Xbox LIVE banner is a component used on Games Library.
_,_,rows=decode(REF/'games_library.png');add('library_banner',crop(rows,164,94,484,80))
for i in range(5):add(f'library_row{i}',crop(rows,164,178+i*82,484,76))
# Actual 2008 menu glyphs are rendered from the supplied Segoe font instead of Nunito.
font=ROOT.parent/'reference/Default-0.7b.2/Fonts/segoeui_r.ttf'
b=font.read_bytes()
with (ROOT/'src/font_data.c').open('w') as f:
 f.write('/* Embedded from the user-provided local Aurora reference font. */\nconst unsigned char blades_font_ttf[] = {\n')
 for i in range(0,len(b),24):f.write(','.join(str(v) for v in b[i:i+24])+',\n')
 f.write('};\nconst unsigned int blades_font_ttf_size = sizeof blades_font_ttf;\n')
header=['#ifndef BLADES_REFERENCE_ASSETS_H','#define BLADES_REFERENCE_ASSETS_H','enum {']
parts=['#include "reference_assets.h"','/* Generated lossless texture components; see tools/bake_widescreen.py. */']
for index,(name,w,h,blob) in enumerate(assets):
 header.append(f' REF_{name.upper()} = {index},')
 parts.append(f'static const unsigned char data_{name}[] = {{')
 for i in range(0,len(blob),32):parts.append(','.join(str(v) for v in blob[i:i+32])+',')
 parts.append('};')
header+=[' REF_ASSET_COUNT','};','typedef struct { int w,h; unsigned int size; const unsigned char *bytes; } ReferenceAsset;','void reference_blit_scaled(int id,int x,int y,int w,int h);','extern const ReferenceAsset reference_assets[REF_ASSET_COUNT];','int reference_assets_init(void);','void reference_blit(int id,int x,int y);','void reference_rect(int id,int sx,int sy,int w,int h,int x,int y);','#endif']
parts.append('const ReferenceAsset reference_assets[REF_ASSET_COUNT] = {')
for name,w,h,blob in assets:parts.append(f'{{{w},{h},sizeof data_{name},data_{name}}},')
parts.append('};')
(ROOT/'src/reference_assets.h').write_text('\n'.join(header)+'\n')
(ROOT/'src/reference_asset_data.c').write_text('\n'.join(parts)+'\n')
print('Baked',len(assets),'components;',sum(len(a[3]) for a in assets),'lossless encoded bytes')
