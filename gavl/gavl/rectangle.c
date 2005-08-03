
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <gavl/gavl.h>

void gavl_rectangle_i_dump(const gavl_rectangle_i_t * r)
  {
  fprintf(stderr, "%dx%d+%d+%d", r->w, r->h, r->x, r->y);
  }

void gavl_rectangle_f_dump(const gavl_rectangle_f_t * r)
  {
  fprintf(stderr, "%fx%f+%f+%f", r->w, r->h, r->x, r->y);
  }

void gavl_rectangle_crop_to_format(gavl_rectangle_i_t * r, const gavl_video_format_t * format)
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

#define GAVL_MIN(x, y) (x < y ? x : y);

void gavl_rectangle_crop_to_format_noscale(gavl_rectangle_i_t * src_rect,
                                           gavl_rectangle_i_t * dst_rect,
                                           const gavl_video_format_t * src_format,
                                           const gavl_video_format_t * dst_format)
  {
  src_rect->w = GAVL_MIN(src_format->image_width,  dst_format->image_width);
  src_rect->h = GAVL_MIN(src_format->image_height, dst_format->image_height);

  dst_rect->w = src_rect->w;
  dst_rect->h = src_rect->h;

  src_rect->x = (src_format->image_width - src_rect->w) / 2;
  src_rect->y = (src_format->image_height - src_rect->h) / 2;

  dst_rect->x = (dst_format->image_width - dst_rect->w) / 2;
  dst_rect->y = (dst_format->image_height - dst_rect->h) / 2;
  
  }

#undef GAVL_MIN

/* Let a rectangle span the whole screen for format */

void gavl_rectangle_i_set_all(gavl_rectangle_i_t * r, const gavl_video_format_t * format)
  {
  r->x = 0;
  r->y = 0;
  r->w = format->image_width;
  r->h = format->image_height;
  
  }

void gavl_rectangle_f_set_all(gavl_rectangle_f_t * r, const gavl_video_format_t * format)
  {
  r->x = 0;
  r->y = 0;
  r->w = format->image_width;
  r->h = format->image_height;
  
  }

void gavl_rectangle_i_crop_left(gavl_rectangle_i_t * r, int num_pixels)
  {
  r->x += num_pixels;
  r->w -= num_pixels;
  }

void gavl_rectangle_i_crop_right(gavl_rectangle_i_t * r, int num_pixels)
  {
  r->w -= num_pixels;
  }

void gavl_rectangle_i_crop_top(gavl_rectangle_i_t * r, int num_pixels)
  {
  r->y += num_pixels;
  r->h -= num_pixels;
  }

void gavl_rectangle_i_crop_bottom(gavl_rectangle_i_t * r, int num_pixels)
  {
  r->h -= num_pixels;
  }

void gavl_rectangle_f_crop_left(gavl_rectangle_f_t * r, double num_pixels)
  {
  r->x += num_pixels;
  r->w -= num_pixels;
  }

void gavl_rectangle_f_crop_right(gavl_rectangle_f_t * r, double num_pixels)
  {
  r->w -= num_pixels;
  }

void gavl_rectangle_f_crop_top(gavl_rectangle_f_t * r, double num_pixels)
  {
  r->y += num_pixels;
  r->h -= num_pixels;
  }

void gavl_rectangle_f_crop_bottom(gavl_rectangle_f_t * r, double num_pixels)
  {
  r->h -= num_pixels;
  }

#define PADD(num, multiple) num -= num % multiple

void gavl_rectangle_i_align(gavl_rectangle_i_t * r, int h_align, int v_align)
  {
  PADD(r->x, h_align);
  PADD(r->w, h_align);

  PADD(r->y, v_align);
  PADD(r->h, v_align);
  }

void gavl_rectangle_f_copy(gavl_rectangle_f_t * dst, const gavl_rectangle_f_t * src)
  {
  memcpy(dst, src, sizeof(*dst));
  }

void gavl_rectangle_i_copy(gavl_rectangle_i_t * dst, const gavl_rectangle_i_t * src)
  {
  memcpy(dst, src, sizeof(*dst));
  }

int gavl_rectangle_i_is_empty(const gavl_rectangle_i_t * r)
  {
  return ((r->w <= 0) || (r->h <= 0)) ? 1 : 0;
  }

int gavl_rectangle_f_is_empty(const gavl_rectangle_f_t * r)
  {
  return ((r->w <= 0.0) || (r->h <= 0.0)) ? 1 : 0;
  }


/* Assuming we take src_rect from a frame in format src_format,
   calculate the optimal dst_rect in dst_format. */

void gavl_rectangle_fit_aspect(gavl_rectangle_i_t * r,
                               const gavl_video_format_t * src_format,
                               const gavl_rectangle_f_t * src_rect,
                               const gavl_video_format_t * dst_format,
                               float zoom, float squeeze)
  {
  float dst_display_aspect;
  float dst_pixel_aspect;
  float src_display_aspect;

  float squeeze_factor;

  squeeze_factor = pow(2.0, squeeze);
  
  src_display_aspect = squeeze_factor * 
    src_rect->w * (float)(src_format->pixel_width) /
    (src_rect->h * (float)(src_format->pixel_height));

  dst_pixel_aspect =
    (float)(dst_format->pixel_width) /
    (float)(dst_format->pixel_height);
  
  dst_display_aspect =
    dst_pixel_aspect * 
    (float)(dst_format->image_width) /
    (float)(dst_format->image_height);

  if(dst_display_aspect > src_display_aspect) /* Bars left and right */
    {
    //    fprintf(stderr, "Bars left and right\n");
    r->w = (int)((float)dst_format->image_height * src_display_aspect * zoom / dst_pixel_aspect + 0.5);
    r->h = (int)((float)dst_format->image_height * zoom + 0.5); 
    //    fprintf(stderr, "Bars left and right %dx%d -> %dx%d (%f, %f)\n", src_rect->w, src_rect->h, r->w, r->h,
    //            (float)(src_rect->w * src_format->pixel_width) /
    //            (float)(src_rect->h * src_format->pixel_height), dst_pixel_aspect);
    }
  else  /* Bars top and bottom */
    {
    //    fprintf(stderr, "Bars top and bottom\n");
    r->w = (int)((float)(dst_format->image_width) * zoom + 0.5);
    r->h = (int)((float)dst_format->image_width   * zoom * dst_pixel_aspect / src_display_aspect + 0.5);
    }
  r->x = (dst_format->image_width - r->w)/2;
  r->y = (dst_format->image_height - r->h)/2;
  }

