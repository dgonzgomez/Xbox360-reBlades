#ifndef BLADES_CHROME_REF_H
#define BLADES_CHROME_REF_H

/* Blade body traced from References/games tab.png.
 * background paints the metal desktop. The shape is the rails plus the glass. */
/* Precompute the four chrome textures once; call with the default draw target. */
void chrome_init(void);
void chrome_draw(int dx, int tint_r, int tint_g, int tint_b, int tint_w, int background);

#endif
