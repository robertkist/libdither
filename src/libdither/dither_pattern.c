#define MODULE_API_EXPORTS
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include "libdither.h"
#include "dither_pattern_data.h"

MODULE_API TilePattern* get_2x2_pattern() { return TilePattern_new(2, 2, 5, tiles2x2); }
MODULE_API TilePattern* get_3x3_v1_pattern() { return TilePattern_new(3, 3, 13, tiles3x3_v1); }
MODULE_API TilePattern* get_3x3_v2_pattern() { return TilePattern_new(3, 3, 10, tiles3x3_v2); }
MODULE_API TilePattern* get_3x3_v3_pattern() { return TilePattern_new(3, 3, 10, tiles3x3_v3); }
MODULE_API TilePattern* get_4x4_pattern() { return TilePattern_new(4, 4, 6, tiles4x4); }
MODULE_API TilePattern* get_5x2_pattern() { return TilePattern_new(5, 2, 7, tiles5x2); }

MODULE_API TilePattern* TilePattern_new(int width, int height, int num_tiles, const int* pattern) {
    TilePattern* self = calloc(1, sizeof(TilePattern));
    self->buffer = (int*)calloc(width * height * num_tiles, sizeof(int));
    memcpy(self->buffer, pattern, width * height * num_tiles * sizeof(int));
    self->width = width;
    self->height = height;
    self->num_tiles = num_tiles;
    return self;
}

MODULE_API void TilePattern_free(TilePattern* self) {
    if(self) {
        free(self->buffer);
        free(self);
        self = NULL;
    }
}

MODULE_API void pattern_dither(const DitherImage* img, const TilePattern *pattern, uint8_t* out) {
    /* Pattern ditherer. Divides the source images into a grid and then chooses from a list of pre-defined
     * 1-bit patterns, based on source brightness, for each grid element */
    int th = pattern->height;
    int tw = pattern->width;
    int tile_size = tw * th;
    int width = (int)((float)img->width / (float)tw);
    int height = (int)((float)img->height / (float)th);
    // diffusion matrix
    double* cur = (double*)calloc(tile_size, sizeof(double));
    double* diffusion = (double*)calloc(tile_size, sizeof(double));
    double init_diffusion = 1.0 / (float)(tile_size);
    for(int i = 0; i < tile_size; i++)
        diffusion[i] = init_diffusion;
    // dither
    for(int y = 0; y < height; y++) {
        for(int x = 0; x < width; x++) {
            // get block
            for(int ty = 0; ty < th; ty++)
                for(int tx = 0; tx < tw; tx++)
                    cur[ty * tw + tx] = img->buffer[(y * th + ty) * img->width + (x * tw + tx)];
            // find closest tile, i.e. smallest distance
            double distance = 1000.0;
            int best_tile = 0;
            for(int n = 0; n < pattern->num_tiles; n++) {
                double d1 = 0.0;
                double d2 = 0.0;
                for(int ty = 0; ty < th; ty++) {
                    for(int tx = 0; tx < tw; tx++) {
                        size_t addr = ty * tw + tx;
                        size_t tile_addr = n * tile_size + addr;
                        d1 += diffusion[addr] * (cur[addr] - pattern->buffer[tile_addr]);
                        d2 += diffusion[addr] * fabs(cur[addr] - pattern->buffer[tile_addr]);
                    }
                }
                double d = fabs(d1) + d2;
                if(d < distance) {
                    distance = d;
                    best_tile = n;
                }
            }
            for(int ty = 0; ty < th; ty++)
                for(int tx = 0; tx < tw; tx++)
                    if(pattern->buffer[best_tile * tile_size + (ty * tw + tx)] == 1)
                        out[(y * th + ty) * img->width + (x * tw + tx)] = 0xff;
        }
    }
    free(cur);
    free(diffusion);
}
