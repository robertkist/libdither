#pragma once
#ifndef DITHERIMAGE_H
#define DITHERIMAGE_H

#include <stdint.h>

struct DitherImage {
    double* buffer;        // buffer for float color values
    uint8_t* transparency; // transparency
    int width;
    int height;
};
typedef struct DitherImage DitherImage;

#endif // DITHERIMAGE_H
