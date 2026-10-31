#include "font.h"
#include "draw.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
static uint32_t a[600*80],b[600*80];
int main(void)
{
    assert(font_init());
    const int sizes[]={27,31,37,38};
    for(unsigned n=0;n<sizeof sizes/sizeof sizes[0];n++) {
        for(int i=0;i<600*80;i++)a[i]=b[i]=0xFF304050;
        draw_target(a,600,80);font_draw(8,12,sizes[n],0xFFFFFFFF,"Entertainment. Xbox LIVE");
        draw_target(b,600,80);font_draw(9,12,sizes[n],0xFFFFFFFF,"Entertainment. Xbox LIVE");
        int ink=0;
        for(int y=0;y<80;y++)for(int x=0;x<599;x++) {
            assert(a[y*600+x]==b[y*600+x+1]);
            ink+=a[y*600+x]!=0xFF304050;
        }
        assert(ink>100&&font_width(sizes[n],"Entertainment. Xbox LIVE")<580);
    }
    draw_target(NULL,0,0);
    puts("Native font sizes preserve glyph pixels under one-pixel translation.");
}
