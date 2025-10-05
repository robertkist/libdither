#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include <string.h>
#include <stdint.h>
#include <math.h>

#include "lodepng.h"
#include "libdither.h"
#include "palettes.h"

static const int PNG_WIDTH = 4; // we assume the PNG is RGBA
static const bool DEBUG = true;

ColorImage* bmp_to_colorimage(char *filename) {
    /* LOAD PNG using lodepng */
    // TODO handle what happens if png isn't RGBA
    unsigned lodepng_error;
    unsigned char* png = 0;
    unsigned width, height;
    lodepng_error = lodepng_decode32_file(&png, &width, &height, filename);
    if(lodepng_error) {
        printf("error %u: %s\n", lodepng_error, lodepng_error_text(lodepng_error));
        return NULL;
    }
    // CREATE COLOR IMAGE
    ColorImage* image = ColorImage_new(width, height);
    size_t size = width * height;
    size_t a = 0;
    for (size_t i = 0; i < size; ++i, a += PNG_WIDTH) {
        ColorImage_set_rgb(image, i, png[a], png[a + 1], png[a + 2], png[a + 3]);
    }
    free(png);
    return image;
}

DitherImage* bmp_to_monoimage(char *filename) {
    /* LOAD PNG using lodepng */
    // TODO handle what happens if png isn't RGBA
    unsigned lodepng_error;
    unsigned char* png = 0;
    unsigned width, height;
    lodepng_error = lodepng_decode32_file(&png, &width, &height, filename);
    if(lodepng_error) {
        printf("error %u: %s\n", lodepng_error, lodepng_error_text(lodepng_error));
        return NULL;
    }
    // create mono image
    DitherImage* image = DitherImage_new(width, height);
    size_t size = width * height;
    size_t a = 0;
    for(size_t y = 0; y < height; y++) {
        for (size_t x = 0; x < width; x++) {
            DitherImage_set_pixel_rgba(image, (int)x, (int)y, png[a], png[a + 1], png[a + 2], png[a + 3], true);
            a += PNG_WIDTH;
        }
    }
    free(png);
    return image;
}

void save_mono_png(uint8_t* out, int width, int height, char* filename) {
    unsigned error;
    unsigned char* png = (unsigned char*)calloc(1, width * height * 4);
    size_t a = 0;
    size_t size = width * height;
    for (size_t i = 0; i < size; i++, a += PNG_WIDTH) {
        png[a] = png[a + 1] = png[a + 2] = out[i] != 0 ? 255 : 0;
        png[a + 3] = out[i] == 128 ? 0 : 255;
    }
    error = lodepng_encode32_file(filename, png, width, height);
    if(error) printf("error %u: %s\n", error, lodepng_error_text(error));
    free(png);
}

void save_png(int* out, int width, int height, BytePalette* pal, char* filename) {
    unsigned error;
    unsigned char* png = (unsigned char*)calloc(1, width * height * 4);
    size_t a = 0;
    size_t size = width * height;
    for (size_t i = 0; i < size; i++, a += PNG_WIDTH) {
        if (out[i] != -1) {
            ByteColor *bc = BytePalette_get(pal, out[i]);
            png[a] = bc->r;
            png[a + 1] = bc->g;
            png[a + 2] = bc->b;
            png[a + 3] = 255;
        } else {
            png[a] = png[a + 1] = png[a + 2] = png[a + 3] = 0;
        }
    }
    error = lodepng_encode32_file(filename, png, width, height);
    if(error) printf("error %u: %s\n", error, lodepng_error_text(error));
    free(png);
}

BytePalette* get_premade_palette() {
    /* returns the EGA palette (for testing) */
    int rgb_channels = 4;
    int pal_len = (size_t)(sizeof(palette_ega) / sizeof(palette_ega[0]) / rgb_channels);
    BytePalette* pal = BytePalette_new(pal_len);
    for (size_t i = 0; i < pal_len; i++) {
        ByteColor ic;
        ic.r = palette_ega[i * 4];
        ic.g = palette_ega[i * 4 + 1];
        ic.b = palette_ega[i * 4 + 2];
        BytePalette_set(pal, i, &ic);
    }
    return pal;
}

int main(int argc, char* argv[]) {
    int colors = 32;  // number of colors for reduced palette
    bool mono_dithering = false; // use mono dithering
    bool ordered_dithering = false; // use ordered dithering (otherwise error diffusion for color)
    bool pal_from_image = true; // reduce original image palette (otherwise use EGA palette)
    enum ColorComparisonMode comparison_mode = LAB2000; // lookup color space for color dithering

    if (mono_dithering) {
        DitherImage *image = bmp_to_monoimage("david.png");
        uint8_t* out = (uint8_t*)calloc(image->width * image->height, sizeof(uint8_t));
        // error diffusion
//        ErrorDiffusionMatrix* em = get_floyd_steinberg_matrix();
//        error_diffusion_dither(image, em, false, 0.0, out);
//        ErrorDiffusionMatrix_free(em);
        // ordered
//        OrderedDitherMatrix* om = get_bayer8x8_matrix();
//        ordered_dither(image, om, 0.0, out);
//        OrderedDitherMatrix_free(om);
        // grid
//        grid_dither(image, 4, 4, 0, false, out);
        // dot
//        DotDiffusionMatrix* ddm = get_default_diffusion_matrix();
//        DotClassMatrix* dcm = get_knuth_class_matrix();
//        dot_diffusion_dither(image, ddm, dcm, out);
//        DotClassMatrix_free(dcm);
//        DotDiffusionMatrix_free(ddm);
        // variable error diffusion
//        variable_error_diffusion_dither(image, Ostromoukhov, true, out);
        // thresholding
//        double threshold = auto_threshold(image);  // determine optimal threshold value
//        threshold_dither(image, threshold, 0.55, out);
        // dbs
//        dbs_dither(image, 0, out);
        // allebach
//        kallebach_dither(image, true, out);
        // riemersma
//        RiemersmaCurve* rc = get_hilbert_curve();
//        riemersma_dither(image, rc, (bool) 0, out);
//        RiemersmaCurve_free(rc);
        // pattern
//        TilePattern* tp = get_2x2_pattern();
//        pattern_dither(image, tp, out);
//        TilePattern_free(tp);
        // dot lippens
        DotClassMatrix* cm = get_dotlippens_class_matrix();
        DotLippensCoefficients* coe = get_dotlippens_coefficients1();
        dotlippens_dither(image, cm, coe, out);
        DotLippensCoefficients_free(coe);
        DotClassMatrix_free(cm);
        // save mono dithered image
        save_mono_png(out, image->width, image->height, "david_OUT.png");
        DitherImage_free(image);
    } else {
        ColorImage *image = bmp_to_colorimage("david.png");
        CachedPalette *palette = CachedPalette_new();
        // create reduced palette
        if (pal_from_image) {  // quantify palette from image
            CachedPalette_from_image(palette, image, colors, MEDIAN_CUT, true, false, false, false);
        } else {
            BytePalette *premade_palette = get_premade_palette();
            CachedPalette_from_BytePalette(palette, premade_palette);
            BytePalette_free(premade_palette);
        }
        CachedPalette_update_cache(palette, comparison_mode, NULL);
        CachedPalette_set_shift(palette, 1, 1, 1);
        int *out = (int *) calloc(image->width * image->height, sizeof(int)); // holds indices to final colors
        if (ordered_dithering) {  // ORDERED DITHERING
            OrderedDitherMatrix *matrix = get_bayer16x16_matrix();
            ordered_dither_color(image, palette, matrix, out);
        } else {  // ERROR DIFFUSION
            ErrorDiffusionMatrix *matrix = get_stucki_matrix();
            error_diffusion_dither_color(image, matrix, palette, false, out);
        }
        // SAVE PNG
        save_png(out, image->width, image->height, palette->target_palette, "david_OUT.png");
        // FREE STUFF
        CachedPalette_free(palette);
        ColorImage_free(image);
    }
    printf("success\n");
    return 0;
}
