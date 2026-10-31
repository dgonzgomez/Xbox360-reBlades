#include "asset_codec.h"
#include <stdlib.h>

uint32_t *asset_decode(const ReferenceAsset *a)
{
        unsigned int pos = 0;
        size_t out = 0, count = (size_t)a->w * a->h;
        int r = 0, g = 0, b = 0;
        uint32_t *pixels = malloc(count * sizeof *pixels);
        if (!pixels) return 0;
        while (pos < a->size && out < count) {
            unsigned char op = a->bytes[pos++];
            unsigned int run = 1;
            if (op & 128) run = (op & 127) + 1;
            else if (op == 64) {
                if (pos + 3 > a->size) break;
                r = a->bytes[pos++]; g = a->bytes[pos++]; b = a->bytes[pos++];
            } else if (op < 64) {
                r += ((op >> 4) & 3) - 2;
                g += ((op >> 2) & 3) - 2;
                b += (op & 3) - 2;
            } else break;
            if (run > count - out || r < 0 || r > 255 || g < 0 || g > 255 || b < 0 || b > 255) break;
            uint32_t color = 0xFF000000u | ((uint32_t)r << 16) | ((uint32_t)g << 8) | (uint32_t)b;
            for (unsigned int n = 0; n < run; n++) pixels[out++] = color;
        }
        if (out != count || pos != a->size) { free(pixels); return 0; }
    return pixels;
}
