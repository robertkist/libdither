#define MODULE_API_EXPORTS
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include "libdither.h"
#include "queue.h"
#include "dither_riemersma_data.h"

const int MAX_ITER = 20; // maximum iterations for curve generation

MODULE_API RiemersmaCurve* RiemersmaCurve_new(int base, int add_adjust, int exp_adjust, const char* axiom, int rule_count, const char* rules[], const char* keys, const int orientation[2], enum AdjustCurve adjust) {
    /* Initializes a new space filling curve but does not generate it yet.
     * Use the create_curve function to actually generate the curve. */
    RiemersmaCurve* self = calloc(1, sizeof(RiemersmaCurve));
    self->axiom = (char*)calloc(strlen(axiom) + 1, sizeof(char));
    strcpy(self->axiom, axiom);
    self->rules = (char**)calloc(rule_count, sizeof(char*));
    self->keys = (char*)calloc(rule_count, sizeof(char));
    for(int i = 0; i < rule_count; i++) {
        self->rules[i] = (char*)calloc(strlen(rules[i]) + 1, sizeof(char));
        strcpy(self->rules[i], rules[i]);
        self->keys[i] = keys[i];
    }
    self->base = base;
    self->add_adjust = add_adjust;
    self->exp_adjust = exp_adjust;
    self->rule_count = rule_count;
    for(int i = 0; i < 2; i ++) {
        self->orientation[i] = orientation[i];
    }
    switch(adjust) {
        case center_xy: self->adjust = 1; break;
        case center_x: self->adjust = 2; break;
        case center_y: self->adjust = 3; break;
        default: self->adjust = 0;
    }
    return self;
}

MODULE_API void RiemersmaCurve_free(RiemersmaCurve* self) {
    if(self) {
        for(int i = 0; i < self->rule_count; i++)
            free(self->rules[i]);
        free(self->rules);
        free(self->axiom);
        free(self->keys);
        free(self);
        self = NULL;
    }
}

MODULE_API RiemersmaCurve* get_hilbert_curve() { return RiemersmaCurve_new(2, 0, 0, hilbert_axiom, 2, hilbert_rules, hilbert_keys, hilbert_orientation, center_none); }
MODULE_API RiemersmaCurve* get_hilbert_mod_curve() { return RiemersmaCurve_new(2, 0, 1, hilbert_mod_axiom, 2, hilbert_mod_rules, hilbert_mod_keys, hilbert_mod_orientation, center_x); }
MODULE_API RiemersmaCurve* get_peano_curve() { return RiemersmaCurve_new(3, 0, 0, peano_axiom, 2, peano_rules, peano_keys, peano_orientation, center_none); }
MODULE_API RiemersmaCurve* get_fass0_curve() { return RiemersmaCurve_new(4, 0, 0, fass0_axiom, 2, fass0_rules, fass0_keys, fass0_orientation, center_none); }
MODULE_API RiemersmaCurve* get_fass1_curve() { return RiemersmaCurve_new(3, 0, 0, fass1_axiom, 2, fass1_rules, fass1_keys, fass1_orientation, center_none); }
MODULE_API RiemersmaCurve* get_fass2_curve() { return RiemersmaCurve_new(4, 0, 0, fass2_axiom, 2, fass2_rules, fass2_keys, fass2_orientation, center_none); }
MODULE_API RiemersmaCurve* get_gosper_curve() { return RiemersmaCurve_new(5, -1, 0, gosper_axiom, 2, gosper_rules, gosper_keys, gosper_orientation, center_none); }
MODULE_API RiemersmaCurve* get_fass_spiral_curve() { return RiemersmaCurve_new(3, 0, 0, fass_spiral_axiom, 3, fass_spiral_rules, fass_spiral_keys, fass_spiral_orientation, center_xy); }

int get_rule_size(char *rule, char rule_key[], int rule_count) {
    /* A helper function for allocating memory for the generated curve */
    int rule_size = 0;
    size_t rule_len = strlen(rule);
    for(size_t i = 0; i < rule_len; i++) {
        for(int j = 0; j < rule_count; j++) {
            if(rule[i] == rule_key[j]) {
                rule_size++;
                break;
            }
        }
    }
    return rule_size;
}

MODULE_API char* create_curve(RiemersmaCurve* curve, int width, int height, int* curve_dim) {
    /* creates a space filling curve */
    // determine iterations required
    int iterations = -1;
    for(int j = 0; j < MAX_ITER; j++) {
        *curve_dim = (int)pow(curve->base, j + curve->exp_adjust) + curve->add_adjust;
        if(*curve_dim > width && *curve_dim > height) {
            iterations = j;
            break;
        }
    }
    if(iterations == -1)
        return NULL;
    // determine memory heuristics
    size_t* rule_len = (size_t*)calloc(curve->rule_count, sizeof(size_t));
    float max_rule_size = 0.0;
    size_t max_rule_strlen = 0;
    for(int i=0; i < curve->rule_count; i ++) {
        rule_len[i] = (size_t)strlen(curve->rules[i]);
        float rs = (float)get_rule_size(curve->rules[i], curve->keys, curve->rule_count);
        rs = rs / (float)rule_len[i];
        if(rs > max_rule_size)
            max_rule_size = rs;
        size_t sl = strlen(curve->rules[i]);
        if(sl > max_rule_strlen)
            max_rule_strlen = sl;
    }
    // generate the curve
    char* axiom = (char*)calloc(strlen(curve->axiom) + 1, sizeof(char));
    strcpy(axiom, curve->axiom);
    for(int i = 0; i < iterations; i++) {
        size_t bufsize = (int)ceil((double)(strlen(axiom) + 1) * max_rule_size + 1) * max_rule_strlen + 1;
        char* out = (char*)calloc(bufsize, sizeof(char));
        char* p = out;
        size_t axlen = strlen(axiom);
        for(size_t j = 0; j < axlen; j++) {
            for(int k = 0; k < curve->rule_count; k++) {
                if(axiom[j] == curve->keys[k]) {
                    memcpy(p, curve->rules[k], rule_len[k]);
                    p += rule_len[k];
                    goto cont;
                }
            }
            *p = axiom[j];
            p++;
        cont:
            continue;
        }
        free(axiom);
        axiom = out;
    }
    free(rule_len);
    return axiom;
}

void riemersma_dither(const DitherImage* img, RiemersmaCurve* rcurve, bool use_riemersma, uint8_t* out) {
    /* Riemersma dither. Uses a space filling curve to distribute the dithering error.
     * parameter use_riemersma: when true, uses a slightly modified version of the Riemersma calculations which may
     *                          improve dithering results
     */
    int max = 16;
    int err_len = use_riemersma? 16 : 8;
    Queue* q_err = Queue_new(err_len);
    // set up weights
    double* weights = calloc(err_len, sizeof(double));
    if(use_riemersma) {  // original riemersma algorithm
        double m = exp(log((float)max) / (float)(err_len - 1));
        double v = 1.0;
        for(int i = 0; i < err_len; i++) {
            weights[i] = round(v);
            v *= m;
        }
    } else {  // modified riemersma algorithm
        double weights_sum = 0.0;
        for(int i = 0; i < err_len; i++) {
            double w = exp2(((float)i / (float)err_len) * 10.0) / 1000.0 * max;
            weights[i] = w;
            weights_sum += w;
        }
        for(int i = 0; i < err_len; i++)
            weights[i] /= weights_sum;
    }
    // generate curve
    int curve_dim;
    char* curve = create_curve(rcurve, img->width, img->height, &curve_dim);
    char* c = curve;
    // position - some curves must be centered in relation to the image
    float xc = (rcurve->adjust == 1 || rcurve->adjust == 2)? 0.5 : 0;
    float yc = (rcurve->adjust == 1 || rcurve->adjust == 3)? 0.5 : 0;
    int x = (int)((float)curve_dim * xc);
    int y = (int)((float)curve_dim * yc);
    // orientation
    int rx = rcurve->orientation[0];
    int ry = rcurve->orientation[1];
    // draw curve and dither
    size_t lc = strlen(curve);
    for(size_t j = 0; j < lc; j++) {
        if (*c == 'F') {
            x += rx;
            y += ry;
            if (x >= 0 && y >= 0 && x < img->width && y < img->height) {
                size_t addr = y * img->width + x;
                double err = 0.0;
                for(int i = 0; i < err_len; i++) {
                    err += q_err->queue[i] * (double)weights[i];
                }
                Queue_rotate(q_err);
                double p = img->buffer[addr];
                if(use_riemersma) {  // original riemersma algorithm
                    if(p + err / max > 0.5) {
                        out[addr] = 0xff;
                        q_err->queue[err_len - 1] = p - 1.0;
                    } else
                        q_err->queue[err_len - 1] = p;
                } else {  // modified riemersma algorithm
                    if(err + p > 0.5) {
                        out[addr] = 0xff;
                        q_err->queue[err_len - 1] = err + p - 1.0;
                    } else
                        q_err->queue[err_len - 1] = err + p;
                }
            }
        } else if (*c == '+') {
            int dx = ry; ry = -rx; rx = dx;
        } else if (*c == '-') {
            int dx = -ry; ry = rx; rx = dx;
        }
        c++;
    }
    free(weights);
    free(curve);
    Queue_delete(q_err);
}
