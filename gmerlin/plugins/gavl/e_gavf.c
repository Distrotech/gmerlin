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

#include <string.h>

#include <config.h>
#include <gmerlin/translation.h>
#include <gmerlin/plugin.h>
#include <gavl/gavf.h>

typedef struct
  {
  int num_audio_streams;
  int num_video_streams;
  int num_text_streams;
  
  struct
    {
    gavl_audio_format_t format;
    const gavl_compression_info_t * ci;
    
    int index;
    } * audio_streams;
  
  struct
    {
    gavl_video_format_t format;

    const gavl_compression_info_t * ci;

    int index;
    } * video_streams;

  struct
    {
    int index;
    } * text_streams;
  
  gavf_t * enc;
  bg_encoder_callbacks_t * cb;
  gavf_options_t * opt;
  
  } bg_gavf_t;

static void * bg_gavf_create()
  {
  bg_gavf_t * ret;
  ret = calloc(1, sizeof(*ret));
  ret->enc = gavf_create();

  ret->opt = gavf_get_options(ret->enc);
  
  return ret;
  }

static void bg_gavf_destroy(void * data)
  {
  bg_gavf_t * f = data;
  
  }

static const bg_parameter_info_t parameters[] =
  {
    {
      .name      = "format",
      .long_name = TRS("Format"),
      .type      = BG_PARAMETER_STRINGLIST,
      .multi_names  = (const char*[]){ "disk", NULL },
      .multi_labels = (const char*[]){ TRS("Disk"), NULL },
    },
    {
      .name        = "sync_distance",
      .long_name   = TRS("Sync distance (ms)"),
      .type        = BG_PARAMETER_INT,
      .val_min     = { .val_i = 20 },
      .val_max     = { .val_i = 10000 },
      .val_default = { .val_i = 500 },
      .help_string = TRS("Distance between sync headers if no stream has keyframes. Smaller distances produce larger overhead but speed up seeking"),
    },
    { /* End */ }
  };

static const bg_parameter_info_t *
bg_gavf_get_parameters(void * data)
  {
  return parameters;
  }

static void
bg_gavf_set_parameter(void * data, const char * name,
                      const bg_parameter_value_t * val)
  {
  bg_gavf_t * f = data;

  if(!name)
    return;
  else if(!strcmp(name, "sync_distance"))
    gavf_options_set_sync_distance(f->opt, val->val_i * 1000);
  }

static void
bg_gavf_set_callbacks(void * data,
                      bg_encoder_callbacks_t * cb)
  {
  bg_gavf_t * f = data;
  f->cb = cb;
  }

static int
bg_gavf_open(void * data, const char * filename,
             const gavl_metadata_t * metadata,
             const bg_chapter_list_t * chapter_list)
  {
  return 0;
  }

static int
bg_gavf_writes_compressed_audio(void * priv,
                                const gavl_audio_format_t * format,
                                const gavl_compression_info_t * info)
  {
  return 1;
  }

static int
bg_gavf_writes_compressed_video(void * priv,
                                const gavl_video_format_t * format,
                                const gavl_compression_info_t * info)
  {
  return 1;
  }

static int
bg_gavf_add_audio_stream(void * data,
                         const gavl_metadata_t * m,
                         const gavl_audio_format_t * format)
  {
  bg_gavf_t * f = data;
  gavl_compression_info_t ci;
  memset(&ci, 0, sizeof(ci));
  
  f->audio_streams =
    realloc(f->audio_streams,
            (f->num_audio_streams+1)*sizeof(*f->audio_streams));

  gavl_audio_format_copy(&f->audio_streams[f->num_audio_streams].format,
                         format);
  f->audio_streams[f->num_audio_streams].index =
    gavf_add_audio_stream(f->enc, &ci, format, m);

  f->num_audio_streams++;
  return f->num_audio_streams-1;
  }

static int
bg_gavf_add_video_stream(void * data,
                         const gavl_metadata_t * m,
                         const gavl_video_format_t * format)
  {
  bg_gavf_t * f = data;
  gavl_compression_info_t ci;
  memset(&ci, 0, sizeof(ci));
  
  f->video_streams =
    realloc(f->video_streams,
            (f->num_video_streams+1)*sizeof(*f->video_streams));

  gavl_video_format_copy(&f->video_streams[f->num_video_streams].format,
                         format);
  
  f->video_streams[f->num_video_streams].index =
    gavf_add_video_stream(f->enc, &ci,
                          &f->video_streams[f->num_video_streams].format, m);
  
  f->num_video_streams++;
  return f->num_video_streams-1;
  }

static int
bg_gavf_add_text_stream(void * data,
                        const gavl_metadata_t * m,
                        int * timescale)
  {
  bg_gavf_t * f = data;
  f->text_streams =
    realloc(f->text_streams,
            (f->num_text_streams+1)*sizeof(*f->text_streams));
  f->text_streams[f->num_text_streams].index =
    gavf_add_text_stream(f->enc, *timescale, m);
  
  f->num_text_streams++;
  return f->num_text_streams-1;
  }

static int
bg_gavf_add_audio_stream_compressed(void * data,
                                    const gavl_metadata_t * m,
                                    const gavl_audio_format_t * format,
                                    const gavl_compression_info_t * info)
  {
  bg_gavf_t * f = data;
  f->audio_streams =
    realloc(f->audio_streams,
            (f->num_audio_streams+1)*sizeof(*f->audio_streams));

  gavl_audio_format_copy(&f->audio_streams[f->num_audio_streams].format,
                         format);
  f->audio_streams[f->num_audio_streams].index =
    gavf_add_audio_stream(f->enc, info, format, m);

  f->num_audio_streams++;
  return f->num_audio_streams-1;
  }

static int
bg_gavf_add_video_stream_compressed(void * data,
                                    const gavl_metadata_t * m,
                                    const gavl_video_format_t * format,
                                    const gavl_compression_info_t * info)
  {
  bg_gavf_t * f = data;
  f->video_streams =
    realloc(f->video_streams,
            (f->num_video_streams+1)*sizeof(*f->video_streams));

  gavl_video_format_copy(&f->video_streams[f->num_video_streams].format,
                         format);
  f->video_streams[f->num_video_streams].index =
    gavf_add_video_stream(f->enc, info, &f->video_streams[f->num_video_streams].format, m);

  f->num_video_streams++;
  return f->num_video_streams-1;
  }

static int bg_gavf_start(void * data)
  {
  return 1;
  }

static void
bg_gavf_get_audio_format(void * data, int stream,
                         gavl_audio_format_t*ret)
  {
  bg_gavf_t * priv;
  priv = data;

  gavl_audio_format_copy(ret, &priv->audio_streams[stream].format);
  }

static void
bg_gavf_get_video_format(void * data, int stream,
                         gavl_video_format_t*ret)
  {
  bg_gavf_t * priv;
  priv = data;

  gavl_video_format_copy(ret, &priv->video_streams[stream].format);
  }

static int
bg_gavf_write_audio_frame(void * data,
                          gavl_audio_frame_t * frame, int stream)
  {
  bg_gavf_t * f = data;
  
  }

static int
bg_gavf_write_video_frame(void * data,
                          gavl_video_frame_t * frame, int stream)
  {
  bg_gavf_t * f = data;
  return gavf_write_video_frame(f->enc, f->video_streams[stream].index,
                                frame);
  }

static int
bg_gavf_write_subtitle_text(void * data,const char * text,
                            int64_t start,
                            int64_t duration, int stream)
  {
  bg_gavf_t * f = data;
  gavl_packet_t p;
  int ret;
  int len;
  
  gavl_packet_init(&p);
  len = strlen(text);

  gavl_packet_alloc(&p, len);
  memcpy(p.data, text, len);
  p.data_len = len;
  p.pts = start;
  p.duration = duration;
  
  ret = gavf_write_packet(f->enc, stream, &p);
  gavf_packet_free(&p);
  return ret;
  }

static int
bg_gavf_close(void * data, int do_delete)
  {
  bg_gavf_t * f = data;

  }

static int
bg_gavf_write_audio_packet(void * data, gavl_packet_t * packet,
                           int stream)
  {
  bg_gavf_t * f = data;
  return gavf_write_packet(f->enc, f->audio_streams[stream].index,
                           packet);
  }

static int
bg_gavf_write_video_packet(void * data, gavl_packet_t * packet,
                           int stream)
  {
  bg_gavf_t * f = data;
  return gavf_write_packet(f->enc, f->audio_streams[stream].index,
                           packet);
  }

const bg_encoder_plugin_t the_plugin =
  {
    .common =
    {
      BG_LOCALE,
      .name =           "e_gavf",       /* Unique short name */
      .long_name =      TRS("GAVF encoder"),
      .description =    TRS("Plugin for encoding the Gmerlin audio video format."),
      .type =           BG_PLUGIN_ENCODER,
      .flags =          BG_PLUGIN_FILE,
      .priority =       5,
      .create =         bg_gavf_create,
      .destroy =        bg_gavf_destroy,
      .get_parameters = bg_gavf_get_parameters,
      .set_parameter =  bg_gavf_set_parameter,
    },
    
    .max_audio_streams =         -1,
    .max_video_streams =         -1,
    .max_subtitle_text_streams = -1,
    
    //    .get_audio_parameters = bg_gavf_get_audio_parameters,
    //    .get_video_parameters = bg_gavf_get_video_parameters,

    .set_callbacks =        bg_gavf_set_callbacks,
    
    .open =                 bg_gavf_open,

    .writes_compressed_audio = bg_gavf_writes_compressed_audio,
    .writes_compressed_video = bg_gavf_writes_compressed_video,
    
    .add_audio_stream =     bg_gavf_add_audio_stream,
    .add_video_stream =     bg_gavf_add_video_stream,
    .add_subtitle_text_stream =     bg_gavf_add_text_stream,

    .add_audio_stream_compressed =     bg_gavf_add_audio_stream_compressed,
    .add_video_stream_compressed =     bg_gavf_add_video_stream_compressed,

    // .set_video_pass =       bg_gavf_set_video_pass,
    // .set_audio_parameter =  bg_gavf_set_audio_parameter,
    // .set_video_parameter =  bg_gavf_set_video_parameter,
    
    .get_audio_format =     bg_gavf_get_audio_format,
    .get_video_format =     bg_gavf_get_video_format,

    .start =                bg_gavf_start,
    
    .write_audio_frame =    bg_gavf_write_audio_frame,
    .write_video_frame =    bg_gavf_write_video_frame,
    .write_subtitle_text =    bg_gavf_write_subtitle_text,

    .write_audio_packet =    bg_gavf_write_audio_packet,
    .write_video_packet =    bg_gavf_write_video_packet,

    .close =                bg_gavf_close,
  };

/* Include this into all plugin modules exactly once
   to let the plugin loader obtain the API version */
BG_GET_PLUGIN_API_VERSION;
