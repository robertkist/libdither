#pragma once
#ifndef COLOR_FLOATPALETTE_H
#define COLOR_FLOATPALETTE_H

#include <stdlib.h>
#include "color_floatcolor.h"

struct FloatPalette {
    double* buffer;
    size_t  size;
};
typedef struct FloatPalette FloatPalette;

FloatPalette* FloatPalette_new(size_t size);
void FloatPalette_set(FloatPalette* self, size_t index, const FloatColor* fc);
void FloatPalette_free(FloatPalette* self);
FloatColor* FloatPalette_get(FloatPalette* self, size_t index);

#endif  // COLOR_FLOATPALETTE_H
