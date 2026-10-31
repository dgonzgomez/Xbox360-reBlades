#ifndef BLADES_STICK_H
#define BLADES_STICK_H
#include "blades.h"
typedef struct { int direction; } BladesStick;
/* Xbox signed axes; positive Y is up. Hysteresis prevents drift and repeats
 * near the dead zone. Dominant axis avoids accidental diagonal blade changes. */
static inline void blades_stick_apply(BladesHeld *held, BladesStick *stick, int x, int y)
{
    int ax=x<0?-x:x, ay=y<0?-y:y, magnitude=ax>ay?ax:ay;
    if(magnitude<8000)stick->direction=0;
    else if(magnitude>=12000)stick->direction=ax>ay?(x<0?1:2):(y<0?3:4);
    held->left |= stick->direction==1;
    held->right |= stick->direction==2;
    held->down |= stick->direction==3;
    held->up |= stick->direction==4;
}
#endif
