#include <string.h>
#include <stdio.h>
#include <gavl/gavl.h>

void gavl_video_format_copy(gavl_video_format_t * dst,
                            const gavl_video_format_t * src)
  {
  memcpy(dst, src, sizeof(*dst));
  }

void gavl_video_format_dump(const gavl_video_format_t * format)
  {
  fprintf(stderr, "Frame size:   %d x %d\n",
          format->frame_width, format->frame_height);
  fprintf(stderr, "Image size:   %d x %d\n",
          format->image_width, format->image_height);
  fprintf(stderr, "Pixel size:   %d x %d\n",
          format->pixel_width, format->pixel_height);
  fprintf(stderr, "Pixel format: %s\n",
          gavl_colorspace_to_string(format->colorspace));

  if(format->framerate_mode != GAVL_FRAMERATE_STILL)
    {
    fprintf(stderr, "Framerate:    %f",
            (float)(format->timescale)/((float)format->frame_duration));
    if(format->frame_duration != 1)
      fprintf(stderr, " [%d / %d]", format->timescale,
              format->frame_duration);
    fprintf(stderr, " fps");
    
    if(format->framerate_mode == GAVL_FRAMERATE_CONSTANT)
      fprintf(stderr, " (Constant)\n");
    else
      fprintf(stderr, " (Not constant)\n");
    
    }
  else
    {
    fprintf(stderr, "Still image\n");
    }
  
  }

/* We always enlarge the image */

void gavl_video_format_fit_to_source(gavl_video_format_t * dst,
                                     const gavl_video_format_t * src)
  {
  int f_1, f_2;
  f_1 = src->pixel_width * dst->pixel_height;
  f_2 = dst->pixel_width * src->pixel_height;
  
  if(f_1 > f_2) /* Make dst wider */
    {
    dst->image_width =
      (src->image_width * f_1) / f_2;
    dst->image_height = src->image_height;
    }
  else if(f_1 < f_2) /* Make dst higher */
    {
    dst->image_height =
      (src->image_height * f_2) / f_1;
    dst->image_width = src->image_width;
    }
  else
    {
    dst->image_width = src->image_width;
    dst->image_height = src->image_height;
    }
  }
