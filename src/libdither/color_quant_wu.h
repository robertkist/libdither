#pragma once
#ifndef COLOR_QUANT_WU
#define COLOR_QUANT_WU

#include "color_bytepalette.h"

BytePalette* wu_quantization(const BytePalette* pal, size_t target_k);

#endif // COLOR_QUANT_WU
