#include "font.h"

#include "draw.h"

#include <stdlib.h>
#include <string.h>

#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

extern const unsigned char blades_font_ttf[];
extern const unsigned int blades_font_ttf_size;

enum { ATLAS_W = 512, ATLAS_H = 512, FIRST = 32, COUNT = 96, MAX_PX = 96 };
typedef struct {
    unsigned char atlas[ATLAS_W * ATLAS_H];
    stbtt_bakedchar glyphs[COUNT];
    float ascent;
} FontSize;
static FontSize *sizes[MAX_PX + 1];
static unsigned char *atlas;
static stbtt_bakedchar *glyphs;
static stbtt_fontinfo font_info;
static int ready, font_ascent;
static uint32_t *mask_buf;
static int mask_w, mask_h;

static FontSize *font_size(int px)
{
    if(!ready || px<1 || px>MAX_PX)return NULL;
    if(!sizes[px]) {
        FontSize *size=malloc(sizeof *size);
        if(!size)return NULL;
        if(stbtt_BakeFontBitmap(blades_font_ttf,0,(float)px,size->atlas,ATLAS_W,ATLAS_H,FIRST,COUNT,size->glyphs)<=0) {
            free(size);return NULL;
        }
        size->ascent=font_ascent*stbtt_ScaleForPixelHeight(&font_info,(float)px);
        sizes[px]=size;
    }
    return sizes[px];
}
int font_init(void)
{
    if(ready)return 1;
    int descent,line_gap;
    if(!stbtt_InitFont(&font_info,blades_font_ttf,0))return 0;
    stbtt_GetFontVMetrics(&font_info,&font_ascent,&descent,&line_gap);
    ready=1;
    /* Warm the menu/Guide sizes before interaction, avoiding first-use bakes. */
    const int used[]={26,27,28,29,30,31,32,34,36,37,38,40,42,44};
    for(unsigned i=0;i<sizeof used/sizeof used[0];i++)if(!font_size(used[i]))return 0;
    return 1;
}

static void blit(const stbtt_aligned_quad *q, uint32_t argb)
{
	int r = (argb >> 16) & 255;
	int g = (argb >> 8) & 255;
	int b = argb & 255;
	int ca = (argb >> 24) & 255;
	float w = q->x1 - q->x0;
	float h = q->y1 - q->y0;
	int pw = (int)(w + 0.5f);
	int ph = (int)(h + 0.5f);
	int row, col;
	if (ca == 0)
		ca = 255;
	if (pw < 1 || ph < 1)
		return;
    int sx0=(int)(q->s0*ATLAS_W+0.5f), sy0=(int)(q->t0*ATLAS_H+0.5f);
    for(row=0;row<ph;row++) {
        int sy=sy0+row;
        for(col=0;col<pw;col++) {
            int sx=sx0+col,cover;
            if(sx<0||sx>=ATLAS_W||sy<0||sy>=ATLAS_H)continue;
			cover = atlas[sy * ATLAS_W + sx];
			if (!cover)
				continue;
			cover = cover * ca / 255;
			if (mask_buf) {
				int px = (int)q->x0 + col;
				int py = (int)q->y0 + row;
				if (px >= 0 && py >= 0 && px < mask_w && py < mask_h)
					mask_buf[py * mask_w + px] = ((uint32_t)cover << 24) | ((uint32_t)r << 16) | ((uint32_t)g << 8) | (uint32_t)b;
			} else if (cover)
				draw_blend((int)q->x0 + col, (int)q->y0 + row, r, g, b, cover);
		}
	}
}

static float advance_text(float x, float baseline, int px, const char *text, uint32_t argb, int draw)
{
	const unsigned char *s;
	FontSize *size=font_size(px);
	if (!size)
		return x;
	atlas=size->atlas;glyphs=size->glyphs;
	for (s = (const unsigned char *)text; *s; s++) {
		stbtt_aligned_quad q;
		float cx, cy;
		int ch = *s;
		if (ch < FIRST || ch >= FIRST + COUNT)
			ch = '?';
		cx = x;
		cy = baseline;
		stbtt_GetBakedQuad(glyphs, ATLAS_W, ATLAS_H, ch - FIRST, &cx, &cy, &q, 1);
		if (draw)
			blit(&q, argb);
		x = cx;
	}
	return x;
}

void font_draw(int x, int y, int px, uint32_t argb, const char *text)
{
	FontSize *size=font_size(px);
	if(!size)return;
	float baseline = y + size->ascent;
	if (!text)
		return;
	advance_text((float)x, baseline, px, text, argb, 1);
}

int font_width(int px, const char *text)
{
	FontSize *size=font_size(px);
	if(!size)return 0;
	float baseline = size->ascent;
	if (!text)
		return 0;
	return (int)(advance_text(0, baseline, px, text, 0, 0) + 0.5f);
}

int font_wrap(int x, int y, int max_w, int px, int line_h, uint32_t argb, const char *text)
{
	char line[128];
	const char *s = text;
	if (!text)
		return y;
	while (*s) {
		int n = 0;
		int last_space = -1;
		while (s[n] && s[n] != '\n' && n < (int)sizeof line - 1) {
			line[n] = s[n];
			line[n + 1] = 0;
			if (s[n] == ' ')
				last_space = n;
			if (font_width(px, line) > max_w && n > 0) {
				if (last_space > 0)
					n = last_space;
				break;
			}
			n++;
		}
		if (n == 0)
			break;
		line[n] = 0;
		font_draw(x, y, px, argb, line);
		y += line_h;
		s += n;
		while (*s == ' ')
			s++;
		if (*s == '\n')
			s++;
	}
	return y;
}

void font_vertical(int x, int y, int h, int px, uint32_t argb, const char *text)
{
	int n = text ? (int)strlen(text) : 0;
	int step = px * 2 / 3;
	int total = n * step;
	int top = y + (h - total) / 2;
	int i;
	if (top < y)
		top = y;
	for (i = 0; i < n; i++) {
		char one[2] = { text[i], 0 };
		int w = font_width(px, one);
		font_draw(x + (px - w) / 2, top + i * step, px, argb, one);
	}
}

void font_draw_down(int x, int top, int px, uint32_t argb, const char *text)
{
	int tw, th, sx, sy, pen, gap;
	uint32_t *buf;
	const char *s;
	if (!text || !text[0])
		return;
	gap = 2;
	tw = 0;
	for (s = text; *s; s++) {
		char one[2] = { *s, 0 };
		tw += font_width(px, one) + gap;
	}
	if (tw > gap)
		tw -= gap;
	th = px + 6;
	if (tw < 1 || th < 1)
		return;
	buf = (uint32_t *)calloc((size_t)tw * (size_t)th, sizeof(uint32_t));
	if (!buf)
		return;
	mask_buf = buf;
	mask_w = tw;
	mask_h = th;
	pen = 0;
	for (s = text; *s; s++) {
		char one[2] = { *s, 0 };
		font_draw(pen, 1, px, argb, one);
		pen += font_width(px, one) + gap;
	}
	mask_buf = 0;
	for (sy = 0; sy < th; sy++) {
		for (sx = 0; sx < tw; sx++) {
			uint32_t p = buf[sy * tw + sx];
			int a = (p >> 24) & 255;
			if (!a)
				continue;
			/* Clockwise: the start of the word is at the top, glyph tops face left. */
			draw_blend(x + sy, top + sx, (p >> 16) & 255, (p >> 8) & 255, p & 255, a);
		}
	}
	free(buf);
}
