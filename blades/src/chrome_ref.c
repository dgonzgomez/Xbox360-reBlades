#include "chrome_ref.h"

#include "draw.h"
#include "blades.h"

#define CHROME_N 24

extern const unsigned short chrome_gl[720];
extern const unsigned short chrome_gr[720];
extern const unsigned short chrome_ll[720];
extern const unsigned short chrome_rr[720];
extern const unsigned char chrome_bg[720 * 3];
extern const unsigned char chrome_glass[720 * CHROME_N * 3];
extern const int chrome_left_off[721];
extern const unsigned char chrome_left_px[];
extern const int chrome_right_off[721];
extern const unsigned char chrome_right_px[];

static int mix(int c, int tint, int w)
{
	return (c * (255 - w) + tint * w) / 255;
}

static void strip(int y, int dx, int x0, int x1, const unsigned char *samples, int tr, int tg, int tb, int tw)
{
	int span = x1 - x0;
	int x;
	if (span <= 1)
		return;
	for (x = x0; x <= x1; x++) {
		int xx = x + dx;
		float f, u;
		int i, r, g, b;
		const unsigned char *a;
		const unsigned char *bp;
		if ((unsigned)xx >= 1280 || (unsigned)y >= 720)
			continue;
		f = (x - x0) * (float)(CHROME_N - 1) / (float)span;
		i = (int)f;
		if (i < 0)
			i = 0;
		if (i >= CHROME_N - 1) {
			i = CHROME_N - 1;
			u = 0.0f;
		} else
			u = f - (float)i;
		a = samples + (y * CHROME_N + i) * 3;
		bp = samples + (y * CHROME_N + (i >= CHROME_N - 1 ? i : i + 1)) * 3;
		r = (int)(a[0] + (bp[0] - a[0]) * u);
		g = (int)(a[1] + (bp[1] - a[1]) * u);
		b = (int)(a[2] + (bp[2] - a[2]) * u);
		if (tw) {
			r = mix(r, tr, tw);
			g = mix(g, tg, tw);
			b = mix(b, tb, tw);
		}
		draw_blend(xx, y, r, g, b, 255);
	}
}

static void chrome_draw_uncached(int dx, int tint_r, int tint_g, int tint_b, int tint_w, int background)
{
	int y;
	for (y = 0; y < 720; y++) {
		int gl = chrome_gl[y];
		int gr = chrome_gr[y];
		int ll = chrome_ll[y];
		int rr = chrome_rr[y];
		if (background) {
			const unsigned char *bg = chrome_bg + y * 3;
			int x;
			for (x = 0; x < 1280; x++)
				draw_blend(x, y, bg[0], bg[1], bg[2], 255);
		}
		if (background > 1 || gr <= gl)
			continue;
		{
			int n = gl - ll;
			const unsigned char *s = chrome_left_px + chrome_left_off[y];
			int x;
			if (n < 0)
				n = 0;
			for (x = 0; x < n; x++) {
				int xx = ll + x + dx;
				if ((unsigned)xx < 1280)
					draw_blend(xx, y, s[x * 3], s[x * 3 + 1], s[x * 3 + 2], 255);
			}
		}
		strip(y, dx, gl, gr, chrome_glass, tint_r, tint_g, tint_b, tint_w);
		{
			int n = rr - gr;
			const unsigned char *s = chrome_right_px + chrome_right_off[y];
			int x;
			if (n < 0)
				n = 0;
			for (x = 0; x < n; x++) {
				int xx = gr + x + dx;
				if ((unsigned)xx < 1280)
					draw_blend(xx, y, s[x * 3], s[x * 3 + 1], s[x * 3 + 2], 255);
			}
		}
	}
}

/* About 17.6 MiB: static chrome is sampled once, then slides use row copies.
 * No per-frame allocation or floating-point resampling of the glass. */
static uint32_t chrome_cache[4][BLADES_WIDTH * BLADES_HEIGHT];
static uint32_t desktop_cache[BLADES_WIDTH * BLADES_HEIGHT];
static const int cache_tints[4][4] = {
	{224, 160, 62, 170}, {0, 0, 0, 0}, {66, 140, 210, 175}, {149, 109, 216, 165}
};
static int cache_ready;

void chrome_init(void)
{
	int i;
	if (cache_ready)
		return;
	draw_target(desktop_cache, BLADES_WIDTH, BLADES_HEIGHT);
	chrome_draw_uncached(0, 0, 0, 0, 0, 2);
	for (i = 0; i < 4; i++) {
		const int *t = cache_tints[i];
		draw_target(chrome_cache[i], BLADES_WIDTH, BLADES_HEIGHT);
		chrome_draw_uncached(0, t[0], t[1], t[2], t[3], 0);
	}
	draw_target(0, 0, 0);
	cache_ready = 1;
}

void chrome_draw(int dx, int tr, int tg, int tb, int tw, int background)
{
	int i, y;
	if (!cache_ready) {
		chrome_draw_uncached(dx, tr, tg, tb, tw, background);
		return;
	}
	if (background)
		draw_argb(0, 0, BLADES_WIDTH, BLADES_HEIGHT, desktop_cache);
	if (background > 1)
		return;
	for (i = 0; i < 4; i++) {
		const int *t = cache_tints[i];
		if (t[0] == tr && t[1] == tg && t[2] == tb && t[3] == tw)
			break;
	}
	if (i == 4) {
		chrome_draw_uncached(dx, tr, tg, tb, tw, 0);
		return;
	}
	for (y = 0; y < BLADES_HEIGHT; y++) {
		int ll = chrome_ll[y], rr = chrome_rr[y];
		if (chrome_gr[y] <= chrome_gl[y] || rr <= ll)
			continue;
		draw_argb(ll + dx, y, rr - ll, 1, chrome_cache[i] + y * BLADES_WIDTH + ll);
	}
}
