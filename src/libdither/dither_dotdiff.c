#define MODULE_API_EXPORTS
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include "libdither.h"
#include "dither_dotdiff_data.h"
#include "hash.h"

struct Point {
    int x;
    int y;
};
typedef struct Point Point;

Point* Point_new(int x, int y) {
    Point* self = calloc(1, sizeof(Point));
    self->x = x;
    self->y = y;
    return self;
}

void Point_delete(Point* self) {
    if(self) {
        free(self);
        self = NULL;
    }
}

MODULE_API DotDiffusionMatrix* get_default_diffusion_matrix() { return DotDiffusionMatrix_new(3, 3, default_diffusion_matrix); }
MODULE_API DotDiffusionMatrix* get_guoliu8_diffusion_matrix() { return DotDiffusionMatrix_new(3, 3, guoliu8_diffusion_matrix); }
MODULE_API DotDiffusionMatrix* get_guoliu16_diffusion_matrix() { return DotDiffusionMatrix_new(3, 3, guoliu16_diffusion_matrix); }
MODULE_API DotClassMatrix* get_mini_knuth_class_matrix() { return DotClassMatrix_new(4, 4, mini_knuth_class_matrix); }
MODULE_API DotClassMatrix* get_knuth_class_matrix() { return DotClassMatrix_new(8, 8, knuth_class_matrix); }
MODULE_API DotClassMatrix* get_optimized_knuth_class_matrix() { return DotClassMatrix_new(8, 8, optimized_knuth_class_matrix); }
MODULE_API DotClassMatrix* get_mese_8x8_class_matrix() { return DotClassMatrix_new(8, 8, mese_8x8_class_matrix); }
MODULE_API DotClassMatrix* get_mese_16x16_class_matrix() { return DotClassMatrix_new(16, 16, mese_16x16_class_matrix); }
MODULE_API DotClassMatrix* get_guoliu_8x8_class_matrix() { return DotClassMatrix_new(8, 8, guoliu_8x8_class_matrix); }
MODULE_API DotClassMatrix* get_guoliu_16x16_class_matrix() { return DotClassMatrix_new(16, 16, guoliu_16x16_class_matrix); }
MODULE_API DotClassMatrix* get_spiral_class_matrix() { return DotClassMatrix_new(8, 8, spiral_class_matrix); }
MODULE_API DotClassMatrix* get_spiral_inverted_class_matrix() { return DotClassMatrix_new(8, 8, spiral_inverted_class_matrix); }

MODULE_API DotClassMatrix* DotClassMatrix_new(int width, int height, const int* matrix) {
    DotClassMatrix* self = calloc(1, sizeof(DotClassMatrix));
    self->buffer = (int*)calloc(width * height, sizeof(int));
    memcpy(self->buffer, matrix, width * height * sizeof(int));
    self->width = width;
    self->height = height;
    return self;
}

MODULE_API void DotClassMatrix_free(DotClassMatrix* self) {
    if(self) {
        free(self->buffer);
        free(self);
        self = NULL;
    }
}

MODULE_API DotDiffusionMatrix* DotDiffusionMatrix_new(int width, int height, const double* matrix) {
    DotDiffusionMatrix* self = calloc(1, sizeof(DotDiffusionMatrix));
    self->buffer = (double*)calloc(width * height, sizeof(double));
    memcpy(self->buffer, matrix, width * height * sizeof(double));
    self->width = width;
    self->height = height;
    return self;
}

MODULE_API void DotDiffusionMatrix_free(DotDiffusionMatrix* self) {
    if(self) {
        free(self->buffer);
        free(self);
        self = NULL;
    }
}

MODULE_API void dot_diffusion_dither(const DitherImage* img, const DotDiffusionMatrix* dmatrix, const DotClassMatrix* cmatrix, uint8_t* out) {
    /* Knuth's dot dither algorithm */
    int blocksize = cmatrix->width;
    PHash* lut = PHash_new(blocksize * blocksize);
    for(int y = 0; y < blocksize; y++)
        for (int x = 0; x < blocksize; x++)
            PHash_insert(lut, cmatrix->buffer[y * blocksize + x], Point_new(x, y));

    double* orig_img = (double*)calloc(img->width * img->height, sizeof(double));
    memcpy(orig_img, img->buffer, img->width * img->height * sizeof(double));

    int pixel_no[9];
    double pixel_weight[9];
    int yyend = (int)ceil((double)img->height / (double)blocksize);
    for(int yy = 0; yy < yyend; yy++) {
        int ofs_y = yy * blocksize;
        int xxend = (int)ceil((double)img->width / (double)blocksize);
        for(int xx = 0; xx < xxend; xx++) {
            int ofs_x = xx * blocksize;
            for(int current_point_no = 0; current_point_no < blocksize * blocksize; current_point_no++) {
                Point *cm = PHash_search(lut, current_point_no);
                if(cm->y + ofs_y >= img->height || cm->x + ofs_x >= img->width)
                    continue;
                int imgx = cm->x + ofs_x;
                int imgy = cm->y + ofs_y;
                double err = orig_img[imgy * img->width + imgx];
                if(err >= 0.5) {
                    out[imgy * img->width + imgx] = 0xff;
                    err -= 1.0;
                }
                size_t j = 0;
                int x = cm->x - 1;
                int y = cm->y - 1;
                int total_err_weight = 0;
                for(int dmy=0; dmy < 3; dmy++) {
                    for(int dmx=0; dmx < 3; dmx++) {
                        int cmy = dmy + y;
                        int cmx = dmx + x;
                        if(-1 < cmx && cmx < blocksize && -1 < cmy && cmy < blocksize) {
                            int point_no = cmatrix->buffer[cmy * blocksize + cmx];
                            if(point_no > current_point_no) {
                                int sub_weight = (int)(dmatrix->buffer[dmy * dmatrix->width + dmx]);
                                total_err_weight += sub_weight;
                                pixel_no[j] = point_no;
                                pixel_weight[j] = (double)sub_weight;
                                j++;
                            }
                        }
                    }
                }
                if(total_err_weight > 0) {
                    err /= (double)total_err_weight;
                    for(size_t i=0; i < j; i++) {
                        int point_no = pixel_no[i];
                        double sub_weight = pixel_weight[i];
                        Point* c = PHash_search(lut, point_no);
                        int cx = c->x + ofs_x;
                        int cy = c->y + ofs_y;
                        if(cx < img->width && cy < img->height)
                            orig_img[cy * img->width + cx] += (err * sub_weight);
                    }
                }
            }
        }
    }
    PHash_delete(lut);
}
