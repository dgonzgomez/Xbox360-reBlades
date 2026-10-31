#ifndef BLADES_FONT_H
#define BLADES_FONT_H

#include <stdint.h>

int font_init(void);
void font_draw(int x, int y, int px, uint32_t argb, const char *text);
int font_width(int px, const char *text);
/* Draws wrapped lines. Returns the y under the last line. */
int font_wrap(int x, int y, int max_w, int px, int line_h, uint32_t argb, const char *text);
void font_vertical(int x, int y, int h, int px, uint32_t argb, const char *text);
/* Whole word, rotated clockwise so it reads downward. */
void font_draw_down(int x, int top, int px, uint32_t argb, const char *text);

#endif
