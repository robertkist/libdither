#define MODULE_API_EXPORTS
#include <stdlib.h>
#include <time.h>
#include "kdtree/kdtree.h"
#include "color_bytepalette.h"
#include "color_bytecolor.h"
#include "libdither.h"

#define MAX_ITER 10    // max of k-means iterations if convergence cannot be reached

static void color_to_point(const ByteColor* color, double* pt) {
    /* convert color to kdtree point */
    pt[0] = color->r;
    pt[1] = color->g;
    pt[2] = color->b;
}

static void pick_k_unique(size_t* out, size_t k, size_t n) {
    /* pick k random unique indices between 0 an n */
    size_t chosen = 0;
    size_t i = 1;
    while (chosen < k) {
        //size_t idx = rand() % n;
        size_t idx = n / i; i++;
        bool exists = false;
        for (size_t j = 0; j < chosen; j++) {
            if (out[j] == idx) {
                exists = true;
                break;
            }
        }
        if (!exists) {
            out[chosen++] = idx;
        }
    }
}

BytePalette* kdtree_quantization(const BytePalette* unique_pal, size_t target_colors) {
    /* kdtree quantization */
    //srand((unsigned int)time(NULL));
    ByteColor* pixels = (ByteColor*)calloc(unique_pal->size, sizeof(ByteColor));
    for (size_t i = 0; i < unique_pal->size; i++) {
        ByteColor_copy(&pixels[i], BytePalette_get(unique_pal, i));
    }


    // initialize centers randomly
    ByteColor* centers = (ByteColor*)calloc(target_colors, sizeof(ByteColor));
    size_t* initial_indices = (size_t*)calloc(target_colors, sizeof(size_t));

    pick_k_unique(initial_indices, target_colors, unique_pal->size);
    for (size_t i = 0; i < target_colors; i++) {
        centers[i] = pixels[initial_indices[i]];
    }
    size_t* assignments = (size_t*)calloc(unique_pal->size, sizeof(size_t));
    for (size_t iter = 0; iter < MAX_ITER; iter++) {
        // build tree with current centers
        struct kdtree *center_tree = kd_create(3);
        for (size_t i = 0; i < target_colors; i++) {
            double pt[3];
            color_to_point(&centers[i], pt);
            kd_insert(center_tree, pt, (void*)i);
        }
        // assign each pixel to nearest center
        for (size_t i = 0; i < unique_pal->size; i++) {
            double pt[3];
            color_to_point(&pixels[i], pt);
            struct kdres *res = kd_nearest(center_tree, pt);
            if (res) {
                size_t idx = (size_t)kd_res_item_data(res);
                assignments[i] = idx;
                kd_res_free(res);
            } else {
                assignments[i] = 0; // fallback
            }
        }
        kd_free(center_tree);

        // update centers by computing mean of assigned pixels
        size_t* count=(size_t*)calloc(target_colors, sizeof(size_t));
        size_t* sum_r=(size_t*)calloc(target_colors, sizeof(size_t));
        size_t* sum_g=(size_t*)calloc(target_colors, sizeof(size_t));
        size_t* sum_b=(size_t*)calloc(target_colors, sizeof(size_t));

        for (size_t i = 0; i < target_colors; i++) {
            count[i] = sum_r[i] = sum_g[i] = sum_b[i] = 0;
        }
        for (size_t i = 0; i < unique_pal->size; i++) {
            size_t c = assignments[i];
            sum_r[c] += pixels[i].r;
            sum_g[c] += pixels[i].g;
            sum_b[c] += pixels[i].b;
            count[c]++;
        }
        for (size_t i = 0; i < target_colors; i++) {
            if (count[i] > 0) {
                centers[i].r = (unsigned char)(sum_r[i] / count[i]);
                centers[i].g = (unsigned char)(sum_g[i] / count[i]);
                centers[i].b = (unsigned char)(sum_b[i] / count[i]);
            }
        }
        free(count); free(sum_r); free(sum_g); free(sum_b);
    }

    BytePalette* outpal = BytePalette_new(target_colors);
    for (size_t i = 0; i < target_colors; ++i) {
        ByteColor bc;
        bc.r = centers[i].r;
        bc.g = centers[i].g;
        bc.b = centers[i].b;
        bc.a = 255;
        BytePalette_set(outpal, i, &bc);
    }
    free(pixels);
    free(assignments);
    free(centers);
    free(initial_indices);
    return outpal;
}
