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
#include <string.h>

#include <config.h>
#include <gmerlin/translation.h>

#include <gmerlin/plugin.h>
#include <gmerlin/pluginfuncs.h>
#include <gmerlin/utils.h>

typedef struct
  {
  FILE * output;  
  int format_index;
  char * filename;
  int titles_written;
  
  gavl_time_t last_time;
  gavl_time_t last_duration;
  
  bg_metadata_t metadata;
  
  bg_encoder_callbacks_t * cb;

  } subtext_t;

static void write_time_srt(FILE * output, gavl_time_t time)
  {
  int msec, sec, m, h;

  msec = (time % GAVL_TIME_SCALE) / (GAVL_TIME_SCALE/1000);
  time /= GAVL_TIME_SCALE;
  sec = time % 60;
  time /= 60;
  m = time % 60;
  time /= 60;
  h = time;

  fprintf(output, "%02d:%02d:%02d,%03d", h, m, sec, msec);
  }

static void write_subtitle_srt(subtext_t * s, const char * text,
                               gavl_time_t time, gavl_time_t duration)
  {
  int i;
  char ** lines;
  /* Index */

  fprintf(s->output, "%d\r\n", s->titles_written + 1);
  
  /* Time */
  write_time_srt(s->output, time);
  fprintf(s->output, " --> ");
  write_time_srt(s->output, time+duration);
  fprintf(s->output, "\r\n");
  
  lines = bg_strbreak(text, '\n');
  i = 0;
  while(lines[i])
    {
    fprintf(s->output, "%s\r\n", lines[i]);
    i++;
    }
  /* Empty line */
  fprintf(s->output, "\r\n");
  bg_strbreak_free(lines);
  }

static void write_header_mpsub(subtext_t * s)
  {
  if(s->metadata.title)
    fprintf(s->output, "TITLE=%s\n", s->metadata.title);

  if(s->metadata.author)
    fprintf(s->output, "AUTHOR=%s\n", s->metadata.author);

  if(s->metadata.comment)
    fprintf(s->output, "NOTE=%s\n", s->metadata.comment);
  fprintf(s->output, "FORMAT=TIME\n\n");
  }

static void write_subtitle_mpsub(subtext_t * s, const char * text,
                                 gavl_time_t time, gavl_time_t duration)
  {
  if(s->last_time != GAVL_TIME_UNDEFINED)
    {
    fprintf(s->output, "%.3f %.3f\n",
            gavl_time_to_seconds(time - (s->last_time + s->last_duration)),
            gavl_time_to_seconds(duration));
    }
  else
    fprintf(s->output, "%.3f %.3f\n",
            gavl_time_to_seconds(time),
            gavl_time_to_seconds(duration));
  
  fprintf(s->output, "%s\n\n", text);
  }

static const struct
  {
  const char * extension;
  const char * name;
  void (*write_subtitle)(subtext_t * s, const char * text,
                         gavl_time_t time, gavl_time_t duration);
  void (*write_header)(subtext_t * s);
  }
formats[] =
  {
    {
      .extension =      "srt",
      .name =           "srt",
      .write_subtitle = write_subtitle_srt,
    },
    {
      .extension =      "sub",
      .name =           "mpsub",
      .write_header =   write_header_mpsub,
      .write_subtitle = write_subtitle_mpsub,
    }
  };

static void * create_subtext()
  {
  subtext_t * ret = calloc(1, sizeof(*ret));
  ret->last_time     = GAVL_TIME_UNDEFINED;
  ret->last_duration = GAVL_TIME_UNDEFINED;
  return ret;
  }

static void set_callbacks_subtext(void * data, bg_encoder_callbacks_t * cb)
  {
  subtext_t * e = data;
  e->cb = cb;
  }

static int open_subtext(void * data, const char * filename,
                        const bg_metadata_t * metadata,
                        const bg_chapter_list_t * chapter_list)
  {
  subtext_t * e;
  e = (subtext_t *)data;
  
  e->filename =
    bg_filename_ensure_extension(filename,
                                 formats[e->format_index].extension);

  if(!bg_encoder_cb_create_output_file(e->cb, e->filename))
    return 0;
  
  e->output = fopen(e->filename, "w");

  if(metadata)
    bg_metadata_copy(&e->metadata, metadata);
  
  return 1;
  }

static int add_subtitle_text_stream_subtext(void * data, const char * language,
                                            int * timescale)
  {
  subtext_t * e;
  e = (subtext_t *)data;
  *timescale = GAVL_TIME_SCALE;
  return 0;
  }

static int start_subtext(void * data)
  {
  subtext_t * e;
  e = (subtext_t *)data;
  
  if(formats[e->format_index].write_header)
    formats[e->format_index].write_header(e);
  
  return 1;
  }

static int write_subtitle_text_subtext(void * data, const char * text,
                                       gavl_time_t start,
                                       gavl_time_t duration, int stream)
  {
  subtext_t * e;
  e = (subtext_t *)data;
  formats[e->format_index].write_subtitle(e, text, start, duration);
  e->titles_written++;

  e->last_time     = start;
  e->last_duration = duration;
  
  return 1;
  }

static int close_subtext(void * data, int do_delete)
  {
  subtext_t * e;
  e = (subtext_t *)data;
  if(e->output)
    {
    fclose(e->output);
    e->output = NULL;
    }
  if(do_delete)
    remove(e->filename);
  return 1;
  }

static void destroy_subtext(void * data)
  {
  subtext_t * e;
  e = (subtext_t *)data;
  if(e->output)
    close_subtext(data, 1);
  if(e->filename)
    free(e->filename);
  free(e);
  }

static const bg_parameter_info_t parameters[] =
  {
    {
      .name =         "format" ,
      .long_name =    TRS("Format"),
      .type =         BG_PARAMETER_STRINGLIST,
      .val_default =  { .val_str = "srt" },
      .multi_names =  (char const *[]){ "srt",           "mpsub",         (char*)0 },
      .multi_labels = (char const *[]){ TRS("Subrip (.srt)"), TRS("MPlayer mpsub"), (char*)0 },
      
    },
    { /* End of parameters */ }
  };

static const bg_parameter_info_t * get_parameters_subtext(void * data)
  {
  return parameters;
  }

static void set_parameter_subtext(void * data, const char * name,
                                  const bg_parameter_value_t * val)
  {
  int i;
  subtext_t * e;
  e = (subtext_t *)data;

  if(!name)
    return;
  if(!strcmp(name, "format"))
    {
    for(i = 0; i < sizeof(formats)/sizeof(formats[0]); i++)
      {
      if(!strcmp(val->val_str, formats[i].name))
        {
        e->format_index = i;
        break;
        }
      }
    }
  }

const bg_encoder_plugin_t the_plugin =
  {
    .common =
    {
      BG_LOCALE,
      .name =           "e_subtext",       /* Unique short name */
      .long_name =      TRS("Text subtitle exporter"),
      .description =    TRS("Plugin for exporting text subtitles. Supported formats are MPSub and SRT"),
      .type =           BG_PLUGIN_ENCODER_SUBTITLE_TEXT,
      .flags =          BG_PLUGIN_FILE,
      .priority =       BG_PLUGIN_PRIORITY_MAX,
      .create =         create_subtext,
      .destroy =        destroy_subtext,
      .get_parameters = get_parameters_subtext,
      .set_parameter =  set_parameter_subtext,
    },

    .max_subtitle_text_streams = 1,
    
    .set_callbacks =        set_callbacks_subtext,
    
    .open =                 open_subtext,

    .add_subtitle_text_stream =     add_subtitle_text_stream_subtext,
    
    .start =                start_subtext,
    
    .write_subtitle_text = write_subtitle_text_subtext,
    .close =             close_subtext,
    
  };

/* Include this into all plugin modules exactly once
   to let the plugin loader obtain the API version */
BG_GET_PLUGIN_API_VERSION;
