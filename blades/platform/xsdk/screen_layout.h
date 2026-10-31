#ifndef BLADES_SCREEN_LAYOUT_H
#define BLADES_SCREEN_LAYOUT_H
#include "xsdk/shader.h"

enum { VERTEX_FLOATS = 4, STRIDE_BYTES = VERTEX_FLOATS * 4, UV_OFFSET_BYTES = 8 };
static const XsdkVertexElement layout[] = {
    { .usage = XSDK_USAGE_POSITION, .format = XSDK_VERTEX_FLOAT2, .offset_bytes = 0 },
    { .usage = XSDK_USAGE_TEXCOORD, .format = XSDK_VERTEX_FLOAT2, .offset_bytes = UV_OFFSET_BYTES },
};
#endif
