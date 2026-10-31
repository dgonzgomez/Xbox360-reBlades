#ifndef BLADES_GUIDE_ART_H
#define BLADES_GUIDE_ART_H
#include "reference_assets.h"
enum { GUIDE_ART_MAIN, GUIDE_ART_EMPTY, GUIDE_ART_RIGHT, GUIDE_ART_WARNING, GUIDE_ART_ERROR, GUIDE_ART_COUNT };
extern const ReferenceAsset guide_art[GUIDE_ART_COUNT];
extern const unsigned short guide_left_edge[720], guide_right_edge[720];
#endif
