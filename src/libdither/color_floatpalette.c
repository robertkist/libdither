#include "color_floatpalette.h"
#include <stdio.h>

FloatPalette* FloatPalette_new(size_t size) {
    /* A color palette holding floatint point values; intended for doing precise calculations */
    FloatPalette* self = (FloatPalette*)malloc(sizeof(FloatPalette));
    self->buffer = (double*)calloc((size_t)(size) * FLOAT_COLOR_RGB_CHANNELS, sizeof(double));
    self->size = size;
    return self;
}

void FloatPalette_set(FloatPalette* self, size_t index, const FloatColor* fc) {
    /* Sets a particular palette color */
    FloatColor* c = (FloatColor*)self->buffer;
    if (index < self->size) {
        c[index].r = fc->r;
        c[index].g = fc->g;
        c[index].b = fc->b;
    }
}

FloatColor* FloatPalette_get(FloatPalette* self, size_t index) {
    /* Returns a particular palette color */
    FloatColor* c = (FloatColor*)self->buffer;
    return &c[index];
}

void FloatPalette_free(FloatPalette* self) {
    /* Frees the palette (i.e. destructor) */
    if(self) {
        free(self->buffer);
        free(self);
        self = NULL;
    }
}
