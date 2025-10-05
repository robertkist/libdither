#define MODULE_API_EXPORTS
#include "color_floatcolor.h"
#include "libdither.h"

inline static double clamp(double v) {
    /* clamps a value between 0.0 and 1.0 */
    return (v > 1.0 ? 1.0 : (v < 0.0 ? 0.0 : v));
}

void FloatColor_set(FloatColor* out, double r, double g, double b) {
    /* sets the individual RGB values of the FloatColor */
    out->r = r;
    out->g = g;
    out->b = b;
}

void FloatColor_add(FloatColor* out, const FloatColor* in) {
    /* adds the values of another float color (no clamping) */
    out->r += in->r;
    out->g += in->g;
    out->b += in->b;
}

void FloatColor_sub(FloatColor* out, const FloatColor* in) {
    /* subtracts the values of another float color (no clamping) */
    out->r -= in->r;
    out->g -= in->g;
    out->b -= in->b;
}

void FloatColor_add_float(FloatColor* out, double value) {
    /* adds a float scalar to all 3 RGB channels */
    out->r += value;
    out->g += value;
    out->b += value;
}

void FloatColor_sub_float(FloatColor* out, double value) {
    /* subtracts a float scalar to all 3 RGB channels */
    out->r -= value;
    out->g -= value;
    out->b -= value;
}

void FloatColor_from_ByteColor(FloatColor* out, const ByteColor* bc) {
    /* converts a bytes color (0 - 255) to float (0.0 - 1.0) */
    out->r = (double)bc->r / 255.0;
    out->g = (double)bc->g / 255.0;
    out->b = (double)bc->b / 255.0;
}

MODULE_API void FloatColor_from_FloatColor(FloatColor* out, const FloatColor* fc2) {
    /* copies a FloatColor's values */
    // TODO function should be renamed to FloatColor_copy for consistency's sake
    out->r = fc2->r;
    out->g = fc2->g;
    out->b = fc2->b;
}

void FloatColor_clamp(FloatColor* fc) {
    /* clamps the float color to the 0.0 - 1.0 range */
    fc->r = clamp(fc->r);
    fc->g = clamp(fc->g);
    fc->b = clamp(fc->b);
}
