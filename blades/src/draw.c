#include "draw.h"

#include "blades.h"

#include <math.h>
#include <string.h>

static uint32_t frame[BLADES_HEIGHT * BLADES_WIDTH];
static uint32_t *fb = frame;
static int fb_w = BLADES_WIDTH;
static int fb_h = BLADES_HEIGHT;
static int clip_on;
static int clip_x0, clip_y0, clip_x1, clip_y1;

void draw_target(uint32_t *buf, int w, int h)
{
	if (!buf) {
		fb = frame;
		fb_w = BLADES_WIDTH;
		fb_h = BLADES_HEIGHT;
		return;
	}
	fb = buf;
	fb_w = w;
	fb_h = h;
}

const uint32_t *blades_pixels(void)
{
	return frame;
}

void draw_clip(int x, int y, int w, int h)
{
	clip_on = 1;
	clip_x0 = x;
	clip_y0 = y;
	clip_x1 = x + w;
	clip_y1 = y + h;
}

void draw_clip_none(void)
{
	clip_on = 0;
}

static int inside(int x, int y)
{
	if ((unsigned)x >= (unsigned)fb_w || (unsigned)y >= (unsigned)fb_h)
		return 0;
	if (!clip_on)
		return 1;
	return x >= clip_x0 && y >= clip_y0 && x < clip_x1 && y < clip_y1;
}

void draw_blend(int x, int y, int r, int g, int b, int a)
{
	uint32_t *p;
	int dr, dg, db;
	if (a <= 0 || !inside(x, y))
		return;
	if (a >= 255) {
		fb[y * fb_w + x] = 0xFF000000u | ((uint32_t)r << 16) | ((uint32_t)g << 8) | (uint32_t)b;
		return;
	}
	p = &fb[y * fb_w + x];
	dr = (*p >> 16) & 255;
	dg = (*p >> 8) & 255;
	db = *p & 255;
	r = (r * a + dr * (255 - a)) / 255;
	g = (g * a + dg * (255 - a)) / 255;
	b = (b * a + db * (255 - a)) / 255;
	*p = 0xFF000000u | ((uint32_t)r << 16) | ((uint32_t)g << 8) | (uint32_t)b;
}

void draw_argb(int x, int y, int w, int h, const uint32_t *pixels)
{
	int x0 = x < 0 ? 0 : x;
	int y0 = y < 0 ? 0 : y;
	int x1 = x + w > fb_w ? fb_w : x + w;
	int y1 = y + h > fb_h ? fb_h : y + h;
	int row;
	if (!pixels || w <= 0 || h <= 0)
		return;
	if (clip_on) {
		if (x0 < clip_x0) x0 = clip_x0;
		if (y0 < clip_y0) y0 = clip_y0;
		if (x1 > clip_x1) x1 = clip_x1;
		if (y1 > clip_y1) y1 = clip_y1;
	}
	if (x1 <= x0 || y1 <= y0)
		return;
	for (row = y0; row < y1; row++)
		memcpy(fb + row * fb_w + x0, pixels + (row - y) * w + x0 - x,
			(size_t)(x1 - x0) * sizeof *pixels);
}

void draw_rgb(int x, int y, int w, int h, const unsigned char *rgb)
{
	int row, col;
	for (row = 0; row < h; row++) {
		const unsigned char *s = rgb + row * w * 3;
		for (col = 0; col < w; col++) {
			draw_blend(x + col, y + row, s[0], s[1], s[2], 255);
			s += 3;
		}
	}
}

static void put(int x, int y, uint32_t argb)
{
	draw_blend(x, y, (argb >> 16) & 255, (argb >> 8) & 255, argb & 255, (argb >> 24) & 255);
}

void draw_fill(int x, int y, int w, int h, uint32_t argb)
{
	int a = (argb >> 24) & 255;
	int r = (argb >> 16) & 255;
	int g = (argb >> 8) & 255;
	int b = argb & 255;
	int row, col;
	if (w <= 0 || h <= 0)
		return;
	if (a >= 255 && !clip_on && x >= 0 && y >= 0 && x + w <= fb_w && y + h <= fb_h) {
		for (row = 0; row < h; row++) {
			uint32_t *line = fb + (y + row) * fb_w + x;
			for (col = 0; col < w; col++)
				line[col] = argb;
		}
		return;
	}
	for (row = 0; row < h; row++)
		for (col = 0; col < w; col++)
			draw_blend(x + col, y + row, r, g, b, a);
}

void draw_dim(int alpha)
{
	unsigned char channel[256];
	if(alpha<=0)return;
	if(alpha>255)alpha=255;
	for(int i=0;i<256;i++)channel[i]=(unsigned char)(i*(255-alpha)/255);
	for(int i=0;i<fb_w*fb_h;i++) {
		uint32_t p=fb[i];
		fb[i]=0xFF000000u|((uint32_t)channel[(p>>16)&255]<<16)|((uint32_t)channel[(p>>8)&255]<<8)|channel[p&255];
	}
}

static uint32_t lerp(uint32_t a, uint32_t b, float t)
{
	int ar = (a >> 16) & 255, ag = (a >> 8) & 255, ab = a & 255;
	int br = (b >> 16) & 255, bg = (b >> 8) & 255, bb = b & 255;
	int r = (int)(ar + (br - ar) * t);
	int g = (int)(ag + (bg - ag) * t);
	int bl = (int)(ab + (bb - ab) * t);
	return 0xFF000000u | ((uint32_t)r << 16) | ((uint32_t)g << 8) | (uint32_t)bl;
}

void draw_gradient(int x, int y, int w, int h, uint32_t top, uint32_t bottom)
{
	int row;
	if (h <= 1) {
		draw_fill(x, y, w, h, top);
		return;
	}
	for (row = 0; row < h; row++)
		draw_fill(x, y + row, w, 1, lerp(top, bottom, row / (float)(h - 1)));
}

static int corner_inset(int row, int h, int radius)
{
	int d, inside;
	if (row < radius)
		d = radius - row;
	else if (row >= h - radius)
		d = row - (h - 1 - radius);
	else
		return 0;
	inside = (int)sqrtf((float)(radius * radius - d * d));
	return radius - inside;
}

void draw_round(int x, int y, int w, int h, int radius, uint32_t argb)
{
	int row;
	if (radius < 1)
		radius = 1;
	if (radius > w / 2)
		radius = w / 2;
	if (radius > h / 2)
		radius = h / 2;
	for (row = 0; row < h; row++) {
		int inset = corner_inset(row, h, radius);
		draw_fill(x + inset, y + row, w - inset * 2, 1, argb);
	}
}

void draw_circle(int cx, int cy, int radius, uint32_t argb)
{
	int y;
	for (y = -radius; y <= radius; y++) {
		int span = (int)sqrtf((float)(radius * radius - y * y));
		draw_fill(cx - span, cy + y, span * 2 + 1, 1, argb);
	}
}

void draw_blade(int x, int y, int w, int h, uint32_t top, uint32_t mid, uint32_t bottom, uint32_t pool)
{
	const int bow = 14;
	const int radius = 28;
	int row, col;
	int pr = (pool >> 16) & 255, pg = (pool >> 8) & 255, pb = pool & 255;
	for (row = 0; row < h; row++) {
		float t = h <= 1 ? 0.0f : row / (float)(h - 1);
		uint32_t base = t < 0.5f ? lerp(top, mid, t / 0.5f) : lerp(mid, bottom, (t - 0.5f) / 0.5f);
		int inset = corner_inset(row, h, radius) + (int)(sinf(t * 3.14159265f) * bow);
		int x0 = x + inset;
		int span = w - inset * 2;
		int br = (base >> 16) & 255, bg = (base >> 8) & 255, bb = base & 255;
		if (span <= 0)
			continue;
		for (col = 0; col < span; col++) {
			float nx = span <= 1 ? 0.0f : col / (float)(span - 1) - 0.5f;
			float ny = (t - 0.58f) / 0.34f;
			float e = nx * nx * 3.4f + ny * ny;
			int r = br, g = bg, b = bb;
			if (e < 1.0f) {
				float k = (1.0f - e) * 0.72f;
				r = (int)(r + (pr - r) * k);
				g = (int)(g + (pg - g) * k);
				b = (int)(b + (pb - b) * k);
			}
			draw_blend(x0 + col, y + row, r, g, b, 255);
		}
	}
}

void draw_tab(int x, int y, int w, int h, int round_left)
{
	const int radius = 18;
	int row;
	for (row = 0; row < h; row++) {
		float t = h <= 1 ? 0.0f : row / (float)(h - 1);
		uint32_t color = lerp(0xFFC8C8C6, 0xFF8E8E8C, t);
		int inset = corner_inset(row, h, radius);
		int x0 = x;
		int span = w;
		if (round_left)
			x0 += inset;
		else
			span -= inset;
		span -= inset ? 0 : 0;
		if (round_left)
			span = w - inset;
		else {
			x0 = x;
			span = w - inset;
		}
		draw_fill(x0, y + row, span, 1, color);
		/* Bright rim on the side that meets the open blade. */
		if (row > radius && row < h - radius) {
			if (round_left)
				draw_fill(x + w - 3, y + row, 3, 1, 0xFFF4F4F2);
			else
				draw_fill(x, y + row, 3, 1, 0xFFF4F4F2);
		}
	}
}

static void icon_disc(int x, int y, int size, uint32_t color)
{
	int c = size / 2;
	draw_circle(x + c, y + c, c - 2, color);
	draw_circle(x + c, y + c, c / 3, 0xFF101010);
}

static void icon_star(int x, int y, int size, uint32_t color)
{
	int c = size / 2;
	draw_circle(x + c, y + c, c / 2, color);
	draw_fill(x + c - 2, y + 4, 4, size - 8, color);
	draw_fill(x + 4, y + c - 2, size - 8, 4, color);
}

static void icon_face(int x, int y, int size, uint32_t color)
{
	int c = size / 2;
	(void)color;
	draw_circle(x + c, y + c, c - 2, 0xFFFFD24A);
	draw_fill(x + c / 2, y + c / 2, 4, 4, 0xFF3A2A10);
	draw_fill(x + size - c / 2 - 4, y + c / 2, 4, 4, 0xFF3A2A10);
	draw_round(x + c / 2, y + c + 2, size / 2, 4, 2, 0xFF3A2A10);
}

static void icon_box(int x, int y, int size, uint32_t color)
{
	draw_round(x + 4, y + 4, size - 8, size - 8, 6, color);
	draw_fill(x + 8, y + size / 2, size - 16, 3, 0x66FFFFFF);
}

void draw_icon(int x, int y, int size, uint32_t color, int shape)
{
	switch (shape) {
	case ICON_STAR:
		icon_star(x, y, size, color);
		break;
	case ICON_FACE:
		icon_face(x, y, size, color);
		break;
	case ICON_BOX:
		icon_box(x, y, size, color);
		break;
	case ICON_CAMERA:
		draw_round(x + 4, y + 8, size - 8, size - 16, 4, color);
		draw_circle(x + size / 2, y + size / 2, size / 6, 0xFF203040);
		break;
	case ICON_NOTE:
		draw_circle(x + size / 3, y + size - size / 3, size / 5, color);
		draw_fill(x + size / 3 + size / 6, y + size / 5, 4, size / 2, color);
		break;
	case ICON_FILM:
		draw_round(x + 3, y + 6, size - 6, size - 12, 3, color);
		draw_fill(x + 6, y + size / 2 - 2, size - 12, 4, 0xFF101820);
		break;
	case ICON_GEAR:
		draw_circle(x + size / 2, y + size / 2, size / 3, color);
		draw_circle(x + size / 2, y + size / 2, size / 7, 0xFF202028);
		break;
	case ICON_PEOPLE:
		draw_circle(x + size / 3, y + size / 3, size / 6, color);
		draw_circle(x + 2 * size / 3, y + size / 3, size / 6, color);
		draw_round(x + size / 8, y + size / 2, size - size / 4, size / 3, 8, color);
		break;
	case ICON_PC:
		draw_round(x + 4, y + 6, size - 8, size / 2, 3, color);
		draw_fill(x + size / 2 - 3, y + size / 2 + 6, 6, size / 5, color);
		break;
	case ICON_CONTROLLER:
		draw_round(x + 4, y + size / 3, size - 8, size / 3, size / 6, color);
		draw_circle(x + size / 3, y + size / 2, 4, 0xFF101010);
		draw_circle(x + 2 * size / 3, y + size / 2, 4, 0xFF101010);
		break;
	case ICON_TRAY:
		draw_round(x + 2, y + size / 3, size - 4, size / 3, 6, color);
		draw_circle(x + size / 2, y + size / 2, size / 8, 0xFF101010);
		break;
	default:
		icon_disc(x, y, size, color);
		break;
	}
}
