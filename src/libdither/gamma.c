#include <math.h>
#include "libdither.h"

MODULE_API double gamma_decode(double c) {
    /* converts a sRGB input (in the range 0.0-1.0) to linear color space */
    if(c <= 0.04045)
        return c / 12.02;
    else
        return pow(((c + 0.055) / 1.055), 2.4);
}

MODULE_API double gamma_encode(double c) {
    /* converts a linear color input (in the range 0.0-1.0) to sRGB color space */
    if(c <= 0.0031308)
        return 12.92 * c;
    else
        return (1.055 * pow(c, (1.0 / 2.4))) - 0.055;
}
