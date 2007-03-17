#include <gavl/gavl.h>
#include <gavl/gavldsp.h>
#include <dsp.h>

void gavl_dsp_interpolate_video_frame(gavl_dsp_context_t * ctx,
                                      gavl_video_format_t * format,
                                      gavl_video_frame_t * src_1,
                                      gavl_video_frame_t * src_2,
                                      gavl_video_frame_t * dst,
                                      float factor)
  {
  int num_planes;
  int sub_v, sub_h;
  int width, height;
  int is_float = 0;
  uint8_t * s1, *s2, *d;
  int factor_i = 0;
  int i, j;
  
  void (*interpolate_int)(uint8_t * src_1, uint8_t * src_2, 
                          uint8_t * dst, int num, int fac);
  
  num_planes = gavl_pixelformat_num_planes(format->pixelformat);
  gavl_pixelformat_chroma_sub(format->pixelformat, &sub_h, &sub_v);
  
  width  = format->image_width;
  height = format->image_height;
  
  switch(format->pixelformat)
    {
    case GAVL_RGB_15:
    case GAVL_BGR_15:
      interpolate_int = ctx->funcs.interpolate_rgb15;
      factor_i = (int)(factor * 255.0 + 0.5);
      break;
    case GAVL_RGB_16:
    case GAVL_BGR_16:
      interpolate_int = ctx->funcs.interpolate_rgb16;
      factor_i = (int)(factor * 255.0 + 0.5);
      break;
    case GAVL_RGB_24:
    case GAVL_BGR_24:
      interpolate_int = ctx->funcs.interpolate_8;
      factor_i = (int)(factor * 255.0 + 0.5);
      width *= 3;
      break;
    case GAVL_RGB_32:
    case GAVL_BGR_32:
    case GAVL_RGBA_32:
    case GAVL_YUVA_32:
      interpolate_int = ctx->funcs.interpolate_8;
      factor_i = (int)(factor * 255.0 + 0.5);
      width *= 4;
      break;
    case GAVL_RGB_48:
      interpolate_int = ctx->funcs.interpolate_16;
      factor_i = (int)(factor * 65535.0 + 0.5);
      width *= 3;
      break;
    case GAVL_RGBA_64:
      interpolate_int = ctx->funcs.interpolate_16;
      factor_i = (int)(factor * 65535.0 + 0.5);
      width *= 4;
      break;
    case GAVL_RGB_FLOAT:
      width *= 3;
      is_float = 1;
      break;
    case GAVL_RGBA_FLOAT:
      width *= 4;
      is_float = 1;
      break;
    case GAVL_YUY2:
    case GAVL_UYVY:
      interpolate_int = ctx->funcs.interpolate_8;
      factor_i = (int)(factor * 255.0 + 0.5);
      width *= 2;
      break;
    case GAVL_YUV_420_P:
    case GAVL_YUV_410_P:
    case GAVL_YUV_411_P:
    case GAVL_YUV_444_P:
    case GAVL_YUVJ_420_P:
    case GAVL_YUVJ_422_P:
    case GAVL_YUVJ_444_P:
    case GAVL_YUV_422_P:
      interpolate_int = ctx->funcs.interpolate_8;
      factor_i = (int)(factor * 255.0 + 0.5);
      break;
    case GAVL_YUV_422_P_16:
    case GAVL_YUV_444_P_16:
      interpolate_int = ctx->funcs.interpolate_16;
      factor_i = (int)(factor * 65535.0 + 0.5);
      break;
    case GAVL_PIXELFORMAT_NONE:
      break;
    }

  if(is_float)
    {
    s1 = src_1->planes[0];
    s2 = src_2->planes[0];
    d  = dst->planes[0];
    for(j = 0; j < height; j++)
      {
      ctx->funcs.interpolate_f(s1, s2, d, width, factor);
      s1 += src_1->strides[0];
      s2 += src_2->strides[0];
      d  += dst->strides[0];
      }
    }
  else
    {
    for(i = 0; i < num_planes; i++)
      {
      s1 = src_1->planes[i];
      s2 = src_2->planes[i];
      d  = dst->planes[i];

      for(j = 0; j < height; j++)
        {
        interpolate_int(s1, s2, d, width, factor_i);
        s1 += src_1->strides[i];
        s2 += src_2->strides[i];
        d  += dst->strides[i];
        }

      if(!i)
        {
        width /= sub_h;
        height /= sub_v;
        }
      }
    }
  }
