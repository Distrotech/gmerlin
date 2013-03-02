/*****************************************************************
 * gmerlin - a general purpose multimedia framework and applications
 *
 * Copyright (c) 2001 - 2012 Members of the Gmerlin project
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
#include <ctype.h>
#include <string.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>

#include <config.h>
#include <gmerlin/translation.h>

#include <gmerlin/cfg_registry.h>
#include <gmerlin/pluginregistry.h>
#include <gmerlin/pluginfuncs.h>

#include <gmerlin/utils.h>
#include <gmerlin/singlepic.h>
#include <gmerlin/log.h>

#include <gavl/metatags.h>

#define LOG_DOMAIN_ENC "singlepicture-encoder"
#define LOG_DOMAIN_DEC "singlepicture-decoder"

static char * get_extensions(bg_plugin_registry_t * reg,
                             uint32_t type_mask, uint32_t flag_mask)
  {
  int num, i;
  char * ret = NULL;
  const bg_plugin_info_t * info;
  
  num = bg_plugin_registry_get_num_plugins(reg, type_mask, flag_mask);
  if(!num)
    return NULL;

  for(i = 0; i < num; i++)
    {
    info = bg_plugin_find_by_index(reg, i,
                                   type_mask, flag_mask);
    ret = bg_strcat(ret, info->extensions);
    if(i < num-1)
      ret = bg_strcat(ret, " ");
    }
  return ret;
  }

/* Input stuff */

static const bg_parameter_info_t parameters_input[] =
  {
    {
      .name =      "timescale",
      .long_name = TRS("Timescale"),
      .type =      BG_PARAMETER_INT,
      .val_min =     { .val_i = 1 },
      .val_max =     { .val_i = 100000 },
      .val_default = { .val_i = 25 }
    },
    {
      .name =      "frame_duration",
      .long_name = TRS("Frame duration"),
      .type =      BG_PARAMETER_INT,
      .val_min =     { .val_i = 1 },
      .val_max =     { .val_i = 100000 },
      .val_default = { .val_i = 1 }
    },
    { /* End of parameters */ }
  };

static const bg_parameter_info_t parameters_input_still[] =
  {
    {
      .name =         "display_time",
      .long_name =    TRS("Display time"),
      .type =         BG_PARAMETER_TIME,
      .val_min =      { .val_time = 0 },
      .val_max =      { .val_time = 3600 * (gavl_time_t)GAVL_TIME_SCALE },
      .val_default = { .val_time = 0 },
      .help_string =  TRS("Time to pass until the next track will be selected. 0 means infinite.")
    },
    { /* End of parameters */ }
  };


typedef struct
  {
  bg_track_info_t track_info;
  bg_plugin_registry_t * plugin_reg;
  
  int timescale;
  int frame_duration;

  gavl_time_t display_time;
  
  char * template;
  
  int64_t frame_start;
  int64_t frame_end;
  int64_t current_frame;

  bg_plugin_handle_t * handle;
  bg_image_reader_plugin_t * image_reader;

  char * filename_buffer;
  int header_read;

  bg_stream_action_t action;

  int do_still;

  gavl_video_source_t * src;
  } input_t;

static const bg_parameter_info_t * get_parameters_input(void * priv)
  {
  return parameters_input;
  }

static const bg_parameter_info_t * get_parameters_input_still(void * priv)
  {
  return parameters_input_still;
  }

static void set_parameter_input(void * priv, const char * name,
                                const bg_parameter_value_t * val)
  {
  input_t * inp = priv;

  if(!name)
    return;

  else if(!strcmp(name, "timescale"))
    {
    inp->timescale = val->val_i;
    }
  else if(!strcmp(name, "frame_duration"))
    {
    inp->frame_duration = val->val_i;
    }
  else if(!strcmp(name, "display_time"))
    {
    inp->display_time = val->val_time;
    if(!inp->display_time)
      inp->display_time = GAVL_TIME_UNDEFINED;
    }
  }

static int open_input(void * priv, const char * filename)
  {
  const bg_plugin_info_t * info;
  char * tmp_string;
  const char * pos;
  const char * pos_start;
  const char * pos_end;
  const gavl_metadata_t * m;
  
  input_t * inp = priv;
  
  /* Check if the first file exists */
  
  if(access(filename, R_OK))
    return 0;
  
  /* Load plugin */

  info = bg_plugin_find_by_filename(inp->plugin_reg,
                                    filename,
                                    BG_PLUGIN_IMAGE_READER);
  if(!info)
    return 0;
  
  inp->handle = bg_plugin_load(inp->plugin_reg, info);
  inp->image_reader = (bg_image_reader_plugin_t*)inp->handle->plugin;
  
  /* Create template */
  
  pos = filename + strlen(filename) - 1;
  while((*pos != '.') && (pos != filename))
    pos--;

  if(pos == filename)
    return 0;

  /* pos points now to the last dot */

  pos--;
  while(!isdigit(*pos) && (pos != filename))
    pos--;
  if(pos == filename)
    return 0;

  /* pos points now to the last digit */

  pos_end = pos+1;

  while(isdigit(*pos) && (pos != filename))
    pos--;

  if(pos != filename)
    pos_start = pos+1;
  else
    pos_start = pos;

  /* Now, cut the pieces together */

  if(pos_start != filename)
    inp->template = bg_strncat(inp->template, filename, pos_start);

  tmp_string = bg_sprintf("%%0%dd", (int)(pos_end - pos_start));
  inp->template = bg_strcat(inp->template, tmp_string);
  free(tmp_string);

  inp->template = bg_strcat(inp->template, pos_end);

  inp->frame_start = strtoll(pos_start, NULL, 10);
  inp->frame_end = inp->frame_start+1;

  inp->filename_buffer = malloc(strlen(filename)+100);

  while(1)
    {
    sprintf(inp->filename_buffer, inp->template, inp->frame_end);
    if(access(inp->filename_buffer, R_OK))
      break;
    inp->frame_end++;
    }
  
  /* Create stream */
  
  inp->track_info.num_video_streams = 1;
  inp->track_info.video_streams =
    calloc(1, sizeof(*inp->track_info.video_streams));

  gavl_metadata_set_long(&inp->track_info.metadata, GAVL_META_APPROX_DURATION,
                    gavl_frames_to_time(inp->timescale,
                                        inp->frame_duration,
                                        inp->frame_end -
                                        inp->frame_start));
    
  /* Get track name */

  bg_set_track_name_default(&inp->track_info, filename);
  
  inp->track_info.flags |= (BG_TRACK_SEEKABLE|BG_TRACK_PAUSABLE);

  inp->current_frame = inp->frame_start;

  sprintf(inp->filename_buffer, inp->template, inp->current_frame);

  /* Load image header */
  
  if(!inp->image_reader->read_header(inp->handle->priv,
                                     inp->filename_buffer,
                                     &inp->track_info.video_streams[0].format))
    return 0;
  inp->track_info.video_streams[0].format.timescale =
    inp->timescale;
  inp->track_info.video_streams[0].format.frame_duration =
    inp->frame_duration;
  inp->track_info.video_streams[0].format.framerate_mode =
    GAVL_FRAMERATE_CONSTANT;
  inp->header_read = 1;

  /* Metadata */

  if(inp->image_reader->get_metadata &&
     (m = inp->image_reader->get_metadata(inp->handle->priv)))
    {
    gavl_metadata_copy(&inp->track_info.metadata, m);
    }
  
  gavl_metadata_set(&inp->track_info.video_streams[0].m,
                    GAVL_META_FORMAT, "Single images");

  
  return 1;
  }

static int open_stills_input(void * priv, const char * filename)
  {
  const char * tag;
  const bg_plugin_info_t * info;
  const gavl_metadata_t * m;
  
  input_t * inp = priv;
  
  /* Check if the first file exists */
  
  if(access(filename, R_OK))
    return 0;
  
  /* First of all, check if there is a plugin for this format */

  info = bg_plugin_find_by_filename(inp->plugin_reg,
                                    filename,
                                    BG_PLUGIN_IMAGE_READER);
  if(!info)
    return 0;
  
  inp->handle = bg_plugin_load(inp->plugin_reg, info);

  if(!inp->handle)
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN_DEC,
           "Plugin %s could not be loaded",
           info->name);
    return 0;
    }
  
  inp->image_reader = (bg_image_reader_plugin_t*)inp->handle->plugin;
  
  /* Create stream */
    
  inp->track_info.num_video_streams = 1;
  inp->track_info.video_streams =
    calloc(1, sizeof(*inp->track_info.video_streams));

  
  inp->track_info.video_streams[0].format.framerate_mode =
    GAVL_FRAMERATE_STILL;
  
  gavl_metadata_set_long(&inp->track_info.metadata, GAVL_META_APPROX_DURATION, inp->display_time);
  
  /* Get track name */

  bg_set_track_name_default(&inp->track_info, filename);

  inp->filename_buffer = bg_strdup(inp->filename_buffer, filename);

  if(!inp->image_reader->read_header(inp->handle->priv,
                                     inp->filename_buffer,
                                     &inp->track_info.video_streams[0].format))
    return 0;
  inp->track_info.video_streams[0].format.timescale = GAVL_TIME_SCALE;
  inp->track_info.video_streams[0].format.frame_duration = 0;
  inp->track_info.video_streams[0].format.framerate_mode = GAVL_FRAMERATE_STILL;

  /* Metadata */

  if(inp->image_reader->get_metadata &&
     (m = inp->image_reader->get_metadata(inp->handle->priv)))
    {
    gavl_metadata_copy(&inp->track_info.metadata, m);
    }

  tag = gavl_metadata_get(&inp->track_info.metadata,
                          GAVL_META_FORMAT);
  if(tag)
    gavl_metadata_set(&inp->track_info.video_streams[0].m,
                      GAVL_META_FORMAT, tag);
  else
    gavl_metadata_set(&inp->track_info.video_streams[0].m,
                      GAVL_META_FORMAT, "Image");
  
  gavl_metadata_set(&inp->track_info.metadata,
                    GAVL_META_FORMAT, "Still image");


  inp->header_read = 1;

  return 1;

  }

static bg_track_info_t * get_track_info_input(void * priv, int track)
  {
  input_t * inp = priv;
  return &inp->track_info;
  }

static int set_video_stream_input(void * priv, int stream,
                                  bg_stream_action_t action)
  {
  input_t * inp = priv;
  inp->action = action;

  /* Close image reader */
  if(inp->action == BG_STREAM_ACTION_READRAW)
    {
    /* Unload the plugin */
    if(inp->handle)
      bg_plugin_unref(inp->handle);
    inp->handle = NULL;
    inp->image_reader = NULL;
    }
  
  return 1;
  }

static int get_compression_info_input(void * priv, int stream,
                                      gavl_compression_info_t * ci)
  {
  int ret;
  input_t * inp = priv;
  if(!inp->image_reader || !inp->image_reader->get_compression_info)
    return 0;
  ret = inp->image_reader->get_compression_info(inp->handle->priv, ci);

  if(ret)
    {
    if(gavl_compression_need_pixelformat(ci->id) &&
       (inp->track_info.video_streams[0].format.pixelformat == GAVL_PIXELFORMAT_NONE))
      return 0;
    }
  return ret;
  }

static gavl_source_status_t
read_video_func_input(void * priv, gavl_video_frame_t ** fp)
  {
  gavl_video_format_t format;
  input_t * inp = priv;
  gavl_video_frame_t * f = *fp;
  
  if(inp->do_still)
    {
    if(inp->current_frame)
      return GAVL_SOURCE_EOF;
    }
  else if(inp->current_frame == inp->frame_end)
    return GAVL_SOURCE_EOF;
  
  if(!inp->header_read)
    {
    if(!inp->do_still)
      sprintf(inp->filename_buffer, inp->template, inp->current_frame);
    
    if(!inp->image_reader->read_header(inp->handle->priv,
                                       inp->filename_buffer,
                                       &format))
      return GAVL_SOURCE_EOF;
    }
  if(!inp->image_reader->read_image(inp->handle->priv, f))
    {
    return GAVL_SOURCE_EOF;
    }
  if(f)
    {
    f->timestamp = (inp->current_frame - inp->frame_start) * inp->frame_duration;

    if(inp->do_still)
      f->duration = -1;
    else
      f->duration = inp->frame_duration;
    }
  inp->header_read = 0;
  inp->current_frame++;
  return GAVL_SOURCE_OK;
  }

static int start_input(void * priv)
  {
  input_t * inp = priv;
  if(inp->action == BG_STREAM_ACTION_DECODE)
    inp->src = gavl_video_source_create(read_video_func_input, inp,
                                        0, &inp->track_info.video_streams[0].format);
  return 1;
  }

static gavl_video_source_t * get_video_source_input(void * priv, int stream)
  {
  input_t * inp = priv;
  return inp->src;
  }

static int has_frame_input(void * priv, int stream)
  {
  return 1;
  }
                           
static int read_video_frame_input(void * priv, gavl_video_frame_t* f,
                                  int stream)
  {
  input_t * inp = priv;
  return gavl_video_source_read_frame(inp->src, &f) == GAVL_SOURCE_OK;
  }

static int read_video_packet_input(void * priv, int stream, gavl_packet_t* p)
  {
  FILE * in;
  int64_t size;
  input_t * inp = priv;
  
  sprintf(inp->filename_buffer, inp->template, inp->current_frame);
  
  if(inp->do_still)
    {
    if(inp->current_frame)
      return 0;
    }
  else if(inp->current_frame == inp->frame_end)
    return 0;
  
  in = fopen(inp->filename_buffer, "rb");
  if(!in)
    return 0;

  fseek(in, 0, SEEK_END);
  size = ftell(in);
  fseek(in, 0, SEEK_SET);

  gavl_packet_alloc(p, size);

  p->data_len = fread(p->data, 1, size, in);
  
  fclose(in);
  
  if(p->data_len < size)
    return 0;

  p->pts = (inp->current_frame - inp->frame_start) * inp->frame_duration;
  
  p->flags = GAVL_PACKET_KEYFRAME;
  
  if(!inp->do_still)
    p->duration = inp->frame_duration;

  inp->current_frame++;
  
  return 1;
  }

static int set_track_input_stills(void * priv, int track)
  {
  input_t * inp = priv;
  
  /* Reset image reader */
  inp->current_frame = 0;
  if(inp->header_read)
    {
    inp->image_reader->read_image(inp->handle->priv,
                                  NULL);
    inp->header_read = 0;
    }
  return 1;
  }

static void seek_input(void * priv, int64_t * time, int scale)
  {
  input_t * inp = priv;
  int64_t time_scaled = gavl_time_rescale(scale, inp->timescale, *time);
  
  inp->current_frame = inp->frame_start + time_scaled / inp->frame_duration;

  time_scaled = (int64_t)inp->current_frame * inp->frame_duration;
  
  *time = gavl_time_rescale(inp->timescale, scale, time_scaled);
  }

static void stop_input(void * priv)
  {
  input_t * inp = priv;
  if(inp->action != BG_STREAM_ACTION_DECODE)
    return;
  }

static void close_input(void * priv)
  {
  input_t * inp = priv;
  if(inp->template)
    {
    free(inp->template);
    inp->template = NULL;
    }
  if(inp->filename_buffer)
    {
    free(inp->filename_buffer);
    inp->filename_buffer = NULL;
    }
  bg_track_info_free(&inp->track_info);
  
  /* Unload the plugin */
  if(inp->handle)
    bg_plugin_unref(inp->handle);
  inp->handle = NULL;
  inp->image_reader = NULL;

  if(inp->src)
    {
    gavl_video_source_destroy(inp->src);
    inp->src = NULL;
    }
  }

static void destroy_input(void* priv)
  {
  close_input(priv);
  free(priv);
  }

static const bg_input_plugin_t input_plugin =
  {
    .common =
    {
      BG_LOCALE,
      .name =           bg_singlepic_input_name,
      .long_name =      TRS("Image video input plugin"),
      .description =    TRS("This plugin reads series of images as a video. It uses the installed image readers."),
      .type =           BG_PLUGIN_INPUT,
      .flags =          BG_PLUGIN_FILE,
      .priority =       5,
      .create =         NULL,
      .destroy =        destroy_input,
      .get_parameters = get_parameters_input,
      .set_parameter =  set_parameter_input
    },
    .open =          open_input,

    //    .get_num_tracks = bg_avdec_get_num_tracks,

    .get_track_info = get_track_info_input,
    .get_video_compression_info = get_compression_info_input,
    /* Set streams */
    .set_video_stream =      set_video_stream_input,
    /*
     *  Start decoding.
     *  Track info is the track, which should be played.
     *  The plugin must take care of the "active" fields
     *  in the stream infos to check out, which streams are to be decoded
     */
    .start =                 start_input,

    .get_video_source = get_video_source_input,
    /* Read one video frame (returns FALSE on EOF) */
    .read_video =      read_video_frame_input,
    .read_video_packet = read_video_packet_input,
    /*
     *  Do percentage seeking (can be NULL)
     *  Media streams are supposed to be seekable, if this
     *  function is non-NULL AND the duration field of the track info
     *  is > 0
     */
    .seek = seek_input,
    /* Stop playback, close all decoders */
    .stop = stop_input,
    .close = close_input,
  };

static const bg_input_plugin_t input_plugin_stills =
  {
    .common =
    {
      BG_LOCALE,
      .name =           bg_singlepic_stills_input_name,
      .long_name =      "Still image input plugin",
      .description =    TRS("This plugin reads images as stills. It uses the installed image readers."),
      .type =           BG_PLUGIN_INPUT,
      .flags =          BG_PLUGIN_FILE,
      .priority =       BG_PLUGIN_PRIORITY_MAX,
      .create =         NULL,
      .destroy =        destroy_input,
      .get_parameters = get_parameters_input_still,
      .set_parameter =  set_parameter_input
    },
    .open =          open_stills_input,

    .set_track =     set_track_input_stills,
    
    //    .get_num_tracks = bg_avdec_get_num_tracks,

    .get_track_info = get_track_info_input,
    /* Set streams */
    .set_video_stream =      set_video_stream_input,
    /*
     *  Start decoding.
     *  Track info is the track, which should be played.
     *  The plugin must take care of the "active" fields
     *  in the stream infos to check out, which streams are to be decoded
     */
    .start =                 start_input,

    .has_still =             has_frame_input,
    
    /* Read one video frame (returns FALSE on EOF) */
    .read_video =      read_video_frame_input,
    /*
     *  Do percentage seeking (can be NULL)
     *  Media streams are supposed to be seekable, if this
     *  function is non-NULL AND the duration field of the track info
     *  is > 0
     */
    //    .seek = seek_input,
    /* Stop playback, close all decoders */
    .stop = stop_input,
    .close = close_input,
  };


const bg_plugin_common_t * bg_singlepic_input_get()
  {
  return (const bg_plugin_common_t*)(&input_plugin);
  }

const bg_plugin_common_t * bg_singlepic_stills_input_get()
  {
  return (const bg_plugin_common_t*)(&input_plugin_stills);
  }

static bg_plugin_info_t * get_input_info(bg_plugin_registry_t * reg,
                                         const bg_input_plugin_t * plugin)
  {
  bg_plugin_info_t * ret;
  
  if(!bg_plugin_registry_get_num_plugins(reg, BG_PLUGIN_IMAGE_READER,
                                         BG_PLUGIN_FILE))
    return NULL;

  ret = bg_plugin_info_create(&plugin->common);
  
  ret->extensions = get_extensions(reg, BG_PLUGIN_IMAGE_READER,
                                   BG_PLUGIN_FILE);
  return ret;
  }

bg_plugin_info_t * bg_singlepic_input_info(bg_plugin_registry_t * reg)
  {
  bg_plugin_info_t * ret;
  ret = get_input_info(reg, &input_plugin);
  if(ret)
    ret->parameters = bg_parameter_info_copy_array(parameters_input);
  return ret;
  }

bg_plugin_info_t * bg_singlepic_stills_input_info(bg_plugin_registry_t * reg)
  {
  bg_plugin_info_t * ret;
  ret = get_input_info(reg, &input_plugin_stills);
  if(ret)
    ret->parameters = bg_parameter_info_copy_array(parameters_input_still);
  return ret;
  }


void * bg_singlepic_input_create(bg_plugin_registry_t * reg)
  {
  input_t * ret;
  ret = calloc(1, sizeof(*ret));

  ret->plugin_reg = reg;

  return ret;
  }

void * bg_singlepic_stills_input_create(bg_plugin_registry_t * reg)
  {
  input_t * ret;
  ret = calloc(1, sizeof(*ret));

  ret->plugin_reg = reg;
  ret->do_still = 1;
  return ret;
  }

/* Encoder stuff */

static const bg_parameter_info_t parameters_encoder[] =
  {
    {
      .name =        "plugin",
      .long_name =   TRS("Plugin"),
      .type =        BG_PARAMETER_MULTI_MENU,
    },
    {
      .name =        "frame_digits",
      .long_name =   TRS("Framenumber digits"),
      .type =        BG_PARAMETER_INT,
      .val_min =     { .val_i = 1 },
      .val_max =     { .val_i = 9 },
      .val_default = { .val_i = 4 },
    },
    {
      .name =        "frame_offset",
      .long_name =   TRS("Framenumber offset"),
      .type =        BG_PARAMETER_INT,
      .val_min =     { .val_i = 0 },
      .val_max =     { .val_i = 1000000 },
      .val_default = { .val_i = 0 },
    }
  };

typedef struct
  {
  char * filename_base;
  
  bg_plugin_handle_t * plugin_handle;
  bg_image_writer_plugin_t * image_writer;
  
  gavl_metadata_t metadata;
  
  bg_parameter_info_t * parameters;

  bg_plugin_registry_t * plugin_reg;
  
  int frame_digits, frame_offset;
  int64_t frame_counter;
  
  char * mask;
  char * filename_buffer;
  
  gavl_video_format_t format;
  
  bg_encoder_callbacks_t * cb;
  
  int have_header;
  
  bg_iw_callbacks_t iw_callbacks;
  
  const gavl_compression_info_t * ci;

  gavl_video_sink_t * sink;
  gavl_packet_sink_t * psink;
  
  } encoder_t;

static int iw_callbacks_create_output_file(void * priv, const char * filename)
  {
  encoder_t * e = priv;
  return bg_encoder_cb_create_output_file(e->cb, filename);
  }

static bg_parameter_info_t *
create_encoder_parameters(bg_plugin_registry_t * plugin_reg)
  {
  bg_parameter_info_t * ret = bg_parameter_info_copy_array(parameters_encoder);
  bg_plugin_registry_set_parameter_info(plugin_reg,
                                        BG_PLUGIN_IMAGE_WRITER,
                                        BG_PLUGIN_FILE, &ret[0]);
  return ret;
  }

static const bg_parameter_info_t * get_parameters_encoder(void * priv)
  {
  encoder_t * enc = priv;
  
  if(!enc->parameters)
    enc->parameters = create_encoder_parameters(enc->plugin_reg);
  return enc->parameters;
  }

static void set_parameter_encoder(void * priv, const char * name, 
                                  const bg_parameter_value_t * val)
  {
  const bg_plugin_info_t * info;
  
  encoder_t * e = priv;
  
  if(!name)
    {
    return;
    }
  else if(!strcmp(name, "plugin"))
    {
    /* Load plugin */

    if(!e->plugin_handle || strcmp(e->plugin_handle->info->name, val->val_str))
      {
      if(e->plugin_handle)
        {
        bg_plugin_unref(e->plugin_handle);
        e->plugin_handle = NULL;
        }
      info = bg_plugin_find_by_name(e->plugin_reg, val->val_str);
      e->plugin_handle = bg_plugin_load(e->plugin_reg, info);
      e->image_writer = (bg_image_writer_plugin_t*)(e->plugin_handle->plugin);
      
      if(e->image_writer->set_callbacks)
        e->image_writer->set_callbacks(e->plugin_handle->priv, &e->iw_callbacks);
      }
    }
  else if(!strcmp(name, "frame_digits"))
    {
    e->frame_digits = val->val_i;
    }
  else if(!strcmp(name, "frame_offset"))
    {
    e->frame_offset = val->val_i;
    }
  else
    {
    if(e->plugin_handle && e->plugin_handle->plugin->set_parameter)
      {
      e->plugin_handle->plugin->set_parameter(e->plugin_handle->priv, name, val);
      }
    }
  }

static void set_callbacks_encoder(void * data, bg_encoder_callbacks_t * cb)
  {
  encoder_t * e = data;
  e->cb = cb;
  }

static void create_mask(encoder_t * e, const char * ext)
  {
  char * tmp_string;
  int filename_len;
  e->mask = bg_strdup(e->mask, e->filename_base);
  
  tmp_string = bg_sprintf("-%%0%d"PRId64".%s", e->frame_digits, ext);
  e->mask = bg_strcat(e->mask, tmp_string);
  free(tmp_string);
  
  filename_len = strlen(e->filename_base) + e->frame_digits + strlen(ext) + 16;
  e->filename_buffer = malloc(filename_len);
  }

static int open_encoder(void * data, const char * filename,
                        const gavl_metadata_t * metadata,
                        const gavl_chapter_list_t * chapter_list)
  {
  encoder_t * e = data;
  
  e->frame_counter = e->frame_offset;
  e->filename_base = bg_strdup(e->filename_base, filename);
  
  if(metadata)
    gavl_metadata_copy(&e->metadata, metadata);
  
  return 1;
  }

static int write_frame_header(encoder_t * e)
  {
  int ret;
  e->have_header = 1;

  /* Create filename */

  sprintf(e->filename_buffer, e->mask, e->frame_counter);
  
  ret = e->image_writer->write_header(e->plugin_handle->priv,
                                      e->filename_buffer,
                                      &e->format, &e->metadata);
  
  if(!ret)
    {
    e->have_header = 0;
    }
  e->frame_counter++;
  return ret;
  }

static gavl_sink_status_t
write_video_func_encoder(void * data, gavl_video_frame_t * frame)
  {
  int ret = 1;
  encoder_t * e = data;

  if(!e->have_header)
    ret = write_frame_header(e);
  if(ret)
    ret = e->image_writer->write_image(e->plugin_handle->priv, frame);
  
  e->have_header = 0;
  
  return ret ? GAVL_SINK_OK : GAVL_SINK_ERROR;
  }

static gavl_sink_status_t write_packet_encoder(void * data, gavl_packet_t * packet)
  {
  FILE * out;
  encoder_t * e = data;
  sprintf(e->filename_buffer, e->mask, e->frame_counter);

  if(!bg_encoder_cb_create_output_file(e->cb, e->filename_buffer))
    return GAVL_SINK_ERROR;
  out = fopen(e->filename_buffer, "wb");
  if(!out)
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN_ENC, "Cannot open file %s: %s",
           e->filename_buffer, strerror(errno));
    return GAVL_SINK_ERROR;
    }
  
  if(fwrite(packet->data, 1, packet->data_len, out) < packet->data_len)
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN_ENC, "Couldn't write data to %s: %s",
           e->filename_buffer, strerror(errno));
    return GAVL_SINK_ERROR;
    }
  fclose(out);
  e->frame_counter++;
  return GAVL_SINK_OK;
  }

static int start_encoder(void * data)
  {
  encoder_t * e = data;

  if(e->have_header)
    e->sink = gavl_video_sink_create(NULL, write_video_func_encoder,
                                     e, &e->format);
  else if(e->ci)
    e->psink = gavl_packet_sink_create(NULL, write_packet_encoder,
                                       e);
  
  return (e->have_header || e->ci) ? 1 : 0;
  }

static gavl_video_sink_t * get_video_sink_encoder(void * priv, int stream)
  {
  encoder_t * e = priv;
  return e->sink;
  }

static gavl_packet_sink_t * get_packet_sink_encoder(void * priv, int stream)
  {
  encoder_t * e = priv;
  return e->psink;
  }

static int writes_compressed_video(void * priv,
                                   const gavl_video_format_t * format,
                                   const gavl_compression_info_t * info)
  {
  int separate = 0;
  
  if(gavl_compression_get_extension(info->id, &separate) &&
     separate)
    return 1;
  return 0;
  }



static int add_video_stream_encoder(void * data,
                                    const gavl_metadata_t * m,
                                    const gavl_video_format_t * format)
  {
  char ** extensions;
  encoder_t * e = data;
  
  gavl_video_format_copy(&e->format, format);

  extensions = bg_strbreak(e->image_writer->extensions, ' ');
  create_mask(e, extensions[0]);
  bg_strbreak_free(extensions);
  
  /* Write image header so we know the format */
  
  write_frame_header(e);
  return 0;
  }

static int
add_video_stream_compressed_encoder(void * data,
                                    const gavl_metadata_t * m,
                                    const gavl_video_format_t * format,
                                    const gavl_compression_info_t * info)
  {
  encoder_t * e = data;
  e->ci = info;
  create_mask(e, gavl_compression_get_extension(e->ci->id, NULL));
  return 0;
  }


#define STR_FREE(s) if(s){free(s);s=NULL;}

static int close_encoder(void * data, int do_delete)
  {
  int64_t i;
  
  encoder_t * e = data;

  if(do_delete)
    {
    for(i = e->frame_offset; i < e->frame_counter; i++)
      {
      sprintf(e->filename_buffer, e->mask, i);
      remove(e->filename_buffer);
      }
    }
  
  STR_FREE(e->mask);
  STR_FREE(e->filename_buffer);
  STR_FREE(e->filename_base);
  
  gavl_metadata_free(&e->metadata);
  
  if(e->plugin_handle)
    {
    bg_plugin_unref(e->plugin_handle);
    e->plugin_handle = NULL;
    }

  if(e->sink)
    {
    gavl_video_sink_destroy(e->sink);
    e->sink = NULL;
    }
  
  return 1;
  }

static void destroy_encoder(void * data)
  {
  encoder_t * e = data;
  close_encoder(data, 0);

  if(e->parameters)
    {
    bg_parameter_info_destroy_array(e->parameters);
    }
  free(e);
  }


const bg_encoder_plugin_t encoder_plugin =
  {
    .common =
    {
      BG_LOCALE,
      .name =           bg_singlepic_encoder_name,
      .long_name =      "Singlepicture encoder",
      .description =    TRS("This plugin encodes a video as a series of images. It uses the installed image writers."),

      .type =           BG_PLUGIN_ENCODER_VIDEO,
      .flags =          BG_PLUGIN_FILE,
      .priority =       BG_PLUGIN_PRIORITY_MAX,
      .create =         NULL,
      .destroy =        destroy_encoder,
      .get_parameters = get_parameters_encoder,
      .set_parameter =  set_parameter_encoder
    },
    
    /* Maximum number of audio/video streams. -1 means infinite */
    
    .max_audio_streams = 0,
    .max_video_streams = 1,

    /* Open a file, filename base is without extension, which
       will be added by the plugin */
    
    .set_callbacks =     set_callbacks_encoder,
    
    .open =              open_encoder,

    .writes_compressed_video = writes_compressed_video,
    
    .add_video_stream =  add_video_stream_encoder,
    .add_video_stream_compressed =  add_video_stream_compressed_encoder,
    
    .start =             start_encoder,

    .get_video_sink =    get_video_sink_encoder,
    .get_video_packet_sink =    get_packet_sink_encoder,
    
    //    .write_video_frame = write_video_frame_encoder,
    //    .write_video_packet = write_video_packet_encoder,
    
    
    /* Close it */

    .close =             close_encoder,
  };

const bg_plugin_common_t * bg_singlepic_encoder_get()
  {
  return (bg_plugin_common_t*)(&encoder_plugin);
  }

bg_plugin_info_t * bg_singlepic_encoder_info(bg_plugin_registry_t * reg)
  {
  bg_plugin_info_t * ret;
  
  if(!bg_plugin_registry_get_num_plugins(reg, BG_PLUGIN_IMAGE_WRITER,
                                         BG_PLUGIN_FILE))
    return NULL;
  ret = bg_plugin_info_create(&encoder_plugin.common);
  ret->parameters = create_encoder_parameters(reg);
  return ret;
  }

void * bg_singlepic_encoder_create(bg_plugin_registry_t * reg)
  {
  encoder_t * ret;
  ret = calloc(1, sizeof(*ret));

  ret->plugin_reg = reg;

  ret->iw_callbacks.data = ret;
  ret->iw_callbacks.create_output_file = iw_callbacks_create_output_file;
  
  return ret;
  }
