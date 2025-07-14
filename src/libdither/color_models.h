#pragma once
#ifndef COLOR_MODELS_H
#define COLOR_MODELS_H

#include "color_floatcolor.h"
#include "color_bytecolor.h"
#include "color_floatpalette.h"

double distance_linear_lab(const FloatColor* a, const FloatColor* b, const FloatColor* weights);
double distance_linear(const FloatColor* a, const FloatColor* b);
double distance_luminance(const FloatColor* a, const FloatColor* b);
double distance_hsv(const FloatColor* a, const FloatColor* b);
double distance_ccir(const FloatColor* a, const FloatColor* b);
double distance_ccir_alt(const FloatColor* a, const FloatColor* b);
double distance_lab94(const FloatColor* b, const FloatColor* a, const FloatColor* weights);
double distance_lab2000(const FloatColor* a, const FloatColor* b, const FloatColor* weights);

void rgb_to_luminance(const FloatColor* c, FloatColor* out);
void rgb_to_lab(const FloatColor* c, FloatColor* out, const FloatColor* illuminant);
void rgb_to_hsv(const FloatColor* c, FloatColor* out);

#endif // COLOR_MODELS_H
