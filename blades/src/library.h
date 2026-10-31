#ifndef BLADES_LIBRARY_H
#define BLADES_LIBRARY_H
#include <stdint.h>
enum { BLADES_MAX_GAMES=256 };
typedef struct {char name[128],path[512];uint32_t title_id;} BladesGame;
/* Read-only GOD discovery: root is Content/0000000000000000. Returns -1 if
 * unavailable, otherwise number of validated package headers, capped at max. */
int library_scan(const char *root,BladesGame *games,int max);
void blades_set_library(const BladesGame *games,int count);
#endif
