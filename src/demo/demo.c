#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include <string.h>
#include <stdint.h>
#include "bmp.h"
#include "libdither.h"


/* A quick demo of ditherlib.
 * This program loads a .bmp file, converts it to linear grayscale, and runs all available ditherers on it,
 * saving the output to individual .bmp files.
 */

void save_and_free_image(char* basename, char* filename, int width, int height, uint8_t* out) {
    // The out buffer is a simple array with the length 'width * height', with one byte-per pixel.
    // If a byte in the buffer is 0 then it's black. 255 is white.
    Bmp* bmp = bmp_rgb24(width, height);
    Pixel p;
    p.r = 255; p.g = 255; p.b = 255;
    for(int y = 0; y < height; y++)
        for (int x = 0; x < width; x++)
            if (out[y * width + x] != 0)
                bmp_setpixel(bmp, x, y, p);
    char fn[2048];
    sprintf(fn, "%s_%s", basename, filename);
    bmp_save(bmp, fn);
    bmp_free(bmp);
    free(out);
}

DitherImage* bmp_to_ditherimage(char *filename, bool correct_gamma) {
    // load a BMP image
    Bmp* bmp = bmp_load(filename);
    if (!bmp)
        return NULL;
    // create an empty DitherImage. DitherImages are in linear color space, storing color values in the 0.0 - 1.0 range
    // DitherImages are the input for all dither functions.
    DitherImage* dither_image = DitherImage_new(bmp->width, bmp->height);
    Pixel p;
    // set pixel in the dither image
    for(int y = 0; y < bmp->height; y++) {
        for (int x = 0; x < bmp->width; x++) {
            // here we read the source pixels from the BMP image. The source pixels are in sRGB color space and the
            // color values range from 0 - 255.
            bmp_getpixel(bmp, x, y, &p);
            // DitherImage_set_pixel converts the color space from sRGB to linear and adjusts the value range from
            // 0 - 255 of the input image to DitherImage's 0.0 - 1.0 range.
            DitherImage_set_pixel(dither_image, x, y, p.r, p.g, p.b, correct_gamma);
        }
    }
    bmp_free(bmp); // free the input BMP image - we don't need it anymore.
    return dither_image;
}

void strip_ext(char* fname) {
    /* strips away a file's extension */
    char *end = fname + strlen(fname);
    while (end > fname && *end != '.')
        --end;
    if (end > fname)
        *end = '\0';
}

int main(int argc, char* argv[]) {
    if(argc == 1) {
        printf("USAGE: demo image.bmp\n");
        return 0;
    }
    char* filename = argv[1];
    char* basename = (char*)calloc(strlen(argv[1]) + 1, sizeof(char));
    strcpy(basename, argv[1]);
    strip_ext(basename);
    DitherImage* dither_image = bmp_to_ditherimage(filename, true);  // load an image as DitherImage
    if(dither_image) {
        uint8_t* out_image = NULL;
        /* All ditherers take the input DitherImage as first parameter, and the output buffer ('out') as last parameter.
         * Ditherers will never modify the DitherImage input.
         * The expected output buffer is a flat buffer with 1-byte-per-pixel and a lenght of image-width * image-height.
         * Black pixels in the buffer have a value of 0 and white pixels have a value of 255. */

        /* Grid Dithering */
        printf("running Grid Ditherer...\n");
        out_image = (uint8_t*)calloc(dither_image->width * dither_image->height, sizeof(uint8_t));
        grid_dither(dither_image, 4, 4, 0, true, out_image);
        save_and_free_image(basename, "grid.bmp", dither_image->width, dither_image->height, out_image);

        /* Dot Diffusion */
        DotClassMatrix* dcm;
        DotDiffusionMatrix* ddm;

        printf("running Dot Diffusion: Knuth...\n");
        out_image = (uint8_t*)calloc(dither_image->width * dither_image->height, sizeof(uint8_t));
        ddm = get_default_diffusion_matrix();
        dcm = get_knuth_class_matrix();
        dot_diffusion_dither(dither_image, ddm, dcm, out_image);
        save_and_free_image(basename, "dd_knuth.bmp", dither_image->width, dither_image->height, out_image);
        DotClassMatrix_free(dcm);
        DotDiffusionMatrix_free(ddm);

        printf("running Dot Diffusion: Mini-Knuth...\n");
        out_image = (uint8_t*)calloc(dither_image->width * dither_image->height, sizeof(uint8_t));
        ddm = get_default_diffusion_matrix();
        dcm = get_mini_knuth_class_matrix();
        dot_diffusion_dither(dither_image, ddm, dcm, out_image);
        save_and_free_image(basename, "dd_mini-knuth.bmp", dither_image->width, dither_image->height, out_image);
        DotClassMatrix_free(dcm);
        DotDiffusionMatrix_free(ddm);

        printf("running Dot Diffusion: Optimized Knuth...\n");
        out_image = (uint8_t*)calloc(dither_image->width * dither_image->height, sizeof(uint8_t));
        ddm = get_default_diffusion_matrix();
        dcm = get_optimized_knuth_class_matrix();
        dot_diffusion_dither(dither_image, ddm, dcm, out_image);
        save_and_free_image(basename, "dd_opt-knuth.bmp", dither_image->width, dither_image->height, out_image);
        DotClassMatrix_free(dcm);
        DotDiffusionMatrix_free(ddm);

        printf("running Dot Diffusion: Mese and Vaidyanathan 8x8...\n");
        out_image = (uint8_t*)calloc(dither_image->width * dither_image->height, sizeof(uint8_t));
        ddm = get_default_diffusion_matrix();
        dcm = get_mese_8x8_class_matrix();
        dot_diffusion_dither(dither_image, ddm, dcm, out_image);
        save_and_free_image(basename, "dd_mese8x8.bmp", dither_image->width, dither_image->height, out_image);
        DotClassMatrix_free(dcm);
        DotDiffusionMatrix_free(ddm);

        printf("running Dot Diffusion: Mese and Vaidyanathan 16x16...\n");
        out_image = (uint8_t*)calloc(dither_image->width * dither_image->height, sizeof(uint8_t));
        ddm = get_default_diffusion_matrix();
        dcm = get_mese_16x16_class_matrix();
        dot_diffusion_dither(dither_image, ddm, dcm, out_image);
        save_and_free_image(basename, "dd_mese16x16.bmp", dither_image->width, dither_image->height, out_image);
        DotClassMatrix_free(dcm);
        DotDiffusionMatrix_free(ddm);

        printf("running Dot Diffusion: Guo Liu 8x8...\n");
        out_image = (uint8_t*)calloc(dither_image->width * dither_image->height, sizeof(uint8_t));
        ddm = get_guoliu8_diffusion_matrix();
        dcm = get_guoliu_8x8_class_matrix();
        dot_diffusion_dither(dither_image, ddm, dcm, out_image);
        save_and_free_image(basename, "dd_guoliu8x8.bmp", dither_image->width, dither_image->height, out_image);
        DotClassMatrix_free(dcm);
        DotDiffusionMatrix_free(ddm);

        printf("running Dot Diffusion: Guo Liu 16x16...\n");
        out_image = (uint8_t*)calloc(dither_image->width * dither_image->height, sizeof(uint8_t));
        ddm = get_guoliu16_diffusion_matrix();
        dcm = get_guoliu_16x16_class_matrix();
        dot_diffusion_dither(dither_image, ddm, dcm, out_image);
        save_and_free_image(basename, "dd_guoliu16x16.bmp", dither_image->width, dither_image->height, out_image);
        DotClassMatrix_free(dcm);
        DotDiffusionMatrix_free(ddm);

        printf("running Dot Diffusion: Spiral...\n");
        out_image = (uint8_t*)calloc(dither_image->width * dither_image->height, sizeof(uint8_t));
        ddm = get_guoliu8_diffusion_matrix();
        dcm = get_spiral_class_matrix();
        dot_diffusion_dither(dither_image, ddm, dcm, out_image);
        save_and_free_image(basename, "dd_spiral.bmp", dither_image->width, dither_image->height, out_image);
        DotClassMatrix_free(dcm);
        DotDiffusionMatrix_free(ddm);

        printf("running Dot Diffusion: Inverted Spiral...\n");
        out_image = (uint8_t*)calloc(dither_image->width * dither_image->height, sizeof(uint8_t));
        ddm = get_guoliu8_diffusion_matrix();
        dcm = get_spiral_inverted_class_matrix();
        dot_diffusion_dither(dither_image, ddm, dcm, out_image);
        save_and_free_image(basename, "dd_inv_spiral.bmp", dither_image->width, dither_image->height, out_image);
        DotClassMatrix_free(dcm);
        DotDiffusionMatrix_free(ddm);

        /* Error Diffusion dithering */
        ErrorDiffusionMatrix* em = NULL;

        printf("running Error Diffusion: Xot...\n");
        out_image = (uint8_t*)calloc(dither_image->width * dither_image->height, sizeof(uint8_t));
        em = get_xot_matrix();
        error_diffusion_dither(dither_image, em, true, 0.0, out_image);
        save_and_free_image(basename, "ed_xot.bmp", dither_image->width, dither_image->height, out_image);
        ErrorDiffusionMatrix_free(em);

        printf("running Error Diffusion: Diagonal Diffusion...\n");
        out_image = (uint8_t*)calloc(dither_image->width * dither_image->height, sizeof(uint8_t));
        em = get_diagonal_matrix();
        error_diffusion_dither(dither_image, em, false, 0.0, out_image);
        save_and_free_image(basename, "ed_diagonal.bmp", dither_image->width, dither_image->height, out_image);
        ErrorDiffusionMatrix_free(em);

        printf("running Error Diffusion: Floyd Steinberg...\n");
        out_image = (uint8_t*)calloc(dither_image->width * dither_image->height, sizeof(uint8_t));
        em = get_floyd_steinberg_matrix();
        error_diffusion_dither(dither_image, em, false, 0.0, out_image);
        save_and_free_image(basename, "ed_floyd-steinberg.bmp", dither_image->width, dither_image->height, out_image);
        ErrorDiffusionMatrix_free(em);

        printf("running Error Diffusion: Shiau Fan 3...\n");
        out_image = (uint8_t*)calloc(dither_image->width * dither_image->height, sizeof(uint8_t));
        em = get_shiaufan3_matrix();
        error_diffusion_dither(dither_image, em, false, 0.0, out_image);
        save_and_free_image(basename, "ed_shiaufan3.bmp", dither_image->width, dither_image->height, out_image);
        ErrorDiffusionMatrix_free(em);

        printf("running Error Diffusion: Shiau Fan 2...\n");
        out_image = (uint8_t*)calloc(dither_image->width * dither_image->height, sizeof(uint8_t));
        em = get_shiaufan2_matrix();
        error_diffusion_dither(dither_image, em, true, 0.0, out_image);
        save_and_free_image(basename, "ed_shiaufan2.bmp", dither_image->width, dither_image->height, out_image);
        ErrorDiffusionMatrix_free(em);

        printf("running Error Diffusion: Shiau Fan 1...\n");
        out_image = (uint8_t*)calloc(dither_image->width * dither_image->height, sizeof(uint8_t));
        em = get_shiaufan1_matrix();
        error_diffusion_dither(dither_image, em, false, 0.0, out_image);
        save_and_free_image(basename, "ed_shiaufan1.bmp", dither_image->width, dither_image->height, out_image);
        ErrorDiffusionMatrix_free(em);

        printf("running Error Diffusion: Stucki...\n");
        out_image = (uint8_t*)calloc(dither_image->width * dither_image->height, sizeof(uint8_t));
        em = get_stucki_matrix();
        error_diffusion_dither(dither_image, em, false, 0.0, out_image);
        save_and_free_image(basename, "ed_stucki.bmp", dither_image->width, dither_image->height, out_image);
        ErrorDiffusionMatrix_free(em);

        printf("running Error Diffusion: 1 Dimensional...\n");
        out_image = (uint8_t*)calloc(dither_image->width * dither_image->height, sizeof(uint8_t));
        em = get_diffusion_1d_matrix();
        error_diffusion_dither(dither_image, em, true, 0.0, out_image);
        save_and_free_image(basename, "ed_1d.bmp", dither_image->width, dither_image->height, out_image);
        ErrorDiffusionMatrix_free(em);

        printf("running Error Diffusion: 2 Dimensional...\n");
        out_image = (uint8_t*)calloc(dither_image->width * dither_image->height, sizeof(uint8_t));
        em = get_diffusion_2d_matrix();
        error_diffusion_dither(dither_image, em, true, 0.0, out_image);
        save_and_free_image(basename, "ed_2d.bmp", dither_image->width, dither_image->height, out_image);
        ErrorDiffusionMatrix_free(em);

        printf("running Error Diffusion: Fake Floyd Steinberg...\n");
        out_image = (uint8_t*)calloc(dither_image->width * dither_image->height, sizeof(uint8_t));
        em = get_fake_floyd_steinberg_matrix();
        error_diffusion_dither(dither_image, em, false, 0.0, out_image);
        save_and_free_image(basename, "ed_fake_floyd_steinberg.bmp", dither_image->width, dither_image->height, out_image);
        ErrorDiffusionMatrix_free(em);

        printf("running Error Diffusion: Jarvis-Judice-Ninke...\n");
        out_image = (uint8_t*)calloc(dither_image->width * dither_image->height, sizeof(uint8_t));
        em = get_jarvis_judice_ninke_matrix();
        error_diffusion_dither(dither_image, em, false, 0.0, out_image);
        save_and_free_image(basename, "ed_jjn.bmp", dither_image->width, dither_image->height, out_image);
        ErrorDiffusionMatrix_free(em);

        printf("running Error Diffusion: Atkinson...\n");
        out_image = (uint8_t*)calloc(dither_image->width * dither_image->height, sizeof(uint8_t));
        em = get_atkinson_matrix();
        error_diffusion_dither(dither_image, em, false, 0.0, out_image);
        save_and_free_image(basename, "ed_atkinson.bmp", dither_image->width, dither_image->height, out_image);
        ErrorDiffusionMatrix_free(em);

        printf("running Error Diffusion: Burkes...\n");
        out_image = (uint8_t*)calloc(dither_image->width * dither_image->height, sizeof(uint8_t));
        em = get_burkes_matrix();
        error_diffusion_dither(dither_image, em, false, 0.0, out_image);
        save_and_free_image(basename, "ed_burkes.bmp", dither_image->width, dither_image->height, out_image);
        ErrorDiffusionMatrix_free(em);

        printf("running Error Diffusion: Sierra 3...\n");
        out_image = (uint8_t*)calloc(dither_image->width * dither_image->height, sizeof(uint8_t));
        em = get_sierra_3_matrix();
        error_diffusion_dither(dither_image, em, false, 0.0, out_image);
        save_and_free_image(basename, "ed_sierra3.bmp", dither_image->width, dither_image->height, out_image);
        ErrorDiffusionMatrix_free(em);

        printf("running Error Diffusion: Sierra 2-Row...\n");
        out_image = (uint8_t*)calloc(dither_image->width * dither_image->height, sizeof(uint8_t));
        em = get_sierra_2row_matrix();
        error_diffusion_dither(dither_image, em, false, 0.0, out_image);
        save_and_free_image(basename, "ed_sierra2row.bmp", dither_image->width, dither_image->height, out_image);
        ErrorDiffusionMatrix_free(em);

        printf("running Error Diffusion: Sierra Lite...\n");
        out_image = (uint8_t*)calloc(dither_image->width * dither_image->height, sizeof(uint8_t));
        em = get_sierra_lite_matrix();
        error_diffusion_dither(dither_image, em, true, 0.0, out_image);
        save_and_free_image(basename, "ed_sierra_lite.bmp", dither_image->width, dither_image->height, out_image);
        ErrorDiffusionMatrix_free(em);

        printf("running Error Diffusion: Steve Pigeon...\n");
        out_image = (uint8_t*)calloc(dither_image->width * dither_image->height, sizeof(uint8_t));
        em = get_steve_pigeon_matrix();
        error_diffusion_dither(dither_image, em, false, 0.0, out_image);
        save_and_free_image(basename, "ed_steve_pigeon.bmp", dither_image->width, dither_image->height, out_image);
        ErrorDiffusionMatrix_free(em);

        printf("running Error Diffusion: Robert Kist...\n");
        out_image = (uint8_t*)calloc(dither_image->width * dither_image->height, sizeof(uint8_t));
        em = get_robert_kist_matrix();
        error_diffusion_dither(dither_image, em, false, 0.0, out_image);
        save_and_free_image(basename, "ed_robert_kist.bmp", dither_image->width, dither_image->height, out_image);
        ErrorDiffusionMatrix_free(em);

        printf("running Error Diffusion: Stevenson-Arce...\n");
        out_image = (uint8_t*)calloc(dither_image->width * dither_image->height, sizeof(uint8_t));
        em = get_stevenson_arce_matrix();
        error_diffusion_dither(dither_image, em, true, 0.0, out_image);
        save_and_free_image(basename, "ed_stevenson_arce.bmp", dither_image->width, dither_image->height, out_image);
        ErrorDiffusionMatrix_free(em);

        /* Ordered Dithering */
        OrderedDitherMatrix* om;

        printf("running Ordered Dithering: Blue Noise...\n");
        out_image = (uint8_t*)calloc(dither_image->width * dither_image->height, sizeof(uint8_t));
        om = get_blue_noise_128x128();
        ordered_dither(dither_image, om, 0.0, out_image);
        save_and_free_image(basename, "od_blue_noise.bmp", dither_image->width, dither_image->height, out_image);
        OrderedDitherMatrix_free(om);

        printf("running Ordered Dithering: Bayer 2x2...\n");
        out_image = (uint8_t*)calloc(dither_image->width * dither_image->height, sizeof(uint8_t));
        om = get_bayer2x2_matrix();
        ordered_dither(dither_image, om, 0.0, out_image);
        save_and_free_image(basename, "od_bayer2x2.bmp", dither_image->width, dither_image->height, out_image);
        OrderedDitherMatrix_free(om);

        printf("running Ordered Dithering: Bayer 3x3...\n");
        out_image = (uint8_t*)calloc(dither_image->width * dither_image->height, sizeof(uint8_t));
        om = get_bayer3x3_matrix();
        ordered_dither(dither_image, om, 0.0, out_image);
        save_and_free_image(basename, "od_bayer3x3.bmp", dither_image->width, dither_image->height, out_image);
        OrderedDitherMatrix_free(om);

        printf("running Ordered Dithering: Bayer 4x4...\n");
        out_image = (uint8_t*)calloc(dither_image->width * dither_image->height, sizeof(uint8_t));
        om = get_bayer4x4_matrix();
        ordered_dither(dither_image, om, 0.0, out_image);
        save_and_free_image(basename, "od_bayer4x4.bmp", dither_image->width, dither_image->height, out_image);
        OrderedDitherMatrix_free(om);

        printf("running Ordered Dithering: Bayer 8x8...\n");
        out_image = (uint8_t*)calloc(dither_image->width * dither_image->height, sizeof(uint8_t));
        om = get_bayer8x8_matrix();
        ordered_dither(dither_image, om, 0.0, out_image);
        save_and_free_image(basename, "od_bayer8x8.bmp", dither_image->width, dither_image->height, out_image);
        OrderedDitherMatrix_free(om);

        printf("running Ordered Dithering: Bayer 16x16...\n");
        out_image = (uint8_t*)calloc(dither_image->width * dither_image->height, sizeof(uint8_t));
        om = get_bayer16x16_matrix();
        ordered_dither(dither_image, om, 0.0, out_image);
        save_and_free_image(basename, "od_bayer16x16.bmp", dither_image->width, dither_image->height, out_image);
        OrderedDitherMatrix_free(om);

        printf("running Ordered Dithering: Bayer 32x32...\n");
        out_image = (uint8_t*)calloc(dither_image->width * dither_image->height, sizeof(uint8_t));
        om = get_bayer32x32_matrix();
        ordered_dither(dither_image, om, 0.0, out_image);
        save_and_free_image(basename, "od_bayer32x32.bmp", dither_image->width, dither_image->height, out_image);
        OrderedDitherMatrix_free(om);

        printf("running Ordered Dithering: Dispersed Dots 1...\n");
        out_image = (uint8_t*)calloc(dither_image->width * dither_image->height, sizeof(uint8_t));
        om = get_dispersed_dots_1_matrix();
        ordered_dither(dither_image, om, 0.0, out_image);
        save_and_free_image(basename, "od_disp_dots1.bmp", dither_image->width, dither_image->height, out_image);
        OrderedDitherMatrix_free(om);

        printf("running Ordered Dithering: Dispersed Dots 2...\n");
        out_image = (uint8_t*)calloc(dither_image->width * dither_image->height, sizeof(uint8_t));
        om = get_dispersed_dots_2_matrix();
        ordered_dither(dither_image, om, 0.0, out_image);
        save_and_free_image(basename, "od_disp_dots2.bmp", dither_image->width, dither_image->height, out_image);
        OrderedDitherMatrix_free(om);

        printf("running Ordered Dithering: Ulichney Void Dispersed Dots...\n");
        out_image = (uint8_t*)calloc(dither_image->width * dither_image->height, sizeof(uint8_t));
        om = get_ulichney_void_dispersed_dots_matrix();
        ordered_dither(dither_image, om, 0.0, out_image);
        save_and_free_image(basename, "od_ulichney_vdd.bmp", dither_image->width, dither_image->height, out_image);
        OrderedDitherMatrix_free(om);

        printf("running Ordered Dithering: Non-Rectangular 1...\n");
        out_image = (uint8_t*)calloc(dither_image->width * dither_image->height, sizeof(uint8_t));
        om = get_non_rectangular_1_matrix();
        ordered_dither(dither_image, om, 0.0, out_image);
        save_and_free_image(basename, "od_non_rect1.bmp", dither_image->width, dither_image->height, out_image);
        OrderedDitherMatrix_free(om);

        printf("running Ordered Dithering: Non-Rectangular 2...\n");
        out_image = (uint8_t*)calloc(dither_image->width * dither_image->height, sizeof(uint8_t));
        om = get_non_rectangular_2_matrix();
        ordered_dither(dither_image, om, 0.0, out_image);
        save_and_free_image(basename, "od_non_rect2.bmp", dither_image->width, dither_image->height, out_image);
        OrderedDitherMatrix_free(om);

        printf("running Ordered Dithering: Non-Rectangular 3...\n");
        out_image = (uint8_t*)calloc(dither_image->width * dither_image->height, sizeof(uint8_t));
        om = get_non_rectangular_3_matrix();
        ordered_dither(dither_image, om, 0.0, out_image);
        save_and_free_image(basename, "od_non_rect3.bmp", dither_image->width, dither_image->height, out_image);
        OrderedDitherMatrix_free(om);

        printf("running Ordered Dithering: Non-Rectangular 4...\n");
        out_image = (uint8_t*)calloc(dither_image->width * dither_image->height, sizeof(uint8_t));
        om = get_non_rectangular_4_matrix();
        ordered_dither(dither_image, om, 0.0, out_image);
        save_and_free_image(basename, "od_non_rect4.bmp", dither_image->width, dither_image->height, out_image);
        OrderedDitherMatrix_free(om);

        printf("running Ordered Dithering: Ulichney Bayer 5x5...\n");
        out_image = (uint8_t*)calloc(dither_image->width * dither_image->height, sizeof(uint8_t));
        om = get_ulichney_bayer_5_matrix();
        ordered_dither(dither_image, om, 0.0, out_image);
        save_and_free_image(basename, "od_ulichney_bayer5.bmp", dither_image->width, dither_image->height, out_image);
        OrderedDitherMatrix_free(om);

        printf("running Ordered Dithering: Ulichney...\n");
        out_image = (uint8_t*)calloc(dither_image->width * dither_image->height, sizeof(uint8_t));
        om = get_ulichney_matrix();
        ordered_dither(dither_image, om, 0.0, out_image);
        save_and_free_image(basename, "od_ulichney.bmp", dither_image->width, dither_image->height, out_image);
        OrderedDitherMatrix_free(om);

        printf("running Ordered Dithering: Clustered Dot 1...\n");
        out_image = (uint8_t*)calloc(dither_image->width * dither_image->height, sizeof(uint8_t));
        om = get_bayer_clustered_dot_1_matrix();
        ordered_dither(dither_image, om, 0.0, out_image);
        save_and_free_image(basename, "od_clustered_dot1.bmp", dither_image->width, dither_image->height, out_image);
        OrderedDitherMatrix_free(om);

        printf("running Ordered Dithering: Clustered Dot 2...\n");
        out_image = (uint8_t*)calloc(dither_image->width * dither_image->height, sizeof(uint8_t));
        om = get_bayer_clustered_dot_2_matrix();
        ordered_dither(dither_image, om, 0.0, out_image);
        save_and_free_image(basename, "od_clustered_dot2.bmp", dither_image->width, dither_image->height, out_image);
        OrderedDitherMatrix_free(om);

        printf("running Ordered Dithering: Clustered Dot 3...\n");
        out_image = (uint8_t*)calloc(dither_image->width * dither_image->height, sizeof(uint8_t));
        om = get_bayer_clustered_dot_3_matrix();
        ordered_dither(dither_image, om, 0.0, out_image);
        save_and_free_image(basename, "od_clustered_dot3.bmp", dither_image->width, dither_image->height, out_image);
        OrderedDitherMatrix_free(om);

        printf("running Ordered Dithering: Clustered Dot 4...\n");
        out_image = (uint8_t*)calloc(dither_image->width * dither_image->height, sizeof(uint8_t));
        om = get_bayer_clustered_dot_4_matrix();
        ordered_dither(dither_image, om, 0.0, out_image);
        save_and_free_image(basename, "od_clustered_dot4.bmp", dither_image->width, dither_image->height, out_image);
        OrderedDitherMatrix_free(om);

        printf("running Ordered Dithering: Clustered Dot 5...\n");
        out_image = (uint8_t*)calloc(dither_image->width * dither_image->height, sizeof(uint8_t));
        om = get_bayer_clustered_dot_5_matrix();
        ordered_dither(dither_image, om, 0.0, out_image);
        save_and_free_image(basename, "od_clustered_dot5.bmp", dither_image->width, dither_image->height, out_image);
        OrderedDitherMatrix_free(om);

        printf("running Ordered Dithering: Clustered Dot 6...\n");
        out_image = (uint8_t*)calloc(dither_image->width * dither_image->height, sizeof(uint8_t));
        om = get_bayer_clustered_dot_6_matrix();
        ordered_dither(dither_image, om, 0.0, out_image);
        save_and_free_image(basename, "od_clustered_dot6.bmp", dither_image->width, dither_image->height, out_image);
        OrderedDitherMatrix_free(om);

        printf("running Ordered Dithering: Clustered Dot 7...\n");
        out_image = (uint8_t*)calloc(dither_image->width * dither_image->height, sizeof(uint8_t));
        om = get_bayer_clustered_dot_7_matrix();
        ordered_dither(dither_image, om, 0.0, out_image);
        save_and_free_image(basename, "od_clustered_dot7.bmp", dither_image->width, dither_image->height, out_image);
        OrderedDitherMatrix_free(om);

        printf("running Ordered Dithering: Clustered Dot 8...\n");
        out_image = (uint8_t*)calloc(dither_image->width * dither_image->height, sizeof(uint8_t));
        om = get_bayer_clustered_dot_8_matrix();
        ordered_dither(dither_image, om, 0.0, out_image);
        save_and_free_image(basename, "od_clustered_dot8.bmp", dither_image->width, dither_image->height, out_image);
        OrderedDitherMatrix_free(om);

        printf("running Ordered Dithering: Clustered Dot 9...\n");
        out_image = (uint8_t*)calloc(dither_image->width * dither_image->height, sizeof(uint8_t));
        om = get_bayer_clustered_dot_9_matrix();
        ordered_dither(dither_image, om, 0.0, out_image);
        save_and_free_image(basename, "od_clustered_dot9.bmp", dither_image->width, dither_image->height, out_image);
        OrderedDitherMatrix_free(om);

        printf("running Ordered Dithering: Clustered Dot 10...\n");
        out_image = (uint8_t*)calloc(dither_image->width * dither_image->height, sizeof(uint8_t));
        om = get_bayer_clustered_dot_10_matrix();
        ordered_dither(dither_image, om, 0.0, out_image);
        save_and_free_image(basename, "od_clustered_dot10.bmp", dither_image->width, dither_image->height, out_image);
        OrderedDitherMatrix_free(om);

        printf("running Ordered Dithering: Clustered Dot 11...\n");
        out_image = (uint8_t*)calloc(dither_image->width * dither_image->height, sizeof(uint8_t));
        om = get_bayer_clustered_dot_11_matrix();
        ordered_dither(dither_image, om, 0.0, out_image);
        save_and_free_image(basename, "od_clustered_dot11.bmp", dither_image->width, dither_image->height, out_image);
        OrderedDitherMatrix_free(om);

        printf("running Ordered Dithering: Central White Point...\n");
        out_image = (uint8_t*)calloc(dither_image->width * dither_image->height, sizeof(uint8_t));
        om = get_central_white_point_matrix();
        ordered_dither(dither_image, om, 0.0, out_image);
        save_and_free_image(basename, "od_ctrl_wp.bmp", dither_image->width, dither_image->height, out_image);
        OrderedDitherMatrix_free(om);

        printf("running Ordered Dithering: Balanced Central White Point...\n");
        out_image = (uint8_t*)calloc(dither_image->width * dither_image->height, sizeof(uint8_t));
        om = get_balanced_centered_point_matrix();
        ordered_dither(dither_image, om, 0.0, out_image);
        save_and_free_image(basename, "od_balanced_ctrl_wp.bmp", dither_image->width, dither_image->height, out_image);
        OrderedDitherMatrix_free(om);

        printf("running Ordered Dithering: Diagonal Ordered...\n");
        out_image = (uint8_t*)calloc(dither_image->width * dither_image->height, sizeof(uint8_t));
        om = get_diagonal_ordered_matrix_matrix();
        ordered_dither(dither_image, om, 0.0, out_image);
        save_and_free_image(basename, "od_diag_ordered.bmp", dither_image->width, dither_image->height, out_image);
        OrderedDitherMatrix_free(om);

        printf("running Ordered Dithering: Ulichney Clustered Dot...\n");
        out_image = (uint8_t*)calloc(dither_image->width * dither_image->height, sizeof(uint8_t));
        om = get_ulichney_clustered_dot_matrix();
        ordered_dither(dither_image, om, 0.0, out_image);
        save_and_free_image(basename, "od_ulichney_clust_dot.bmp", dither_image->width, dither_image->height, out_image);
        OrderedDitherMatrix_free(om);

        printf("running Ordered Dithering: ImageMagick 5x5 Circle...\n");
        out_image = (uint8_t*)calloc(dither_image->width * dither_image->height, sizeof(uint8_t));
        om = get_magic5x5_circle_matrix();
        ordered_dither(dither_image, om, 0.0, out_image);
        save_and_free_image(basename, "od_magic5x5_circle.bmp", dither_image->width, dither_image->height, out_image);
        OrderedDitherMatrix_free(om);

        printf("running Ordered Dithering: ImageMagick 6x6 Circle...\n");
        out_image = (uint8_t*)calloc(dither_image->width * dither_image->height, sizeof(uint8_t));
        om = get_magic6x6_circle_matrix();
        ordered_dither(dither_image, om, 0.0, out_image);
        save_and_free_image(basename, "od_magic6x6_circle.bmp", dither_image->width, dither_image->height, out_image);
        OrderedDitherMatrix_free(om);

        printf("running Ordered Dithering: ImageMagick 7x7 Circle...\n");
        out_image = (uint8_t*)calloc(dither_image->width * dither_image->height, sizeof(uint8_t));
        om = get_magic7x7_circle_matrix();
        ordered_dither(dither_image, om, 0.0, out_image);
        save_and_free_image(basename, "od_magic7x7_circle.bmp", dither_image->width, dither_image->height, out_image);
        OrderedDitherMatrix_free(om);

        printf("running Ordered Dithering: ImageMagick 4x4 45-degrees...\n");
        out_image = (uint8_t*)calloc(dither_image->width * dither_image->height, sizeof(uint8_t));
        om = get_magic4x4_45_matrix();
        ordered_dither(dither_image, om, 0.0, out_image);
        save_and_free_image(basename, "od_magic4x4_45.bmp", dither_image->width, dither_image->height, out_image);
        OrderedDitherMatrix_free(om);

        printf("running Ordered Dithering: ImageMagick 6x6 45-degrees...\n");
        out_image = (uint8_t*)calloc(dither_image->width * dither_image->height, sizeof(uint8_t));
        om = get_magic6x6_45_matrix();
        ordered_dither(dither_image, om, 0.0, out_image);
        save_and_free_image(basename, "od_magic6x6_45.bmp", dither_image->width, dither_image->height, out_image);
        OrderedDitherMatrix_free(om);

        printf("running Ordered Dithering: ImageMagick 8x8 45-degrees...\n");
        out_image = (uint8_t*)calloc(dither_image->width * dither_image->height, sizeof(uint8_t));
        om = get_magic8x8_45_matrix();
        ordered_dither(dither_image, om, 0.0, out_image);
        save_and_free_image(basename, "od_magic8x8_45.bmp", dither_image->width, dither_image->height, out_image);
        OrderedDitherMatrix_free(om);

        printf("running Ordered Dithering: ImageMagick 4x4...\n");
        out_image = (uint8_t*)calloc(dither_image->width * dither_image->height, sizeof(uint8_t));
        om = get_magic4x4_matrix();
        ordered_dither(dither_image, om, 0.0, out_image);
        save_and_free_image(basename, "od_magic4x4.bmp", dither_image->width, dither_image->height, out_image);
        OrderedDitherMatrix_free(om);

        printf("running Ordered Dithering: ImageMagick 6x6...\n");
        out_image = (uint8_t*)calloc(dither_image->width * dither_image->height, sizeof(uint8_t));
        om = get_magic6x6_matrix();
        ordered_dither(dither_image, om, 0.0, out_image);
        save_and_free_image(basename, "od_magic6x6.bmp", dither_image->width, dither_image->height, out_image);
        OrderedDitherMatrix_free(om);

        printf("running Ordered Dithering: ImageMagick 8x8...\n");
        out_image = (uint8_t*)calloc(dither_image->width * dither_image->height, sizeof(uint8_t));
        om = get_magic8x8_matrix();
        ordered_dither(dither_image, om, 0.0, out_image);
        save_and_free_image(basename, "od_magic8x8.bmp", dither_image->width, dither_image->height, out_image);
        OrderedDitherMatrix_free(om);

        printf("running Ordered Dithering: Variable 2x2 Matix...\n");
        out_image = (uint8_t*)calloc(dither_image->width * dither_image->height, sizeof(uint8_t));
        om = get_variable_2x2_matrix(55);
        ordered_dither(dither_image, om, 0.0, out_image);
        save_and_free_image(basename, "od_variable2x2.bmp", dither_image->width, dither_image->height, out_image);
        OrderedDitherMatrix_free(om);

        printf("running Ordered Dithering: Variable 4x4 Matix...\n");
        out_image = (uint8_t*)calloc(dither_image->width * dither_image->height, sizeof(uint8_t));
        om = get_variable_4x4_matrix(14);
        ordered_dither(dither_image, om, 0.0, out_image);
        save_and_free_image(basename, "od_variable4x4.bmp", dither_image->width, dither_image->height, out_image);
        OrderedDitherMatrix_free(om);

        printf("running Ordered Dithering: Interleaved Gradient Noise...\n");
        out_image = (uint8_t*)calloc(dither_image->width * dither_image->height, sizeof(uint8_t));
        om = get_interleaved_gradient_noise(4, 52.9829189, 0.06711056, 0.00583715);
        ordered_dither(dither_image, om, 0.0, out_image);
        save_and_free_image(basename, "od_interleaved_gradient.bmp", dither_image->width, dither_image->height, out_image);
        OrderedDitherMatrix_free(om);

        printf("running Ordered Dithering: Blue Noise image based...\n");
        DitherImage* matrix_image = bmp_to_ditherimage("blue_noise.bmp", false);
        out_image = (uint8_t*)calloc(dither_image->width * dither_image->height, sizeof(uint8_t));
        om = get_matrix_from_image(matrix_image);
        ordered_dither(dither_image, om, 0.0, out_image);
        save_and_free_image(basename, "od_blue_noise_image.bmp", dither_image->width, dither_image->height, out_image);
        OrderedDitherMatrix_free(om);
        DitherImage_free(matrix_image);

        /* Dot-Diffusion Dithering */
        DotClassMatrix* cm;
        DotDiffusionMatrix* dm;

        /* Variable Error Diffusion Dithering */
        printf("running Variable Error Diffusion: Ostromoukhov...\n");
        out_image = (uint8_t*)calloc(dither_image->width * dither_image->height, sizeof(uint8_t));
        variable_error_diffusion_dither(dither_image, Ostromoukhov, true, out_image);
        save_and_free_image(basename, "ved_ostromoukhov.bmp", dither_image->width, dither_image->height, out_image);

        printf("running Variable Error Diffusion: Zhou Fang...\n");
        out_image = (uint8_t*)calloc(dither_image->width * dither_image->height, sizeof(uint8_t));
        variable_error_diffusion_dither(dither_image, Zhoufang, true, out_image);
        save_and_free_image(basename, "ved_zhoufang.bmp", dither_image->width, dither_image->height, out_image);

        /* Thresholding */
        printf("running Thresholding...\n");
        out_image = (uint8_t*)calloc(dither_image->width * dither_image->height, sizeof(uint8_t));
        double threshold = auto_threshold(dither_image);  // determine optimal threshold value
        threshold_dither(dither_image, threshold, 0.55, out_image);
        save_and_free_image(basename, "threshold.bmp", dither_image->width, dither_image->height, out_image);

        /* Direct Binary Search (DBS) Dithering */
        for (int i = 0; i < 7; i++) {
            char filename[2048];
            printf("running Direct Binary Search (DBS): formula %i...\n", i);
            out_image = (uint8_t *) calloc(dither_image->width * dither_image->height, sizeof(uint8_t));
            dbs_dither(dither_image, i, out_image);
            sprintf(filename, "dbs%i.bmp", i);
            save_and_free_image(basename, filename, dither_image->width, dither_image->height, out_image);
        }

        /* Kacker and Allebach dithering */
        printf("running Kacker and Allebach dithering...\n");
        out_image = (uint8_t*)calloc(dither_image->width * dither_image->height, sizeof(uint8_t));
        kallebach_dither(dither_image, true, out_image);
        save_and_free_image(basename, "kallebach.bmp", dither_image->width, dither_image->height, out_image);

        /* Riemersma Dithering */
        RiemersmaCurve* rc;

        for (int i = 0; i <= 1; i++) {
            char filename[2048];
            char* namepart = i == 0 ? "rim_mod" : "rim";
            char* descpart = i == 0 ? "Modified " : "";

            printf("running %sRiemersma dithering: Hilbert curve...\n", descpart);
            out_image = (uint8_t *) calloc(dither_image->width * dither_image->height, sizeof(uint8_t));
            rc = get_hilbert_curve();
            riemersma_dither(dither_image, rc, (bool) i, out_image);
            sprintf(filename, "%s_hilbert.bmp", namepart);
            save_and_free_image(basename, filename, dither_image->width, dither_image->height, out_image);
            RiemersmaCurve_free(rc);

            printf("running %sRiemersma dithering: modified Hilbert curve...\n", descpart);
            out_image = (uint8_t *) calloc(dither_image->width * dither_image->height, sizeof(uint8_t));
            rc = get_hilbert_mod_curve();
            riemersma_dither(dither_image, rc, (bool) i, out_image);
            sprintf(filename, "%s_hilbert_mod.bmp", namepart);
            save_and_free_image(basename, filename, dither_image->width, dither_image->height, out_image);
            RiemersmaCurve_free(rc);

            printf("running %sRiemersma dithering: Peano curve...\n", descpart);
            out_image = (uint8_t *) calloc(dither_image->width * dither_image->height, sizeof(uint8_t));
            rc = get_peano_curve();
            riemersma_dither(dither_image, rc, (bool) i, out_image);
            sprintf(filename, "%s_peano.bmp", namepart);
            save_and_free_image(basename, filename, dither_image->width, dither_image->height, out_image);
            RiemersmaCurve_free(rc);

            printf("running %sRiemersma dithering: Fass-0 curve...\n", descpart);
            out_image = (uint8_t *) calloc(dither_image->width * dither_image->height, sizeof(uint8_t));
            rc = get_fass0_curve();
            riemersma_dither(dither_image, rc, (bool) i, out_image);
            sprintf(filename, "%s_fass0.bmp", namepart);
            save_and_free_image(basename, filename, dither_image->width, dither_image->height, out_image);
            RiemersmaCurve_free(rc);

            printf("running %sRiemersma dithering: Fass-1 curve...\n", descpart);
            out_image = (uint8_t *) calloc(dither_image->width * dither_image->height, sizeof(uint8_t));
            rc = get_fass1_curve();
            riemersma_dither(dither_image, rc, (bool) i, out_image);
            sprintf(filename, "%s_fass1.bmp", namepart);
            save_and_free_image(basename, filename, dither_image->width, dither_image->height, out_image);
            RiemersmaCurve_free(rc);

            printf("running %sRiemersma dithering: Fass-2 curve...\n", descpart);
            out_image = (uint8_t *) calloc(dither_image->width * dither_image->height, sizeof(uint8_t));
            rc = get_fass2_curve();
            riemersma_dither(dither_image, rc, (bool) i, out_image);
            sprintf(filename, "%s_fass2.bmp", namepart);
            save_and_free_image(basename, filename, dither_image->width, dither_image->height, out_image);
            RiemersmaCurve_free(rc);

            printf("running %sRiemersma dithering: Gosper curve...\n", descpart);
            out_image = (uint8_t *) calloc(dither_image->width * dither_image->height, sizeof(uint8_t));
            rc = get_gosper_curve();
            riemersma_dither(dither_image, rc, (bool) i, out_image);
            sprintf(filename, "%s_gosper.bmp", namepart);
            save_and_free_image(basename, filename, dither_image->width, dither_image->height, out_image);
            RiemersmaCurve_free(rc);

            printf("running %sRiemersma dithering: Fass Spiral...\n", descpart);
            out_image = (uint8_t *) calloc(dither_image->width * dither_image->height, sizeof(uint8_t));
            rc = get_fass_spiral_curve();
            riemersma_dither(dither_image, rc, (bool) i, out_image);
            sprintf(filename, "%s_fass_spiral.bmp", namepart);
            save_and_free_image(basename, filename, dither_image->width, dither_image->height, out_image);
            RiemersmaCurve_free(rc);
        }

        /* Pattern dithering */
        TilePattern* tp;

        printf("running Pattern dithering: 2x2 pattern...\n");
        out_image = (uint8_t*)calloc(dither_image->width * dither_image->height, sizeof(uint8_t));
        tp = get_2x2_pattern();
        pattern_dither(dither_image, tp, out_image);
        save_and_free_image(basename, "pattern2x2.bmp", dither_image->width, dither_image->height, out_image);
        TilePattern_free(tp);

        printf("running Pattern dithering: 3x3 pattern v1...\n");
        out_image = (uint8_t*)calloc(dither_image->width * dither_image->height, sizeof(uint8_t));
        tp = get_3x3_v1_pattern();
        pattern_dither(dither_image, tp, out_image);
        save_and_free_image(basename, "pattern3x3_v1.bmp", dither_image->width, dither_image->height, out_image);
        TilePattern_free(tp);

        printf("running Pattern dithering: 3x3 pattern v2...\n");
        out_image = (uint8_t*)calloc(dither_image->width * dither_image->height, sizeof(uint8_t));
        tp = get_3x3_v2_pattern();
        pattern_dither(dither_image, tp, out_image);
        save_and_free_image(basename, "pattern3x3_v2.bmp", dither_image->width, dither_image->height, out_image);
        TilePattern_free(tp);

        printf("running Pattern dithering: 3x3 pattern v3...\n");
        out_image = (uint8_t*)calloc(dither_image->width * dither_image->height, sizeof(uint8_t));
        tp = get_3x3_v3_pattern();
        pattern_dither(dither_image, tp, out_image);
        save_and_free_image(basename, "pattern3x3_v3.bmp", dither_image->width, dither_image->height, out_image);
        TilePattern_free(tp);

        printf("running Pattern dithering: 4x4 pattern...\n");
        out_image = (uint8_t*)calloc(dither_image->width * dither_image->height, sizeof(uint8_t));
        tp = get_4x4_pattern();
        pattern_dither(dither_image, tp, out_image);
        save_and_free_image(basename, "pattern4x4.bmp", dither_image->width, dither_image->height, out_image);
        TilePattern_free(tp);

        printf("running Pattern dithering: 5x2 pattern...\n");
        out_image = (uint8_t*)calloc(dither_image->width * dither_image->height, sizeof(uint8_t));
        tp = get_5x2_pattern();
        pattern_dither(dither_image, tp, out_image);
        save_and_free_image(basename, "pattern5x2.bmp", dither_image->width, dither_image->height, out_image);
        TilePattern_free(tp);

        /* Dot Lippens */
        DotLippensCoefficients* coe;

        printf("running Lippens and Philips: v1...\n");
        out_image = (uint8_t*)calloc(dither_image->width * dither_image->height, sizeof(uint8_t));
        cm = get_dotlippens_class_matrix();
        coe = get_dotlippens_coefficients1();
        dotlippens_dither(dither_image, cm, coe, out_image);
        save_and_free_image(basename, "dlippens1.bmp", dither_image->width, dither_image->height, out_image);
        DotLippensCoefficients_free(coe);
        DotClassMatrix_free(cm);

        printf("running Lippens and Philips: v2...\n");
        out_image = (uint8_t*)calloc(dither_image->width * dither_image->height, sizeof(uint8_t));
        cm = get_dotlippens_class_matrix();
        coe = get_dotlippens_coefficients2();
        dotlippens_dither(dither_image, cm, coe, out_image);
        save_and_free_image(basename, "dlippens2.bmp", dither_image->width, dither_image->height, out_image);
        DotLippensCoefficients_free(coe);
        DotClassMatrix_free(cm);

        printf("running Lippens and Philips: v3...\n");
        out_image = (uint8_t*)calloc(dither_image->width * dither_image->height, sizeof(uint8_t));
        cm = get_dotlippens_class_matrix();
        coe = get_dotlippens_coefficients3();
        dotlippens_dither(dither_image, cm, coe, out_image);
        save_and_free_image(basename, "dlippens3.bmp", dither_image->width, dither_image->height, out_image);
        DotLippensCoefficients_free(coe);
        DotClassMatrix_free(cm);

        printf("running Lippens and Philips: Guo Liu 16x16...\n");
        out_image = (uint8_t*)calloc(dither_image->width * dither_image->height, sizeof(uint8_t));
        cm = get_guoliu_16x16_class_matrix();
        coe = get_dotlippens_coefficients1();
        dotlippens_dither(dither_image, cm, coe, out_image);
        save_and_free_image(basename, "dlippens_guoliu16.bmp", dither_image->width, dither_image->height, out_image);
        DotLippensCoefficients_free(coe);
        DotClassMatrix_free(cm);

        printf("running Lippens and Philips: Mese and Vaidyanathan 16x16...\n");
        out_image = (uint8_t*)calloc(dither_image->width * dither_image->height, sizeof(uint8_t));
        cm = get_mese_16x16_class_matrix();
        coe = get_dotlippens_coefficients1();
        dotlippens_dither(dither_image, cm, coe, out_image);
        save_and_free_image(basename, "dlippens_mese16.bmp", dither_image->width, dither_image->height, out_image);
        DotLippensCoefficients_free(coe);
        DotClassMatrix_free(cm);

        printf("running Lippens and Philips: Knuth...\n");
        out_image = (uint8_t*)calloc(dither_image->width * dither_image->height, sizeof(uint8_t));
        cm = get_knuth_class_matrix();
        coe = get_dotlippens_coefficients1();
        dotlippens_dither(dither_image, cm, coe, out_image);
        save_and_free_image(basename, "dlippens_knuth.bmp", dither_image->width, dither_image->height, out_image);
        DotLippensCoefficients_free(coe);
        DotClassMatrix_free(cm);

        DitherImage_free(dither_image);
    }
    return 0;
}
