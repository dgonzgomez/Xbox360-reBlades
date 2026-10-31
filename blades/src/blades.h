#ifndef BLADES_APP_H
#define BLADES_APP_H

#include <stdint.h>

enum { BLADES_WIDTH = 1280, BLADES_HEIGHT = 720 };

/* Buttons currently held. Repeats are handled inside blades_frame. */
typedef struct BladesHeld {
	int up, down, left, right, a, b, y;
	int x, guide;
} BladesHeld;

void blades_init(void);
/* dt is seconds since the previous frame. */
void blades_frame(const BladesHeld *held, float dt);
const uint32_t *blades_pixels(void);
/* Consume the next UI cue for the platform audio adapter; 0 means none. */
int blades_take_sound(void);
/* Do not compete with XAM's own Guide/dialogs; release buttons before resuming. */
void blades_system_ui_state(int open);
void blades_set_user(const char *gamertag);
void blades_set_clock(int hour,int minute);

#endif
