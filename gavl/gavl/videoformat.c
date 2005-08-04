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
  fprintf(stderr, "  Frame size:       %d x %d\n",
          format->frame_width, format->frame_height);
  fprintf(stderr, "  Image size:       %d x %d\n",
          format->image_width, format->image_height);
  fprintf(stderr, "  Pixel size:       %d x %d\n",
          format->pixel_width, format->pixel_height);
  fprintf(stderr, "  Pixel format:     %s\n",
          gavl_pixelformat_to_string(format->pixelformat));

  if(format->framerate_mode != GAVL_FRAMERATE_STILL)
    {
    fprintf(stderr, "  Framerate:        %f",
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
  fprintf(stderr, "  Interlace mode:   %s\n", gavl_interlace_mode_to_string(format->interlace_mode));  

  if(format->pixelformat == GAVL_YUV_420_P)
    {
    fprintf(stderr, "  Chroma placement: %s\n", gavl_chroma_placement_to_string(format->chroma_placement));
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

typedef struct
  {
  gavl_interlace_mode_t mode;
  char * name;
  } interlace_mode_tab_t;

interlace_mode_tab_t interlace_mode_tab[] =
  {
    { GAVL_INTERLACE_NONE, "None (Progressive)" },
    { GAVL_INTERLACE_TOP_FIRST, "Top field first" },
    { GAVL_INTERLACE_BOTTOM_FIRST, "Bottom field first" }
  };

static int num_interlace_modes = sizeof(interlace_mode_tab)/sizeof(interlace_mode_tab_t);


const char * gavl_interlace_mode_to_string(gavl_interlace_mode_t mode)
  {
  int i;
  for(i = 0; i < num_interlace_modes; i++)
    {
    if(interlace_mode_tab[i].mode == mode)
      return interlace_mode_tab[i].name;
    }
  return (const char*)0;
  }


typedef struct
  {
  gavl_chroma_placement_t mode;
  char * name;
  } chroma_placement_tab_t;

chroma_placement_tab_t chroma_placement_tab[] =
  {
    { GAVL_CHROMA_PLACEMENT_DEFAULT, "MPEG-1/JPEG" },
    { GAVL_CHROMA_PLACEMENT_MPEG2, "MPEG-2" },
    { GAVL_CHROMA_PLACEMENT_DVPAL, "DV PAL" }
  };

static int num_chroma_placements = sizeof(chroma_placement_tab)/sizeof(chroma_placement_tab_t);

const char * gavl_chroma_placement_to_string(gavl_chroma_placement_t mode)
  {
  int i;
  for(i = 0; i < num_chroma_placements; i++)
    {
    if(chroma_placement_tab[i].mode == mode)
      return chroma_placement_tab[i].name;
    }
  return (const char*)0;
  
  }

void gavl_video_format_get_chroma_offset(gavl_video_format_t * format, int field, int plane, float * off_x, float * off_y)
  {
  int sub_h, sub_v;
  if(!plane)
    {
    *off_x = 0.0;
    *off_y = 0.0;
    return;
    }
  gavl_pixelformat_chroma_sub(format->pixelformat, &sub_h, &sub_v);

  if((sub_h != 2) || (sub_v != 2))
    {
    *off_x = 0.0;
    *off_y = 0.0;
    return;
    }

  switch(format->chroma_placement)
    {
    case GAVL_CHROMA_PLACEMENT_DEFAULT:
      *off_x = 0.5;
      *off_y = 0.5;
      break;
    case GAVL_CHROMA_PLACEMENT_MPEG2:
      if(format->interlace_mode == GAVL_INTERLACE_NONE)
        {
        *off_x = 0.0;
        *off_y = 0.5;
        }
      else if(field == 0) /* Top field */
        {
        *off_x = 0.0;
        *off_y = 0.25;
        }
      else /* Bottom field */
        {
        *off_x = 0.0;
        *off_y = 0.75;
        }
      break;
    case GAVL_CHROMA_PLACEMENT_DVPAL:
      if(plane == 1) /* Cb */
        {
        *off_x = 0.0;
        *off_y = 1.0;
        }
      else           /* Cr */
        {
        *off_x = 0.0;
        *off_y = 0.0;
        }
    }
  
  }
