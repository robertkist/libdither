#include "color_colorimage.h"
#include "libdither.h"

MODULE_API ColorImage* ColorImage_new(int width, int height) {
    /* Creates a new ColorImage (i.e. constructor). The image is stored both in sRGB and linear space */
    // TODO b_linear should be renamed; the buffer is floating point. should be up to the user if they want to
    //      put linear colors in it.
    ColorImage* self = (ColorImage*)malloc(sizeof(ColorImage));
    self->b_linear = (FloatColor*)calloc((size_t)(width * height), sizeof(FloatColor));
    self->b_srgb = (ByteColor*)calloc((size_t)(width * height), sizeof(ByteColor));
    self->width = width;
    self->height = height;
    return self;
}

MODULE_API void ColorImage_free(ColorImage* self) {
    /* Frees the ColorImage (i.e. destructor) */
    if(self) {
        free(self->b_linear);
        free(self->b_srgb);
        free(self);
        self = NULL;
    }
}

MODULE_API void ColorImage_set_rgb(ColorImage* self, size_t addr, uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    /* Sets a pixel in the image, both in the sRGB and float buffers */
    ByteColor bc;
    self->b_srgb[addr].r = bc.r = r;
    self->b_srgb[addr].g = bc.g = g;
    self->b_srgb[addr].b = bc.b = b;
    self->b_srgb[addr].a = a;
    FloatColor fc;
    FloatColor_from_ByteColor(&fc, &bc);
    self->b_linear[addr].r = fc.r;
    self->b_linear[addr].g = fc.g;
    self->b_linear[addr].b = fc.b;
}

void ColorImage_get_srgb(const ColorImage* self, size_t addr, ByteColor* color) {
    /* returns a color from the sRGB buffer */
    color->r = self->b_srgb[addr].r;
    color->g = self->b_srgb[addr].g;
    color->b = self->b_srgb[addr].b;
    color->a = self->b_srgb[addr].a;
}
