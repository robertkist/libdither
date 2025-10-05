#pragma once
#ifndef COLOR_BYTEPALETTE_H
#define COLOR_BYTEPALETTE_H

#include <stdlib.h>
#include <stdbool.h>
#include "color_colorimage.h"
#include "color_floatpalette.h"

struct BytePalette {
    uint8_t* buffer;
    size_t size;
};
typedef struct BytePalette BytePalette;

#endif  // COLOR_BYTEPALETTE_H
