#pragma once
#ifndef MATRICES_H
#define MATRICES_H

struct Private_GenericDitherMatrix {
    double divisor;
    int* buffer;  // buffer for flat matrix array
    int  width;
    int  height;
};

struct Private_IntMatrix {
    int* buffer;  // buffer for flat matrix array
    int  width;
    int  height;
};

struct Private_DoubleMatrix {
    double* buffer;  // buffer for flat matrix array
    int width;
    int height;
};

struct Private_TilePattern {
    int* buffer;  // buffer for flat tiles array
    int  width;
    int  height;
    int num_tiles;
};

struct Private_RiemersmaCurve {
    char* axiom;
    char** rules;
    char* keys;
    int orientation[2];
    int base;
    int add_adjust;
    int exp_adjust;
    int rule_count;
    int adjust;
};

struct Private_DotLippensData {
    int* cm;
    int cm_width;
    int cm_height;
    int* coe;
    int coe_width;
    int coe_height;
    double coe_sum;
};

#endif  // MATRICES_H
