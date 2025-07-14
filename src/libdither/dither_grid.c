#define MODULE_API_EXPORTS
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <stdbool.h>
#include "libdither.h"

inline int32_t MIN(int32_t a, int32_t b) { return((a) < (b) ? a : b); }

MODULE_API void grid_dither(const DitherImage* img, int w, int h, int min_pixels, bool alt_algorithm, uint8_t* out) {
    srand((uint32_t)time(NULL));
    size_t dimensions = (size_t)(img->width * img->height);
    for(size_t i = 0; i < dimensions; i++)
        out[i] = 0xff;
    int grid_width = w;
    int grid_height = h;
    int grid_area = grid_width * grid_height;
    int max_pixels = grid_area;
    double maxn = (double)(max_pixels * max_pixels) / ((double)grid_area / 4.0);
    for(int y = 0; y < img->height; y += grid_height) {
        for(int x = 0; x < img->width; x += grid_width) {
            double sum_intensity = 0.0;
            int samplecount = 0;
            for (int yy = 0; yy < grid_height; yy++)
                for (int xx = 0; xx < grid_width; xx++, samplecount++)
                    if (y + yy < img->height && x + xx < img->width)
                        sum_intensity += img->buffer[(y + yy) * img->width + x + xx];

            double avg_intensity = sum_intensity / (double)samplecount;
            double npow = (1.0 - avg_intensity) * max_pixels;
            double n = (npow * npow) / ((double)samplecount / 4.0);
            if(n < min_pixels)
                n = 0.0;
            if(alt_algorithm) {
                int* o = calloc((size_t)grid_area, sizeof(int));
                int limit = (int)round((n * (double)grid_area) / maxn);
                int c = 0;
                for(int i = 0; i < grid_area; i++) {
                    while(true) {
                        int xr = rand() % (grid_width);
                        int yr = rand() % (grid_height);
                        if(o[yr * grid_width + xr] == 0) {
                            if(x + xr < img->width && y + yr < img->height) {
                                out[(y + yr) * img->width + x + xr] = 0;
                            }
                            o[yr * grid_width + xr] = 1;
                            c++;
                            break;
                        }
                    }
                    if(c > limit)
                        break;
                }
                free(o);
            } else {
                for (int i = 0; i < (int) n; i++) {
                    int xx = x + (rand() % (MIN(x + grid_width, img->width) - x));
                    int yy = y + (rand() % (MIN(y + grid_height, img->height) - y));
                    if (xx < img->width && yy < img->height)
                        out[yy * img->width + xx] = 0;
                }
            }
        }
    }
    // apply transparency
    for (size_t i = 0; i < (size_t)(img->width * img->height); i++) {
        out[i] = img->transparency[i] != 0 ? out[i] : 128;
    }
}
