#pragma once
#ifndef COLOR_COLORIMAGE_H
#define COLOR_COLORIMAGE_H

#include <stdlib.h>
#include "color_floatcolor.h"
#include "color_bytecolor.h"

struct ColorImage {
    FloatColor* b_linear;
    ByteColor* b_srgb;
    int width;
    int height;
};
typedef struct ColorImage ColorImage;

void ColorImage_get_srgb(const ColorImage* self, size_t addr, ByteColor* color);

#endif // COLOR_COLORIMAGE_H

