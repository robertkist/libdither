#include <stdlib.h>
#include <string.h>
#include "color_bytepalette.h"
#include "libdither.h"

MODULE_API BytePalette* BytePalette_new(size_t size) {
    /* creates a new BytePalette with 'size' color entries */
    BytePalette* self = (BytePalette*)malloc(sizeof(BytePalette));
    self->buffer = (uint8_t*)calloc((size_t)(size) * BYTE_COLOR_RGB_CHANNELS, sizeof(uint8_t));
    self->size = size;
    return self;
}

BytePalette* BytePalette_copy(const BytePalette* in) {
    /* makes a deep copy of a BytePalette */
    BytePalette* out = BytePalette_new(in->size);
    memcpy(out->buffer, in->buffer, (size_t)(in->size) * BYTE_COLOR_RGB_CHANNELS * sizeof(uint8_t));
    return out;
}

ByteColor* BytePalette_get(const BytePalette* self, size_t index) {
    /* returns a pointer to a ByteColor in the palette */
    ByteColor* c = (ByteColor*)self->buffer;
    return &c[index];
}

void BytePalette_set(BytePalette* self, size_t index, const ByteColor* c) {
    /* set a color in the palette */
    ByteColor* p = (ByteColor*)self->buffer;
    p[index].r = c->r;
    p[index].g = c->g;
    p[index].b = c->b;
    p[index].a = c->a;
}

MODULE_API void BytePalette_free(BytePalette* self) {
    /* frees the BytePalette / i.e. destructor */
    if(self) {
        free(self->buffer);
        free(self);
        self = NULL;
    }
}
