/*****************************************************************
 * gmerlin - a general purpose multimedia framework and applications
 *
 * Copyright (c) 2001 - 2008 Members of the Gmerlin project
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <config.h>
#include <gmerlin/translation.h>
#include <gmerlin/plugin.h>
#include <gmerlin/utils.h>

#include <png.h>

#include "pngwriter.h"

typedef struct
  {
  bg_pngwriter_t w; /* Must be first in the struct */
  FILE * xml_file;

  gavl_video_format_t format;
  int subtitles_read;
  gavl_video_frame_t * subframe;
  
  char * filename_template;
  char * filename;
  int subtitles_written;
  } spumux_t;

static void * create_spumux()
  {
  spumux_t * ret;
  ret = calloc(1, sizeof(*ret));
  ret->w.bit_mode = BITS_8;
  ret->subframe = gavl_video_frame_create(NULL);
  return ret;
  }

static char const * const extension = "xml";

static const char * get_extension_spumux(void * priv)
  {
  return extension;
  }

static int open_spumux(void * priv, const char * filename,
                       bg_metadata_t * metadata,
                       bg_chapter_list_t * chapter_list)
  {
  char * pos;
  spumux_t * spumux = (spumux_t*)priv;
  
  spumux->filename = bg_strdup(spumux->filename, filename);
  spumux->filename_template = bg_strdup(spumux->filename_template, filename);

  pos = strrchr(spumux->filename_template, '.');
  if(pos) *pos = '\0';

  spumux->filename_template = bg_strcat(spumux->filename_template, "_%05d.png");
  
  spumux->xml_file = fopen(filename, "w");

  fprintf(spumux->xml_file, "<subpictures>\n  <stream>\n");

  return 1;
  }

static int add_subtitle_overlay_stream_spumux(void * priv, const char * language,
                                              gavl_video_format_t * format)
  {
  spumux_t * spumux = (spumux_t*)priv;
  gavl_video_format_copy((&spumux->format), format);
  spumux->format.pixelformat = GAVL_RGBA_32;
  spumux->format.timescale = GAVL_TIME_SCALE;
  return 1;
  }

static int start_spumux(void * priv)
  {
  return 1;
  }

static void get_subtitle_overlay_format_spumux(void * priv, int stream,
                                               gavl_video_format_t*ret)
  {
  spumux_t * spumux = (spumux_t*)priv;
  gavl_video_format_copy(ret, (&spumux->format));
  }

static void print_time(FILE * out, gavl_time_t time, gavl_video_format_t * format)
  {
  int h, m, s, f;

  f = gavl_time_to_frames(format->timescale, format->frame_duration, time % GAVL_TIME_SCALE);
  time /= GAVL_TIME_SCALE;
  s = time % 60;
  time /= 60;
  m = time % 60;
  time /= 60;
  h = time % 60;
  fprintf(out, "%02d:%02d:%02d.%02d", h, m, s, f);
  }

static int write_subtitle_overlay_spumux(void * priv, gavl_overlay_t * ovl, int stream)
  {
  char * image_filename;
  spumux_t * spumux = (spumux_t*)priv;
  gavl_video_format_t tmp_format;
  gavl_video_format_copy(&tmp_format, (&spumux->format));

  tmp_format.image_width  = ovl->ovl_rect.w;
  tmp_format.image_height = ovl->ovl_rect.h;
  tmp_format.frame_width  = tmp_format.image_width;
  tmp_format.frame_height = tmp_format.image_height;
  
  gavl_video_frame_get_subframe(spumux->format.pixelformat, ovl->frame, spumux->subframe, &ovl->ovl_rect);

  image_filename = bg_sprintf(spumux->filename_template, spumux->subtitles_written);

  if(!bg_pngwriter_write_header(priv, image_filename,
                                &tmp_format))
    return 0;
  
  if(!bg_pngwriter_write_image(priv, spumux->subframe))
    return 0;
  
  fprintf(spumux->xml_file, "    <spu start=\"");
  print_time(spumux->xml_file, ovl->frame->timestamp, &spumux->format);

  fprintf(spumux->xml_file, "\" end=\"");
  print_time(spumux->xml_file, ovl->frame->timestamp+ovl->frame->duration, &spumux->format);
  fprintf(spumux->xml_file, "\" xoffset=\"%d\" yoffset=\"%d\" image=\"%s\"/>\n", ovl->dst_x, ovl->dst_y,
          image_filename);
  
  free(image_filename);
    
  spumux->subtitles_written++;
  return 1;
  }

static int close_spumux(void * priv, int do_delete)
  {
  int i;
  spumux_t * spumux = (spumux_t*)priv;
  char * image_filename;

  fprintf(spumux->xml_file, "  </stream>\n</subpictures>\n");
  fclose(spumux->xml_file);
  spumux->xml_file = NULL;
  
  if(do_delete)
    {
    for(i = 0; i < spumux->subtitles_written; i++)
      {
      image_filename = bg_sprintf(spumux->filename_template, spumux->subtitles_written);
      remove(image_filename);
      free(image_filename);
      }
    remove(spumux->filename);
    }
  return 1;
  }

static void destroy_spumux(void * priv)
  {
  spumux_t * spumux = (spumux_t*)priv;
  if(spumux->xml_file)
    close_spumux(priv, 1);
  
  if(spumux->filename) free(spumux->filename);
  if(spumux->filename_template) free(spumux->filename_template);
  if(spumux->subframe)
    {
    gavl_video_frame_null(spumux->subframe);
    gavl_video_frame_destroy(spumux->subframe);
    }
  free(spumux);
  }


static const bg_parameter_info_t parameters[] =
  {
    {
      .name =        "compression",
      .long_name =   TRS("Compression level"),
      .type =        BG_PARAMETER_SLIDER_INT,
      .val_min =     { .val_i = 0 },
      .val_max =     { .val_i = 9 },
      .val_default = { .val_i = 9 },
    },
    { /* End of parameters */ }
  };

static const bg_parameter_info_t * get_parameters_spumux(void * priv)
  {
  return parameters;
  }



const bg_encoder_plugin_t the_plugin =
  {
    .common =
    {
      BG_LOCALE,
      .name =           "e_spumux",       /* Unique short name */
      .long_name =      TRS("spumux overlay exporter"),
      .description =    TRS("Exports overlay subtitles into the format used by spumux\
 (http://dvdauthor.sourceforge.net)"),
      .type =           BG_PLUGIN_ENCODER_SUBTITLE_OVERLAY,
      .flags =          BG_PLUGIN_FILE,
      .priority =       BG_PLUGIN_PRIORITY_MAX,
      .create =         create_spumux,
      .destroy =        destroy_spumux,
      .get_parameters = get_parameters_spumux,
      .set_parameter =  bg_pngwriter_set_parameter,
      //   get_error:      get_error_spumux,
    },

    .max_subtitle_overlay_streams = 1,
    
    .get_extension =        get_extension_spumux,
    
    .open =                 open_spumux,

    .add_subtitle_overlay_stream =     add_subtitle_overlay_stream_spumux,
    
    .start =                start_spumux,

    .get_subtitle_overlay_format = get_subtitle_overlay_format_spumux,
    
    .write_subtitle_overlay = write_subtitle_overlay_spumux,
    .close =             close_spumux,
  };

/* Include this into all plugin modules exactly once
   to let the plugin loader obtain the API version */
BG_GET_PLUGIN_API_VERSION;
