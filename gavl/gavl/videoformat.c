#include <stdio.h>
#include <gavl/gavl.h>

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
  fprintf(stderr, "Framerate:    %f",
          (float)(format->framerate_num)/((float)format->framerate_den));

  if(format->framerate_den != 1)
    fprintf(stderr, " [%d / %d]", format->framerate_num,
            format->framerate_den);
  fprintf(stderr, " fps");

  if(!format->free_framerate)
    fprintf(stderr, " (Constant)\n");
  else
    fprintf(stderr, " (Not constant)\n");
  }
