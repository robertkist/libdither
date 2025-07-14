#define MODULE_API_EXPORTS
#include <stdlib.h>
#include <stdio.h>
#include "ditherimage.h"
#include "libdither.h"

/*
 * DitherImage is a greyscale buffer in linear color space.
 * DitherImages serve as source input for various dithering algorithms.
 * */

MODULE_API DitherImage* DitherImage_new(int width, int height) {
    DitherImage* self = calloc(1, sizeof(DitherImage));
    self->width = width;
    self->height = height;
    self->buffer = calloc((size_t)(width * height), sizeof(double));
    self->transparency = calloc((size_t)(width * height), sizeof(uint8_t));
    return self;
}

MODULE_API void DitherImage_free(DitherImage* self) {
    if(self) {
        free(self->buffer);
        free(self->transparency);
        free(self);
        self = NULL;
    }
}

MODULE_API void DitherImage_set_pixel_rgba(DitherImage* self, int x, int y, int r, int g, int b, int a, bool correct_gamma) {
    /* takes sRGB inputs (r, g, b, a) in the range 0 - 255. supports transparency */
    if(x < self -> width && y < self -> height) {
        // convert sRGB values to linear color space
        double dr, dg, db;
        if(correct_gamma) {
            dr = gamma_decode(r / 255.0);
            dg = gamma_decode(g / 255.0);
            db = gamma_decode(b / 255.0);
        } else {
            dr = r / 255.0;
            dg = g / 255.0;
            db = b / 255.0;
        }
        // weigh RGB values based on human perception to create final greyscale value, and set transparency
        size_t addr = (size_t)(y * self->width + x);
        self->buffer[addr] = dr * 0.299 + dg * 0.586 + db * 0.114;
        self->transparency[addr] = (uint8_t)a;
    }
}

MODULE_API void DitherImage_set_pixel(DitherImage* self, int x, int y, int r, int g, int b, bool correct_gamma) {
    /* This function is deprecated and exists to provide compatibility to old libdither versions.
     * takes sRGB inputs (r, g, b) in the range 0 - 255.
     * does not suppor transparency. All pixels are opaqe.
     * */
    DitherImage_set_pixel_rgba(self, x, y, r, g, b, 255, correct_gamma);
}

MODULE_API double DitherImage_get_pixel(DitherImage* self, int x, int y) {
    /* returns greyscale pixel value in linear color space in the range 0.0 - 1.0 */
    return self->buffer[y * self->width + x];
}


MODULE_API uint8_t DitherImage_get_transparency(DitherImage* self, int x, int y) {
    /* returns greyscale pixel value in linear color space in the range 0.0 - 1.0 */
    return self->transparency[y * self->width + x];
}
