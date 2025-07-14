#pragma once
#ifndef COLOR_BYTECOLOR_H
#define COLOR_BYTECOLOR_H

#include <stdint.h>

#define BYTE_COLOR_RGB_CHANNELS 4 // r,g,b,a

struct ByteColor {
    uint8_t r, g, b, a;
};
typedef struct ByteColor ByteColor;

void ByteColor_copy(ByteColor* out, const ByteColor* bc);

#endif // COLOR_BYTECOLOR_H
