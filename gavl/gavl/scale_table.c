/*****************************************************************

  scale_table.c

  Copyright (c) 2005 by Burkhard Plaum - plaum@ipf.uni-stuttgart.de

  http://gmerlin.sourceforge.net

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111, USA.

*****************************************************************/

#include <stdlib.h>
#include <stdio.h>

#include <gavl/gavl.h>
#include <scale.h>

/*
 * Creation of the scale tables, the most ugly part.
 * The tables are for one dimension only, for 2D scaling, we'll have 2 tables.
 * Nearest neighbour and bilinear scaling routines scale in 2 dimensions at once
 * using 2 tables. Higher order scaling is done for each dimension separately.
 * 
 * We have 3 values: src_size (double), src_off (double)
 * and dst_size (int).
 *
 * The scale factor is dst_size / src_size. src_off is the coordinate of the
 * first destination pixel in source coordinates.
 *
 * When src_offset is zero, we'll have a pattern like this
 * (for upscaling by a factor of 2):
 *
 * src: | x     | x     |
 * dst: | x | x | x | x |
 * 
 * For a src_offset of -0.25, we'll have
 *
 * src: |   x   |   x   |
 * dst: | x | x | x | x |
 *
 * The table is created in the follwing steps:
 *
 * - Calculate the floating point filter coefficiens for each destination pixel
 *   
 * - Shift the indixes of the source pixels such that no source pixels are outside
 *   the image. The nonexistant pixels beyond the borders (whose values are needed
 *   for scaling) are assumed to have the same color as the nearest pixel at the
 *   image border
 *
 * - Convert the floating point coefficients to integer with the specified bit
 *   resolution requsted by the scaling routine.
 *
 */


/* Conversion between src and dst coordinates */

#define DST_TO_SRC(c) ((double)c)/scale_factor+src_off

#define ROUND(val) (val >= 0.0) ? (int)(val+0.5):(int)(val-0.5)

static void shift_borders(gavl_video_scale_table_t * tab, int src_width);

static void normalize_table(gavl_video_scale_table_t * tab);

void gavl_video_scale_table_init(gavl_video_scale_table_t * tab,
                                 gavl_video_options_t * opt,
                                 double src_off, double src_size,
                                 int dst_size, int src_width)
  {
  double t;
  int i, j, src_index_min, src_index_nearest;
  double src_index_f;
    
  gavl_video_scale_get_weight weight_func;
  
  double scale_factor;

  //  src_off = -0.25;
  
  /* Get the kernel generator */

  //  fprintf(stderr, "src_off: %f\n", src_off);
  
  weight_func = gavl_video_scale_get_weight_func(opt, &(tab->factors_per_pixel));

  //  fprintf(stderr, "tab->factors_per_pixel: %d, src_width: %d\n",
  //          tab->factors_per_pixel, src_width);
  
  if(tab->factors_per_pixel > src_width)
    {
    switch(src_width)
      {
      case 1:
        opt->scale_mode = GAVL_SCALE_NEAREST;
        //        fprintf(stderr, "gavl: Changing scale mode to nearest (image too small)\n");
        break;
      case 2:
      case 3:
        opt->scale_mode = GAVL_SCALE_BILINEAR;
        //        fprintf(stderr, "gavl: Changing scale mode to bilinear (image too small)\n");
        break;
      default:
        opt->scale_mode = GAVL_SCALE_CUBIC_BSPLINE;
        //        fprintf(stderr, "gavl: Changing scale mode to bspline (image too small)\n");
        break;
      }
    weight_func = gavl_video_scale_get_weight_func(opt, &(tab->factors_per_pixel));
    }
  
  //  fprintf(stderr, "gavl_video_scale_table_init: %f %f %d %d\n", 
  //          src_off, src_size, dst_size, tab->factors_per_pixel);
  
  /* (Re)allocate memory */

  if(tab->pixels_alloc < dst_size)
    {
    tab->pixels_alloc = dst_size + 128;
    tab->pixels = realloc(tab->pixels, tab->pixels_alloc * sizeof(*(tab->pixels)));
    }

  tab->num_pixels = dst_size;
  
  if(tab->factors_alloc < dst_size * tab->factors_per_pixel)
    {
    tab->factors_alloc =  dst_size * tab->factors_per_pixel + 128;
    tab->factors = realloc(tab->factors, tab->factors_alloc * sizeof(*(tab->factors)));
    }
  
  scale_factor = (double)(dst_size) / src_size;
  
  for(i = 0; i < dst_size; i++)
    {
    /* Set the factor pointers */
    tab->pixels[i].factor = tab->factors + i * tab->factors_per_pixel;

    /* src_index_f:       Fractional source index */
    /* src_index_nearest: Nearest integer source index */
    
    src_index_f = DST_TO_SRC((double)i);

    //    fprintf(stderr, "dst: %d -> src: %f (offset: s: %.2f)\n", i, src_index_f,
    //            src_off);
    
    //    if(src_index_f > src_size - 1.0)
    //      src_index_f = src_size - 1.0;
    
    src_index_nearest = ROUND(src_index_f);

    src_index_min = src_index_nearest - tab->factors_per_pixel/2;
    
    if(((double)src_index_nearest < src_index_f) && !(tab->factors_per_pixel % 2))
      {
      src_index_min++;
      }

    tab->pixels[i].index = src_index_min;

    //    fprintf(stderr, "src_index_f: %f, src_index_nearest: %d, src_index_min: %d, dst_index: %d\n",
    //            src_index_f, src_index_nearest, src_index_min, i);
    
    /* For nearest neighbour, we don't need any factors */
    if(tab->factors_per_pixel == 1)
      {
      if(tab->pixels[i].index < 0)
        tab->pixels[i].index = 0;
      if(tab->pixels[i].index > src_width - 1)
        tab->pixels[i].index = src_width - 1;
      continue;
      }
    
    /* Normalized distance of the destination pixel to the first source pixel
       in src coordinates */    
    t = src_index_f - src_index_min;
    
    for(j = 0; j < tab->factors_per_pixel; j++)
      {
      tab->pixels[i].factor[j].fac_f = weight_func(opt, t);
      //      fprintf(stderr, "t: %f, w: %f\n", t, weight_func(opt, t));
      t -= 1.0;
      }
    }

  //  fprintf(stderr, "Before shift\n");
  //  gavl_video_scale_table_dump(tab);
  
  shift_borders(tab, src_width);
  
  if(opt->scale_mode == GAVL_SCALE_SINC_LANCZOS)
    normalize_table(tab);
  
  //  fprintf(stderr, "After shift %d\n", src_width);
  //if(deinterlace || (total_fields == 2))
  //      gavl_video_scale_table_dump(tab);
  
  }

static void normalize_table(gavl_video_scale_table_t * tab)
  {
  int i, j;
  float sum;
  
  for(i = 0; i < tab->num_pixels; i++)
    {
    sum = 0.0;
    for(j = 0; j < tab->factors_per_pixel; j++)
      sum += tab->pixels[i].factor[j].fac_f;

    // fprintf(stderr, "sum: %f\n", sum);
    
    for(j = 0; j < tab->factors_per_pixel; j++)
      tab->pixels[i].factor[j].fac_f /= sum;
    }
  }

/* Shift the borders of the table such that no indices are out of range.
   The values in the pixels beyond the border are assumed to be the same as
   the border pixel. */

/* shift_up() for shift = 2, num = 5
   
   | f0 | f1 | f2 | f3 | f4 |

   ->

   | S  | f3 | f4 |  0 |  0 |

   with S = f0 + f1 + f2
   
*/
   
static void shift_up(gavl_video_scale_factor_t * fac, int num, int shift)
  {
  int j;
  /* Add all out-of-range coefficients up into the first
     coefficient of the new array */
  for(j = 0; j < shift; j++)
    fac[shift].fac_f += fac[j].fac_f;

  /* Move Coefficients to new locations */
  for(j = 0; j < num - shift; j++)
    fac[j].fac_f = fac[j + shift].fac_f;
  
  /* Zero unused entries */
  for(j = num - shift; j < num; j++)
    fac[j].fac_f = 0.0;
  }

/* shift_down() for shift = 2, num = 5
   
   | f0 | f1 | f2 | f3 | f4 |

   ->

   | 0  |  0 | f0 | f1 | S  |

   with S = f2 + f3 + f4
   
*/

static void shift_down(gavl_video_scale_factor_t * fac, int num, int shift)
  {
  int j;
  //  fprintf(stderr, "Shift down 1 %d\n", shift);
  /* Add all out-of-range coefficients up into the first
     coefficient of the new array */
  for(j = num - shift; j < num; j++)
    fac[num - shift - 1].fac_f += fac[j].fac_f;

  /* Move Coefficients to new locations */
  for(j = num - 1; j >= shift; j--)
    fac[j].fac_f = fac[j-shift].fac_f;
  
  /* Zero unused entries */
  for(j = 0; j < shift; j++)
    fac[j].fac_f = 0.0;
  }

static void shift_borders(gavl_video_scale_table_t * tab, int src_width)
  {
  int i, shift;
  
  for(i = 0; i < tab->num_pixels; i++)
    {
    //    fprintf(stderr, "Shift borders1 %d %d %d\n", tab->pixels[i].index,
    //            tab->factors_per_pixel, src_width);
    if(tab->pixels[i].index < 0)
      {
      shift = -tab->pixels[i].index;
      shift_up(tab->pixels[i].factor, tab->factors_per_pixel, shift);
      tab->pixels[i].index = 0;
      }

    if(tab->pixels[i].index + tab->factors_per_pixel > src_width)
      {
      shift = tab->pixels[i].index + tab->factors_per_pixel - src_width;
      shift_down(tab->pixels[i].factor, tab->factors_per_pixel, shift);
      tab->pixels[i].index = src_width - tab->factors_per_pixel;
      }
    //    fprintf(stderr, "Shift borders2 %d %d %d\n", tab->pixels[i].index,
    //            tab->factors_per_pixel, src_width);
    
    }
  
  }

void gavl_video_scale_table_dump(gavl_video_scale_table_t * tab)
  {
  int i, j;
  float sum;
  fprintf(stderr, "Scale table:\n");
  for(i = 0; i < tab->num_pixels; i++)
    {
    sum = 0.0;
    fprintf(stderr, " dst: %d", i);

    for(j = 0; j < tab->factors_per_pixel; j++)
      {
      //      fprintf(stderr, ", fac[%d]: %f [%d]", tab->pixels[i].index + j, tab->pixels[i].factor[j].fac_f,
      //              tab->pixels[i].factor[j].fac_i);

      fprintf(stderr, ", fac[%d]: %f ", tab->pixels[i].index + j, tab->pixels[i].factor[j].fac_f);
      
      sum += tab->pixels[i].factor[j].fac_f;
      }
    
    fprintf(stderr, ", sum: %f\n", sum);
    }
  }


void gavl_video_scale_table_init_int(gavl_video_scale_table_t * tab,
                                     int bits)
  {
  int fac_max_i, i, j;
  float fac_max_f;
  int sum, index;
  int min_index, max_index;
  //  fprintf(stderr, "gavl_video_scale_table_init_int: %d\n", bits);
  
  //  fac_max_i = (1<<bits) - 1;
  fac_max_i = (1<<bits);
  fac_max_f = (float)(fac_max_i);

  index = 0;
  
  for(i = 0; i < tab->num_pixels; i++)
    {
    min_index = index;
    max_index = index;
    
    sum = 0;
    for(j = 0; j < tab->factors_per_pixel; j++)
      {
      tab->factors[index].fac_i =
        (int)(fac_max_f * tab->factors[index].fac_f+0.5);
      sum += tab->factors[index].fac_i;

      if(j)
        {
        if(tab->factors[index].fac_i > tab->factors[max_index].fac_i)
          max_index = index;
        if(tab->factors[index].fac_i < tab->factors[min_index].fac_i)
          min_index = index;
        }
      index++;
      }
#if 1
    if(sum > fac_max_i)
      tab->factors[max_index].fac_i -= (sum - fac_max_i);
    else if(sum < fac_max_i)
      tab->factors[min_index].fac_i += (fac_max_i - sum);
    //    fprintf(stderr, "sum: %08x\n", sum);
#endif
    }
  }

void gavl_video_scale_table_cleanup(gavl_video_scale_table_t * tab)
  {
  if(tab->pixels)
    free(tab->pixels);
  if(tab->factors)
    free(tab->factors);
  }

void gavl_video_scale_table_get_src_indices(gavl_video_scale_table_t * tab,
                                            int * start, int * size)
  {
  *start = tab->pixels[0].index;
  *size = tab->pixels[tab->num_pixels-1].index + tab->factors_per_pixel - *start;
  }

void gavl_video_scale_table_shift_indices(gavl_video_scale_table_t * tab,
                                          int shift)
  {
  int i;
  if(!shift)
    return;
  for(i = 0; i < tab->num_pixels; i++)
    {
    tab->pixels[tab->num_pixels].index += shift;
    }
  }

