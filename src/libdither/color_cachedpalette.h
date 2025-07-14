#pragma once
#ifndef COLOR_CACHEDPALETTE_H
#define COLOR_CACHEDPALETTE_H

#include <stdint.h>
#include <stdbool.h>
#include "color_bytepalette.h"
#include "uthash/uthash.h"
#include "tetrapal/tetrapal.h"

enum ColorComparisonMode {
    LUMINANCE = 0,
    SRGB = 1,
    LINEAR = 2,
    HSV = 3,
    LAB76 = 4,
    LAB94 = 5,
    LAB2000 = 6,
    SRGB_CCIR = 7,
    LINEAR_CCIR = 8,
    TETRAPAL = 9,
};

enum QuantizationMethod {
    MEDIAN_CUT = 0,
    WU = 1,
    KDTREE = 2,
};

struct PaletteHashEntry {
    long key;
    size_t index;
    UT_hash_handle hh1;
};
typedef struct PaletteHashEntry PaletteHashEntry;

struct CachedPalette {
    PaletteHashEntry* hash;
    Tetrapal* tetrapal;
    FloatPalette* lookup_palette;
    BytePalette* target_palette;
    FloatColor lab_illuminant;
    FloatColor lab_weights;
    enum ColorComparisonMode mode;
    uint8_t r_shift, g_shift, b_shift; // for faster cache lookups at reduced precision
    bool reduce;

};
typedef struct CachedPalette CachedPalette;

size_t CachedPalette_find_closest_color(CachedPalette* self, const FloatColor *c);

#endif // COLOR_CACHEDPALETTE_H
