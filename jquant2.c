/*
 * jquant2.c
 *
 * Copyright (C) 1991, 1992, 1993, Thomas G. Lane.
 * This file is part of the Independent JPEG Group's software.
 * For conditions of distribution and use, see the accompanying README file.
 *
 * This file contains 2-pass color quantization (color mapping) routines.
 * These routines are invoked via the methods color_quant_prescan,
 * color_quant_doit, and color_quant_init/term.
 */

#include "jinclude.h"

#ifdef QUANT_2PASS_SUPPORTED


/*
 * This module implements the well-known Heckbert paradigm for color
 * quantization.  Most of the ideas used here can be traced back to
 * Heckbert's seminal paper
 *   Heckbert, Paul.  "Color Image Quantization for Frame Buffer Display",
 *   Proc. SIGGRAPH '82, Computer Graphics v.16 #3 (July 1982), pp 297-304.
 *
 * In the first pass over the image, we accumulate a histogram showing the
 * usage count of each possible color.  (To keep the histogram to a reasonable
 * size, we reduce the precision of the input; typical practice is to retain
 * 5 or 6 bits per color, so that 8 or 4 different input values are counted
 * in the same histogram cell.)  Next, the color-selection step begins with a
 * box representing the whole color space, and repeatedly splits the "largest"
 * remaining box until we have as many boxes as desired colors.  Then the mean
 * color in each remaining box becomes one of the possible output colors.
 * The second pass over the image maps each input pixel to the closest output
 * color (optionally after applying a Floyd-Steinberg dithering correction).
 * This mapping is logically trivial, but making it go fast enough requires
 * considerable care.
 *
 * Heckbert-style quantizers vary a good deal in their policies for choosing
 * the "largest" box and deciding where to cut it.  The particular policies
 * used here have proved out well in experimental comparisons, but better ones
 * may yet be found.
 *
 * The most significant difference between this quantizer and others is that
 * this one is intended to operate in YCbCr colorspace, rather than RGB space
 * as is usually done.  Actually we work in scaled YCbCr colorspace, where
 * Y distances are inflated by a factor of 2 relative to Cb or Cr distances.
 * The empirical evidence is that distances in this space correspond to
 * perceptual color differences more closely than do distances in RGB space;
 * and working in this space is inexpensive within a JPEG decompressor, since
 * the input data is already in YCbCr form.  (We could transform to an even
 * more perceptually linear space such as Lab or Luv, but that is very slow
 * and doesn't yield much better results than scaled YCbCr.)
 */

#define Y_SCALE 2		/* scale Y distances up by this much */

#define MAXNUMCOLORS  (MAXJSAMPLE+1) /* maximum size of colormap */


/*
 * First we have the histogram data structure and routines for creating it.
 *
 * For work in YCbCr space, it is useful to keep more precision for Y than
 * for Cb or Cr.  We recommend keeping 6 bits for Y and 5 bits each for Cb/Cr.
 * If you have plenty of memory and cycles, 6 bits all around gives marginally
 * better results; if you are short of memory, 5 bits all around will save
 * some space but degrade the results.
 * To maintain a fully accurate histogram, we'd need to allocate a "long"
 * (preferably unsigned long) for each cell.  In practice this is overkill;
 * we can get by with 16 bits per cell.  Few of the cell counts will overflow,
 * and clamping those that do overflow to the maximum value will give close-
 * enough results.  This reduces the recommended histogram size from 256Kb
 * to 128Kb, which is a useful savings on PC-class machines.
 * (In the second pass the histogram space is re-used for pixel mapping data;
 * in that capacity, each cell must be able to store zero to the number of
 * desired colors.  16 bits/cell is plenty for that too.)
 * Since the JPEG code is intended to run in small memory model on 80x86
 * machines, we can't just allocate the histogram in one chunk.  Instead
 * of a true 3-D array, we use a row of pointers to 2-D arrays.  Each
 * pointer corresponds to a Y value (typically 2^6 = 64 pointers) and
 * each 2-D array has 2^5^2 = 1024 or 2^6^2 = 4096 entries.  Note that
 * on 80x86 machines, the pointer row is in near memory but the actual
 * arrays are in far memory (same arrangement as we use for image arrays).
 */

#ifndef HIST_Y_BITS		/* so you can override from Makefile */
#define HIST_Y_BITS  6		/* bits of precision in Y histogram */
#endif
#ifndef HIST_C_BITS		/* so you can override from Makefile */
#define HIST_C_BITS  5		/* bits of precision in Cb/Cr histogram */
#endif

#define HIST_Y_ELEMS  (1<<HIST_Y_BITS) /* # of elements along histogram axes */
#define HIST_C_ELEMS  (1<<HIST_C_BITS)

/* These are the amounts to shift an input value to get a histogram index.
 * For a combination 8/12 bit implementation, would need variables here...
 */

#define Y_SHIFT  (BITS_IN_JSAMPLE-HIST_Y_BITS)
#define C_SHIFT  (BITS_IN_JSAMPLE-HIST_C_BITS)


typedef UINT16 histcell;	/* histogram cell; MUST be an unsigned type */

typedef histcell FAR * histptr;	/* for pointers to histogram cells */

typedef histcell hist1d[HIST_C_ELEMS]; /* typedefs for the array */
typedef hist1d FAR * hist2d;	/* type for the Y-level pointers */
typedef hist2d * hist3d;	/* type for top-level pointer */

static hist3d histogram;	/* pointer to the histogram */


/*
 * Prescan some rows of pixels.
 * In this module the prescan simply updates the histogram, which has been
 * initialized to zeroes by color_quant_init.
 * Note: workspace is probably not useful for this routine, but it is passed
 * anyway to allow some code sharing within the pipeline controller.
 */

METHODDEF void
color_quant_prescan (decompress_info_ptr cinfo, int num_rows,
		     JSAMPIMAGE image_data, JSAMPARRAY workspace)
{
  register JSAMPROW ptr0, ptr1, ptr2;
  register histptr histp;
  register int c0, c1, c2;
  int row;
  long col;
  long width = cinfo->image_width;

  for (row = 0; row < num_rows; row++) {
    ptr0 = image_data[0][row];
    ptr1 = image_data[1][row];
    ptr2 = image_data[2][row];
    for (col = width; col > 0; col--) {
      /* get pixel value and index into the histogram */
      c0 = GETJSAMPLE(*ptr0++) >> Y_SHIFT;
      c1 = GETJSAMPLE(*ptr1++) >> C_SHIFT;
      c2 = GETJSAMPLE(*ptr2++) >> C_SHIFT;
      histp = & histogram[c0][c1][c2];
      /* increment, check for overflow and undo increment if so. */
      /* We assume unsigned representation here! */
      if (++(*histp) == 0)
	(*histp)--;
    }
  }
}


/*
 * Now we have the really interesting routines: selection of a colormap
 * given the completed histogram.
 * These routines work with a list of "boxes", each representing a rectangular
 * subset of the input color space (to histogram precision).
 */

typedef struct {
	/* The bounds of the box (inclusive); expressed as histogram indexes */
	int c0min, c0max;
	int c1min, c1max;
	int c2min, c2max;
	/* The number of nonzero histogram cells within this box */
	long colorcount;
      } box;
typedef box * boxptr;

static boxptr boxlist;		/* array with room for desired # of boxes */
static int numboxes;		/* number of boxes currently in boxlist */

static JSAMPARRAY my_colormap;	/* the finished colormap (in YCbCr space) */


LOCAL boxptr
find_biggest_color_pop (void)
/* Find the splittable box with the largest color population */
/* Returns NULL if no splittable boxes remain */
{
  register boxptr boxp;
  register int i;
  register long max = 0;
  boxptr which = NULL;
  
  for (i = 0, boxp = boxlist; i < numboxes; i++, boxp++) {
    if (boxp->colorcount > max) {
      if (boxp->c0max > boxp->c0min || boxp->c1max > boxp->c1min ||
	  boxp->c2max > boxp->c2min) {
	which = boxp;
	max = boxp->colorcount;
      }
    }
  }
  return which;
}


LOCAL boxptr
find_biggest_volume (void)
/* Find the splittable box with the largest (scaled) volume */
/* Returns NULL if no splittable boxes remain */
{
  register boxptr boxp;
  register int i;
  register INT32 max = 0;
  register INT32 norm, c0,c1,c2;
  boxptr which = NULL;
  
  /* We use 2-norm rather than real volume here.
   * Some care is needed since the differences are expressed in
   * histogram-cell units; if HIST_Y_BITS != HIST_C_BITS, we have to
   * adjust the scaling to get the proper scaled-YCbCr-space distance.
   * This code won't work right if HIST_Y_BITS < HIST_C_BITS,
   * but that shouldn't ever be true.
   * Note norm > 0 iff box is splittable, so need not check separately.
   */
  
  for (i = 0, boxp = boxlist; i < numboxes; i++, boxp++) {
    c0 = (boxp->c0max - boxp->c0min) * Y_SCALE;
    c1 = (boxp->c1max - boxp->c1min) << (HIST_Y_BITS-HIST_C_BITS);
    c2 = (boxp->c2max - boxp->c2min) << (HIST_Y_BITS-HIST_C_BITS);
    norm = c0*c0 + c1*c1 + c2*c2;
    if (norm > max) {
      which = boxp;
      max = norm;
    }
  }
  return which;
}


LOCAL void
update_box (boxptr boxp)
/* Shrink the min/max bounds of a box to enclose only nonzero elements, */
/* and recompute its population */
{
  histptr histp;
  int c0,c1,c2;
  int c0min,c0max,c1min,c1max,c2min,c2max;
  long ccount;
  
  c0min = boxp->c0min;  c0max = boxp->c0max;
  c1min = boxp->c1min;  c1max = boxp->c1max;
  c2min = boxp->c2min;  c2max = boxp->c2max;
  
  if (c0max > c0min)
    for (c0 = c0min; c0 <= c0max; c0++)
      for (c1 = c1min; c1 <= c1max; c1++) {
	histp = & histogram[c0][c1][c2min];
	for (c2 = c2min; c2 <= c2max; c2++)
	  if (*histp++ != 0) {
	    boxp->c0min = c0min = c0;
	    goto have_c0min;
	  }
      }
 have_c0min:
  if (c0max > c0min)
    for (c0 = c0max; c0 >= c0min; c0--)
      for (c1 = c1min; c1 <= c1max; c1++) {
	histp = & histogram[c0][c1][c2min];
	for (c2 = c2min; c2 <= c2max; c2++)
	  if (*histp++ != 0) {
	    boxp->c0max = c0max = c0;
	    goto have_c0max;
	  }
      }
 have_c0max:
  if (c1max > c1min)
    for (c1 = c1min; c1 <= c1max; c1++)
      for (c0 = c0min; c0 <= c0max; c0++) {
	histp = & histogram[c0][c1][c2min];
	for (c2 = c2min; c2 <= c2max; c2++)
	  if (*histp++ != 0) {
	    boxp->c1min = c1min = c1;
	    goto have_c1min;
	  }
      }
 have_c1min:
  if (c1max > c1min)
    for (c1 = c1max; c1 >= c1min; c1--)
      for (c0 = c0min; c0 <= c0max; c0++) {
	histp = & histogram[c0][c1][c2min];
	for (c2 = c2min; c2 <= c2max; c2++)
	  if (*histp++ != 0) {
	    boxp->c1max = c1max = c1;
	    goto have_c1max;
	  }
      }
 have_c1max:
  if (c2max > c2min)
    for (c2 = c2min; c2 <= c2max; c2++)
      for (c0 = c0min; c0 <= c0max; c0++) {
	histp = & histogram[c0][c1min][c2];
	for (c1 = c1min; c1 <= c1max; c1++, histp += HIST_C_ELEMS)
	  if (*histp != 0) {
	    boxp->c2min = c2min = c2;
	    goto have_c2min;
	  }
      }
 have_c2min:
  if (c2max > c2min)
    for (c2 = c2max; c2 >= c2min; c2--)
      for (c0 = c0min; c0 <= c0max; c0++) {
	histp = & histogram[c0][c1min][c2];
	for (c1 = c1min; c1 <= c1max; c1++, histp += HIST_C_ELEMS)
	  if (*histp != 0) {
	    boxp->c2max = c2max = c2;
	    goto have_c2max;
	  }
      }
 have_c2max:
  
  /* Now scan remaining volume of box and compute population */
  ccount = 0;
  for (c0 = c0min; c0 <= c0max; c0++)
    for (c1 = c1min; c1 <= c1max; c1++) {
      histp = & histogram[c0][c1][c2min];
      for (c2 = c2min; c2 <= c2max; c2++, histp++)
	if (*histp != 0) {
	  ccount++;
	}
    }
  boxp->colorcount = ccount;
}


LOCAL void
median_cut (int desired_colors)
/* Repeatedly select and split the largest box until we have enough boxes */
{
  int n,lb;
  int c0,c1,c2,cmax;
  register boxptr b1,b2;

  while (numboxes < desired_colors) {
    /* Select box to split */
    /* Current algorithm: by population for first half, then by volume */
    if (numboxes*2 <= desired_colors) {
      b1 = find_biggest_color_pop();
    } else {
      b1 = find_biggest_volume();
    }
    if (b1 == NULL)		/* no splittable boxes left! */
      break;
    b2 = &boxlist[numboxes];	/* where new box will go */
    /* Copy the color bounds to the new box. */
    b2->c0max = b1->c0max; b2->c1max = b1->c1max; b2->c2max = b1->c2max;
    b2->c0min = b1->c0min; b2->c1min = b1->c1min; b2->c2min = b1->c2min;
    /* Choose which axis to split the box on.
     * Current algorithm: longest scaled axis.
     * See notes in find_biggest_volume about scaling...
     */
    c0 = (b1->c0max - b1->c0min) * Y_SCALE;
    c1 = (b1->c1max - b1->c1min) << (HIST_Y_BITS-HIST_C_BITS);
    c2 = (b1->c2max - b1->c2min) << (HIST_Y_BITS-HIST_C_BITS);
    cmax = c0; n = 0;
    if (c1 > cmax) { cmax = c1; n = 1; }
    if (c2 > cmax) { n = 2; }
    /* Choose split point along selected axis, and update box bounds.
     * Current algorithm: split at halfway point.
     * (Since the box has been shrunk to minimum volume,
     * any split will produce two nonempty subboxes.)
     * Note that lb value is max for lower box, so must be < old max.
     */
    switch (n) {
    case 0:
      lb = (b1->c0max + b1->c0min) / 2;
      b1->c0max = lb;
      b2->c0min = lb+1;
      break;
    case 1:
      lb = (b1->c1max + b1->c1min) / 2;
      b1->c1max = lb;
      b2->c1min = lb+1;
      break;
    case 2:
      lb = (b1->c2max + b1->c2min) / 2;
      b1->c2max = lb;
      b2->c2min = lb+1;
      break;
    }
    /* Update stats for boxes */
    update_box(b1);
    update_box(b2);
    numboxes++;
  }
}


LOCAL void
compute_color (boxptr boxp, int icolor)
/* Compute representative color for a box, put it in my_colormap[icolor] */
{
  /* Current algorithm: mean weighted by pixels (not colors) */
  /* Note it is important to get the rounding correct! */
  histptr histp;
  int c0,c1,c2;
  int c0min,c0max,c1min,c1max,c2min,c2max;
  long count;
  long total = 0;
  long c0total = 0;
  long c1total = 0;
  long c2total = 0;
  
  c0min = boxp->c0min;  c0max = boxp->c0max;
  c1min = boxp->c1min;  c1max = boxp->c1max;
  c2min = boxp->c2min;  c2max = boxp->c2max;
  
  for (c0 = c0min; c0 <= c0max; c0++)
    for (c1 = c1min; c1 <= c1max; c1++) {
      histp = & histogram[c0][c1][c2min];
      for (c2 = c2min; c2 <= c2max; c2++) {
	if ((count = *histp++) != 0) {
	  total += count;
	  c0total += ((c0 << Y_SHIFT) + ((1<<Y_SHIFT)>>1)) * count;
	  c1total += ((c1 << C_SHIFT) + ((1<<C_SHIFT)>>1)) * count;
	  c2total += ((c2 << C_SHIFT) + ((1<<C_SHIFT)>>1)) * count;
	}
      }
    }
  
  my_colormap[0][icolor] = (JSAMPLE) ((c0total + (total>>1)) / total);
  my_colormap[1][icolor] = (JSAMPLE) ((c1total + (total>>1)) / total);
  my_colormap[2][icolor] = (JSAMPLE) ((c2total + (total>>1)) / total);
}


LOCAL void
remap_colormap (decompress_info_ptr cinfo)
/* Remap the internal colormap to the output colorspace */
{
  /* This requires a little trickery since color_convert expects to
   * deal with 3-D arrays (a 2-D sample array for each component).
   * We must promote the colormaps into one-row 3-D arrays.
   */
  short ci;
  JSAMPARRAY input_hack[3];
  JSAMPARRAY output_hack[10];	/* assume no more than 10 output components */

  for (ci = 0; ci < 3; ci++)
    input_hack[ci] = &(my_colormap[ci]);
  for (ci = 0; ci < cinfo->color_out_comps; ci++)
    output_hack[ci] = &(cinfo->colormap[ci]);

  (*cinfo->methods->color_convert) (cinfo, 1,
				    (long) cinfo->actual_number_of_colors,
				    input_hack, output_hack);
}


LOCAL void
select_colors (decompress_info_ptr cinfo)
/* Master routine for color selection */
{
  int desired = cinfo->desired_number_of_colors;
  int i;

  /* Allocate workspace for box list */
  boxlist = (boxptr) (*cinfo->emethods->alloc_small) (desired * SIZEOF(box));
  /* Initialize one box containing whole space */
  numboxes = 1;
  boxlist[0].c0min = 0;
  boxlist[0].c0max = MAXJSAMPLE >> Y_SHIFT;
  boxlist[0].c1min = 0;
  boxlist[0].c1max = MAXJSAMPLE >> C_SHIFT;
  boxlist[0].c2min = 0;
  boxlist[0].c2max = MAXJSAMPLE >> C_SHIFT;
  /* Shrink it to actually-used volume and set its statistics */
  update_box(& boxlist[0]);
  /* Perform median-cut to produce final box list */
  median_cut(desired);
  /* Compute the representative color for each box, fill my_colormap[] */
  for (i = 0; i < numboxes; i++)
    compute_color(& boxlist[i], i);
  cinfo->actual_number_of_colors = numboxes;
  /* Produce an output colormap in the desired output colorspace */
  remap_colormap(cinfo);
  TRACEMS1(cinfo->emethods, 1, "Selected %d colors for quantization",
	   numboxes);
  /* Done with the box list */
  (*cinfo->emethods->free_small) ((void *) boxlist);
}


/*
 * These routines are concerned with the time-critical task of mapping input
 * colors to the nearest color in the selected colormap.
 *
 * We re-use the histogram space as an "inverse color map", essentially a
 * cache for the results of nearest-color searches.  All colors within a
 * histogram cell will be mapped to the same colormap entry, namely the one
 * closest to the cell's center.  This may not be quite the closest entry to
 * the actual input color, but it's almost as good.  A zero in the cache
 * indicates we haven't found the nearest color for that cell yet; the array
 * is cleared to zeroes before starting the mapping pass.  When we find the
 * nearest color for a cell, its colormap index plus one is recorded in the
 * cache for future use.  The pass2 scanning routines call fill_inverse_cmap
 * when they need to use an unfilled entry in the cache.
 *
 * Our method of efficiently finding nearest colors is based on the "locally
 * sorted search" idea described by Heckbert and on the incremental distance
 * calculation described by Spencer W. Thomas in chapter III.1 of Graphics
 * Gems II (James Arvo, ed.  Academic Press, 1991).  Thomas points out that
 * the distances from a given colormap entry to each cell of the histogram can
 * be computed quickly using an incremental method: the differences between
 * distances to adjacent cells themselves differ by a constant.  This allows a
 * fairly fast implementation of the "brute force" approach of computing the
 * distance from every colormap entry to every histogram cell.  Unfortunately,
 * it needs a work array to hold the best-distance-so-far for each histogram
 * cell (because the inner loop has to be over cells, not colormap entries).
 * The work array elements have to be INT32s, so the work array would need
 * 256Kb at our recommended precision.  This is not feasible in DOS machines.
 * Another disadvantage of the brute force approach is that it computes
 * distances to every cell of the cubical histogram.  When working with YCbCr
 * input, only about a quarter of the cube represents realizable colors, so
 * many of the cells will never be used and filling them is wasted effort.
 *
 * To get around these problems, we apply Thomas' method to compute the
 * nearest colors for only the cells within a small subbox of the histogram.
 * The work array need be only as big as the subbox, so the memory usage
 * problem is solved.  A subbox is processed only when some cell in it is
 * referenced by the pass2 routines, so we will never bother with cells far
 * outside the realizable color volume.  An additional advantage of this
 * approach is that we can apply Heckbert's locality criterion to quickly
 * eliminate colormap entries that are far away from the subbox; typically
 * three-fourths of the colormap entries are rejected by Heckbert's criterion,
 * and we need not compute their distances to individual cells in the subbox.
 * The speed of this approach is heavily influenced by the subbox size: too
 * small means too much overhead, too big loses because Heckbert's criterion
 * can't eliminate as many colormap entries.  Empirically the best subbox
 * size seems to be about 1/512th of the histogram (1/8th in each direction).
 *
 * Thomas' article also describes a refined method which is asymptotically
 * faster than the brute-force method, but it is also far more complex and
 * cannot efficiently be applied to small subboxes.  It is therefore not
 * useful for programs intended to be portable to DOS machines.  On machines
 * with plenty of memory, filling the whole histogram in one shot with Thomas'
 * refined method might be faster than the present code --- but then again,
 * it might not be any faster, and it's certainly more complicated.
 */


#ifndef BOX_Y_LOG		/* so you can override from Makefile */
#define BOX_Y_LOG  (HIST_Y_BITS-3) /* log2(hist cells in update box, Y axis) */
#endif
#ifndef BOX_C_LOG		/* so you can override from Makefile */
#define BOX_C_LOG  (HIST_C_BITS-3) /* log2(hist cells in update box, C axes) */
#endif

#define BOX_Y_ELEMS  (1<<BOX_Y_LOG) /* # of hist cells in update box */
#define BOX_C_ELEMS  (1<<BOX_C_LOG)

#define BOX_Y_SHIFT  (Y_SHIFT + BOX_Y_LOG)
#define BOX_C_SHIFT  (C_SHIFT + BOX_C_LOG)


/*
 * The next three routines implement inverse colormap filling.  They could
 * all be folded into one big routine, but splitting them up this way saves
 * some stack space (the mindist[] and bestdist[] arrays need not coexist)
 * and may allow some compilers to produce better code by registerizing more
 * inner-loop variables.
 */

LOCAL int
find_nearby_colors (decompress_info_ptr cinfo, int minc0, int minc1, int minc2,
		    JSAMPLE colorlist[])
/* Locate the colormap entries close enough to an update box to be candidates
 * for the nearest entry to some cell(s) in the update box.  The update box
 * is specified by the center coordinates of its first cell.  The number of
 * candidate colormap entries is returned, and their colormap indexes are
 * placed in colorlist[].
 * This routine uses Heckbert's "locally sorted search" criterion to select
 * the colors that need further consideration.
 */
{
  int numcolors = cinfo->actual_number_of_colors;
  int maxc0, maxc1, maxc2;
  int centerc0, centerc1, centerc2;
  int i, x, ncolors;
  INT32 minmaxdist, min_dist, max_dist, tdist;
  INT32 mindist[MAXNUMCOLORS];	/* min distance to colormap entry i */

  /* Compute true coordinates of update box's upper corner and center.
   * Actually we compute the coordinates of the center of the upper-corner
   * histogram cell, which are the upper bounds of the volume we care about.
   * Note that since ">>" rounds down, the "center" values may be closer to
   * min than to max; hence comparisons to them must be "<=", not "<".
   */
  maxc0 = minc0 + ((1 << BOX_Y_SHIFT) - (1 << Y_SHIFT));
  centerc0 = (minc0 + maxc0) >> 1;
  maxc1 = minc1 + ((1 << BOX_C_SHIFT) - (1 << C_SHIFT));
  centerc1 = (minc1 + maxc1) >> 1;
  maxc2 = minc2 + ((1 << BOX_C_SHIFT) - (1 << C_SHIFT));
  centerc2 = (minc2 + maxc2) >> 1;

  /* For each color in colormap, find:
   *  1. its minimum squared-distance to any point in the update box
   *     (zero if color is within update box);
   *  2. its maximum squared-distance to any point in the update box.
   * Both of these can be found by considering only the corners of the box.
   * We save the minimum distance for each color in mindist[];
   * only the smallest maximum distance is of interest.
   * Note we have to scale Y to get correct distance in scaled space.
   */
  minmaxdist = 0x7FFFFFFFL;

  for (i = 0; i < numcolors; i++) {
    /* We compute the squared-c0-distance term, then add in the other two. */
    x = GETJSAMPLE(my_colormap[0][i]);
    if (x < minc0) {
      tdist = (x - minc0) * Y_SCALE;
      min_dist = tdist*tdist;
      tdist = (x - maxc0) * Y_SCALE;
      max_dist = tdist*tdist;
    } else if (x > maxc0) {
      tdist = (x - maxc0) * Y_SCALE;
      min_dist = tdist*tdist;
      tdist = (x - minc0) * Y_SCALE;
      max_dist = tdist*tdist;
    } else {
      /* within cell range so no contribution to min_dist */
      min_dist = 0;
      if (x <= centerc0) {
	tdist = (x - maxc0) * Y_SCALE;
	max_dist = tdist*tdist;
      } else {
	tdist = (x - minc0) * Y_SCALE;
	max_dist = tdist*tdist;
      }
    }

    x = GETJSAMPLE(my_colormap[1][i]);
    if (x < minc1) {
      tdist = x - minc1;
      min_dist += tdist*tdist;
      tdist = x - maxc1;
      max_dist += tdist*tdist;
    } else if (x > maxc1) {
      tdist = x - maxc1;
      min_dist += tdist*tdist;
      tdist = x - minc1;
      max_dist += tdist*tdist;
    } else {
      /* within cell range so no contribution to min_dist */
      if (x <= centerc1) {
	tdist = x - maxc1;
	max_dist += tdist*tdist;
      } else {
	tdist = x - minc1;
	max_dist += tdist*tdist;
      }
    }

    x = GETJSAMPLE(my_colormap[2][i]);
    if (x < minc2) {
      tdist = x - minc2;
      min_dist += tdist*tdist;
      tdist = x - maxc2;
      max_dist += tdist*tdist;
    } else if (x > maxc2) {
      tdist = x - maxc2;
      min_dist += tdist*tdist;
      tdist = x - minc2;
      max_dist += tdist*tdist;
    } else {
      /* within cell range so no contribution to min_dist */
      if (x <= centerc2) {
	tdist = x - maxc2;
	max_dist += tdist*tdist;
      } else {
	tdist = x - minc2;
	max_dist += tdist*tdist;
      }
    }

    mindist[i] = min_dist;	/* save away the results */
    if (max_dist < minmaxdist)
      minmaxdist = max_dist;
  }

  /* Now we know that no cell in the update box is more than minmaxdist
   * away from some colormap entry.  Therefore, only colors that are
   * within minmaxdist of some part of the box need be considered.
   */
  ncolors = 0;
  for (i = 0; i < numcolors; i++) {
    if (mindist[i] <= minmaxdist)
      colorlist[ncolors++] = (JSAMPLE) i;
  }
  return ncolors;
}


LOCAL void
find_best_colors (decompress_info_ptr cinfo, int minc0, int minc1, int minc2,
		  int numcolors, JSAMPLE colorlist[], JSAMPLE bestcolor[])
/* Find the closest colormap entry for each cell in the update box,
 * given the list of candidate colors prepared by find_nearby_colors.
 * Return the indexes of the closest entries in the bestcolor[] array.
 * This routine uses Thomas' incremental distance calculation method to
 * find the distance from a colormap entry to successive cells in the box.
 */
{
  int ic0, ic1, ic2;
  int i, icolor;
  register INT32 * bptr;	/* pointer into bestdist[] array */
  JSAMPLE * cptr;		/* pointer into bestcolor[] array */
  INT32 dist0, dist1;		/* initial distance values */
  register INT32 dist2;		/* current distance in inner loop */
  INT32 xx0, xx1;		/* distance increments */
  register INT32 xx2;
  INT32 inc0, inc1, inc2;	/* initial values for increments */
  /* This array holds the distance to the nearest-so-far color for each cell */
  INT32 bestdist[BOX_Y_ELEMS * BOX_C_ELEMS * BOX_C_ELEMS];

  /* Initialize best-distance for each cell of the update box */
  bptr = bestdist;
  for (i = BOX_Y_ELEMS*BOX_C_ELEMS*BOX_C_ELEMS-1; i >= 0; i--)
    *bptr++ = 0x7FFFFFFFL;
  
  /* For each color selected by find_nearby_colors,
   * compute its distance to the center of each cell in the box.
   * If that's less than best-so-far, update best distance and color number.
   * Note we have to scale Y to get correct distance in scaled space.
   */
  
  /* Nominal steps between cell centers ("x" in Thomas article) */
#define STEP_Y  ((1 << Y_SHIFT) * Y_SCALE)
#define STEP_C  (1 << C_SHIFT)
  
  for (i = 0; i < numcolors; i++) {
    icolor = GETJSAMPLE(colorlist[i]);
    /* Compute (square of) distance from minc0/c1/c2 to this color */
    inc0 = (minc0 - (int) GETJSAMPLE(my_colormap[0][icolor])) * Y_SCALE;
    dist0 = inc0*inc0;
    inc1 = minc1 - (int) GETJSAMPLE(my_colormap[1][icolor]);
    dist0 += inc1*inc1;
    inc2 = minc2 - (int) GETJSAMPLE(my_colormap[2][icolor]);
    dist0 += inc2*inc2;
    /* Form the initial difference increments */
    inc0 = inc0 * (2 * STEP_Y) + STEP_Y * STEP_Y;
    inc1 = inc1 * (2 * STEP_C) + STEP_C * STEP_C;
    inc2 = inc2 * (2 * STEP_C) + STEP_C * STEP_C;
    /* Now loop over all cells in box, updating distance per Thomas method */
    bptr = bestdist;
    cptr = bestcolor;
    xx0 = inc0;
    for (ic0 = BOX_Y_ELEMS-1; ic0 >= 0; ic0--) {
      dist1 = dist0;
      xx1 = inc1;
      for (ic1 = BOX_C_ELEMS-1; ic1 >= 0; ic1--) {
	dist2 = dist1;
	xx2 = inc2;
	for (ic2 = BOX_C_ELEMS-1; ic2 >= 0; ic2--) {
	  if (dist2 < *bptr) {
	    *bptr = dist2;
	    *cptr = (JSAMPLE) icolor;
	  }
	  dist2 += xx2;
	  xx2 += 2 * STEP_C * STEP_C;
	  bptr++;
	  cptr++;
	}
	dist1 += xx1;
	xx1 += 2 * STEP_C * STEP_C;
      }
      dist0 += xx0;
      xx0 += 2 * STEP_Y * STEP_Y;
    }
  }
}


LOCAL void
fill_inverse_cmap (decompress_info_ptr cinfo, int c0, int c1, int c2)
/* Fill the inverse-colormap entries in the update box that contains */
/* histogram cell c0/c1/c2.  (Only that one cell MUST be filled, but */
/* we can fill as many others as we wish.) */
{
  int minc0, minc1, minc2;	/* lower left corner of update box */
  int ic0, ic1, ic2;
  register JSAMPLE * cptr;	/* pointer into bestcolor[] array */
  register histptr cachep;	/* pointer into main cache array */
  /* This array lists the candidate colormap indexes. */
  JSAMPLE colorlist[MAXNUMCOLORS];
  int numcolors;		/* number of candidate colors */
  /* This array holds the actually closest colormap index for each cell. */
  JSAMPLE bestcolor[BOX_Y_ELEMS * BOX_C_ELEMS * BOX_C_ELEMS];

  /* Convert cell coordinates to update box ID */
  c0 >>= BOX_Y_LOG;
  c1 >>= BOX_C_LOG;
  c2 >>= BOX_C_LOG;

  /* Compute true coordinates of update box's origin corner.
   * Actually we compute the coordinates of the center of the corner
   * histogram cell, which are the lower bounds of the volume we care about.
   */
  minc0 = (c0 << BOX_Y_SHIFT) + ((1 << Y_SHIFT) >> 1);
  minc1 = (c1 << BOX_C_SHIFT) + ((1 << C_SHIFT) >> 1);
  minc2 = (c2 << BOX_C_SHIFT) + ((1 << C_SHIFT) >> 1);
  
  /* Determine which colormap entries are close enough to be candidates
   * for the nearest entry to some cell in the update box.
   */
  numcolors = find_nearby_colors(cinfo, minc0, minc1, minc2, colorlist);

  /* Determine the actually nearest colors. */
  find_best_colors(cinfo, minc0, minc1, minc2, numcolors, colorlist,
		   bestcolor);

  /* Save the best color numbers (plus 1) in the main cache array */
  c0 <<= BOX_Y_LOG;		/* convert ID back to base cell indexes */
  c1 <<= BOX_C_LOG;
  c2 <<= BOX_C_LOG;
  cptr = bestcolor;
  for (ic0 = 0; ic0 < BOX_Y_ELEMS; ic0++) {
    for (ic1 = 0; ic1 < BOX_C_ELEMS; ic1++) {
      cachep = & histogram[c0+ic0][c1+ic1][c2];
      for (ic2 = 0; ic2 < BOX_C_ELEMS; ic2++) {
	*cachep++ = (histcell) (GETJSAMPLE(*cptr++) + 1);
      }
    }
  }
}


/*
 * These routines perform second-pass scanning of the image: map each pixel to
 * the proper colormap index, and output the indexes to the output file.
 *
 * output_workspace is a one-component array of pixel dimensions at least
 * as large as the input image strip; it can be used to hold the converted
 * pixels' colormap indexes.
 */

METHODDEF void
pass2_nodither (decompress_info_ptr cinfo, int num_rows,
		JSAMPIMAGE image_data, JSAMPARRAY output_workspace)
/* This version performs no dithering */
{
  register JSAMPROW ptr0, ptr1, ptr2, outptr;
  register histptr cachep;
  register int c0, c1, c2;
  int row;
  long col;
  long width = cinfo->image_width;

  /* Convert data to colormap indexes, which we save in output_workspace */
  for (row = 0; row < num_rows; row++) {
    ptr0 = image_data[0][row];
    ptr1 = image_data[1][row];
    ptr2 = image_data[2][row];
    outptr = output_workspace[row];
    for (col = width; col > 0; col--) {
      /* get pixel value and index into the cache */
      c0 = GETJSAMPLE(*ptr0++) >> Y_SHIFT;
      c1 = GETJSAMPLE(*ptr1++) >> C_SHIFT;
      c2 = GETJSAMPLE(*ptr2++) >> C_SHIFT;
      cachep = & histogram[c0][c1][c2];
      /* If we have not seen this color before, find nearest colormap entry */
      /* and update the cache */
      if (*cachep == 0)
	fill_inverse_cmap(cinfo, c0,c1,c2);
      /* Now emit the colormap index for this cell */
      *outptr++ = (JSAMPLE) (*cachep - 1);
    }
  }
  /* Emit converted rows to the output file */
  (*cinfo->methods->put_pixel_rows) (cinfo, num_rows, &output_workspace);
}


/* Declarations for Floyd-Steinberg dithering.
 *
 * Errors are accumulated into the array fserrors[], at a resolution of
 * 1/16th of a pixel count.  The error at a given pixel is propagated
 * to its not-yet-processed neighbors using the standard F-S fractions,
 *		...	(here)	7/16
 *		3/16	5/16	1/16
 * We work left-to-right on even rows, right-to-left on odd rows.
 *
 * We can get away with a single array (holding one row's worth of errors)
 * by using it to store the current row's errors at pixel columns not yet
 * processed, but the next row's errors at columns already processed.  We
 * need only a few extra variables to hold the errors immediately around the
 * current column.  (If we are lucky, those variables are in registers, but
 * even if not, they're probably cheaper to access than array elements are.)
 *
 * The fserrors[] array has (#columns + 2) entries; the extra entry at
 * each end saves us from special-casing the first and last pixels.
 * Each entry is three values long, one value for each color component.
 *
 * Note: on a wide image, we might not have enough room in a PC's near data
 * segment to hold the error array; so it is allocated with alloc_medium.
 */

#ifdef EIGHT_BIT_SAMPLES
typedef INT16 FSERROR;		/* 16 bits should be enough */
typedef int LOCFSERROR;		/* use 'int' for calculation temps */
#else
typedef INT32 FSERROR;		/* may need more than 16 bits */
typedef INT32 LOCFSERROR;	/* be sure calculation temps are big enough */
#endif

typedef FSERROR FAR *FSERRPTR;	/* pointer to error array (in FAR storage!) */

static FSERRPTR fserrors;	/* accumulated errors */
static boolean on_odd_row;	/* flag to remember which row we are on */


METHODDEF void
pass2_dither (decompress_info_ptr cinfo, int num_rows,
	      JSAMPIMAGE image_data, JSAMPARRAY output_workspace)
/* This version performs Floyd-Steinberg dithering */
{
  register LOCFSERROR cur0, cur1, cur2;	/* current error or pixel value */
  LOCFSERROR belowerr0, belowerr1, belowerr2; /* error for pixel below cur */
  LOCFSERROR bpreverr0, bpreverr1, bpreverr2; /* error for below/prev col */
  register FSERRPTR errorptr;	/* => fserrors[] at column before current */
  JSAMPROW ptr0, ptr1, ptr2;	/* => current input pixel */
  JSAMPROW outptr;		/* => current output pixel */
  histptr cachep;
  int dir;			/* +1 or -1 depending on direction */
  int dir3;			/* 3*dir, for advancing errorptr */
  int row;
  long col;
  long width = cinfo->image_width;
  JSAMPLE *range_limit = cinfo->sample_range_limit;
  JSAMPROW colormap0 = my_colormap[0];
  JSAMPROW colormap1 = my_colormap[1];
  JSAMPROW colormap2 = my_colormap[2];
  SHIFT_TEMPS

  /* Convert data to colormap indexes, which we save in output_workspace */
  for (row = 0; row < num_rows; row++) {
    ptr0 = image_data[0][row];
    ptr1 = image_data[1][row];
    ptr2 = image_data[2][row];
    outptr = output_workspace[row];
    if (on_odd_row) {
      /* work right to left in this row */
      ptr0 += width - 1;	/* so point to rightmost pixel */
      ptr1 += width - 1;
      ptr2 += width - 1;
      outptr += width - 1;
      dir = -1;
      dir3 = -3;
      errorptr = fserrors + (width+1)*3; /* point to entry after last column */
      on_odd_row = FALSE;	/* flip for next time */
    } else {
      /* work left to right in this row */
      dir = 1;
      dir3 = 3;
      errorptr = fserrors;	/* point to entry before first real column */
      on_odd_row = TRUE;	/* flip for next time */
    }
    /* Preset error values: no error propagated to first pixel from left */
    cur0 = cur1 = cur2 = 0;
    /* and no error propagated to row below yet */
    belowerr0 = belowerr1 = belowerr2 = 0;
    bpreverr0 = bpreverr1 = bpreverr2 = 0;

    for (col = width; col > 0; col--) {
      /* curN holds the error propagated from the previous pixel on the
       * current line.  Add the error propagated from the previous line
       * to form the complete error correction term for this pixel, and
       * round the error term (which is expressed * 16) to an integer.
       * RIGHT_SHIFT rounds towards minus infinity, so adding 8 is correct
       * for either sign of the error value.
       * Note: errorptr points to *previous* column's array entry.
       */
      cur0 = RIGHT_SHIFT(cur0 + errorptr[dir3+0] + 8, 4);
      cur1 = RIGHT_SHIFT(cur1 + errorptr[dir3+1] + 8, 4);
      cur2 = RIGHT_SHIFT(cur2 + errorptr[dir3+2] + 8, 4);
      /* Form pixel value + error, and range-limit to 0..MAXJSAMPLE.
       * The maximum error is +- MAXJSAMPLE; this sets the required size
       * of the range_limit array.
       */
      cur0 += GETJSAMPLE(*ptr0);
      cur1 += GETJSAMPLE(*ptr1);
      cur2 += GETJSAMPLE(*ptr2);
      cur0 = GETJSAMPLE(range_limit[cur0]);
      cur1 = GETJSAMPLE(range_limit[cur1]);
      cur2 = GETJSAMPLE(range_limit[cur2]);
      /* Index into the cache with adjusted pixel value */
      cachep = & histogram[cur0 >> Y_SHIFT][cur1 >> C_SHIFT][cur2 >> C_SHIFT];
      /* If we have not seen this color before, find nearest colormap */
      /* entry and update the cache */
      if (*cachep == 0)
	fill_inverse_cmap(cinfo, cur0>>Y_SHIFT, cur1>>C_SHIFT, cur2>>C_SHIFT);
      /* Now emit the colormap index for this cell */
      { register int pixcode = *cachep - 1;
	*outptr = (JSAMPLE) pixcode;
	/* Compute representation error for this pixel */
	cur0 -= GETJSAMPLE(colormap0[pixcode]);
	cur1 -= GETJSAMPLE(colormap1[pixcode]);
	cur2 -= GETJSAMPLE(colormap2[pixcode]);
      }
      /* Compute error fractions to be propagated to adjacent pixels.
       * Add these into the running sums, and simultaneously shift the
       * next-line error sums left by 1 column.
       */
      { register LOCFSERROR bnexterr, delta;

	bnexterr = cur0;	/* Process component 0 */
	delta = cur0 * 2;
	cur0 += delta;		/* form error * 3 */
	errorptr[0] = (FSERROR) (bpreverr0 + cur0);
	cur0 += delta;		/* form error * 5 */
	bpreverr0 = belowerr0 + cur0;
	belowerr0 = bnexterr;
	cur0 += delta;		/* form error * 7 */
	bnexterr = cur1;	/* Process component 1 */
	delta = cur1 * 2;
	cur1 += delta;		/* form error * 3 */
	errorptr[1] = (FSERROR) (bpreverr1 + cur1);
	cur1 += delta;		/* form error * 5 */
	bpreverr1 = belowerr1 + cur1;
	belowerr1 = bnexterr;
	cur1 += delta;		/* form error * 7 */
	bnexterr = cur2;	/* Process component 2 */
	delta = cur2 * 2;
	cur2 += delta;		/* form error * 3 */
	errorptr[2] = (FSERROR) (bpreverr2 + cur2);
	cur2 += delta;		/* form error * 5 */
	bpreverr2 = belowerr2 + cur2;
	belowerr2 = bnexterr;
	cur2 += delta;		/* form error * 7 */
      }
      /* At this point curN contains the 7/16 error value to be propagated
       * to the next pixel on the current line, and all the errors for the
       * next line have been shifted over.  We are therefore ready to move on.
       */
      ptr0 += dir;		/* Advance pixel pointers to next column */
      ptr1 += dir;
      ptr2 += dir;
      outptr += dir;
      errorptr += dir3;		/* advance errorptr to current column */
    }
    /* Post-loop cleanup: we must unload the final error values into the
     * final fserrors[] entry.  Note we need not unload belowerrN because
     * it is for the dummy column before or after the actual array.
     */
    errorptr[0] = (FSERROR) bpreverr0; /* unload prev errs into array */
    errorptr[1] = (FSERROR) bpreverr1;
    errorptr[2] = (FSERROR) bpreverr2;
  }
  /* Emit converted rows to the output file */
  (*cinfo->methods->put_pixel_rows) (cinfo, num_rows, &output_workspace);
}


/*
 * Initialize for two-pass color quantization.
 */

METHODDEF void
color_quant_init (decompress_info_ptr cinfo)
{
  int i;

  /* Lower bound on # of colors ... somewhat arbitrary as long as > 0 */
  if (cinfo->desired_number_of_colors < 8)
    ERREXIT(cinfo->emethods, "Cannot request less than 8 quantized colors");
  /* Make sure colormap indexes can be represented by JSAMPLEs */
  if (cinfo->desired_number_of_colors > MAXNUMCOLORS)
    ERREXIT1(cinfo->emethods, "Cannot request more than %d quantized colors",
	     MAXNUMCOLORS);

  /* Allocate and zero the histogram */
  histogram = (hist3d) (*cinfo->emethods->alloc_small)
				(HIST_Y_ELEMS * SIZEOF(hist2d));
  for (i = 0; i < HIST_Y_ELEMS; i++) {
    histogram[i] = (hist2d) (*cinfo->emethods->alloc_medium)
				(HIST_C_ELEMS*HIST_C_ELEMS * SIZEOF(histcell));
    jzero_far((void FAR *) histogram[i],
	      HIST_C_ELEMS*HIST_C_ELEMS * SIZEOF(histcell));
  }

  /* Allocate storage for the internal and external colormaps. */
  /* We do this now since it is FAR storage and may affect the memory */
  /* manager's space calculations. */
  my_colormap = (*cinfo->emethods->alloc_small_sarray)
			((long) cinfo->desired_number_of_colors,
			 (long) 3);
  cinfo->colormap = (*cinfo->emethods->alloc_small_sarray)
			((long) cinfo->desired_number_of_colors,
			 (long) cinfo->color_out_comps);

  /* Allocate Floyd-Steinberg workspace if necessary */
  /* This isn't needed until pass 2, but again it is FAR storage. */
  if (cinfo->use_dithering) {
    size_t arraysize = (size_t) ((cinfo->image_width + 2L) *
				 (3 * SIZEOF(FSERROR)));

    fserrors = (FSERRPTR) (*cinfo->emethods->alloc_medium) (arraysize);
    /* Initialize the propagated errors to zero. */
    jzero_far((void FAR *) fserrors, arraysize);
    on_odd_row = FALSE;
  }

  /* Indicate number of passes needed, excluding the prescan pass. */
  cinfo->total_passes++;	/* I always use one pass */
}


/*
 * Perform two-pass quantization: rescan the image data and output the
 * converted data via put_color_map and put_pixel_rows.
 * The source_method is a routine that can scan the image data; it can
 * be called as many times as desired.  The processing routine called by
 * source_method has the same interface as color_quantize does in the
 * one-pass case, except it must call put_pixel_rows itself.  (This allows
 * me to use multiple passes in which earlier passes don't output anything.)
 */

METHODDEF void
color_quant_doit (decompress_info_ptr cinfo, quantize_caller_ptr source_method)
{
  int i;

  /* Select the representative colors */
  select_colors(cinfo);
  /* Pass the external colormap to the output module. */
  /* NB: the output module may continue to use the colormap until shutdown. */
  (*cinfo->methods->put_color_map) (cinfo, cinfo->actual_number_of_colors,
				    cinfo->colormap);
  /* Re-zero the histogram so pass 2 can use it as nearest-color cache */
  for (i = 0; i < HIST_Y_ELEMS; i++) {
    jzero_far((void FAR *) histogram[i],
	      HIST_C_ELEMS*HIST_C_ELEMS * SIZEOF(histcell));
  }
  /* Perform pass 2 */
  if (cinfo->use_dithering)
    (*source_method) (cinfo, pass2_dither);
  else
    (*source_method) (cinfo, pass2_nodither);
}


/*
 * Finish up at the end of the file.
 */

METHODDEF void
color_quant_term (decompress_info_ptr cinfo)
{
  /* no work (we let free_all release the histogram/cache and colormaps) */
  /* Note that we *mustn't* free the external colormap before free_all, */
  /* since output module may use it! */
}


/*
 * Map some rows of pixels to the output colormapped representation.
 * Not used in two-pass case.
 */

METHODDEF void
color_quantize (decompress_info_ptr cinfo, int num_rows,
		JSAMPIMAGE input_data, JSAMPARRAY output_data)
{
  ERREXIT(cinfo->emethods, "Should not get here!");
}


/*
 * The method selection routine for 2-pass color quantization.
 */

GLOBAL void
jsel2quantize (decompress_info_ptr cinfo)
{
  if (cinfo->two_pass_quantize) {
    /* Make sure jdmaster didn't give me a case I can't handle */
    if (cinfo->num_components != 3 || cinfo->jpeg_color_space != CS_YCbCr)
      ERREXIT(cinfo->emethods, "2-pass quantization only handles YCbCr input");
    cinfo->methods->color_quant_init = color_quant_init;
    cinfo->methods->color_quant_prescan = color_quant_prescan;
    cinfo->methods->color_quant_doit = color_quant_doit;
    cinfo->methods->color_quant_term = color_quant_term;
    cinfo->methods->color_quantize = color_quantize;
    /* Quantized grayscale output is normally done by jquant1.c (which will do
     * a much better job of it).  But if the program is configured with only
     * 2-pass quantization, then I have to do the job.  In this situation,
     * jseldcolor's clearing of the Cb/Cr component_needed flags is incorrect,
     * because I will look at those components before conversion.
     */
    cinfo->cur_comp_info[1]->component_needed = TRUE;
    cinfo->cur_comp_info[2]->component_needed = TRUE;
  }
}

#endif /* QUANT_2PASS_SUPPORTED */
