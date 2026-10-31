#ifndef BLADES_GUIDE_H
#define BLADES_GUIDE_H
#include "blades.h"
enum { GUIDE_ACTION_NONE, GUIDE_ACTION_DASHBOARD };
int guide_init(void);
/* Returns whether the overlay changed and needs a new composite. */
int guide_frame(const BladesHeld *held,float dt);
int guide_active(void);
void guide_paint(void);
void guide_notice(const char *title,const char *message);
void guide_dismiss(void);
int guide_take_sound(void);
int guide_take_action(void);
/* Platform-supplied status. NULL means no signed-in account known to the app. */
void guide_set_user(const char *gamertag);
void guide_set_clock(int hour,int minute);
#endif
