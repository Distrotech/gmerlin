
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include <gavl/gavl.h>
#include <scale.h>

// #define DUMP_TABLES

static void alloc_coeffs(gavl_video_scale_coeff_t ** coeffs,
                         int * coeffs_alloc, int num)
  {
  if(*coeffs_alloc < num)
    {
    *coeffs_alloc = num + 128;
    *coeffs = realloc(*coeffs, *coeffs_alloc * sizeof(**coeffs));
    }
  }

static void init_coeffs_nearest(gavl_video_scale_coeff_t * coeffs,
                                int src_size, int dst_size)
  {
  int i;
  double scale_factor;
  double src_index_f;
  double dst_index_f;

  scale_factor = (float)(dst_size) / (float)(src_size);
    
  for(i = 0; i < dst_size; i++)
    {
    dst_index_f = (double)i + 0.5;
    src_index_f = dst_index_f / scale_factor;
    //    fprintf(stderr, "i: %d\n", i);
    coeffs[i].index[0] = (int)(src_index_f);
    }

#ifdef DUMP_TABLES
  for(i = 0; i < dst_size; i++)
    {
    fprintf(stderr, "dst[%d] = src[%d]\n", i, coeffs[i].index[0]);
    }
#endif

  }

static void init_coeffs_bilinear(gavl_video_scale_coeff_t * coeffs,
                                 int src_size, int dst_size,
                                 unsigned int factor_max)
  {
  double scale_factor;
  double src_index_1_f;
  double dst_index_f;
  int i;
  double factor_2;
  
  scale_factor = (float)(dst_size) / (float)(src_size);

  if(src_size == 1)
    {
    for(i = 0; i < dst_size; i++)
      {
      coeffs[i].index[0] = 0;
      coeffs[i].index[1] = 0;
      coeffs[i].factor[0] = factor_max;
      coeffs[i].factor[1] = 0;
      }
    return;
    }
  
  for(i = 0; i < dst_size; i++)
    {
    dst_index_f = (double)i + 0.5;
    src_index_1_f = dst_index_f / scale_factor;

    //    printf("dst_index_f: %f src_index_1_f: %f\n", dst_index_f, 
    //           src_index_1_f);

    coeffs[i].index[0] = (int)(floor(src_index_1_f-0.5));
    
    
    if(coeffs[i].index[0] < 0)
      {
      coeffs[i].index[0]++;
      factor_2 = 0.0;
      }
    else if(coeffs[i].index[0] >= src_size-1)
      {
      coeffs[i].index[0]--;
      factor_2 = 1.0;
      }
    else
      factor_2 = (src_index_1_f - coeffs[i].index[0]) - 0.5;

    coeffs[i].index[1] = coeffs[i].index[0] + 1;
    
    coeffs[i].factor[1] = (int)((float)(factor_max) * factor_2 + 0.5);
    if(coeffs[i].factor[1] > factor_max)
      coeffs[i].factor[1] = factor_max;

    coeffs[i].factor[0] = factor_max - coeffs[i].factor[1];
    
    // factor = 1.0 - (src_index_1_f - (double)src_index_1_i);
    // printf("dst[%d] = %f * src[%d] + %f * src[%d]\n",
    // i, 1.0 - factor_2, src_index_1_i, factor_2, src_index_1_i + 1);
    }

#ifdef DUMP_TABLES
  for(i = 0; i < dst_size; i++)
    fprintf(stderr, "dst[%d] = src[%d] * %f + dst[%d] * %f\n",
            i, coeffs[i].index[0], (float)(coeffs[i].factor[0])/(float)(factor_max),
            coeffs[i].index[1], (float)(coeffs[i].factor[1])/(float)(factor_max));
#endif
  }

static void init_plane(gavl_video_scale_table_t * tab,
                       gavl_scale_mode_t scale_mode,
                       int src_w, int src_h,
                       int dst_w, int dst_h,
                       int plane, int factor_max)
  {
  /* Allocate coefficient arrays */
  tab[plane].num_coeffs_h = dst_w;
  tab[plane].num_coeffs_v = dst_h;

  if(src_w != dst_w)
    {
    alloc_coeffs(&(tab[plane].coeffs_h),
                 &(tab[plane].coeffs_h_alloc), dst_w);

    switch(scale_mode)
      {
      case GAVL_SCALE_AUTO:
      case GAVL_SCALE_NEAREST:
        init_coeffs_nearest(tab[plane].coeffs_h,
                            src_w, dst_w);
        break;
      case GAVL_SCALE_BILINEAR:
        init_coeffs_bilinear(tab[plane].coeffs_h,
                             src_w, dst_w, factor_max);
        break;
      }

    }

  
  if(src_h != dst_h)
    {
    alloc_coeffs(&(tab[plane].coeffs_v),
                 &(tab[plane].coeffs_v_alloc), dst_h);

    switch(scale_mode)
      {
      case GAVL_SCALE_AUTO:
      case GAVL_SCALE_NEAREST:
        init_coeffs_nearest(tab[plane].coeffs_v,
                            src_h, dst_h);
        break;
      case GAVL_SCALE_BILINEAR:
        init_coeffs_bilinear(tab[plane].coeffs_v,
                             src_h, dst_h, factor_max);
        break;
      }
    }
  }

/* Initialize scale table */

/* If the destination size is no required multiple of the colorspace, we'll
 * shrink the image
 */

#define PADD(num, multiple) num -= num % multiple

static void init_scale_table(gavl_video_scale_table_t * tab,
                             gavl_scale_mode_t scale_mode,
                             gavl_colorspace_t colorspace,
                             int src_w, int src_h,
                             int dst_w, int dst_h,
                             gavl_scale_funcs_t * funcs)
  {
  int factor_max = 0;
  
  switch(colorspace)
    {
    case GAVL_COLORSPACE_NONE:
      return;
      break;
    case GAVL_RGB_15:
    case GAVL_BGR_15:
      factor_max = 0xff;
      init_plane(tab,
                 scale_mode,
                 src_w, src_h,
                 dst_w, dst_h,
                 0, factor_max);
      tab[0].scanline_func = funcs->scale_15_16;
      break;
    case GAVL_RGB_16:
    case GAVL_BGR_16:
      factor_max = 0xff;
      init_plane(tab,
                 scale_mode,
                 src_w, src_h,
                 dst_w, dst_h,
                 0, factor_max);
      tab[0].scanline_func = funcs->scale_16_16;
      tab[0].src_line_1 = malloc(src_w * 4);
      tab[0].src_line_2 = malloc(src_w * 4);
      break;
    case GAVL_RGB_24:
    case GAVL_BGR_24:
      factor_max = 0xff;
      init_plane(tab,
                 scale_mode,
                 src_w, src_h,
                 dst_w, dst_h,
                 0, factor_max);
      tab[0].scanline_func = funcs->scale_24_24;
      break;
    case GAVL_RGB_32:
    case GAVL_BGR_32:
      factor_max = 0xff;
      init_plane(tab,
                 scale_mode,
                 src_w, src_h,
                 dst_w, dst_h,
                 0, factor_max);
      tab[0].scanline_func = funcs->scale_24_32;
      break;
    case GAVL_RGBA_32:
      factor_max = 0xff;
      init_plane(tab,
                 scale_mode,
                 src_w, src_h,
                 dst_w, dst_h,
                 0, factor_max);
      tab[0].scanline_func = funcs->scale_32_32;
      break;
    case GAVL_YUY2:
      PADD(dst_w, 2);
      factor_max = 0xff;
      init_plane(tab,
                 scale_mode,
                 src_w, src_h,
                 dst_w, dst_h,
                 0, factor_max);
      init_plane(tab,
                 scale_mode,
                 src_w/2, src_h,
                 dst_w/2, dst_h,
                 1, factor_max);


      tab[0].scanline_func = funcs->scale_yuy2;
      break;
    case GAVL_UYVY:
      PADD(dst_w, 2);
      factor_max = 0xff;
      init_plane(tab,
                 scale_mode,
                 src_w, src_h,
                 dst_w, dst_h,
                 0, factor_max);

      init_plane(tab,
                 scale_mode,
                 src_w/2, src_h,
                 dst_w/2, dst_h,
                 1, factor_max);
      
      tab[0].scanline_func = funcs->scale_uyvy;

      tab[0].byte_advance = 2;
      tab[1].byte_advance = 4;
      
      break;
    case GAVL_YUV_420_P:
    case GAVL_YUVJ_420_P:
      PADD(dst_w, 2);
      PADD(dst_h, 2);
      factor_max = 0xff;

      init_plane(tab,
                 scale_mode,
                 src_w, src_h,
                 dst_w, dst_h,
                 0, factor_max);

      init_plane(tab,
                 scale_mode,
                 src_w/2, src_h/2,
                 dst_w/2, dst_h/2,
                 1, factor_max);

      init_plane(tab,
                 scale_mode,
                 src_w/2, src_h/2,
                 dst_w/2, dst_h/2,
                 2, factor_max);
            
      tab[0].scanline_func = funcs->scale_8;
      tab[1].scanline_func = funcs->scale_8;
      tab[2].scanline_func = funcs->scale_8;

      tab[0].byte_advance = 1;
      tab[1].byte_advance = 1;
      tab[2].byte_advance = 1;
      break;
    case GAVL_YUV_422_P:
    case GAVL_YUVJ_422_P:
      PADD(dst_w, 2);
      factor_max = 0xff;

      init_plane(tab,
                 scale_mode,
                 src_w, src_h,
                 dst_w, dst_h,
                 0, factor_max);

      init_plane(tab,
                 scale_mode,
                 src_w/2, src_h,
                 dst_w/2, dst_h,
                 1, factor_max);

      init_plane(tab,
                 scale_mode,
                 src_w/2, src_h,
                 dst_w/2, dst_h,
                 2, factor_max);
            
      tab[0].scanline_func = funcs->scale_8;
      tab[1].scanline_func = funcs->scale_8;
      tab[2].scanline_func = funcs->scale_8;

      tab[0].byte_advance = 1;
      tab[1].byte_advance = 1;
      tab[2].byte_advance = 1;

      break;
    case GAVL_YUV_444_P:
    case GAVL_YUVJ_444_P:
      factor_max = 0xff;

      init_plane(tab,
                 scale_mode,
                 src_w, src_h,
                 dst_w, dst_h,
                 0, factor_max);

      init_plane(tab,
                 scale_mode,
                 src_w, src_h,
                 dst_w, dst_h,
                 1, factor_max);

      init_plane(tab,
                 scale_mode,
                 src_w, src_h,
                 dst_w, dst_h,
                 2, factor_max);

      tab[0].scanline_func = funcs->scale_8;
      tab[1].scanline_func = funcs->scale_8;
      tab[2].scanline_func = funcs->scale_8;
      tab[0].byte_advance = 1;
      tab[1].byte_advance = 1;
      tab[2].byte_advance = 1;
      
      break;
    case GAVL_YUV_411_P:
      PADD(dst_w, 4);
      factor_max = 0xff;

      init_plane(tab,
                 scale_mode,
                 src_w, src_h,
                 dst_w, dst_h,
                 0, factor_max);

      init_plane(tab,
                 scale_mode,
                 src_w/4, src_h,
                 dst_w/4, dst_h,
                 1, factor_max);

      init_plane(tab,
                 scale_mode,
                 src_w/4, src_h,
                 dst_w/4, dst_h,
                 2, factor_max);

      tab[0].scanline_func = funcs->scale_8;
      tab[1].scanline_func = funcs->scale_8;
      tab[2].scanline_func = funcs->scale_8;
      tab[0].byte_advance = 1;
      tab[1].byte_advance = 1;
      tab[2].byte_advance = 1;


      break;
    case GAVL_YUV_410_P:
      PADD(dst_w, 4);
      PADD(dst_h, 4);
      factor_max = 0xff;

      init_plane(tab,
                 scale_mode,
                 src_w, src_h,
                 dst_w, dst_h,
                 0, factor_max);

      init_plane(tab,
                 scale_mode,
                 src_w/4, src_h/4,
                 dst_w/4, dst_h/4,
                 1, factor_max);

      init_plane(tab,
                 scale_mode,
                 src_w/4, src_h/4,
                 dst_w/4, dst_h/4,
                 2, factor_max);

      tab[0].scanline_func = funcs->scale_8;
      tab[1].scanline_func = funcs->scale_8;
      tab[2].scanline_func = funcs->scale_8;
      tab[0].byte_advance = 1;
      tab[1].byte_advance = 1;
      tab[2].byte_advance = 1;
      break;
    }

  
  }

#define FREE(ptr) if(ptr) free(ptr);

static void free_scale_table(gavl_video_scale_table_t * tab)
  {
  FREE(tab->coeffs_h);
  FREE(tab->coeffs_v);
  FREE(tab->src_line_1);
  FREE(tab->src_line_2);
  }

#undef FREE

/* Public functions */

void gavl_video_scaler_destroy(gavl_video_scaler_t * s)
  {
  int i;

  gavl_video_frame_null(s->src);
  gavl_video_frame_null(s->dst);

  gavl_video_frame_destroy(s->src);
  gavl_video_frame_destroy(s->dst);

  for(i = 0; i < 4; i++)
    free_scale_table(&(s->table[i]));
  free(s);  
  }

gavl_video_scaler_t * gavl_video_scaler_create()
  {
  gavl_video_scaler_t * ret;
  ret = calloc(1, sizeof(*ret));

  ret->src = gavl_video_frame_create((gavl_video_format_t*)0);
  ret->dst = gavl_video_frame_create((gavl_video_format_t*)0);
  
  return ret;
  }


void gavl_video_scaler_init(gavl_video_scaler_t * scaler,
                            gavl_scale_mode_t scale_mode,
                            gavl_colorspace_t colorspace,
                            int src_x, int src_y,
                            int src_w, int src_h,
                            int dst_x, int dst_y,
                            int dst_w, int dst_h)
  {
  gavl_scale_funcs_t funcs;

  int scale_x, scale_y;
  
  scaler->src_x = src_x;
  scaler->src_y = src_y;
  scaler->dst_x = dst_x;
  scaler->dst_y = dst_y;
  scaler->colorspace = colorspace;
  
  scale_x = (src_w == dst_w) ? 0 : 1;
  scale_y = (src_h == dst_h) ? 0 : 1;

  scaler->num_planes = gavl_colorspace_num_planes(colorspace);
  
  gavl_init_scale_funcs_c(&funcs,
                          scale_mode,
                          scale_x, scale_y);

  init_scale_table(scaler->table,
                   scale_mode,
                   colorspace,
                   src_w, src_h,
                   dst_w, dst_h,
                   &funcs);
  }

void gavl_video_scaler_scale(gavl_video_scaler_t * s,
                             gavl_video_frame_t * src,
                             gavl_video_frame_t * dst)
  {
  uint8_t * dst_ptr;
  int i, j;
  
  gavl_video_frame_get_subframe(s->colorspace, src, 
                                s->src, s->src_x, s->src_y);

  gavl_video_frame_get_subframe(s->colorspace, dst, 
                                s->dst, s->dst_x, s->dst_y);
  
  for(i = 0; i < s->num_planes; i++)
    {
    dst_ptr = s->dst->planes[i];
    for(j = 0; j < s->table[i].num_coeffs_v; j++)
      {
      s->table[i].scanline_func(s, s->src->planes[i],
                                s->src->strides[i],
                                dst_ptr,
                                i, j);
      dst_ptr += s->dst->strides[i];
      }
    }
  }
