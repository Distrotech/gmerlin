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
  uint8_t * s1, *s2, *d;
  int i, j;
  
  void (*interpolate)(uint8_t * src_1, uint8_t * src_2, 
                      uint8_t * dst, int num, float fac) =
    (void (*)(uint8_t *, uint8_t *, uint8_t *, int, float))0;
  
  num_planes = gavl_pixelformat_num_planes(format->pixelformat);
  gavl_pixelformat_chroma_sub(format->pixelformat, &sub_h, &sub_v);
  
  width  = format->image_width;
  height = format->image_height;
  
  switch(format->pixelformat)
    {
    case GAVL_RGB_15:
    case GAVL_BGR_15:
      interpolate = ctx->funcs.interpolate_rgb15;
      break;
    case GAVL_RGB_16:
    case GAVL_BGR_16:
      interpolate = ctx->funcs.interpolate_rgb16;
      break;
    case GAVL_RGB_24:
    case GAVL_BGR_24:
      interpolate = ctx->funcs.interpolate_8;
      width *= 3;
      break;
    case GAVL_RGB_32:
    case GAVL_BGR_32:
    case GAVL_RGBA_32:
    case GAVL_YUVA_32:
      interpolate = ctx->funcs.interpolate_8;
      width *= 4;
      break;
    case GAVL_RGB_48:
      interpolate = ctx->funcs.interpolate_16;
      width *= 3;
      break;
    case GAVL_RGBA_64:
      interpolate = ctx->funcs.interpolate_16;
      width *= 4;
      break;
    case GAVL_RGB_FLOAT:
      width *= 3;
      interpolate = ctx->funcs.interpolate_f;
      break;
    case GAVL_RGBA_FLOAT:
      width *= 4;
      interpolate = ctx->funcs.interpolate_f;
      break;
    case GAVL_YUY2:
    case GAVL_UYVY:
      interpolate = ctx->funcs.interpolate_8;
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
      interpolate = ctx->funcs.interpolate_8;
      break;
    case GAVL_YUV_422_P_16:
    case GAVL_YUV_444_P_16:
      interpolate = ctx->funcs.interpolate_16;
      break;
    case GAVL_PIXELFORMAT_NONE:
      break;
    }

  for(i = 0; i < num_planes; i++)
    {
    s1 = src_1->planes[i];
    s2 = src_2->planes[i];
    d  = dst->planes[i];
    
    for(j = 0; j < height; j++)
      {
      interpolate(s1, s2, d, width, factor);
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
