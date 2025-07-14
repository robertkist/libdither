#pragma once
#ifndef COLOR_FLOATCOLOR_H
#define COLOR_FLOATCOLOR_H

#include "color_bytecolor.h"

#define FLOAT_COLOR_RGB_CHANNELS 3  // r,g,b / h,s,v / l,a,b / x,y,z

struct FloatColor {
    union {         // first color component
        double r;   // rgb: r
        double h;   // hsv: h // munsell: hue
        double l;   // lab: l
        double x;   // xyz: x
    };
    union {         // second color component
        double g;   // rgb: g
        double s;   // hsv: s
        double a;   // lab: a
        double y;   // xyz: y
        double c;   // munsell: chroma
    };
    union {         // third color component
        double b;   // rgb: b // lab: b
        double v;   // hsv: v // munsell: value
        double z;   // xyz: z
    };
};
typedef struct FloatColor FloatColor;

void FloatColor_set(FloatColor* out, double r, double g, double b);
void FloatColor_add(FloatColor* out, const FloatColor* in);
void FloatColor_sub(FloatColor* out, const FloatColor* in);
void FloatColor_add_float(FloatColor* out, double value);
void FloatColor_sub_float(FloatColor* out, double value);

void FloatColor_from_ByteColor(FloatColor* out, const ByteColor* bc);
void FloatColor_from_FloatColor(FloatColor* out, const FloatColor* fc2);
void FloatColor_clamp(FloatColor* ic);

#endif  // COLOR_FLOATCOLOR_H
