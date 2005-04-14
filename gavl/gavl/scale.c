
#include <stdlib.h>
#include <math.h>

#include <gavl/gavl.h>
#include <scale.h>

static void dump_coeffs(gavl_video_scale_coeff_t * coeffs, int num)
  {
  
  }
                                                           

static void alloc_coeffs(gavl_video_scale_coeff_t ** coeffs,
                         int * coeffs_alloc, int num)
  {
  if(*coeffs_alloc < num)
    {
    *coeffs_alloc += 128;
    *coeffs = realloc(*coeffs, *coeffs_alloc * sizeof(**coeffs));
    }
  }

static void init_coeffs_nearest(gavl_video_scale_coeff_t ** coeffs,
                                int * coeffs_alloc,
                                int src_size, int dst_size)
  {
  int i;
  for(i = 0; i < dst_size; i++)
    {
    
    }
  }

static void init_coeffs_bilinear(gavl_video_scale_coeff_t ** coeffs,
                                 int * coeffs_alloc,
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
      (*coeffs)[i].index[0] = 0;
      (*coeffs)[i].index[1] = 0;
      (*coeffs)[i].factor[0] = factor_max;
      (*coeffs)[i].factor[1] = 0;
      }
    return;
    }
  
  for(i = 0; i < dst_size; i++)
    {
    dst_index_f = (double)i + 0.5;
    src_index_1_f = dst_index_f / scale_factor;

    //    printf("dst_index_f: %f src_index_1_f: %f\n", dst_index_f, 
    //           src_index_1_f);

    (*coeffs)[i].index[0] = (int)(floor(src_index_1_f-0.5));
    
    
    if((*coeffs)[i].index[0] < 0)
      {
      (*coeffs)[i].index[0]++;
      factor_2 = 0.0;
      }
    else if((*coeffs)[i].index[0] >= src_size-1)
      {
      (*coeffs)[i].index[0]--;
      factor_2 = 1.0;
      }
    else
      factor_2 = (src_index_1_f - (*coeffs)[i].index[0]) - 0.5;

    (*coeffs)[i].index[1] = (*coeffs)[i].index[0] + 1;
    
    (*coeffs)[i].factor[1] = (int)((float)(factor_max) * factor_2 + 0.5);
    if((*coeffs)[i].factor[1] > factor_max)
      (*coeffs)[i].factor[1] = factor_max;

    (*coeffs)[i].factor[0] = factor_max - (*coeffs)[i].factor[1];
    
    // factor = 1.0 - (src_index_1_f - (double)src_index_1_i);
    // printf("dst[%d] = %f * src[%d] + %f * src[%d]\n",
    // i, 1.0 - factor_2, src_index_1_i, factor_2, src_index_1_i + 1);
    }
  }

/* Initialize scale table */

static void init_scale_table(gavl_video_scale_table_t * tab,
                             gavl_scale_mode_t scale_mode,
                             gavl_colorspace_t colorspace,
                             int src_w, int src_h,
                             int dst_w, int dst_h,
                             int plane,
                             gavl_scale_funcs_t * funcs)
  {
  int i;

  switch(colorspace)
    {
    case GAVL_COLORSPACE_NONE:
      return;
      break;
    case GAVL_RGB_15:
    case GAVL_BGR_15:
      break;
    case GAVL_RGB_16:
    case GAVL_BGR_16:
      break;
    case GAVL_RGB_24:
    case GAVL_BGR_24:
      break;
    case GAVL_RGB_32:
    case GAVL_BGR_32:
      break;
    case GAVL_RGBA_32:
      break;
    case GAVL_YUY2:
      break;
    case GAVL_UYVY:
      break;
    case GAVL_YUV_420_P:
    case GAVL_YUVJ_420_P:
      break;
    case GAVL_YUV_422_P:
    case GAVL_YUVJ_422_P:
      break;
    case GAVL_YUV_444_P:
    case GAVL_YUVJ_444_P:
      break;
    case GAVL_YUV_411_P:
      break;
    case GAVL_YUV_410_P:
      break;
    }

  
  }

static void free_scale_table(gavl_video_scale_table_t * tab)
  {
  free(tab->coeffs_h);
  free(tab->coeffs_v);
  
  }

/* Public functions */

void gavl_video_scaler_destroy(gavl_video_scaler_t * s)
  {
  int i;

  gavl_video_frame_null(s->src);
  gavl_video_frame_null(s->dst);

  gavl_video_frame_destroy(s->src);
  gavl_video_frame_destroy(s->dst);

  for(i = 0; i < s->num_planes; i++)
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
  int i;
  gavl_scale_funcs_t funcs;

  int scale_x, scale_y;
  
  scaler->src_x = src_x;
  scaler->src_y = src_y;
  scaler->dst_x = dst_x;
  scaler->dst_y = dst_y;
  scaler->colorspace = colorspace;
  
  scale_x = (src_w == dst_w) ? 0 : 1;
  scale_y = (src_h == dst_h) ? 0 : 1;

  gavl_init_scale_funcs_c(&funcs,
                          scale_mode,
                          scale_x, scale_y);

  scaler->num_planes = gavl_colorspace_num_planes(colorspace);

  for(i = 0; i < scaler->num_planes; i++)
    {
    
    }

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
