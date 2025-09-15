/**********************************************************************
	    C Implementation of Wu's Color Quantizer (v. 2)
	    (see Graphics Gems vol. II, pp. 126-133)
Author:	Xiaolin Wu
	Dept. of Computer Science
	Univ. of Western Ontario
	London, Ontario N6A 5B7
	wu@csd.uwo.ca
Algorithm: Greedy orthogonal bipartition of RGB space for variance
	   minimization aided by inclusion-exclusion tricks.
	   For speed no nearest neighbor search is done. Slightly
	   better performance can be expected by more sophisticated
	   but more expensive versions.
The author thanks Tom Lane at Tom_Lane@G.GP.CS.CMU.EDU for much of
additional documentation and a cure to a previous bug.
Free to distribute, comments and suggestions are appreciated.
**********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include "color_quant_wu.h"
#include "color_bytepalette.h"
#include "libdither.h"

#define MAXCOLOR 256
#define	RED	2
#define	GREEN 1
#define BLUE 0

struct Box {
    int r0;			 /* min value, exclusive */
    int r1;			 /* max value, inclusive */
    int g0;
    int g1;
    int b0;
    int b1;
    int vol;
};
typedef struct Box Box;

/* Histogram is in elements 1..HISTSIZE along each axis,
 * element 0 is for base or marginal value
 * NB: these must start out 0!
 */

struct Shared {
    double m2[33][33][33];
    long wt[33][33][33], mr[33][33][33], mg[33][33][33], mb[33][33][33];
    BytePalette *Ipal;  /* input palette */
    size_t size;        /*image size*/
    size_t K;           /*color look-up table size*/
    unsigned short *Qadd;
};
typedef struct Shared Shared;

Shared* shared = NULL;


static void Hist3d(long* vwt, long* vmr, long* vmg, long* vmb, double* m2_) {
    /* build 3-D color histogram of counts, r/g/b, c^2 */
    int ind, r, g, b;
    int inr, ing, inb, table[256];
    for(size_t i = 0; i < 256; ++i)
        table[i] = (int)(i * i);
    shared->Qadd = (unsigned short*)malloc(sizeof(short)*shared->size);
    if (shared->Qadd == NULL) {
        printf("WARNING: Wu quantification - Not enough space\n");
        exit(1);
    }
    for(size_t i = 0; i < shared->size; ++i) {
        ByteColor* bc = BytePalette_get(shared->Ipal, i);
        r = bc->r;
        g = bc->g;
        b = bc->b;
        inr = (r >> 3) + 1;
        ing = (g >> 3) + 1;
        inb = (b >> 3) + 1;
        ind = (inr << 10) + (inr << 6) + inr + (ing << 5) + ing + inb;
        shared->Qadd[i] = (unsigned short)ind;
        /* [inr][ing][inb] */
        ++vwt[ind];
        vmr[ind] += r;
        vmg[ind] += g;
        vmb[ind] += b;
        m2_[ind] += (double)(table[r] + table[g] + table[b]);
    }
}

/* At conclusion of the histogram step, we can interpret
 *   wt[r][g][b] = sum over voxel of P(c)
 *   mr[r][g][b] = sum over voxel of r*P(c)  ,  similarly for mg, mb
 *   m2[r][g][b] = sum over voxel of c^2*P(c)
 * Actually each of these should be divided by 'size' to give the usual
 * interpretation of P() as ranging from 0 to 1, but we needn't do that here.
 */

/* We now convert histogram into moments so that we can rapidly calculate
 * the sums of the above quantities over any desired box.
 */

static void M3d(long* vwt, long* vmr, long* vmg, long* vmb, double* m2_) {
    /* compute cumulative moments. */
    unsigned short ind1, ind2;
    uint8_t i, r, g, b;
    long line, line_r, line_g, line_b, area[33], area_r[33], area_g[33], area_b[33];
    double line2, area2[33];
    for(r = 1; r <= 32; ++r) {
        for(i = 0; i <= 32; ++i) {
            area[i] = area_r[i] = area_g[i] = area_b[i] = 0;
            area2[i] = 0.0;
        }
        for(g = 1; g <= 32; ++g) {
            line = line_r = line_g = line_b = 0;
            line2 = 0.0;
            for(b = 1; b <= 32; ++b) {
                ind1 = (unsigned short)((r << 10) + (r << 6) + r + (g << 5) + g + b); /* [r][g][b] */
                line += vwt[ind1];
                line_r += vmr[ind1];
                line_g += vmg[ind1];
                line_b += vmb[ind1];
                line2 += m2_[ind1];
                area[b] += line;
                area_r[b] += line_r;
                area_g[b] += line_g;
                area_b[b] += line_b;
                area2[b] += line2;
                ind2 = ind1 - 1089; /* [r-1][g][b] */
                vwt[ind1] = vwt[ind2] + area[b];
                vmr[ind1] = vmr[ind2] + area_r[b];
                vmg[ind1] = vmg[ind2] + area_g[b];
                vmb[ind1] = vmb[ind2] + area_b[b];
                m2_[ind1] = m2_[ind2] + area2[b];
            }
        }
    }
}

static long Vol(Box* cube, long mmt[33][33][33]) {
    /* Compute sum over a box of any given statistic */
    return(mmt[cube->r1][cube->g1][cube->b1]
           -mmt[cube->r1][cube->g1][cube->b0]
           -mmt[cube->r1][cube->g0][cube->b1]
           +mmt[cube->r1][cube->g0][cube->b0]
           -mmt[cube->r0][cube->g1][cube->b1]
           +mmt[cube->r0][cube->g1][cube->b0]
           +mmt[cube->r0][cube->g0][cube->b1]
           -mmt[cube->r0][cube->g0][cube->b0]);
}

/* The next two routines allow a slightly more efficient calculation
 * of Vol() for a proposed subbox of a given box.  The sum of Top()
 * and Bottom() is the Vol() of a subbox split in the given direction
 * and with the specified new upper bound.
 */

static long Bottom(Box* cube, uint8_t dir, long mmt[33][33][33]) {
    /* Compute part of Vol(cube, mmt) that doesn't depend on r1, g1, or b1 */
    /* (depending on dir) */
    switch(dir){
        case RED:
            return(-mmt[cube->r0][cube->g1][cube->b1]
                   +mmt[cube->r0][cube->g1][cube->b0]
                   +mmt[cube->r0][cube->g0][cube->b1]
                   -mmt[cube->r0][cube->g0][cube->b0]);
            break;
        case GREEN:
            return(-mmt[cube->r1][cube->g0][cube->b1]
                   +mmt[cube->r1][cube->g0][cube->b0]
                   +mmt[cube->r0][cube->g0][cube->b1]
                   -mmt[cube->r0][cube->g0][cube->b0]);
            break;
        case BLUE:
            return(-mmt[cube->r1][cube->g1][cube->b0]
                   +mmt[cube->r1][cube->g0][cube->b0]
                   +mmt[cube->r0][cube->g1][cube->b0]
                   -mmt[cube->r0][cube->g0][cube->b0]);
            break;
    }
    return 0;
}

static long Top(Box* cube, uint8_t dir, int pos, long mmt[33][33][33]) {
    /* Compute remainder of Vol(cube, mmt), substituting pos for */
    /* r1, g1, or b1 (depending on dir) */
    switch(dir){
        case RED:
            return (mmt[pos][cube->g1][cube->b1]
                    -mmt[pos][cube->g1][cube->b0]
                    -mmt[pos][cube->g0][cube->b1]
                    +mmt[pos][cube->g0][cube->b0]);
            break;
        case GREEN:
            return (mmt[cube->r1][pos][cube->b1]
                    -mmt[cube->r1][pos][cube->b0]
                    -mmt[cube->r0][pos][cube->b1]
                    +mmt[cube->r0][pos][cube->b0]);
            break;
        case BLUE:
            return (mmt[cube->r1][cube->g1][pos]
                    -mmt[cube->r1][cube->g0][pos]
                    -mmt[cube->r0][cube->g1][pos]
                    +mmt[cube->r0][cube->g0][pos]);
            break;
    }
    return 0;
}

static double Var(Box* cube) {
    /* Compute the weighted variance of a box */
    /* NB: as with the raw statistics, this is really the variance * size */
    double dr, dg, db, xx;
    dr = (double)Vol(cube, shared->mr);
    dg = (double)Vol(cube, shared->mg);
    db = (double)Vol(cube, shared->mb);
    xx =   shared->m2[cube->r1][cube->g1][cube->b1]
          -shared->m2[cube->r1][cube->g1][cube->b0]
          -shared->m2[cube->r1][cube->g0][cube->b1]
          +shared->m2[cube->r1][cube->g0][cube->b0]
          -shared->m2[cube->r0][cube->g1][cube->b1]
          +shared->m2[cube->r0][cube->g1][cube->b0]
          +shared->m2[cube->r0][cube->g0][cube->b1]
          -shared->m2[cube->r0][cube->g0][cube->b0];
    return (xx - (dr * dr + dg * dg + db * db) / (double)Vol(cube, shared->wt) );
}

/* We want to minimize the sum of the variances of two subboxes.
 * The sum(c^2) terms can be ignored since their sum over both subboxes
 * is the same (the sum for the whole box) no matter where we split.
 * The remaining terms have a minus sign in the variance formula,
 * so we drop the minus sign and MAXIMIZE the sum of the two terms.
 */

static double Maximize(Box* cube, uint8_t dir, int first, int last, int* cut,
               long whole_r, long whole_g, long whole_b, long whole_w) {
    long half_r, half_g, half_b, half_w;
    long base_r, base_g, base_b, base_w;
    int i;
    double temp, max;
    base_r = Bottom(cube, dir, shared->mr);
    base_g = Bottom(cube, dir, shared->mg);
    base_b = Bottom(cube, dir, shared->mb);
    base_w = Bottom(cube, dir, shared->wt);
    max = 0.0;
    *cut = -1;
    for(i=first; i<last; ++i){
        half_r = base_r + Top(cube, dir, i, shared->mr);
        half_g = base_g + Top(cube, dir, i, shared->mg);
        half_b = base_b + Top(cube, dir, i, shared->mb);
        half_w = base_w + Top(cube, dir, i, shared->wt);
        /* now half_x is sum over lower half of box, if split at i */
        if (half_w == 0) {      /* subbox could be empty of pixels! */
            continue;           /* never split into an empty box */
        } else
            temp = (double)(half_r * half_r + half_g * half_g + half_b * half_b) / (double)half_w;
        half_r = whole_r - half_r;
        half_g = whole_g - half_g;
        half_b = whole_b - half_b;
        half_w = whole_w - half_w;
        if (half_w == 0) {      /* subbox could be empty of pixels! */
            continue;           /* never split into an empty box */
        } else
            temp += (double)(half_r * half_r + half_g * half_g + half_b * half_b) / (double)half_w;
        if (temp > max) {
            max = temp;
            *cut = i;
        }
    }
    return(max);
}

static int Cut(Box* set1, Box* set2) {
    uint8_t dir;
    int cutr, cutg, cutb;
    double maxr, maxg, maxb;
    long whole_r, whole_g, whole_b, whole_w;
    whole_r = Vol(set1, shared->mr);
    whole_g = Vol(set1, shared->mg);
    whole_b = Vol(set1, shared->mb);
    whole_w = Vol(set1, shared->wt);
    maxr = Maximize(set1, RED, set1->r0+1, set1->r1, &cutr, whole_r, whole_g, whole_b, whole_w);
    maxg = Maximize(set1, GREEN, set1->g0+1, set1->g1, &cutg, whole_r, whole_g, whole_b, whole_w);
    maxb = Maximize(set1, BLUE, set1->b0+1, set1->b1, &cutb, whole_r, whole_g, whole_b, whole_w);
    if((maxr >= maxg) && (maxr >= maxb)) {
        dir = RED;
        if (cutr < 0)
            return 0; /* can't split the box */
    }
    else
    if( (maxg>=maxr)&&(maxg>=maxb) )
        dir = GREEN;
    else
        dir = BLUE;
    set2->r1 = set1->r1;
    set2->g1 = set1->g1;
    set2->b1 = set1->b1;
    switch (dir) {
        case RED:
            set2->r0 = set1->r1 = cutr;
            set2->g0 = set1->g0;
            set2->b0 = set1->b0;
            break;
        case GREEN:
            set2->g0 = set1->g1 = cutg;
            set2->r0 = set1->r0;
            set2->b0 = set1->b0;
            break;
        case BLUE:
            set2->b0 = set1->b1 = cutb;
            set2->r0 = set1->r0;
            set2->g0 = set1->g0;
            break;
    }
    set1->vol=(set1->r1 - set1->r0) * (set1->g1 - set1->g0) * (set1->b1 - set1->b0);
    set2->vol=(set2->r1 - set2->r0) * (set2->g1 - set2->g0) * (set2->b1 - set2->b0);
    return 1;
}

static void Mark(Box* cube, size_t label, uint8_t* tag) {
    int r, g, b;
    for(r = cube->r0 + 1; r <= cube->r1; ++r)
        for(g = cube->g0 + 1; g <= cube->g1; ++g)
            for(b = cube->b0 + 1; b <= cube->b1; ++b)
                tag[(r << 10) + (r << 6) + r + (g << 5) + g + b] = (uint8_t)label;
}

BytePalette* wu_quantization(const BytePalette* pal, size_t target_k) {
    /* performs the Wu color quantization */
    // TODO add a check that target_k <= MAXCOLOR
    Box cube[MAXCOLOR];
    uint8_t *tag;
    uint8_t lut_r[MAXCOLOR], lut_g[MAXCOLOR], lut_b[MAXCOLOR]; /* output */
    size_t next;
    size_t k;
    long weight;
    double vv[MAXCOLOR], temp;

    /* reset global variables */
    shared = (Shared*)calloc(1, sizeof(Shared));

    /* input R,G,B components into Ir, Ig, Ib; set size to width*height */
    shared->Ipal = BytePalette_copy(pal);
    shared->size = pal->size;
    shared->K = target_k;

    Hist3d((long*)shared->wt, (long*)shared->mr, (long*)shared->mg, (long*)shared->mb, (double*)shared->m2);
    M3d((long*)shared->wt, (long*)shared->mr, (long*)shared->mg, (long*)shared->mb, (double*)shared->m2);

    cube[0].r0 = cube[0].g0 = cube[0].b0 = 0;
    cube[0].r1 = cube[0].g1 = cube[0].b1 = 32;
    next = 0;

    for(size_t i = 1; i < shared->K; ++i){
        if (Cut(&cube[next], &cube[i])) {
            /* volume test ensures we won't try to cut one-cell box */
            vv[next] = (cube[next].vol > 1) ? Var(&cube[next]) : 0.0;
            vv[i] = (cube[i].vol > 1) ? Var(&cube[i]) : 0.0;
        } else {
            vv[next] = 0.0;   /* don't try to split this box again */
            i--;              /* didn't create box i */
        }
        next = 0;
        temp = vv[0];
        for(size_t kk = 1; kk <= i; ++kk)
            if (vv[kk] > temp) {
                temp = vv[kk];
                next = kk;
            }
        if (temp <= 0.0) {
            shared->K = i + 1;
            fprintf(stderr, "WARNING: Wu quantification - Only got %zu boxes\n", shared->K);
            break;
        }
    }
    /* the space for array m2 can be freed now */
    BytePalette* wuPalette = BytePalette_new(shared->K);
    tag = (uint8_t *)malloc(33 * 33 * 33);
    if (tag == NULL) {
        printf("WARNING: Wu quantification - Not enough space\n");
        exit(1);
    }
    for(k = 0; k < shared->K; ++k){
        Mark(&cube[k], k, tag);
        weight = Vol(&cube[k], shared->wt);
        if (weight) {
            lut_r[k] = (uint8_t)(Vol(&cube[k], shared->mr) / weight);
            lut_g[k] = (uint8_t)(Vol(&cube[k], shared->mg) / weight);
            lut_b[k] = (uint8_t)(Vol(&cube[k], shared->mb) / weight);
        }
        else{
            fprintf(stderr, "WARNING: Wu quantification - bogus box %zu\n", k);
            lut_r[k] = lut_g[k] = lut_b[k] = 0;
        }
        ByteColor bc;
        bc.r = lut_r[k];
        bc.g = lut_g[k];
        bc.b = lut_b[k];
        bc.a = 255;
        BytePalette_set(wuPalette, k, &bc);
    }

    for(size_t i=0; i<shared->size; ++i) shared->Qadd[i] = tag[shared->Qadd[i]];
    /* output lut_r, lut_g, lut_b as color look-up table contents, Qadd as the quantized image (array of table addresses). */
    free(shared);
    return wuPalette;
}
