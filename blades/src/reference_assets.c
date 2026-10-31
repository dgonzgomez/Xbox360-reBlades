#include "reference_assets.h"
#include "asset_codec.h"
#include "draw.h"
#include <stdint.h>
#include <stdlib.h>

static uint32_t *decoded[REF_ASSET_COUNT];

int reference_assets_init(void)
{
    for (int id = 0; id < REF_ASSET_COUNT; id++) {
        const ReferenceAsset *a = &reference_assets[id];
        if (decoded[id]) continue;
        uint32_t *pixels = asset_decode(a);
        if (!pixels) return 0;
        decoded[id] = pixels;
    }
    return 1;
}

void reference_rect(int id, int sx, int sy, int w, int h, int x, int y)
{
    if (id < 0 || id >= REF_ASSET_COUNT || !decoded[id] || w <= 0 || h <= 0) return;
    const ReferenceAsset *a = &reference_assets[id];
    if (sx < 0 || sy < 0 || sx + w > a->w || sy + h > a->h) return;
    for (int row = 0; row < h; row++)
        draw_argb(x, y + row, w, 1, decoded[id] + (sy + row) * a->w + sx);
}

void reference_blit(int id, int x, int y)
{
    if (id < 0 || id >= REF_ASSET_COUNT || !decoded[id]) return;
    draw_argb(x, y, reference_assets[id].w, reference_assets[id].h, decoded[id]);
}

void reference_blit_scaled(int id,int x,int y,int w,int h)
{
    if(id<0 || id>=REF_ASSET_COUNT || !decoded[id] || w<=0 || h<=0) return;
    const ReferenceAsset *a=&reference_assets[id];
    for(int row=0;row<h;row++) for(int col=0;col<w;col++) {
        uint32_t p=decoded[id][(row*a->h/h)*a->w+col*a->w/w];
        draw_blend(x+col,y+row,(p>>16)&255,(p>>8)&255,p&255,255);
    }
}
