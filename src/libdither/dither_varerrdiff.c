#define MODULE_API_EXPORTS
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include <string.h>
#include "libdither.h"
#include "dither_varerrdiff_data.h"


MODULE_API void variable_error_diffusion_dither(const DitherImage* img, enum VarDitherType type, bool serpentine, uint8_t* out) {
    /* Variable Error Diffusion, implementing Ostromoukhov's and Zhou Fang's approach */
    srand((uint32_t)time(NULL));
    // dither matrix
    const int m_offset_x[2][3] = {{1, -1, 0}, {-1, 1, 0}};
    const int m_offset_y[2][3] = {{0, 1, 1}, {0, 1, 1}};

    double* buffer = calloc(img->width * img->height, sizeof(double));
    const long* divs;
    const long* coefs;
    if(type == Ostromoukhov) {
        coefs = ostro_coefs;
        divs = ostro_divs;
    } else {  // zhoufang
        coefs = zhoufang_coef;
        divs = zhoufang_divs;
        memcpy(buffer, img->buffer, img->width * img->height * sizeof(double));
    }
    // serpentine direction setup
    int direction = 0; // FORWARD
    int direction_toggle = 1;
    if(serpentine) direction_toggle = 2;
    // start dithering
    for(int y = 0; y < img->height; y++) {
        int start, end, step;
        // get direction
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
            double err;
            int coef_offs;
            size_t addr = y * img->width + x;
            double px = img->buffer[addr];
            // dither function
            if(type == Ostromoukhov) {   // ostro
                err = buffer[addr] + px;
                if (err > 0.5) {
                    out[addr] = 0xff;
                    err -= 1.0;
                }
            } else {  // zhoufang
                err = buffer[addr];
                if (px >= 0.5)
                    px = 1.0 - px;
                double threshold = (128.0 + (rand() % 128) * (rand_scale[(int)(px * 128.0)] / 100.0)) / 256.0;
                if (err >= threshold) {
                    out[addr] = 0xff;
                    err = buffer[addr] - 1.0;
                }
            }
            coef_offs = (int) (px * 255.0 + 0.5);
            // distribute the error
            err /= (double)divs[coef_offs];
            for(int i = 0; i < 3; i++) {
                int xx = x + m_offset_x[direction][i];
                if(-1 < xx && xx < img->width) {
                    int yy = y + m_offset_y[direction][i];
                    if(yy < img->height) {
                        buffer[yy * img->width + xx] += err * (double)coefs[coef_offs * 3 + i];
                    }
                }
            }
        }
        direction = (y + 1) % direction_toggle;
    }
    free(buffer);
}
