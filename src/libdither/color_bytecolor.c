#include "color_bytecolor.h"

void ByteColor_copy(ByteColor* out, const ByteColor* bc) {
    /* copies a ByteColor's values */
    out->r = bc->r;
    out->g = bc->g;
    out->b = bc->b;
    out->a = bc->a;
}
