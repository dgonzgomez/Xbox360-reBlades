#ifndef BLADES_SPRITES_REF_H
#define BLADES_SPRITES_REF_H

enum {
	SPR_ICON0 = 0,
	SPR_ICON1,
	SPR_ICON2,
	SPR_ICON3,
	SPR_ICON4,
	SPR_ICON5,
	SPR_LOGO,
	SPR_SHOT,
	SPR_ESRB,
	SPR_CONTENT,
	SPR_Y,
	SPR_X,
	SPR_B,
	SPR_A,
	SPR_GAMES_LABEL,
	SPR_COUNT
};

void sprite_blit(int id, int x, int y);
void sprite_blit_scaled(int id, int x, int y, int w, int h);

#endif
