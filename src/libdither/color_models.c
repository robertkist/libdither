#define MODULE_API_EXPORTS
#define _USE_MATH_DEFINES
#include <math.h>
#include "color_models.h"
#include "libdither.h"

#ifndef M_PI
#define M_PI (3.14159265358979323846)
#endif

// quick degree-to-radians and radians-to-degrees conversion
#define DEG2RAD(deg) ((deg) * (M_PI / 180.0))
#define RAD2DEG(rad) ((rad) * (180.0 / M_PI))

// CCIR weights, based on human perception
#define CCIR_WR 299.0
#define CCIR_WG 587.0
#define CCIR_WB 114.0
#define CCIR_FAC 0.75

// pre-calculated LAB helper constants
static const double LAB94_K1 = 1.0 / 3.0;
static const double LAB94_K2 = 16.0 / 116.0;

static inline double MAX3d(double a, double b, double c) {
    /* returns the biggest number of the 3 inputs */
    return ((a > b)? (a > c ? a : c) : (b > c ? b : c));
}

static inline double MIN3d(double a, double b, double c) {
    /* returns the smallest number of the 3 inputs */
    return ((a < b)? (a < c ? a : c) : (b < c ? b : c));
}

void rgb_to_hsv(const FloatColor* c, FloatColor* out) {
    /* sRGB to Hue / Saturation / Value (HSV) color */
    // https://stackoverflow.com/questions/3018313/algorithm-to-convert-rgb-to-hsv-and-hsv-to-rgb-in-range-0-255-for-both
    double h = 0.0, s = 0.0, v, min, max, delta;
    min = MIN3d(c->r, c->g, c->b);
    max = MAX3d(c->r, c->g, c->b);
    v = max;
    delta = max - min;
    if (delta >= 0.00001) {
        if (max <= 0.0) {
            h = 0.0; //NAN;
        } else {
            s = (delta / max);
            if (c->r >= max) {
                h = (c->g - c->b) / delta;        // between yellow & magenta
            } else if (c->g >= max) {
                h = 2.0 + (c->b - c->r) / delta;  // between cyan & yellow
            } else {
                h = 4.0 + (c->r - c->g) / delta;  // between magenta & cyan
            }
            h *= 60.0;  // degrees
            if (h < 0.0) {
                h += 360.0;
            }
        }
    }
    FloatColor_set(out, DEG2RAD(h), s, v);
}

MODULE_API void rgb_to_linear(const FloatColor* fc, FloatColor* out) {
    /* sRGB (byte) to linear float color */
    if (fc->r < 0.04045) {
        out->r = fc->r * 0.0773993808;
    } else {
        out->r = pow(fc->r * 0.9478672986 + 0.0521327014, 2.4);
    }
    if (fc->g < 0.04045) {
        out->g = fc->g * 0.0773993808;
    } else {
        out->g = pow(fc->g * 0.9478672986 + 0.0521327014, 2.4);
    }
    if (fc->b < 0.04045) {
        out->b = fc->b * 0.0773993808;
    } else {
        out->b = pow(fc->b * 0.9478672986 + 0.0521327014, 2.4);
    }
}

void rgb_to_lab(const FloatColor* c, FloatColor* out, const FloatColor* illuminant) {
    /* sRGB to L*a*b (aka LAB) color */
    FloatColor rgb, xyz;
    // sRGB to linear RGB (remove gamma)
    rgb.r = (c->r > 0.04045) ? pow((c->r + 0.055) / 1.055, 2.4) : (c->r / 12.92);
    rgb.g = (c->g > 0.04045) ? pow((c->g + 0.055) / 1.055, 2.4) : (c->g / 12.92);
    rgb.b = (c->b > 0.04045) ? pow((c->b + 0.055) / 1.055, 2.4) : (c->b / 12.92);
    // linear RGB to XYZ
    xyz.x = (rgb.r * 0.4124564 + rgb.g * 0.3575761 + rgb.b * 0.1804375);
    xyz.y = (rgb.r * 0.2126729 + rgb.g * 0.7151522 + rgb.b * 0.0721750);
    xyz.z = (rgb.r * 0.0193339 + rgb.g * 0.1191920 + rgb.b * 0.9503041);
    // apply illuminant
    xyz.x /= illuminant->x;
    xyz.z /= illuminant->z;
    // apply f(t) / nonlinear mapping that models how human vision (XYZ to LAB)
    double x = (xyz.x > 0.008856) ? pow(xyz.x, LAB94_K1) : (7.787 * xyz.x + LAB94_K2);
    double y = (xyz.y > 0.008856) ? pow(xyz.y, LAB94_K1) : (7.787 * xyz.y + LAB94_K2);
    double z = (xyz.z > 0.008856) ? pow(xyz.z, LAB94_K1) : (7.787 * xyz.z + LAB94_K2);
    // compute LAB color
    out->l = 116.0 * y - 16.0;
    out->a = 500.0 * (x - y);
    out->b = 200.0 * (y - z);
}

void rgb_to_luminance(const FloatColor* c, FloatColor* out) {
    /* sRGB to human perceived brightness */
    double l = 0.2126 * (double)c->r + 0.7152 * (double)c->g + 0.0722 * (double)c->b;
    FloatColor_set(out, l, l, l);
}

double distance_linear(const FloatColor* a, const FloatColor* b) {
    /* Euclidean distance */
    double dist_r = a->r - b->r;
    double dist_g = a->g - b->g;
    double dist_b = a->b - b->b;
    return sqrt(dist_r * dist_r + dist_g * dist_g + dist_b * dist_b);
}

double distance_luminance(const FloatColor* a, const FloatColor* b) {
    /* luminance (brightness) difference */
    return fabs(a->l - b->l);
}

double distance_hsv(const FloatColor* a, const FloatColor* b) {
    /* Hue / Saturation / Value (HSV) distance */
    double wplane1 = sin(a->h) * a->s * a->s - sin(b->h) * b->s * b->s;
    double wplane2 = cos(a->h) * a->s * a->s - cos(b->h) * b->s * b->s;
    double wvalue = a->v - b->v;
    return fabs(wplane1 * wplane1 * 0.7 + wplane2 * wplane2* 0.7 + wvalue * wvalue * 3.0);
}

double distance_ccir(const FloatColor* a, const FloatColor* b) {
    /* color distance taking human perception into account */
    // also see: https://groups.google.com/g/sci.engr.color/c/KJeeUkitd_A
    double luma1 = (a->r * CCIR_WR + a->g * CCIR_WG + a->b * CCIR_WB) / 255000.0; // 255 * 1000
    double luma2 = (b->r * CCIR_WR + b->g * CCIR_WG + b->b * CCIR_WB) / 255000.0; // 255 * 1000
    double lumadiff = luma1 - luma2;
    double diffR = (a->r - b->r) / 255.0;
    double diffG = (a->g - b->g) / 255.0;
    double diffB = (a->b - b->b) / 255.0;
    return sqrt(diffR * diffR * (CCIR_WR / 1000.0) + diffG * diffG * (CCIR_WG / 1000.0) +
                diffB * diffB * (CCIR_WB / 1000.0)) * CCIR_FAC +
           lumadiff * lumadiff;
}

double distance_lab2000(const FloatColor* a, const FloatColor* b, const FloatColor* weights) {
    /* L*a*b color distance - LAB2000 */
    double C1 = sqrt(a->a * a->a + a->b * a->b);
    double C2 = sqrt(b->a * b->a + b->b * b->b);
    double C_ave = (C1 + C2) / 2.0;
    double G = 0.5 * (1.0 - sqrt(pow(C_ave, 7) / (pow(C_ave, 7) + pow(25.0, 7))));
    double a1p = (1.0 + G) * a->a;
    double a2p = (1.0 + G) * b->a;
    double C1p = sqrt(a1p * a1p + a->b * a->b);
    double C2p = sqrt(a2p * a2p + b->b * b->b);
    double h1p = atan2(a->b, a1p);
    if (h1p < 0) h1p += 2.0 * M_PI;
    double h2p = atan2(b->b, a2p);
    if (h2p < 0) h2p += 2.0 * M_PI;
    // calculate differences
    double dLp = b->l - a->l; // L2 - L1
    double dCp = C2p - C1p;
    double dhp;
    if (C1p * C2p == 0) {
        dhp = 0.0;
    } else {
        double deltah = h2p - h1p;
        if (fabs(deltah) <= M_PI) {
            dhp = deltah;
        } else if (deltah > M_PI) {
            dhp = deltah - 2.0 * M_PI;
        } else {
            dhp = deltah + 2.0 * M_PI;
        }
    }
    double dHp = 2.0 * sqrt(C1p * C2p) * sin(dhp / 2.0);
    // calculate averages
    double Lp_ave = (a->l + b->l) / 2.0;
    double Cp_ave = (C1p + C2p) / 2.0;
    double hp_ave;
    if (C1p * C2p == 0) {
        hp_ave = h1p + h2p;
    } else {
        double deltah = fabs(h1p - h2p);
        if (deltah <= M_PI) {
            hp_ave = (h1p + h2p) / 2.0;
        } else {
            if (h1p + h2p < 2.0 * M_PI) {
                hp_ave = (h1p + h2p + 2.0 * M_PI) / 2.0;
            } else {
                hp_ave = (h1p + h2p - 2.0 * M_PI) / 2.0;
            }
        }
    }
    // calculate T
    double hp_ave_deg = (hp_ave);
    double T = 1.0
               - 0.17 * cos(DEG2RAD(hp_ave_deg - 30.0))
               + 0.24 * cos(DEG2RAD(2.0 * hp_ave_deg))
               + 0.32 * cos(DEG2RAD(3.0 * hp_ave_deg + 6.0))
               - 0.20 * cos(DEG2RAD(4.0 * hp_ave_deg - 63.0));
    // calculate delta_theta, R_C, S_L, S_C, S_H, R_T
    double tpow = (hp_ave_deg - 275.0) / 25.0;
    double delta_theta = DEG2RAD(30.0) * exp(-(tpow * tpow));
    double R_C = 2.0 * sqrt(pow(Cp_ave, 7) / (pow(Cp_ave, 7) + pow(25.0, 7)));
    double S_Lpow = Lp_ave - 50.0;
    S_Lpow *= S_Lpow;
    double S_L = 1.0 + ((0.015 * S_Lpow) / sqrt(20 + S_Lpow));
    double S_C = 1.0 + 0.045 * Cp_ave;
    double S_H = 1.0 + 0.015 * Cp_ave * T;
    double R_T = -sin(2.0 * delta_theta) * R_C;
    // calculate deltaE
    double dE_pow1 = dLp / (weights->v * S_L);
    double dE_pow2 = dCp / (weights->c * S_C);
    double dE_pow3 = dHp / (weights->h * S_H);
    return sqrt(dE_pow1 * dE_pow1 + dE_pow2 * dE_pow2 + dE_pow3 * dE_pow3 +
                R_T * (dCp / (weights->c * S_C)) * (dHp / (weights->h * S_H)));
}

double distance_lab94(const FloatColor* b, const FloatColor* a, const FloatColor* weights) {
    /* L*a*b color distance - LAB94 */
    double deltaL = a->l - b->l;
    double c1 = sqrt(a->a * a->a + a->b * a->b);
    double c2 = sqrt(b->a * b->a + b->b * b->b);
    double deltaC = c1 - c2;
    double deltaA = a->a - b->a;
    double deltaB = a->b - b->b;
    double deltaH_sq = deltaA * deltaA + deltaB * deltaB - deltaC * deltaC;
    deltaH_sq = fmax(0.0, deltaH_sq);
    double C_avg = (c1 + c2) / 2.0;
    double sC = 1.0 + 0.045 * C_avg;
    double sH = 1.0 + 0.015 * C_avg;
    double termL = deltaL / weights->v;
    double termC = deltaC / (sC * weights->c);
    double termH = sqrt(deltaH_sq) / (sH * weights->h);
    return sqrt(termL * termL + termC * termC + termH * termH);
}
