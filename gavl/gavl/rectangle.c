
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <gavl/gavl.h>

void gavl_rectangle_dump(const gavl_rectangle_t * r)
  {
  fprintf(stderr, "%dx%d+%d+%d", r->w, r->h, r->x, r->y);
  }

void gavl_rectangle_crop_to_format(gavl_rectangle_t * r, const gavl_video_format_t * format)
  {
  if(r->x < 0)
    {
    r->w += r->x;
    r->x = 0;
    }
  if(r->y < 0)
    {
    r->h += r->y;
    r->y = 0;
    }

  /* Check if rectangle is empty */
    
  if((r->x > format->image_width) || (r->y > format->image_height) ||
     (r->w < 0) || (r->h < 0))
    {
    r->x = 0;
    r->y = 0;
    r->w = 0;
    r->h = 0;
    return;
    }

  if(r->w > format->image_width - r->x)
    r->w = format->image_width - r->x;

  if(r->h > format->image_height - r->y)
    r->h = format->image_height - r->y;
  
  }

static void crop_to_format_scale(gavl_rectangle_t * src_rect,
                                 gavl_rectangle_t * dst_rect,
                                 int crop_left,
                                 int crop_right,
                                 int crop_top,
                                 int crop_bottom,
                                 double scale_factor_x, double scale_factor_y)
  {
  //fprintf(stderr, "crop_to_format_scale %d %d %d %d\n",
  //        crop_left, crop_right, crop_top, crop_bottom);
  
  if(crop_left)
    {
    src_rect->x += crop_left;
    dst_rect->x += (int)(scale_factor_x * (double)crop_left + 0.5);
    }
  if(crop_right || crop_left)
    {
    src_rect->w -= (crop_left + crop_right);
    dst_rect->w -= (int)(scale_factor_x * (double)(crop_left + crop_right) + 0.5);
    }

  if(crop_top)
    {
    src_rect->y += crop_top;
    dst_rect->y += (int)(scale_factor_y * (double)crop_top + 0.5);
    }

  if(crop_top || crop_bottom)
    {
    src_rect->h -= (crop_top + crop_bottom);
    dst_rect->h -= (int)(scale_factor_y * (double)(crop_top + crop_bottom) + 0.5);
    }
  }

void gavl_rectangle_crop_to_format_scale(gavl_rectangle_t * src_rect,
                                         gavl_rectangle_t * dst_rect,
                                         const gavl_video_format_t * src_format,
                                         const gavl_video_format_t * dst_format)
  {
  int crop_left;
  int crop_right;
  int crop_top;
  int crop_bottom;
  
  double scale_factor_x;
  double scale_factor_y;

  scale_factor_x = (double)(dst_rect->w)/(double)(src_rect->w);
  scale_factor_y = (double)(dst_rect->h)/(double)(src_rect->h);

  /* Crop to source */

  crop_left   = 0;
  crop_right  = 0;
  crop_top    = 0;
  crop_bottom = 0;
  
  if(src_rect->x < 0)
    crop_left -= src_rect->x;
  
  if(src_rect->x + src_rect->w > src_format->image_width)
    crop_right += src_rect->x + src_rect->w - src_format->image_width;

  if(src_rect->y < 0)
    crop_top -= src_rect->y;
  
  if(src_rect->y + src_rect->h > src_format->image_height)
    crop_bottom += src_rect->y + src_rect->h - src_format->image_height;

  crop_to_format_scale(src_rect,
                       dst_rect,
                       crop_left,
                       crop_right,
                       crop_top,
                       crop_bottom,
                       scale_factor_x, scale_factor_y);
  
  /* Crop to destination */

  crop_left   = 0;
  crop_right  = 0;
  crop_top    = 0;
  crop_bottom = 0;

  if(dst_rect->x < 0)
    crop_left = -dst_rect->x;
  
  if(dst_rect->x + dst_rect->w > dst_format->image_width)
    crop_right = dst_rect->x + dst_rect->w - dst_format->image_width;

  if(dst_rect->y < 0)
    crop_top = -dst_rect->y;
  
  if(dst_rect->y + dst_rect->h > dst_format->image_height)
    crop_bottom = dst_rect->y + dst_rect->h - dst_format->image_height;


  crop_to_format_scale(dst_rect,
                       src_rect,
                       crop_left,
                       crop_right,
                       crop_top,
                       crop_bottom,
                       1.0/scale_factor_x, 1.0/scale_factor_y);
  
  }


/* Let a rectangle span the whole screen for format */

void gavl_rectangle_set_all(gavl_rectangle_t * r, const gavl_video_format_t * format)
  {
  r->x = 0;
  r->y = 0;
  r->w = format->image_width;
  r->h = format->image_height;
  
  }

void gavl_rectangle_crop_left(gavl_rectangle_t * r, int num_pixels)
  {
  r->x += num_pixels;
  r->w -= num_pixels;
  }

void gavl_rectangle_crop_right(gavl_rectangle_t * r, int num_pixels)
  {
  r->w -= num_pixels;
  }

void gavl_rectangle_crop_top(gavl_rectangle_t * r, int num_pixels)
  {
  r->y += num_pixels;
  r->h -= num_pixels;
  }

void gavl_rectangle_crop_bottom(gavl_rectangle_t * r, int num_pixels)
  {
  r->h -= num_pixels;
  }

#define PADD(num, multiple) num -= num % multiple

void gavl_rectangle_align(gavl_rectangle_t * r, int h_align, int v_align)
  {
  PADD(r->x, h_align);
  PADD(r->w, h_align);

  PADD(r->y, v_align);
  PADD(r->h, v_align);
  }

void gavl_rectangle_subsample(gavl_rectangle_t * dst, const gavl_rectangle_t * src,
                              int sub_h, int sub_v)
  {
  dst->x = src->x / sub_h;
  dst->w = src->w / sub_h;

  dst->y = src->y / sub_v;
  dst->h = src->h / sub_v;
  }

void gavl_rectangle_copy(gavl_rectangle_t * dst, const gavl_rectangle_t * src)
  {
  memcpy(dst, src, sizeof(*dst));
  }

int gavl_rectangle_is_empty(const gavl_rectangle_t * r)
  {
  return ((r->w <= 0) || (r->h <= 0)) ? 1 : 0;
  }

/* Assuming we take src_rect from a frame in format src_format,
   calculate the optimal dst_rect in dst_format. */

void gavl_rectangle_fit_aspect(gavl_rectangle_t * r,
                               const gavl_video_format_t * src_format,
                               const gavl_rectangle_t * src_rect,
                               gavl_video_format_t * dst_format,
                               float zoom, float squeeze)
  {
  float dst_display_aspect;
  float dst_pixel_aspect;
  float src_display_aspect;

  float squeeze_factor;

  squeeze_factor = pow(2.0, squeeze);
  
  src_display_aspect = squeeze_factor * 
    (float)(src_rect->w * src_format->pixel_width) /
    (float)(src_rect->h * src_format->pixel_height);

  dst_pixel_aspect =
    (float)(dst_format->pixel_width) /
    (float)(dst_format->pixel_height);
  
  dst_display_aspect =
    dst_pixel_aspect * 
    (float)(dst_format->image_width) /
    (float)(dst_format->image_height);

  if(dst_display_aspect > src_display_aspect) /* Bars left and right */
    {
    r->w = (int)((float)dst_format->image_height * src_display_aspect * zoom / dst_pixel_aspect + 0.5);
    r->h = (int)((float)dst_format->image_height * zoom + 0.5); 
    }
  else  /* Bars top and bottom */
    {
    r->w = (int)((float)(dst_format->image_width) * zoom + 0.5);
    r->h = (int)((float)dst_format->image_width   * zoom * dst_pixel_aspect / src_display_aspect + 0.5);
    }
  r->x = (dst_format->image_width - r->w)/2;
  r->y = (dst_format->image_height - r->h)/2;
  }

