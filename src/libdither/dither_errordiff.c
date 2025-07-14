#define MODULE_API_EXPORTS
#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "libdither.h"
#include "color_floatpalette.h"
#include "color_models.h"
#include "random.h"
#include "dither_errordiff_data.h"

/* ***** BUILT-IN DIFFUSION MATRICES ***** */

MODULE_API ErrorDiffusionMatrix* get_xot_matrix(void) { return ErrorDiffusionMatrix_new(14, 10, 355, matrix_xot); }
MODULE_API ErrorDiffusionMatrix* get_diagonal_matrix(void) { return ErrorDiffusionMatrix_new(3, 2, 16, matrix_diagonal); }
MODULE_API ErrorDiffusionMatrix* get_floyd_steinberg_matrix(void) { return ErrorDiffusionMatrix_new(3, 2, 16, matrix_floyd_steinberg); }
MODULE_API ErrorDiffusionMatrix* get_shiaufan3_matrix(void) { return ErrorDiffusionMatrix_new(4, 2, 8, matrix_shiaufan_3); }
MODULE_API ErrorDiffusionMatrix* get_shiaufan2_matrix(void) { return ErrorDiffusionMatrix_new(4, 2, 16, matrix_shiaufan_2); }
MODULE_API ErrorDiffusionMatrix* get_shiaufan1_matrix(void) { return ErrorDiffusionMatrix_new(5, 2, 16, matrix_shiaufan_1); }
MODULE_API ErrorDiffusionMatrix* get_stucki_matrix(void) { return ErrorDiffusionMatrix_new(5, 3, 42, matrix_stucki); }
MODULE_API ErrorDiffusionMatrix* get_diffusion_1d_matrix(void) { return ErrorDiffusionMatrix_new(2, 1, 1, matrix_diffusion_1d); }
MODULE_API ErrorDiffusionMatrix* get_diffusion_2d_matrix(void) { return ErrorDiffusionMatrix_new(2, 2, 2, matrix_diffusion_2d); }
MODULE_API ErrorDiffusionMatrix* get_fake_floyd_steinberg_matrix(void) { return ErrorDiffusionMatrix_new(2, 2, 8, matrix_fake_floyd_steinberg); }
MODULE_API ErrorDiffusionMatrix* get_jarvis_judice_ninke_matrix(void) { return ErrorDiffusionMatrix_new(5, 3, 48, matrix_jarvis_judice_ninke); }
MODULE_API ErrorDiffusionMatrix* get_atkinson_matrix(void) { return ErrorDiffusionMatrix_new(4, 3, 8, matrix_atkinson); }
MODULE_API ErrorDiffusionMatrix* get_burkes_matrix(void) { return ErrorDiffusionMatrix_new(5, 2, 32, matrix_burkes); }
MODULE_API ErrorDiffusionMatrix* get_sierra_3_matrix(void) { return ErrorDiffusionMatrix_new(5, 3, 32, matrix_sierra_3); }
MODULE_API ErrorDiffusionMatrix* get_sierra_2row_matrix(void) { return ErrorDiffusionMatrix_new(5, 2, 16, matrix_sierra_2row); }
MODULE_API ErrorDiffusionMatrix* get_sierra_lite_matrix(void) { return ErrorDiffusionMatrix_new(3, 2, 4, matrix_sierra_lite); }
MODULE_API ErrorDiffusionMatrix* get_steve_pigeon_matrix(void) { return ErrorDiffusionMatrix_new(5, 3, 14, matrix_steve_pigeon); }
MODULE_API ErrorDiffusionMatrix* get_robert_kist_matrix(void) { return ErrorDiffusionMatrix_new(5, 3, 220, matrix_robert_kist); }
MODULE_API ErrorDiffusionMatrix* get_stevenson_arce_matrix(void) { return ErrorDiffusionMatrix_new(7, 4, 200, matrix_stevenson_arce); }

/* ***** ERROR DIFFUSION MATRIX OBJECT STRUCT ***** */

MODULE_API ErrorDiffusionMatrix* ErrorDiffusionMatrix_new(int width, int height, double divisor, const int* matrix) {
    ErrorDiffusionMatrix* self = calloc(1, sizeof(ErrorDiffusionMatrix));
    size_t size = (size_t)(width * height);
    self->buffer = (int*)calloc(size, sizeof(int));
    memcpy(self->buffer, matrix, size * sizeof(int));
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
            size_t addr = (size_t)(y * img->width + x);
            if (img->transparency[addr] != 0) { // dither all not fully transparent pixels
                double err = buffer[addr];
                if (sigma > 0.0)
                    threshold = box_muller(sigma, 0.5);
                if (err > threshold) {
                    out[addr] = 0xff;
                    err -= 1.0;
                }
                err /= m->divisor;
                for (int g = 0; g < matrix_length; g++) {
                    int xx = x + m_offset_x[g + matrix_length * direction];
                    if (-1 < xx && xx < img->width) {
                        int yy = y + m_offset_y[g];
                        if (yy < img->height) {
                            buffer[yy * img->width + xx] += err * m_weights[g + matrix_length * direction];
                        }
                    }
                }

            } else
                out[addr] = 128;
        }
        direction = (y + 1) % direction_toggle;
    }
    free(buffer);
    free(m_weights);
    free(m_offset_x);
    free(m_offset_y);
}

MODULE_API void error_diffusion_dither_color(const ColorImage* img, const ErrorDiffusionMatrix* m,
                                             CachedPalette* lookup_pal, bool serpentine, int* out) {
    // prepare the matrix...
    int i = 0;
    int j = 0;
    double *m_weights = NULL;
    int *m_offset_x = NULL;
    int *m_offset_y = NULL;
    int matrix_length = 0;
    size_t image_size = (size_t)(img->width * img->height);
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
    FloatColor* buffer = (FloatColor*)calloc(image_size, sizeof(FloatColor));
    memcpy(buffer, img->b_linear, image_size * sizeof(FloatColor));

    ByteColor bc;
    int direction = 0; // FORWARD
    int direction_toggle = 1;
    if(serpentine) direction_toggle = 2;
    for(int y = 0; y < img->height; y++) {
        int start, end, step;
        if (direction == 0) {
            start = 0;
            end = img->width;
            step = 1;
        } else {
            start = img->width - 1;
            end = -1;
            step = -1;
        }
        for (int x = start; x != end; x += step) {
            FloatColor  error_new;
            size_t addr = (size_t)(y * img->width + x);
            ColorImage_get_srgb(img, addr, &bc);
            if (bc.a != 0) {  // dither all not fully transparent pixels
                FloatColor *color = &buffer[addr];         // get error (linear)
                FloatColor_clamp(color);

                size_t index = CachedPalette_find_closest_color(lookup_pal, color); // get closest
                out[addr] = (int) index; // set out image
                ByteColor *srgb_b = BytePalette_get(lookup_pal->target_palette, index); // get sRGB color

                FloatColor_from_ByteColor(&error_new, srgb_b);
                FloatColor_sub(color, &error_new); // calculate new error
                for (int g = 0; g < matrix_length; g++) {
                    int xx = x + m_offset_x[g + matrix_length * direction];
                    if (-1 < xx && xx < img->width) {
                        int yy = y + m_offset_y[g];
                        if (yy < img->height) {
                            double dd = (double) m_weights[g + matrix_length * direction] / m->divisor;
                            addr = (size_t)(yy * img->width + xx);
                            buffer[addr].r += (color->r * dd);
                            buffer[addr].g += (color->g * dd);
                            buffer[addr].b += (color->b * dd);
                        }
                    }
                }
            } else {
                out[addr] = -1;  // transparent
            }
        }
        direction = (y + 1) % direction_toggle;
    }
    free(buffer);
    free(m_weights);
    free(m_offset_x);
    free(m_offset_y);
}
