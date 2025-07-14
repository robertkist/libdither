#pragma once
#ifndef DITHERLIB_H
#define DITHERLIB_H

#ifdef __cplusplus
extern "C"{
#endif

#ifdef _WIN32
#  ifndef __MINGW32__
#    ifdef MODULE_API_EXPORTS
#      define MODULE_API __declspec(dllexport)
#    else
#      define MODULE_API __declspec(dllimport)
#    endif
#  else
#    define MODULE_API
#  endif
#else
#  define MODULE_API
#endif

#include <stdbool.h>
#include <stdint.h>
#include "matrices.h"

#include "color_colorimage.h"
#include "color_cachedpalette.h"
#include "color_floatpalette.h"
#include "color_bytepalette.h"
#include "ditherimage.h"

// default LAB weights (Munsell color system)
#define LAB_W_HUE 0.91      // hue = color
#define LAB_W_VALUE 0.96 // value = darkness/lightness
#define LAB_W_CHROMA 0.84   // chroma = greyness; less weight prevents that some bright colors become too grey

/* returns the version number of this library */
MODULE_API const char* libdither_version(void);
/* sRGB to linear color space conversion */
MODULE_API double gamma_decode(double c);
/* linear color to sRGB space conversion */
MODULE_API double gamma_encode(double c);

/* ************************************************* */
/* **** DITHERIMAGE - INPUT IMAGE FOR MONO DITHERERS **** */
/* ************************************************* */

/* create a new DitherImage */
MODULE_API DitherImage* DitherImage_new(int width, int height);
/* frees the DitherImage's memory */
MODULE_API void DitherImage_free(DitherImage* self);
/* Sets a pixel; r, g and b are sRGB color values in the range 0 - 255 */
MODULE_API void DitherImage_set_pixel_rgba(DitherImage* self, int x, int y, int r, int g, int b, int a, bool correct_gamma);
MODULE_API void DitherImage_set_pixel(DitherImage* self, int x, int y, int r, int g, int b, bool correct_gamma);
/* Returns a pixel. Returned pixels are in linear color space in the value range 0.0 - 1.0 */
MODULE_API double DitherImage_get_pixel(DitherImage* self, int x, int y);
MODULE_API uint8_t DitherImage_get_transparency(DitherImage* self, int x, int y);

/* ********************************************* */
/* **** BOSCH HERMAN INSPIRED GRID DITHERER **** */
/* ********************************************* */

/* Uses grid dither algorithm to dither an image.
 * w: grid width
 * h: grid height
 * min_pixels: minimum amount of pixels to set to black in each grid. Must be between 0 and width * height.
 *             for best results it is recommended to have this number at most at (width * height / 2)
 * algorithm: when true uses a modified algorithm that yields contrast that is more true to the input image */
MODULE_API void grid_dither(const DitherImage* img, int w, int h, int min_pixels, bool alt_algorithm, uint8_t* out);

/* ********************************** */
/* **** ERROR DIFFUSION DITHERER **** */
/* ********************************** */

/* data-structure for holding an error diffusion matrix */
typedef struct Private_GenericDitherMatrix ErrorDiffusionMatrix;
/* creates a new error diffusion matrix */
MODULE_API ErrorDiffusionMatrix* ErrorDiffusionMatrix_new(int width, int height, double divisor, const int* matrix);
/* frees the error diffusion matrix's memory */
MODULE_API void ErrorDiffusionMatrix_free(ErrorDiffusionMatrix* self);
/* Uses the error diffusion dither algorithm to dither an image.
 * m: an error diffusion matrix structure. This library contains many built-in matrices
 * serpentine: if the image should be traversed from top to bottom in a serpentine (left-to-right, right-to-left, etc.) manner
 * sigma: introduces jitter to the dither output to make it appear less regular. Recommended range: 0.0 - 1.0 */
MODULE_API void error_diffusion_dither(const DitherImage* img, const ErrorDiffusionMatrix* m, bool serpentine, double sigma, uint8_t* out);
/* below functions return different error diffusion matrices which can be used as input for 'error_diffusion_dither' */
MODULE_API ErrorDiffusionMatrix* get_xot_matrix(void);
MODULE_API ErrorDiffusionMatrix* get_diagonal_matrix(void);
MODULE_API ErrorDiffusionMatrix* get_floyd_steinberg_matrix(void);
MODULE_API ErrorDiffusionMatrix* get_shiaufan3_matrix(void);
MODULE_API ErrorDiffusionMatrix* get_shiaufan2_matrix(void);
MODULE_API ErrorDiffusionMatrix* get_shiaufan1_matrix(void);
MODULE_API ErrorDiffusionMatrix* get_stucki_matrix(void);
MODULE_API ErrorDiffusionMatrix* get_diffusion_1d_matrix(void);
MODULE_API ErrorDiffusionMatrix* get_diffusion_2d_matrix(void);
MODULE_API ErrorDiffusionMatrix* get_fake_floyd_steinberg_matrix(void);
MODULE_API ErrorDiffusionMatrix* get_jarvis_judice_ninke_matrix(void);
MODULE_API ErrorDiffusionMatrix* get_atkinson_matrix(void);
MODULE_API ErrorDiffusionMatrix* get_burkes_matrix(void);
MODULE_API ErrorDiffusionMatrix* get_sierra_3_matrix(void);
MODULE_API ErrorDiffusionMatrix* get_sierra_2row_matrix(void);
MODULE_API ErrorDiffusionMatrix* get_sierra_lite_matrix(void);
MODULE_API ErrorDiffusionMatrix* get_steve_pigeon_matrix(void);
MODULE_API ErrorDiffusionMatrix* get_robert_kist_matrix(void);
MODULE_API ErrorDiffusionMatrix* get_stevenson_arce_matrix(void);

/* ************************** */
/* **** ORDERED DITHERER **** */
/* ************************** */

/* data-structure for holding an ordered dithering matrix */
typedef struct Private_GenericDitherMatrix OrderedDitherMatrix;
/* creates a new ordered dithering matrix */
MODULE_API OrderedDitherMatrix* OrderedDitherMatrix_new(int width, int height, double divisor, const int* matrix);
/* frees the ordered dithering matrix's memory */
MODULE_API void OrderedDitherMatrix_free(OrderedDitherMatrix *self);
/* Uses the ordered dither algorithm to dither an image.
 * matrix: an OrderedDitherMatrix which determines how the image will be dithered
 * sigma: introduces jitter to the dither output to make it appear less regular. Recommended range 0.0 - 0.2 */
MODULE_API void ordered_dither(const DitherImage* img, const OrderedDitherMatrix* matrix, double sigma, uint8_t* out);
/* below functions return different ordered dither matrices which can be used as input for 'ordered_dither' */
MODULE_API OrderedDitherMatrix* get_blue_noise_128x128(void);
MODULE_API OrderedDitherMatrix* get_bayer2x2_matrix(void);
MODULE_API OrderedDitherMatrix* get_bayer3x3_matrix(void);
MODULE_API OrderedDitherMatrix* get_bayer4x4_matrix(void);
MODULE_API OrderedDitherMatrix* get_bayer8x8_matrix(void);
MODULE_API OrderedDitherMatrix* get_bayer16x16_matrix(void);
MODULE_API OrderedDitherMatrix* get_bayer32x32_matrix(void);
MODULE_API OrderedDitherMatrix* get_dispersed_dots_1_matrix(void);
MODULE_API OrderedDitherMatrix* get_dispersed_dots_2_matrix(void);
MODULE_API OrderedDitherMatrix* get_ulichney_void_dispersed_dots_matrix(void);
MODULE_API OrderedDitherMatrix* get_non_rectangular_1_matrix(void);
MODULE_API OrderedDitherMatrix* get_non_rectangular_2_matrix(void);
MODULE_API OrderedDitherMatrix* get_non_rectangular_3_matrix(void);
MODULE_API OrderedDitherMatrix* get_non_rectangular_4_matrix(void);
MODULE_API OrderedDitherMatrix* get_ulichney_bayer_5_matrix(void);
MODULE_API OrderedDitherMatrix* get_ulichney_matrix(void);
MODULE_API OrderedDitherMatrix* get_bayer_clustered_dot_1_matrix(void);
MODULE_API OrderedDitherMatrix* get_bayer_clustered_dot_2_matrix(void);
MODULE_API OrderedDitherMatrix* get_bayer_clustered_dot_3_matrix(void);
MODULE_API OrderedDitherMatrix* get_bayer_clustered_dot_4_matrix(void);
MODULE_API OrderedDitherMatrix* get_bayer_clustered_dot_5_matrix(void);
MODULE_API OrderedDitherMatrix* get_bayer_clustered_dot_6_matrix(void);
MODULE_API OrderedDitherMatrix* get_bayer_clustered_dot_7_matrix(void);
MODULE_API OrderedDitherMatrix* get_bayer_clustered_dot_8_matrix(void);
MODULE_API OrderedDitherMatrix* get_bayer_clustered_dot_9_matrix(void);
MODULE_API OrderedDitherMatrix* get_bayer_clustered_dot_10_matrix(void);
MODULE_API OrderedDitherMatrix* get_bayer_clustered_dot_11_matrix(void);
MODULE_API OrderedDitherMatrix* get_central_white_point_matrix(void);
MODULE_API OrderedDitherMatrix* get_balanced_centered_point_matrix(void);
MODULE_API OrderedDitherMatrix* get_diagonal_ordered_matrix_matrix(void);
MODULE_API OrderedDitherMatrix* get_ulichney_clustered_dot_matrix(void);
MODULE_API OrderedDitherMatrix* get_magic5x5_circle_matrix(void);
MODULE_API OrderedDitherMatrix* get_magic6x6_circle_matrix(void);
MODULE_API OrderedDitherMatrix* get_magic7x7_circle_matrix(void);
MODULE_API OrderedDitherMatrix* get_magic4x4_45_matrix(void);
MODULE_API OrderedDitherMatrix* get_magic6x6_45_matrix(void);
MODULE_API OrderedDitherMatrix* get_magic8x8_45_matrix(void);
MODULE_API OrderedDitherMatrix* get_magic4x4_matrix(void);
MODULE_API OrderedDitherMatrix* get_magic6x6_matrix(void);
MODULE_API OrderedDitherMatrix* get_magic8x8_matrix(void);
/* Below functions return ordered dither matrices that change based on the function's inputs */
/* step: a value starting from 0 that changes appearance of the final dither output. recommended range: 0 - 100 */
MODULE_API OrderedDitherMatrix* get_variable_2x2_matrix(int step);
/* step: a value starting from 0 that changes appearance of the final dither output. recommended range: 0 - 100 */
MODULE_API OrderedDitherMatrix* get_variable_4x4_matrix(int step);
/* size: size (dimensions - width and height) of the dither matrix in pixels
  * a, b, c: influence the rotation of the matrix; ranges: a (0.0 - 1.0), b (0.0 - 1.0), c (0.0 - 100.0) */
MODULE_API OrderedDitherMatrix* get_interleaved_gradient_noise(int size, double a, double b, double c);
/* load a texture as dither matrix. Can be used to load, e.g. a Blue Noise texture for Blue Noise dithering.
 * input texture must be converted manually to a DitherImage */
MODULE_API OrderedDitherMatrix* get_matrix_from_image(const DitherImage* img);

/* ******************************** */
/* **** DOT DIFFUSION DITHERER **** */
/* ******************************** */

/* data-structure for holding the class matrix */
typedef struct Private_IntMatrix DotClassMatrix;
/* data-structure for holding the diffusion matrix */
typedef struct Private_DoubleMatrix DotDiffusionMatrix;
/* creates a new class matrix */
MODULE_API DotClassMatrix* DotClassMatrix_new(int width, int height, const int* matrix);
/* frees the class matrix's memory */
MODULE_API void DotClassMatrix_free(DotClassMatrix* self);
/* creates a new diffusion matrix */
MODULE_API DotDiffusionMatrix* DotDiffusionMatrix_new(int width, int height, const double* matrix);
/* frees the dithering matrix's memory */
MODULE_API void DotDiffusionMatrix_free(DotDiffusionMatrix* self);
/* Uses grid dither algorithm to dither an image - allows for different combination of class and diffusion matrices */
MODULE_API void dot_diffusion_dither(const DitherImage* img, const DotDiffusionMatrix* dmatrix, const DotClassMatrix* cmatrix, uint8_t* out);
/* below functions return different ordered dither matrices which can be used as input for 'dot_diffusion_dither' */
MODULE_API DotDiffusionMatrix* get_default_diffusion_matrix(void);
MODULE_API DotDiffusionMatrix* get_guoliu8_diffusion_matrix(void);
MODULE_API DotDiffusionMatrix* get_guoliu16_diffusion_matrix(void);
MODULE_API DotClassMatrix* get_mini_knuth_class_matrix(void);
MODULE_API DotClassMatrix* get_knuth_class_matrix(void);
MODULE_API DotClassMatrix* get_optimized_knuth_class_matrix(void);
MODULE_API DotClassMatrix* get_mese_8x8_class_matrix(void);
MODULE_API DotClassMatrix* get_mese_16x16_class_matrix(void);
MODULE_API DotClassMatrix* get_guoliu_8x8_class_matrix(void);
MODULE_API DotClassMatrix* get_guoliu_16x16_class_matrix(void);
MODULE_API DotClassMatrix* get_spiral_class_matrix(void);
MODULE_API DotClassMatrix* get_spiral_inverted_class_matrix(void);

/* ******************************************* */
/* **** VARIABLE ERROR DIFFUSION DITHERER **** */
/* ******************************************* */

/* enum for specifying the specific algorithm for 'variable_error_diffusion_dither' */
enum VarDitherType{Ostromoukhov, Zhoufang};
/* Uses variable error diffusion dither algorithm to dither an image.
 * type: Ostromoukhov or Zhoufang
 * serpentine: if the image should be traversed from top to bottom in a serpentine (left-to-right, right-to-left, etc.) manner */
MODULE_API void variable_error_diffusion_dither(const DitherImage* img, enum VarDitherType type, bool serpentine, uint8_t* out);

/* **************************** */
/* **** THRESHOLD DITHERER **** */
/* **************************** */

/* automatically determines best threshold for the given image. Use as input for threshold in 'threshold_dither' */
MODULE_API double auto_threshold(const DitherImage* img);
/* Uses thresholding algorithm to dither an image.
 * threshold: threshold for dithering a pixel as black. from 0.0 to 1.0.
 * noise: amount of noise. from 0.0 to 1.0. Recommended 0.55 */
MODULE_API void threshold_dither(const DitherImage* img, double threshold, double noise, uint8_t* out);

/* ********************** */
/* **** DBS DITHERER **** */
/* ********************** */

/* Uses the direct binary search (DBS) dither algorithm to dither an image. */
// v: value from 0-7. The higher the value, the coarser the output dither will be.
MODULE_API void dbs_dither(const DitherImage* img, int v, uint8_t* out);

/* *************************************** */
/* **** KACKER AND ALLEBACH DITHERING **** */
/* *************************************** */

/* Uses the Kacker and Allebach dither algorithm to dither an image.
 * random: when false, dither output will always be the same for the same image; otherwise there will be randomness */
MODULE_API void kallebach_dither(const DitherImage* img, bool random, uint8_t* out);

/* **************************** */
/* **** RIEMERSMA DITHERER **** */
/* **************************** */

/* enum for curve adjustment, as different space filling curves expand into different directions */
enum AdjustCurve{center_none, center_x, center_y, center_xy};
/* data-structure for holding a space filling curve */
typedef struct Private_RiemersmaCurve RiemersmaCurve;
/* creates a new space filling curve, but doesn't calculate it yet */
MODULE_API RiemersmaCurve* RiemersmaCurve_new(int base, int add_adjust, int exp_adjust, const char* axiom, int rule_count, const char* rules[], const char* keys, const int orientation[2], enum AdjustCurve adjust);
/* frees the curve's memory */
MODULE_API void RiemersmaCurve_free(RiemersmaCurve* self);
/* calculates the entire space filling curve */
MODULE_API char* create_curve(RiemersmaCurve* curve, int width, int height, int* curve_dim);
/* Uses the Riemersma dither algorithm to dither an image.
 * use_riemersma: when false, uses a slightly improved algorithm for better visual results. */
MODULE_API void riemersma_dither(const DitherImage* img, RiemersmaCurve* curve, bool use_riemersma, uint8_t* out);
/* below functions return different curves which can be used as input for 'riemersma_dither' */
MODULE_API RiemersmaCurve* get_hilbert_curve(void);
MODULE_API RiemersmaCurve* get_hilbert_mod_curve(void);
MODULE_API RiemersmaCurve* get_peano_curve(void);
MODULE_API RiemersmaCurve* get_fass0_curve(void);
MODULE_API RiemersmaCurve* get_fass1_curve(void);
MODULE_API RiemersmaCurve* get_fass2_curve(void);
MODULE_API RiemersmaCurve* get_gosper_curve(void);
MODULE_API RiemersmaCurve* get_fass_spiral_curve(void);

/* ************************** */
/* **** PATTERN DITHERER **** */
/* ************************** */

/* data-structure for holding an ordered dithering matrix */
typedef struct Private_TilePattern TilePattern;
/* creates a new tile pattern matrix */
MODULE_API TilePattern* TilePattern_new(int width, int height, int num_tiles, const int* pattern);
/* frees the tile pattern's memory */
MODULE_API void TilePattern_free(TilePattern* self);
/* Uses the pattern dither algorithm to dither an image. */
MODULE_API void pattern_dither(const DitherImage* img, const TilePattern *pattern, uint8_t* out);
/* below functions return tile patterns which can be used as input for 'pattern_dither' */
MODULE_API TilePattern* get_2x2_pattern(void);
MODULE_API TilePattern* get_3x3_v1_pattern(void);
MODULE_API TilePattern* get_3x3_v2_pattern(void);
MODULE_API TilePattern* get_3x3_v3_pattern(void);
MODULE_API TilePattern* get_4x4_pattern(void);
MODULE_API TilePattern* get_5x2_pattern(void);

/* ****************************************** */
/* **** LIPPENS AND PHILIPS DOT DITHERER **** */
/* ****************************************** */

/* data-structure for holding matices for Lippens & Philips ditherer */
typedef struct Private_IntMatrix DotLippensCoefficients;
/* creates a new matrices for Lippens & Philips ditherer */
MODULE_API DotLippensCoefficients* DotLippensCoefficients_new(int width, int height, const int* coefficients);
/* frees the matrices */
MODULE_API void DotLippensCoefficients_free(DotLippensCoefficients* self);
/* Function for calculating a Lippens & Philips class matrix */
MODULE_API int* create_dot_lippens_class_matrix(void);
/* Uses Lippens and Philip's dot dither algorithm to dither an image. */
MODULE_API void dotlippens_dither(const DitherImage* img, const DotClassMatrix* class_matrix, const DotLippensCoefficients* coefficients, uint8_t* out);
/* below functions return matrices which can be used as input for 'dotlippens_dither' */
MODULE_API DotClassMatrix* get_dotlippens_class_matrix(void);
MODULE_API DotLippensCoefficients* get_dotlippens_coefficients1(void);
MODULE_API DotLippensCoefficients* get_dotlippens_coefficients2(void);
MODULE_API DotLippensCoefficients* get_dotlippens_coefficients3(void);














/* ************************************************* */
/* **** COLOR - INPUT IMAGE FOR COLOR DITHERERS **** */
/* ************************************************* */

static const FloatColor D93_XYZ = { {0.98074}, {1.0}, {1.18232} }; // 9300 K, very cool light
static const FloatColor D75_XYZ = { {0.94972}, {1.0}, {1.22638} }; // 7500 K, Cooler daylight
static const FloatColor D65_XYZ = { {0.95047}, {1.0}, {1.08883} }; // 6504 K, sRGB base (working white-point)
static const FloatColor D55_XYZ = { {0.95682}, {1.0}, {0.92149} }; // 5500 K, Mid-morning/mid-afternoon daylight, photography
static const FloatColor D50_XYZ = { {0.96422}, {1.0}, {0.82521} }; // 5003 K, Warm daylight, print
//    older illuminants
static const FloatColor A_XYZ = { {1.09850}, {1.0}, {0.35585} }; // 2856 K, indoor lighting, tungsten bulb
static const FloatColor B_XYZ = { {0.99072}, {1.0}, {0.85365} }; // direct sunlight
static const FloatColor C_XYZ = { {0.98074}, {1.0}, {1.18232} }; // Average northern daylight
//    base
static const FloatColor E_XYZ = { {1.0}, {1.0}, {1.0} }; // Equal-energy spectrum (X=Y=Z), baseline
//    fluorescent light
static const FloatColor F1_XYZ = { {0.92834}, {1.0}, {1.03665} }; // 6430 K, Daylight fluorescent
static const FloatColor F2_XYZ = { {0.99186}, {1.0}, {0.67393} }; // 4230 K, Cool white fluorescent
static const FloatColor F3_XYZ = { {1.03896}, {1.0}, {0.65555} }; // 3450 K, White fluorescent
static const FloatColor F7_XYZ = { {0.95041}, {1.0}, {1.08747} }; // 6500 K, Broad-band fluorescent (D65 sim)
static const FloatColor F11_XYZ = { {1.00962}, {1.0}, {0.08747} }; // 4000 K, TL84 (commercial standard)

MODULE_API ColorImage* ColorImage_new(int width, int height);
MODULE_API void ColorImage_set_rgb(ColorImage* self, size_t addr, uint8_t r, uint8_t g, uint8_t b, uint8_t a);
MODULE_API void ColorImage_free(ColorImage* self);

MODULE_API CachedPalette* CachedPalette_new(void);
MODULE_API void CachedPalette_update_cache(CachedPalette* self, enum ColorComparisonMode mode, const FloatColor* illuminant);
MODULE_API void CachedPalette_free(CachedPalette* self);
MODULE_API void CachedPalette_from_image(CachedPalette* self, const ColorImage* image, size_t target_colors, enum QuantizationMethod quantization_method, bool unique, bool include_bw, bool include_rgb, bool include_cmy);
MODULE_API void CachedPalette_from_BytePalette(CachedPalette* self, const BytePalette* pal);
MODULE_API void CachedPalette_set_shift(CachedPalette* self, uint8_t r_shift, uint8_t g_shift, uint8_t b_shift);
MODULE_API void CachedPalette_free_cache(CachedPalette* self);
MODULE_API void CachedPalette_set_lab_weights(CachedPalette* self, FloatColor* weights);

MODULE_API BytePalette* BytePalette_new(size_t size);
MODULE_API void BytePalette_free(BytePalette* self);

MODULE_API void error_diffusion_dither_color(const ColorImage* img, const ErrorDiffusionMatrix* m, CachedPalette* lookup_pal, bool serpentine, int* out);
MODULE_API void ordered_dither_color(const ColorImage* image, CachedPalette* lookup_pal, const OrderedDitherMatrix* matrix, int* out);

MODULE_API void rgb_to_linear(const FloatColor* c, FloatColor* out);






#ifdef __cplusplus
}
#endif

#endif  // DITHERLIB_H
