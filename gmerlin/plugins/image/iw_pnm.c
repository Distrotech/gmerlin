/*****************************************************************
 * gmerlin - a general purpose multimedia framework and applications
 *
 * Copyright (c) 2001 - 2011 Members of the Gmerlin project
 * gmerlin-general@lists.sourceforge.net
 * http://gmerlin.sourceforge.net
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 * *****************************************************************/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <errno.h>

#include <config.h>
#include <gmerlin/translation.h>
#include <gmerlin/plugin.h>
#include <gmerlin/pluginfuncs.h>
#include <gmerlin/utils.h>

#include <gmerlin/log.h>
#define LOG_DOMAIN "iw_pnm"

#define BINARY_RGB    6
#define ASCII_RGB     3

#define BINARY_GRAY   5
#define ASCII_GRAY    2

typedef struct
  {
  FILE *output;
  char *comment;
  uint32_t Width;
  uint32_t Height;
  gavl_video_format_t format;
  int binary;
  bg_iw_callbacks_t * cb;
  int is_gray;
  } pnm_t;

static void * create_pnm()
  {
  pnm_t * ret;
  ret = calloc(1, sizeof(*ret));
  return ret;
  }

static void destroy_pnm(void* priv)
  {
  pnm_t * pnm = priv;
  if(pnm->comment)
    free(pnm->comment);
  free(pnm);
  }

static void set_callbacks_pnm(void * data, bg_iw_callbacks_t * cb)
  {
  pnm_t * e = data;
  e->cb = cb;
  }

static int write_header_pnm(void * priv, const char * filename,
                            gavl_video_format_t * format, const bg_metadata_t * m)
  {
  int sig = 0;
  const char * ext;
  char * real_filename;
  pnm_t * p = priv;

  p->Width =  format->image_width;
  p->Height =  format->image_height;

  if(gavl_pixelformat_is_gray(format->pixelformat))
    {
    format->pixelformat = GAVL_GRAY_8;
    p->is_gray = 1;
    ext = "pgm";
    }
  else
    {
    format->pixelformat = GAVL_RGB_24;
    p->is_gray = 0;
    ext = "ppm";
    }
  
  real_filename = bg_filename_ensure_extension(filename, ext);
  
  if(!bg_iw_cb_create_output_file(p->cb, real_filename))
    {
    free(real_filename);
    return 0;
    }
  
  p->output = fopen(real_filename,"wb");
  free(real_filename);
  
  if(!p->output)
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Cannot open %s: %s",
           real_filename, strerror(errno));
    return 0;
    }
 

  if(p->binary)
    {
    if(p->is_gray)
      sig = BINARY_GRAY;
    else
      sig = BINARY_RGB;
    }
  else
    {
    if(p->is_gray)
      sig = ASCII_GRAY;
    else
      sig = ASCII_RGB;
    }
  
  /* Write the header lines */  
  fprintf(p->output,"P%d\n# %s\n%d %d\n255\n", sig, p->comment, p->Width, p->Height);
  return 1;
  }

static int write_image_pnm(void *priv, gavl_video_frame_t *frame)
  {
  int i, j;
  pnm_t *p = priv;
  uint8_t * frame_ptr;

  int bytes_per_pixel;
  if(p->is_gray)
    bytes_per_pixel = 1;
  else
    bytes_per_pixel = 3;
  
  /* write image data binary */
  if(p->binary)
    {
    frame_ptr = frame->planes[0];
    
    for (i = 0; i < p->Height; i++)
      {
      fwrite(frame_ptr, bytes_per_pixel, p->Width, p->output);
      frame_ptr += frame->strides[0];
      }
    }
  /* write image data ascii */
  else
    {
    uint8_t * frame_ptr_start;
    frame_ptr_start = frame->planes[0];
    for (i = 0; i < p->Height; i++)
      {
      frame_ptr = frame_ptr_start ;

      for(j = 0; j < p->Width * bytes_per_pixel; j++)
        {
        fprintf(p->output," %d", *frame_ptr);
        frame_ptr++;
        }
      fprintf(p->output,"\n");
      frame_ptr_start += frame->strides[0];
      }
    }
    
  fclose(p->output);
  return 1;
  }


/* Configuration stuff */

static const bg_parameter_info_t parameters[] =
  {
    {
      .name =        "format",
      .long_name =   TRS("Format"),
      .type =        BG_PARAMETER_STRINGLIST,
      .multi_names = (char const *[]){ "binary", "ascii", NULL },
      .multi_labels =  (char const *[]){ TRS("Binary"), TRS("ASCII"), NULL },

      .val_default = { .val_str = "binary" },
    },
    {
      .name =        "comment",
      .long_name =   TRS("Comment"),
      .type =        BG_PARAMETER_STRING,

      .val_default = { .val_str = "Created with gmerlin" },
      .help_string = TRS("Comment which will be written in front of every file")
    },
   { /* End of parameters */ }
  };

static const bg_parameter_info_t * get_parameters_pnm(void * p)
  {
  return parameters;
  }

static void set_parameter_pnm(void * p, const char * name,
                              const bg_parameter_value_t * val)
  {
  pnm_t * pnm;
  pnm = p;

  if(!name)
    return;

  else if(!strcmp(name, "format"))
    {
    if(!strcmp(val->val_str, "binary"))
      pnm->binary = 1;
    else if(!strcmp(val->val_str, "ascii"))
      pnm->binary = 0;
    }
  else if(!strcmp(name, "comment"))
    pnm->comment = bg_strdup(pnm->comment, val->val_str);
  }

const bg_image_writer_plugin_t the_plugin =
  {
    .common =
    {
      BG_LOCALE,
      .name =           "iw_pnm",
      .long_name =      TRS("PPM/PGM writer"),
      .description =    TRS("Writer for PPM/PGM images"),
      .type =           BG_PLUGIN_IMAGE_WRITER,
      .flags =          BG_PLUGIN_FILE,
      .priority =       5,
      .create =         create_pnm,
      .destroy =        destroy_pnm,
      .get_parameters = get_parameters_pnm,
      .set_parameter =  set_parameter_pnm
    },
    .extensions = "ppm pgm",
    .set_callbacks = set_callbacks_pnm,
    .write_header =  write_header_pnm,
    .write_image =   write_image_pnm,
  };

/* Include this into all plugin modules exactly once
   to let the plugin loader obtain the API version */
BG_GET_PLUGIN_API_VERSION;

