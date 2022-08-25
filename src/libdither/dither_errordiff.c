#define MODULE_API_EXPORTS
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include "libdither.h"
#include "random.h"
#include "dither_errordiff_data.h"

/* ***** BUILT-IN DIFFUSION MATRICES ***** */

MODULE_API ErrorDiffusionMatrix* get_xot_matrix() { return ErrorDiffusionMatrix_new(14, 10, 355, matrix_xot); }
MODULE_API ErrorDiffusionMatrix* get_diagonal_matrix() { return ErrorDiffusionMatrix_new(3, 2, 16, matrix_diagonal); }
MODULE_API ErrorDiffusionMatrix* get_floyd_steinberg_matrix() { return ErrorDiffusionMatrix_new(3, 2, 16, matrix_floyd_steinberg); }
MODULE_API ErrorDiffusionMatrix* get_shiaufan3_matrix() { return ErrorDiffusionMatrix_new(4, 2, 8, matrix_shiaufan_3); }
MODULE_API ErrorDiffusionMatrix* get_shiaufan2_matrix() { return ErrorDiffusionMatrix_new(4, 2, 16, matrix_shiaufan_2); }
MODULE_API ErrorDiffusionMatrix* get_shiaufan1_matrix() { return ErrorDiffusionMatrix_new(5, 2, 16, matrix_shiaufan_1); }
MODULE_API ErrorDiffusionMatrix* get_stucki_matrix() { return ErrorDiffusionMatrix_new(5, 3, 42, matrix_stucki); }
MODULE_API ErrorDiffusionMatrix* get_diffusion_1d_matrix() { return ErrorDiffusionMatrix_new(2, 1, 1, matrix_diffusion_1d); }
MODULE_API ErrorDiffusionMatrix* get_diffusion_2d_matrix() { return ErrorDiffusionMatrix_new(2, 2, 2, matrix_diffusion_2d); }
MODULE_API ErrorDiffusionMatrix* get_fake_floyd_steinberg_matrix() { return ErrorDiffusionMatrix_new(2, 2, 8, matrix_fake_floyd_steinberg); }
MODULE_API ErrorDiffusionMatrix* get_jarvis_judice_ninke_matrix() { return ErrorDiffusionMatrix_new(5, 3, 48, matrix_jarvis_judice_ninke); }
MODULE_API ErrorDiffusionMatrix* get_atkinson_matrix() { return ErrorDiffusionMatrix_new(4, 3, 8, matrix_atkinson); }
MODULE_API ErrorDiffusionMatrix* get_burkes_matrix() { return ErrorDiffusionMatrix_new(5, 2, 32, matrix_burkes); }
MODULE_API ErrorDiffusionMatrix* get_sierra_3_matrix() { return ErrorDiffusionMatrix_new(5, 3, 32, matrix_sierra_3); }
MODULE_API ErrorDiffusionMatrix* get_sierra_2row_matrix() { return ErrorDiffusionMatrix_new(5, 2, 16, matrix_sierra_2row); }
MODULE_API ErrorDiffusionMatrix* get_sierra_lite_matrix() { return ErrorDiffusionMatrix_new(3, 2, 4, matrix_sierra_lite); }
MODULE_API ErrorDiffusionMatrix* get_steve_pigeon_matrix() { return ErrorDiffusionMatrix_new(5, 3, 14, matrix_steve_pigeon); }
MODULE_API ErrorDiffusionMatrix* get_robert_kist_matrix() { return ErrorDiffusionMatrix_new(5, 3, 220, matrix_robert_kist); }
MODULE_API ErrorDiffusionMatrix* get_stevenson_arce_matrix() { return ErrorDiffusionMatrix_new(7, 4, 200, matrix_stevenson_arce); }

/* ***** ERROR DIFFUSION MATRIX OBJECT STRUCT ***** */

MODULE_API ErrorDiffusionMatrix* ErrorDiffusionMatrix_new(int width, int height, double divisor, const int* matrix) {
    ErrorDiffusionMatrix* self = calloc(1, sizeof(ErrorDiffusionMatrix));
    self->buffer = (int*)calloc(width * height, sizeof(int));
    memcpy(self->buffer, matrix, width * height * sizeof(int));
    self->width = width;
    self->height = height;
    self->divisor = divisor;
    return self;
}

MODULE_API void ErrorDiffusionMatrix_free(ErrorDiffusionMatrix* self) {
    if(self) {
        free(self->buffer);
        free(self);
        self = NULL;
    }
}

/* ***** ERROR DIFFUSION DITHER FUNCTION ***** */

MODULE_API void error_diffusion_dither(const DitherImage* img,
                                       const ErrorDiffusionMatrix* m,
                                       bool serpentine,
                                       double sigma,
                                       uint8_t* out) {
    /* Error Diffusion dithering
     * img: source image to be dithered
     * serpentine:
     * sigma: jitter
     * setPixel: callback function to set a pixel
     */
    // prepare the matrix...
    int i = 0;
    int j = 0;
    double *m_weights = NULL;
    int *m_offset_x = NULL;
    int *m_offset_y = NULL;
    int matrix_length = 0;
    for(int y = 0; y < m->height; y++) {
        for(int x = 0; x < m->width; x++) {
            int value = m->buffer[y * m->width + x];
            if(value == -1) {
                matrix_length = (m->width * m->height - i - 1);
                m_weights = calloc((size_t)matrix_length * 2, sizeof(double ));
                m_offset_x = calloc((size_t)matrix_length * 2, sizeof(int));
                m_offset_y = calloc((size_t)matrix_length, sizeof(int));
            } else if(value > 0) {
                m_weights[j] = (double)value;
                m_weights[j + matrix_length] = (double)value;
                m_offset_x[j] = (x - i);
                m_offset_x[j + matrix_length] = -(x - i);
                m_offset_y[j] = y;
                j++;
            } if(matrix_length == 0) {
                i++;
            }
        }
    }
    // do the error diffusion...
    double* buffer = calloc((size_t)(img->width * img->height), sizeof(double));
    memcpy(buffer,img->buffer, (size_t)(img->width * img->height) * sizeof(double));
    int direction = 0; // FORWARD
    int direction_toggle = 1;
    if(serpentine) direction_toggle = 2;

    double threshold = 0.5;
    for(int y = 0; y < img->height; y++) {
        int start, end, step;
        if(direction == 0) {
            start = 0;
            end = img->width;
            step = 1;
        } else {
            start = img->width - 1;
            end = -1;
            step = -1;
        }
        for (int x = start; x != end; x += step) {
            size_t addr = y * img->width + x;
            double err = buffer[addr];
            if(sigma > 0.0)
                threshold = box_muller(sigma, 0.5);
            if(err > threshold) {
                out[addr] = 0xff;
                err -= 1.0;
            }
            err /= m->divisor;
            for(int g = 0; g < matrix_length; g++) {
                int xx = x + m_offset_x[g + matrix_length * direction];
                if(-1 < xx && xx < img->width) {
                    int yy = y + m_offset_y[g];
                    if(yy < img->height) {
                        buffer[yy * img->width + xx] += err * m_weights[g + matrix_length * direction];
                    }
                }
            }
        }
        direction = (y + 1) % direction_toggle;
    }
    free(buffer);
    free(m_weights);
    free(m_offset_x);
    free(m_offset_y);
}
