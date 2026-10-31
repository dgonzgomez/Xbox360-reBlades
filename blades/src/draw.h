#ifndef BLADES_DRAW_H
#define BLADES_DRAW_H

#include <stdint.h>

void draw_clip(int x, int y, int w, int h);
void draw_clip_none(void);
/* Draw into buf, or back into the screen when buf is null. */
void draw_target(uint32_t *buf, int w, int h);

/* Copy opaque ARGB pixels, respecting target bounds and the active clip. */
void draw_argb(int x, int y, int w, int h, const uint32_t *pixels);

void draw_rgb(int x, int y, int w, int h, const unsigned char *rgb);

void draw_fill(int x, int y, int w, int h, uint32_t argb);
/* Full target dimming, preserving the exact black source-over blend. */
void draw_dim(int alpha);
void draw_gradient(int x, int y, int w, int h, uint32_t top, uint32_t bottom);
void draw_blend(int x, int y, int r, int g, int b, int a);
void draw_round(int x, int y, int w, int h, int radius, uint32_t argb);
void draw_circle(int cx, int cy, int radius, uint32_t argb);
/* Glass blade: vertical gradient, a light pool, and sides that bow inward. */
void draw_blade(int x, int y, int w, int h, uint32_t top, uint32_t mid, uint32_t bottom, uint32_t pool);
void draw_tab(int x, int y, int w, int h, int round_left);

void draw_icon(int x, int y, int size, uint32_t color, int shape);

enum {
	ICON_DISC = 0,
	ICON_STAR,
	ICON_FACE,
	ICON_BOX,
	ICON_CAMERA,
	ICON_NOTE,
	ICON_FILM,
	ICON_GEAR,
	ICON_PEOPLE,
	ICON_PC,
	ICON_CONTROLLER,
	ICON_TRAY,
};

#endif
