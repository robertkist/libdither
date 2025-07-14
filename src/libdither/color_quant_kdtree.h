#pragma once
#ifndef COLOR_QUANT_KDTREE
#define COLOR_QUANT_KDTREE

#include <stdlib.h>
#include "color_bytepalette.h"

BytePalette* kdtree_quantization(const BytePalette* unique_pal, size_t target_colors);

#endif // COLOR_QUANT_KDTREE
