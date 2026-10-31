/* Host-side check of the actual compiled shader's fetch records against the
 * same vertex layout passed to xsdk_shader_set_vertex_layout on the console.
 * Record encoding follows xsdk/src/shader.c; this does not execute GPU code. */
#include <stdio.h>
#include "blades_shaders.h"
#include "screen_layout.h"

int main(void)
{
    const uint32_t *container = screen_vert.container;
    size_t words = screen_vert.container_bytes / 4;
    if (words < 9 || container[6] % 4) return 2;
    size_t record = container[6] / 4;
    if (record > words || words - record < 8) return 2;
    size_t fetch = record + 8 + (size_t)container[record + 6] * 2;
    size_t count = container[record + 7];
    if (!count || fetch > words || count > words - fetch) return 2;
    for (size_t i = 0; i < count; i++) {
        uint32_t patch = container[fetch + i];
        unsigned usage = (patch >> 12) & 15;
        unsigned index = (patch >> 16) & 15;
        int found = 0;
        for (size_t j = 0; j < sizeof layout / sizeof layout[0]; j++)
            if (layout[j].usage == usage && layout[j].usage_index == index) found = 1;
        if (!found) {
            fprintf(stderr, "Missing shader input: usage %u index %u\n", usage, index);
            return 1;
        }
    }
    printf("Compiled Xbox shader: all %zu vertex inputs match the runtime layout.\n", count);
    return 0;
}
