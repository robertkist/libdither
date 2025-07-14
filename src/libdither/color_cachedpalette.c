#define MODULE_API_EXPORTS
#include <stdio.h>
#include <math.h>
#include "libdither.h"
#include "color_cachedpalette.h"
#include "color_models.h"
#include "color_quant_mediancut.h"
#include "color_quant_wu.h"
#include "color_quant_kdtree.h"
#include "tetrapal/tetrapal.h"

#define DBL_MAX 1.7976931348623158e+308

#define IDXD 0  // darkest color
#define IDXL 1  // lightest color
#define IDXR 2  // reddest color
#define IDXG 3  // greenest color
#define IDXB 4  // bluest color
#define IDXC 5  // cyan-est color
#define IDXM 6  // magenta-est color
#define IDXY 7  // yellowest color

static void create_lookup_palette(CachedPalette* self);

struct ByteColorHashEntry {
    /* helper for uthash */
    long key;                   /* key */
    ByteColor color;            /* sRGB color */
    UT_hash_handle hh2;         /* makes this structure hashable */
};

struct ColorExtremes {
    /* helper to find color extremes within a palette */
    double distance[8];
    ByteColor color[8];      // extreme colors
    FloatColor ref_color[8]; // reference color extremes to compare against
    bool include_cmy, include_rgb, include_bw;
};
typedef struct ColorExtremes ColorExtremes;

inline static long key_from_rgb(double r, double g, double b) {
    /* returns a 'long' key composed of floating point r,g,b values; used for hashing */
    long rr = ((long)(r * 255.0) << 16) & 0x00ff0000;
    long gg = ((long)(g * 255.0) << 8) & 0x0000ff00;
    long bb = (long)(b * 255.0) & 0x000000ff;
    return rr | gg | bb;
}

inline static long key_from_srgb(const ByteColor* bc) {
    /* returns a 'long' key composed of integer r,g,b values; used for hashing */
    long rr = ((long)bc->r << 16) & 0x00ff0000;
    long gg = ((long)bc->g << 8) & 0x0000ff00;
    long bb = (long)bc->b & 0x000000ff;
    return rr | gg | bb;
}

MODULE_API CachedPalette* CachedPalette_new(void) {
    /* Constructor */
    CachedPalette* self = (CachedPalette*)calloc(1, sizeof(CachedPalette));
    self->lookup_palette = NULL;
    self->target_palette = NULL;
    self->tetrapal = NULL;
    self->mode = LINEAR;
    self->hash = NULL;
    self->r_shift = 0;
    self->g_shift = 0;
    self->b_shift = 0;
    self->reduce = false;
    self->lab_weights.h = LAB_W_HUE;
    self->lab_weights.c = LAB_W_CHROMA;
    self->lab_weights.v = LAB_W_VALUE;
    return self;
}

MODULE_API void CachedPalette_set_lab_weights(CachedPalette* self, FloatColor* weights) {
    /* sets weights for LAB color comparison; will be used during color lookup - no need to call
     * CachedPalette_update_cache() after calling this function */
    FloatColor_from_FloatColor(&self->lab_weights, weights);
}

MODULE_API void CachedPalette_set_shift(CachedPalette* self, uint8_t r_shift, uint8_t g_shift, uint8_t b_shift) {
    /* bit-shifts colors during the lookup, which results in a smaller cache. Cache lookups will become faster
     * but less accurate */
    self->r_shift = r_shift;
    self->g_shift = g_shift;
    self->b_shift = b_shift;
    self->reduce = r_shift !=0 || g_shift !=0 || b_shift !=0;
}

MODULE_API void CachedPalette_update_cache(CachedPalette* self, enum ColorComparisonMode mode,
                                           const FloatColor* lab_illuminant) {
    /* updates the lookup cache when the color comparison mode changes */
    FloatPalette_free(self->lookup_palette);
    CachedPalette_free_cache(self);
    if (self->tetrapal != NULL) {
        tetrapal_free(self->tetrapal);
        self->tetrapal = NULL;
    }
    if (lab_illuminant == NULL) {
        FloatColor_from_FloatColor(&self->lab_illuminant, &D65_XYZ);
    } else {
        FloatColor_from_FloatColor(&self->lab_illuminant, lab_illuminant);
    }
    self->mode = mode;
    create_lookup_palette(self);
    if (mode == TETRAPAL) {
        float* floatpal = (float*)calloc(self->lookup_palette->size * 3, sizeof(float));
        for (size_t i = 0; i < self->lookup_palette->size; i++) {
            FloatColor* fc = FloatPalette_get(self->lookup_palette, i);
            floatpal[i * 3] = (float)fc->r;
            floatpal[i * 3 + 1] = (float)fc->g;
            floatpal[i * 3 + 2] = (float)fc->b;
        }
        self->tetrapal = tetrapal_new(floatpal, (int)self->lookup_palette->size);
        free(floatpal);
        // TODO could we free lookup_palette here and set it to NULL when using tetrapal?
    } else {
        self->tetrapal = NULL;
    }
}

static size_t get_tetrapal_index(Tetrapal* tetrapal, FloatColor* fc) {
    /* returns the closest Tetrapal color */
    size_t index = 0;
    int candidates[4] = {0, 0, 0, 0};;
    float weights[4] = {0, 0, 0, 0};
    float pixel[3];
    pixel[0] = (float)fc->r;
    pixel[1] = (float)fc->g;
    pixel[2] = (float)fc->b;
    weights[0] = weights[1] = weights[2] = weights[3] = 0;
    tetrapal_interpolate(tetrapal, pixel, candidates, weights);
    float w = 0.0;
    for (int i = 0; i < 4; i++) {
        if (weights[i] > w) {
            w = weights[i];
            index = (size_t)candidates[i];
        }
    }
    return index;
}

static void create_lookup_palette(CachedPalette* self) {
    /* creates the actual lookup palette, which is used for color-distance calculations. The lookup palette
     * is in the color space in which the distance calculation is performed in */
    self->lookup_palette = FloatPalette_new(self->target_palette->size);
    for (size_t i = 0; i < self->target_palette->size; i++) {
        ByteColor* bc = BytePalette_get(self->target_palette, i);
        FloatColor fc, target_color;
        FloatColor_from_ByteColor(&fc, bc);
        switch (self->mode) {
            case HSV:
                rgb_to_hsv(&fc, &target_color);
                break;
            case TETRAPAL:
            case LINEAR_CCIR:
            case LINEAR:
                rgb_to_linear(&fc, &target_color);
                break;
            case LUMINANCE:
                rgb_to_luminance(&fc, &target_color);
                break;
            case LAB76:
            case LAB94:
            case LAB2000:
                rgb_to_lab(&fc, &target_color, &self->lab_illuminant);
                break;
            case SRGB_CCIR:
            case SRGB:
            default:
                FloatColor_from_FloatColor(&target_color, &fc);
                break;
        }
        FloatPalette_set(self->lookup_palette, i, &target_color);
    }
}

static size_t find_closest_color(const CachedPalette* palette, const FloatColor* x) {
    /* returns the index of the color in the reduced palette with the smallest distance.
     * converts the input FloatColor into the color space in which the distance is calculated */
    double (*functionPtr)(const FloatColor*, const FloatColor*);
    double (*functionPtrLab)(const FloatColor*, const FloatColor*, const FloatColor*);
    functionPtr = &distance_linear;
    FloatColor fc;
    switch(palette->mode) {    // convert input color
        case HSV:
            rgb_to_hsv(x, &fc);
            functionPtr = &distance_hsv;
            break;
        case SRGB_CCIR:
            FloatColor_from_FloatColor(&fc, x);
            functionPtr = &distance_ccir;
            break;
        case SRGB:
            FloatColor_from_FloatColor(&fc, x);
            break;
        case LINEAR_CCIR:
            rgb_to_linear(x, &fc);
            functionPtr = &distance_ccir;
            break;
        case LINEAR:
            rgb_to_linear(x, &fc);
            break;
        case LUMINANCE:
            rgb_to_luminance(x, &fc);
            functionPtr = &distance_luminance;
            break;
        case LAB76:
            rgb_to_lab(x, &fc, &palette->lab_illuminant);
            break;
        case LAB94:
            rgb_to_lab(x, &fc, &palette->lab_illuminant);
            functionPtrLab = &distance_lab94;
            break;
        case LAB2000:
            rgb_to_lab(x, &fc, &palette->lab_illuminant);
            functionPtrLab = &distance_lab2000;
            break;
        case TETRAPAL:
            rgb_to_linear(x, &fc);
            return get_tetrapal_index(palette->tetrapal, &fc);
    }
    double lowest = DBL_MAX;
    size_t index = 0;
    size_t i;
    if (palette->mode == LAB94 || palette->mode == LAB2000) {
        for (i = 0; i < palette->lookup_palette->size; i++) {
            double delta = fabs((*functionPtrLab)(FloatPalette_get(palette->lookup_palette, i),
                                                                   &fc, &palette->lab_weights));
            if (delta < lowest) {
                lowest = delta;
                index = i;
            }
        }
    } else {
        for (i = 0; i < palette->lookup_palette->size; i++) {
            double delta = fabs((*functionPtr)(FloatPalette_get(palette->lookup_palette, i), &fc));
            if (delta < lowest) {
                lowest = delta;
                index = i;
            }
        }
    }
    return index;
}

size_t CachedPalette_find_closest_color(CachedPalette* self, const FloatColor *c) {
    /* color lookup with caching */
    /* c is a linear float color with an error */
    /* self->palette is a palette in lab color */
    long key;
    if (self->reduce) { // reduce source color's depth for less caching (sacrifice accuracy for speed)
        ByteColor bc;
        bc.r = ((uint8_t)(c->r * 255.0) >> self->r_shift);
        bc.g = ((uint8_t)(c->g * 255.0) >> self->g_shift);
        bc.b = ((uint8_t)(c->b * 255.0) >> self->b_shift);
        key = key_from_srgb(&bc);
    } else {
        key = key_from_rgb(c->r, c->g, c->b);
    }
    PaletteHashEntry* hash_item;
    HASH_FIND(hh1, self->hash, &key, sizeof(long), hash_item);
    if (hash_item == NULL) { // not in cache
        size_t index = find_closest_color(self, c);
        hash_item = malloc(sizeof *hash_item);
        hash_item->key = key;
        hash_item->index = index;
        HASH_ADD(hh1, self->hash, key, sizeof(long), hash_item);
        return index;
    }
    return hash_item->index;
}

MODULE_API void CachedPalette_free(CachedPalette* self) {
    /* frees the cached palette (i.e. destructor) */
    if (self) {
        FloatPalette_free(self->lookup_palette);
        BytePalette_free(self->target_palette);
        CachedPalette_free_cache(self);
        if (self->tetrapal != NULL)
            tetrapal_free(self->tetrapal);
        free(self);
        self = NULL;
    }
}

MODULE_API void CachedPalette_free_cache(CachedPalette* self) {
    /* frees the cache, which is built during lookups, only */
    if (self->hash != NULL) {
        PaletteHashEntry *hash_item, *tmp;
        HASH_ITER(hh1, self->hash, hash_item, tmp) {
            HASH_DELETE(hh1, self->hash, hash_item);  /* delete; users advances to next */
            free(hash_item);
        }
    }
    self->hash = NULL;
}

MODULE_API void CachedPalette_from_BytePalette(CachedPalette* self, const BytePalette* pal) {
    /* creates a cached palette from a given (reduced) byte palette */
    self->target_palette = BytePalette_copy(pal);
}

static void init_color_extremes_struct(ColorExtremes* ce, bool include_bw, bool include_rgb, bool include_cmy) {
    /* helper for initializing the data structures used for looking up color extremes */
    for (int i = 0; i < 8; i++) {
        FloatColor_set(&ce->ref_color[i], 0.0, 0.0, 0.0);
        ce->distance[i] = 1.0;
    }
    ce->ref_color[IDXL].r = ce->ref_color[IDXL].g = ce->ref_color[IDXL].b = 1.0;
    ce->ref_color[IDXR].r = ce->ref_color[IDXG].g = ce->ref_color[IDXB].b = 1.0;
    ce->ref_color[IDXC].r = ce->ref_color[IDXC].r = 1.0;
    ce->ref_color[IDXM].g = ce->ref_color[IDXM].b = 1.0;
    ce->ref_color[IDXY].r = ce->ref_color[IDXY].b = 1.0;
    ce->include_cmy = include_cmy;
    ce->include_rgb = include_rgb;
    ce->include_bw = include_bw;
}

static void include_extremes(ColorExtremes* ce, const ByteColor* bc) {
    /* finds extreme colors in source image: lightest/darkets, reddest, bluest, etc. for black-and-white,
     * RGB and CMY.
     * Uses euclidian distance as the source image is expected to be in sRGB space. */
    double distance;
    FloatColor fc;
    FloatColor_from_ByteColor(&fc, bc);
    if (ce->include_bw) {
        distance = distance_linear(&fc, &ce->ref_color[IDXD]);
        if (distance < ce->distance[IDXD]) { ce->distance[IDXD] = distance; ByteColor_copy(&ce->color[IDXD], bc); }
        distance = distance_linear(&fc, &ce->ref_color[IDXL]);
        if (distance < ce->distance[IDXL]) { ce->distance[IDXL] = distance; ByteColor_copy(&ce->color[IDXL], bc); }
    }
    if (ce->include_rgb) {
        distance = distance_linear(&fc, &ce->ref_color[IDXR]);
        if (distance < ce->distance[IDXR]) { ce->distance[IDXR] = distance; ByteColor_copy(&ce->color[IDXR], bc); }
        distance = distance_linear(&fc, &ce->ref_color[IDXG]);
        if (distance < ce->distance[IDXG]) { ce->distance[IDXG] = distance; ByteColor_copy(&ce->color[IDXG], bc); }
        distance = distance_linear(&fc, &ce->ref_color[IDXB]);
        if (distance < ce->distance[IDXB]) { ce->distance[IDXB] = distance; ByteColor_copy(&ce->color[IDXB], bc); }
    }
    if (ce->include_cmy) {
        distance = distance_linear(&fc, &ce->ref_color[IDXC]);
        if (distance < ce->distance[IDXC]) { ce->distance[IDXC] = distance; ByteColor_copy(&ce->color[IDXC], bc); }
        distance = distance_linear(&fc, &ce->ref_color[IDXM]);
        if (distance < ce->distance[IDXM]) { ce->distance[IDXM] = distance; ByteColor_copy(&ce->color[IDXM], bc); }
        distance = distance_linear(&fc, &ce->ref_color[IDXY]);
        if (distance < ce->distance[IDXY]) { ce->distance[IDXY] = distance; ByteColor_copy(&ce->color[IDXY], bc); }
    }
}

static size_t add_extreme_colors(ColorExtremes* ce, BytePalette* outPal, size_t target_colors) {
    /* helper function for finding color extremes in source image */
    size_t offset = 0;
    if (ce->include_bw && target_colors >= 2) {
        BytePalette_set(outPal, offset++, &ce->color[IDXD]);
        BytePalette_set(outPal, offset++, &ce->color[IDXL]);
    }
    if (ce->include_rgb && (target_colors - offset) >= 3) {
        BytePalette_set(outPal, offset++, &ce->color[IDXR]);
        BytePalette_set(outPal, offset++, &ce->color[IDXG]);
        BytePalette_set(outPal, offset++, &ce->color[IDXB]);
    }
    if (ce->include_cmy && (target_colors - offset) >= 3 ) {
        BytePalette_set(outPal, offset++, &ce->color[IDXC]);
        BytePalette_set(outPal, offset++, &ce->color[IDXM]);
        BytePalette_set(outPal, offset++, &ce->color[IDXY]);
    }
    return offset;
}

static BytePalette* get_image_palette(const ColorImage* image, ColorExtremes* ce, bool unique) {
    /* returns the palette of an image */
    // TODO future improvement: we don't have to iterate over the entire image to find the extremes, we
    //                          only have to iterate over its palette (which could be smaller than width*height)
    ByteColor bc;
    BytePalette* image_palette;
    size_t image_size = (size_t)(image->width * image->height);
    if (unique) { // creates a palette comprised entirely of unique colors in the image
        struct ByteColorHashEntry *hash_entries = NULL;
        struct ByteColorHashEntry *hash_item = NULL;
        struct ByteColorHashEntry* tmp;
        hash_entries = NULL;
        size_t hash_color_count = 0;
        // find unique colors
        for (size_t i = 0; i < image_size; i++) {
            ColorImage_get_srgb(image, i, &bc);
            if (bc.a != 0) {                        // only count not fully transparent pixels
                long key = key_from_srgb(&bc);
                HASH_FIND(hh2, hash_entries, &key, sizeof(long), hash_item);
                if (hash_item == NULL) {
                    hash_item = malloc(sizeof *hash_item);
                    hash_item->key = key;
                    ByteColor_copy(&hash_item->color, &bc);
                    HASH_ADD(hh2, hash_entries, key, sizeof(long), hash_item);
                    hash_color_count++;
                }
                include_extremes(ce, &bc);
            }
        }
        // gather hash items and convert to palette
        image_palette = BytePalette_new(hash_color_count);
        size_t i = 0;
        HASH_ITER(hh2, hash_entries, hash_item, tmp) {
            ByteColor_copy(BytePalette_get(image_palette, i++), &hash_item->color);
        }
        // free hash
        HASH_ITER(hh2, hash_entries, hash_item, tmp) {
            HASH_DELETE(hh2, hash_entries, hash_item);  /* delete; advances to next */
            free(hash_item);
        }
    } else { // creates a histogram of the image; i.e. a palette comprising of every pixel's color of the image
        image_palette = BytePalette_new(image_size);
        for (size_t i = 0; i < image_size; i++) {
            ColorImage_get_srgb(image, i, &bc);
            if (bc.a != 0) {       // only count not fully transparency pixels
                BytePalette_set(image_palette, i, &bc);
                include_extremes(ce, &bc);
            }
        }
    }
    return image_palette;
}

static BytePalette* quantify_colors(const BytePalette* unique_pal,
                                    enum QuantizationMethod quantization_method, size_t target_colors) {
    /* quantifies (i.e. reduces) the (source) palette with the given method to the specified number of colors */
    BytePalette* pal;
    switch (quantization_method) {
        case WU:
            pal = wu_quantization(unique_pal, target_colors);
            break;
        case KDTREE:
            pal = kdtree_quantization(unique_pal, target_colors);
            break;
        case MEDIAN_CUT:
        default:
            pal = median_cut(unique_pal, target_colors);
            break;
    }
    return pal;
}

MODULE_API void CachedPalette_from_image(CachedPalette* self, const ColorImage* image, size_t target_colors,
                                         enum QuantizationMethod quantization_method,
                                         bool unique, bool include_bw, bool include_rgb, bool include_cmy) {
    /* This function gets all unique colors of the image and then creates a reduced
     * target palette.
     * unique-colors: true - counts unique colors once
     *                false - also counts the number of appearances (# of pixels) of each color
     * */
    // get all unique colors in image: using a hash we ensure we don't get duplicates
    ColorExtremes ce;
    init_color_extremes_struct(&ce, include_bw, include_rgb, include_cmy);
    BytePalette_free(self->target_palette); // we're going to replace the existing target_palette...
    BytePalette* unique_pal = get_image_palette(image, &ce, unique);
    // do the quantization
    BytePalette *pal = NULL;
    if (unique_pal->size <= target_colors) {  // original palette has fewer colors than quantization target
        self->target_palette = unique_pal;    // we just return the original palette of unique colors...
        return;
    } else if (!include_bw && !include_rgb && !include_cmy) {  // user doesn't want 'extreme' colors included
        pal = quantify_colors(unique_pal, quantization_method, target_colors);
        BytePalette_free(unique_pal);
        self->target_palette = pal;
        return;
    } else { // include extreme colors in palette
        // user wants 'extreme' colors included
        BytePalette *outPal = BytePalette_new(target_colors);
        size_t offset = add_extreme_colors(&ce, outPal, target_colors);
        // reduce palette and merge palette
        if (target_colors - offset > 0) {
            pal = quantify_colors(unique_pal, quantization_method, target_colors - offset);
            BytePalette_free(unique_pal);
            for (size_t i = offset; i < target_colors; i++) { // merge
                ByteColor *c = BytePalette_get(pal, i - offset);
                BytePalette_set(outPal, i, c);
            }
            BytePalette_free(pal);
            self->target_palette = outPal;
            return;
        }
        // no space for reduced palette because 'extreme' colors take up all the space
        self->target_palette = outPal;
        return;
    }
}
