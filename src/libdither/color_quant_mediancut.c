#include <string.h>
#include <math.h>
#include "libdither.h"
#include "color_quant_mediancut.h"
#include "color_bytepalette.h"

/* return larger of two intehers */
static inline int MAXi(int a, int b) { return((a) > (b) ? a : b); }

/* return smaller of two integers */
static inline int MINi(int a, int b) { return((a) < (b) ? a : b); }

/* color bucket used for sorting */
struct Bucket {
    uint8_t* buffer;
    size_t size; // number of colors
    int range;
    int channel;
    ByteColor average;
};
typedef struct Bucket Bucket;

/* current color channel used for sorting */
static int sort_color_channel = 0;

static void Bucket_update_range(Bucket* self) {
    /* calculates range for RGB channels in the bucket and determines channel with the biggest range */
    ByteColor *ptr = (ByteColor*)self->buffer;
    int lower_red = 255, lower_green = 255, lower_blue = 255;
    int upper_red = 0, upper_green = 0, upper_blue = 0;
    for (size_t i = 0; i < self->size; i++) {
        ByteColor* c = &ptr[i];
        lower_red = MINi(lower_red, c->r);
        lower_green = MINi(lower_green, c->g);
        lower_blue = MINi(lower_blue, c->b);
        upper_red = MAXi(upper_red, c->r);
        upper_green = MAXi(upper_green, c->g);
        upper_blue = MAXi(upper_blue, c->b);
    }
    int red = upper_red - lower_red;
    int green = upper_green - lower_green;
    int blue = upper_blue - lower_blue;
    self->range = MAXi(MAXi(red, green), blue);
    // find channel with max range
    if (self->range == red) self->channel = 0;
    else if (self->range == green) self->channel = 1;
    else if (self->range == blue) self->channel = 2;
}

static Bucket* Bucket_new(uint8_t* pal, size_t num_colors) {
    /* creates a new bucket (i.e. constructor) */
    Bucket* self = (Bucket*)calloc(1, sizeof(Bucket));
    self->size = num_colors;
    self->buffer = (uint8_t*)calloc(num_colors * 4, sizeof(uint8_t));
    memcpy(self->buffer, pal, num_colors * 4 * sizeof(uint8_t));
    Bucket_update_range(self);
    return self;
}

static int compare(const void* a,const void* b) {
    /* comparison function */
    uint8_t* c1 = (uint8_t*)a;
    uint8_t* c2 = (uint8_t*)b;
    return c1[sort_color_channel] - c2[sort_color_channel];
}

static void Bucket_sort(Bucket* self) {
    /* sorts colors in a bucket by one of the RGB color channels */
    sort_color_channel = self->channel;
    qsort(self->buffer, self->size, sizeof(ByteColor), compare);
}

static void Bucket_free(Bucket* self) {
    /* frees a bucket (i.e. destructor) */
    if(self) {
        free(self->buffer);
        free(self);
        self = NULL;
    }
}

static Bucket* Bucket_split(Bucket* self, bool upper) {
    /* splits a bucket in two */
    if (self->size == 1) {
        return self;
    }
    Bucket_sort(self);
    size_t upper_size = self->size / 2;
    size_t lower_size = self->size - upper_size;
    if (upper) {
        return Bucket_new(self->buffer + lower_size * 4, upper_size);
    } else {
        return Bucket_new(self->buffer, lower_size);
    }
}

static void Bucket_average(Bucket* self) {
    /* finds the bucket's RGB color channel averages */
    size_t red = 0;
    size_t green = 0;
    size_t blue = 0;
    ByteColor *ptr = (ByteColor*)self->buffer;
    if (self->size == 1) {
        ByteColor*c = &ptr[0];
        self->average.r = c->r;
        self->average.g = c->g;
        self->average.b = c->b;
    } else {
        for (size_t i = 0; i < self->size; i++) {
            ByteColor*c = &ptr[i];
            red += c->r;
            green += c->g;
            blue += c->b;
        }
        self->average.r = (uint8_t)round((double)red / (double)self->size);
        self->average.g = (uint8_t)round((double)green / (double)self->size);
        self->average.b = (uint8_t)round((double)blue / (double)self->size);
    }
    self->average.a = 255;
}

BytePalette* median_cut(const BytePalette* palette, size_t out_cols) {
    /* performs the median cut color quantization algorithm */
    if (out_cols >= palette->size) {
        return NULL;
    }
    Bucket** bucket_list;
    bucket_list = (Bucket**)calloc(out_cols + 1, sizeof(Bucket*));
    bucket_list[0] = Bucket_new(palette->buffer, palette->size);
    size_t num_buckets = 0;
    for (size_t i = 0; i < out_cols; i++) {
        int max_range = 0;
        size_t max_bucket = 0;
        for (size_t j = 0; j < num_buckets; j++) {
            if (bucket_list[j]->range > max_range && bucket_list[j]->size > 1) {
                max_range = bucket_list[j]->range;
                max_bucket = j;
            }
        }
        Bucket* upper_bucket = Bucket_split(bucket_list[max_bucket], true);
        Bucket* lower_bucket = Bucket_split(bucket_list[max_bucket], false);
        Bucket_free(bucket_list[max_bucket]);
        bucket_list[max_bucket] = lower_bucket;
        bucket_list[++num_buckets] = upper_bucket;
    }
    BytePalette* out = BytePalette_new(out_cols);
    for (size_t i = 0; i < num_buckets; i++) {
        Bucket_average(bucket_list[i]);
        BytePalette_set(out, i, &bucket_list[i]->average);
    }
    for (size_t i = 0; i < num_buckets; i++) {
        if (bucket_list[i] != NULL) {
            Bucket_free(bucket_list[i]);
        }
    }
    free(bucket_list);
    return out;
}
