#pragma once
#ifndef COLOR_QUANT_MEDIANCUT_H
#define COLOR_QUANT_MEDIANCUT_H

#include <stdlib.h>
#include "color_bytepalette.h"

BytePalette* median_cut(const BytePalette* palette, size_t out_cols);

#endif // COLOR_QUANT_MEDIANCUT_H
