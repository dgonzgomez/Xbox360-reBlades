#ifndef BLADES_ASSET_CODEC_H
#define BLADES_ASSET_CODEC_H
#include "reference_assets.h"
#include <stdint.h>
/* Decode validated lossless RGB delta/run data; caller owns the returned pixels. */
uint32_t *asset_decode(const ReferenceAsset *asset);
#endif
