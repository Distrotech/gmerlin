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
#include <ctype.h>
#include <string.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <config.h>
#include <translation.h>


#include <cfg_registry.h>
#include <pluginregistry.h>
#include <config.h>
#include <utils.h>
#include <singlepic.h>

#if 0
char * bg_singlepic_ouput_name = "e_singlepic";
char * bg_singlepic_input_name = "i_singlepic";
char * bg_singlepic_stills_input_name = "i_singlepic_stills";
#endif

static char * get_extensions(bg_plugin_registry_t * reg,
                             uint32_t type_mask, uint32_t flag_mask)
  {
  int num, i;
  char * ret = (char*)0;
  const bg_plugin_info_t * info;
  
  num = bg_plugin_registry_get_num_plugins(reg, type_mask, flag_mask);
  if(!num)
    return (char*)0;

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

static bg_parameter_info_t parameters_input[] =
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

static bg_parameter_info_t parameters_input_still[] =
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
  } input_t;

static bg_parameter_info_t * get_parameters_input(void * priv)
  {
  return parameters_input;
  }

static bg_parameter_info_t * get_parameters_input_still(void * priv)
  {
  return parameters_input_still;
  }

static void set_parameter_input(void * priv, const char * name,
                                const bg_parameter_value_t * val)
  {
  input_t * inp = (input_t *)priv;

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
  struct stat stat_buf;
  char * tmp_string;
  const char * pos;
  const char * pos_start;
  const char * pos_end;
  
  input_t * inp = (input_t *)priv;
  
  /* Check if the first file exists */
  
  if(stat(filename, &stat_buf))
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

  inp->filename_buffer = malloc(strlen(filename)+1);

  while(1)
    {
    sprintf(inp->filename_buffer, inp->template, inp->frame_end);
    if(stat(inp->filename_buffer, &stat_buf))
      break;
    inp->frame_end++;
    }

  /* Create stream */
    
  inp->track_info.num_video_streams = 1;
  inp->track_info.video_streams =
    calloc(1, sizeof(*inp->track_info.video_streams));
  
  inp->track_info.duration = gavl_frames_to_time(inp->timescale,
                                                 inp->frame_duration,
                                                 inp->frame_end -
                                                 inp->frame_start);
  
  /* Get track name */

  bg_set_track_name_default(&(inp->track_info), filename);
  
  inp->track_info.seekable = 1;
  return 1;
  }

static int open_stills_input(void * priv, const char * filename)
  {
  const bg_plugin_info_t * info;
  struct stat stat_buf;
  input_t * inp = (input_t *)priv;
  
  /* Check if the first file exists */
  
  if(stat(filename, &stat_buf))
    return 0;
  
  /* First of all, check if there is a plugin for this format */

  info = bg_plugin_find_by_filename(inp->plugin_reg,
                                    filename,
                                    BG_PLUGIN_IMAGE_READER);
  if(!info)
    return 0;
  
  inp->handle = bg_plugin_load(inp->plugin_reg, info);
  inp->image_reader = (bg_image_reader_plugin_t*)inp->handle->plugin;
  
  /* Create stream */
    
  inp->track_info.num_video_streams = 1;
  inp->track_info.video_streams =
    calloc(1, sizeof(*inp->track_info.video_streams));
  
  inp->track_info.video_streams[0].is_still = 1;
  
  inp->track_info.duration = inp->display_time;
  
  /* Get track name */

  bg_set_track_name_default(&(inp->track_info), filename);

  inp->filename_buffer = bg_strdup(inp->filename_buffer, filename);
  return 1;

  }

static bg_track_info_t * get_track_info_input(void * priv, int track)
  {
  input_t * inp = (input_t *)priv;
  return &(inp->track_info);
  }

static int set_video_stream_input(void * priv, int stream,
                                  bg_stream_action_t action)
  {
  input_t * inp = (input_t *)priv;
  inp->action = action;
  return 1;
  }

static int start_input(void * priv)
  {
  input_t * inp = (input_t *)priv;

  if(inp->action != BG_STREAM_ACTION_DECODE)
    return 1;

  inp->current_frame = inp->frame_start;
  
  if(inp->do_still)
    {
    inp->track_info.video_streams[0].description =
      bg_strdup(NULL, "Still Image");
    }
  else
    {
    inp->track_info.video_streams[0].description =
      bg_strdup(NULL, "Single images");
    sprintf(inp->filename_buffer, inp->template, inp->current_frame);
    }
  
  if(inp->do_still)
    {
    if(!inp->image_reader->read_header(inp->handle->priv,
                                       inp->filename_buffer,
                                       &(inp->track_info.video_streams[0].format)))
      return 0;
    inp->track_info.video_streams[0].format.timescale = 0;
    inp->track_info.video_streams[0].format.frame_duration = 0;
    inp->track_info.video_streams[0].format.framerate_mode = GAVL_FRAMERATE_STILL;
    }
  else
    {
    if(!inp->image_reader->read_header(inp->handle->priv,
                                       inp->filename_buffer,
                                       &(inp->track_info.video_streams[0].format)))
      return 0;
    inp->track_info.video_streams[0].format.timescale =
      inp->timescale;
    inp->track_info.video_streams[0].format.frame_duration =
      inp->frame_duration;
    inp->track_info.video_streams[0].format.framerate_mode =
      GAVL_FRAMERATE_CONSTANT;
    }
  inp->header_read = 1;
  return 1;
  }

static int read_video_frame_input(void * priv, gavl_video_frame_t* f,
                                  int stream)
  {
  gavl_video_format_t format;
  input_t * inp = (input_t *)priv;
  
  if(inp->do_still)
    {
    if(inp->current_frame)
      return 0;
    }
  else if(inp->current_frame == inp->frame_end)
    return 0;
  
  if(!inp->header_read)
    {
    sprintf(inp->filename_buffer, inp->template, inp->current_frame);
    
    if(!inp->image_reader->read_header(inp->handle->priv,
                                       inp->filename_buffer,
                                       &format))
      return 0;
    }
  if(!inp->image_reader->read_image(inp->handle->priv, f))
    {
    return 0;
    }
  if(f)
    {
    f->timestamp = (inp->current_frame - inp->frame_start) * inp->frame_duration;
    }
  inp->header_read = 0;
  inp->current_frame++;
  
  return 1;
  }

static int set_track_input_stills(void * priv, int track)
  {
  input_t * inp = (input_t *)priv;
  
  /* Reset image reader */
  inp->current_frame = 0;
  if(inp->header_read)
    {
    inp->image_reader->read_image(inp->handle->priv,
                                  (gavl_video_frame_t*)0);
    inp->header_read = 0;
    }
  return 1;
  }

static void seek_input(void * priv, gavl_time_t * time)
  {
  input_t * inp = (input_t *)priv;

  inp->current_frame = inp->frame_start + gavl_time_to_frames(inp->timescale,
                                                              inp->frame_duration,
                                                              *time);
  *time = gavl_frames_to_time(inp->timescale,
                              inp->frame_duration,
                              inp->current_frame - inp->frame_start);
  }

static void stop_input(void * priv)
  {
  input_t * inp = (input_t *)priv;
  if(inp->action != BG_STREAM_ACTION_DECODE)
    return;
  }

static void close_input(void * priv)
  {
  input_t * inp = (input_t *)priv;
  if(inp->template)
    {
    free(inp->template);
    inp->template = (char*)0;
    }
  if(inp->filename_buffer)
    {
    free(inp->filename_buffer);
    inp->filename_buffer = (char*)0;
    }
  bg_track_info_free(&(inp->track_info));
  
  /* Unload the plugin */
  if(inp->handle)
    bg_plugin_unref(inp->handle);
  inp->handle = NULL;
  inp->image_reader = NULL;
  }

static void destroy_input(void* priv)
  {
  close_input(priv);
  free(priv);
  }

static bg_input_plugin_t input_plugin =
  {
    .common =
    {
      BG_LOCALE,
      .name =           bg_singlepic_input_name,
      .long_name =      TRS("Image video input plugin"),
      .description =    TRS("This plugin reads series of images as a video. It uses the installed image readers."),
      .extensions =     NULL, /* Filled in later */
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
    /* Set streams */
    .set_video_stream =      set_video_stream_input,
    /*
     *  Start decoding.
     *  Track info is the track, which should be played.
     *  The plugin must take care of the "active" fields
     *  in the stream infos to check out, which streams are to be decoded
     */
    .start =                 start_input,
    /* Read one video frame (returns FALSE on EOF) */
    .read_video_frame =      read_video_frame_input,
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

static bg_input_plugin_t input_plugin_stills =
  {
    .common =
    {
      BG_LOCALE,
      .name =           bg_singlepic_stills_input_name,
      .long_name =      "Still image input plugin",
      .description =    TRS("This plugin reads images as stills. It uses the installed image readers."),
      .extensions =     NULL, /* Filled in later */
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
    /* Read one video frame (returns FALSE on EOF) */
    .read_video_frame =      read_video_frame_input,
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


bg_plugin_common_t * bg_singlepic_input_get()
  {
  return (bg_plugin_common_t*)(&input_plugin);
  }

bg_plugin_common_t * bg_singlepic_stills_input_get()
  {
  return (bg_plugin_common_t*)(&input_plugin_stills);
  }

static bg_plugin_info_t * get_input_info(bg_plugin_registry_t * reg,
                                         bg_input_plugin_t * plugin)
  {
  bg_plugin_info_t * ret;
  
  if(!bg_plugin_registry_get_num_plugins(reg, BG_PLUGIN_IMAGE_READER,
                                         BG_PLUGIN_FILE))
    return (bg_plugin_info_t *)0;
  
  ret = calloc(1, sizeof(*ret));

  ret->gettext_domain      = bg_strdup(ret->gettext_domain, plugin->common.gettext_domain);
  
  ret->gettext_directory   = bg_strdup(ret->gettext_directory,
                                       plugin->common.gettext_directory);
  
  
  ret->name      = bg_strdup(ret->name, plugin->common.name);
  ret->long_name = bg_strdup(ret->long_name, plugin->common.long_name);
  ret->description = bg_strdup(ret->description, plugin->common.description);
  
  ret->extensions = get_extensions(reg, BG_PLUGIN_IMAGE_READER,
                                   BG_PLUGIN_FILE);
  ret->priority  =  plugin->common.priority;
  ret->type  =  plugin->common.type;
  ret->flags =  plugin->common.flags;
  return ret;
  }

bg_plugin_info_t * bg_singlepic_input_info(bg_plugin_registry_t * reg)
  {
  bg_plugin_info_t * ret;
  ret = get_input_info(reg, &input_plugin);
  ret->parameters = bg_parameter_info_copy_array(parameters_input);
  return ret;
  }

bg_plugin_info_t * bg_singlepic_stills_input_info(bg_plugin_registry_t * reg)
  {
  bg_plugin_info_t * ret;
  ret = get_input_info(reg, &input_plugin_stills);
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

static bg_parameter_info_t parameters_encoder[] =
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
  char * first_filename;
  
  bg_plugin_handle_t * plugin_handle;
  bg_image_writer_plugin_t * image_writer;
  
  bg_parameter_info_t * parameters;

  bg_plugin_registry_t * plugin_reg;
  
  int frame_digits, frame_offset;
  int64_t frame_counter;
  
  char * extension;
  char * extension_mask;
  char * mask;
  char * filename_buffer;
  
  gavl_video_format_t format;

  int have_header;
  } encoder_t;

static bg_parameter_info_t *
create_encoder_parameters(bg_plugin_registry_t * plugin_reg)
  {
  int i;
  bg_parameter_info_t * ret;
  ret =
    calloc(sizeof(parameters_encoder)/sizeof(parameters_encoder[0])+1,
           sizeof(*ret));
  
  for(i = 0; i < sizeof(parameters_encoder)/sizeof(parameters_encoder[0]); i++)
    {
    bg_parameter_info_copy(&ret[i], &parameters_encoder[i]);
    }
  bg_plugin_registry_set_parameter_info(plugin_reg,
                                        BG_PLUGIN_IMAGE_WRITER,
                                        BG_PLUGIN_FILE, &ret[0]);
  return ret;
  }

static bg_parameter_info_t * get_parameters_encoder(void * priv)
  {
  encoder_t * enc = (encoder_t *)priv;
  
  if(!enc->parameters)
    enc->parameters = create_encoder_parameters(enc->plugin_reg);
  return enc->parameters;
  }

static void set_parameter_encoder(void * priv, const char * name, 
                                  const bg_parameter_value_t * val)
  {
  const bg_plugin_info_t * info;
  
  encoder_t * e;
  e = (encoder_t *)priv;
  
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
        e->plugin_handle = (bg_plugin_handle_t*)0;
        }
      info = bg_plugin_find_by_name(e->plugin_reg, val->val_str);
      e->plugin_handle = bg_plugin_load(e->plugin_reg, info);
      e->image_writer = (bg_image_writer_plugin_t*)(e->plugin_handle->plugin);
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


static const char * get_extension_encoder(void * data)
  {
  const char * plugin_extension;
  
  encoder_t * e;
  e = (encoder_t *)data;

  if(!e->extension)
    {
    /* Create extension */
    
    plugin_extension = e->image_writer->get_extension(e->plugin_handle->priv);
    e->extension_mask = bg_sprintf("-%%0%d"PRId64"%s", e->frame_digits, plugin_extension);
    e->extension      = bg_sprintf(e->extension_mask, (int64_t)e->frame_offset);

    }
  return e->extension;
  }


static int open_encoder(void * data, const char * filename,
                        bg_metadata_t * metadata,
                        bg_chapter_list_t * chapter_list)
  {
  encoder_t * e;
  int filename_len;
  
  e = (encoder_t *)data;

  /* Create the final mask */

  e->mask = bg_strdup(e->mask, filename);

  filename_len = strlen(filename);
  
  e->mask[filename_len - strlen(get_extension_encoder(data))] = '\0';

  e->mask = bg_strcat(e->mask, e->extension_mask);


  e->filename_buffer = malloc(filename_len+1);

  e->frame_counter = e->frame_offset;

  e->first_filename = bg_sprintf(e->mask, e->frame_counter);

  return 1;
  }

static const char * get_filename_encoder(void * data)
  {
  encoder_t * e;
  e = (encoder_t *)data;
  return e->first_filename;
  }

static int write_frame_header(encoder_t * e)
  {
  int ret;
  e->have_header = 1;

  /* Create filename */

  sprintf(e->filename_buffer, e->mask, e->frame_counter);
  
  ret = e->image_writer->write_header(e->plugin_handle->priv,
                                      e->filename_buffer,
                                      &(e->format));

  if(!ret)
    {
    e->have_header = 0;
    }
  e->frame_counter++;
  return ret;
  }

static int start_encoder(void * data)
  {
  encoder_t * e;
  e = (encoder_t *)data;
  return e->have_header;
  }
     
static int add_video_stream_encoder(void * data, gavl_video_format_t * format)
  {
  encoder_t * e;
  e = (encoder_t *)data;

  gavl_video_format_copy(&(e->format), format);

  /* Write image header so we know the format */
  
  write_frame_header(e);
  return 0;
  }

static void get_video_format_encoder(void * data, int stream,
                                     gavl_video_format_t * format)
  {
  encoder_t * e;

  e = (encoder_t *)data;
  gavl_video_format_copy(format, &(e->format));
  }

static int
write_video_frame_encoder(void * data, gavl_video_frame_t * frame,
                          int stream)
  {
  int ret = 1;
  encoder_t * e;

  e = (encoder_t *)data;

  if(!e->have_header)
    {
    ret = write_frame_header(e);
    }
  if(ret)
    ret = e->image_writer->write_image(e->plugin_handle->priv, frame);
  
  e->have_header = 0;
  return ret;
  }

#define STR_FREE(s) if(s){free(s);s=(char*)0;}

static int close_encoder(void * data, int do_delete)
  {
  int64_t i;
  
  encoder_t * e;
  e = (encoder_t *)data;

  if(do_delete)
    {
    for(i = e->frame_offset; i < e->frame_counter; i++)
      {
      sprintf(e->filename_buffer, e->mask, i);
      remove(e->filename_buffer);
      }
    }
  
  STR_FREE(e->extension);
  STR_FREE(e->extension_mask);
  STR_FREE(e->mask);
  STR_FREE(e->filename_buffer);
  STR_FREE(e->first_filename);
  
  if(e->plugin_handle)
    {
    bg_plugin_unref(e->plugin_handle);
    e->plugin_handle = (bg_plugin_handle_t*)0;
    }
  return 1;
  }

static void destroy_encoder(void * data)
  {
  encoder_t * e;
  e = (encoder_t *)data;
  close_encoder(data, 0);

  if(e->parameters)
    {
    bg_parameter_info_destroy_array(e->parameters);
    }
  free(e);
  }


bg_encoder_plugin_t encoder_plugin =
  {
    .common =
    {
      BG_LOCALE,
      .name =           bg_singlepic_encoder_name,
      .long_name =      "Singlepicture encoder",
      .description =    TRS("This plugin encodes a video as a series of images. It uses the installed image writers."),

      .extensions =     NULL, /* Filled in later */
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

    .get_extension =     get_extension_encoder,
    
    /* Open a file, filename base is without extension, which
       will be added by the plugin */
        
    .open =              open_encoder,
    .get_filename =      get_filename_encoder,
    
    .add_video_stream =  add_video_stream_encoder,
    
    .get_video_format =  get_video_format_encoder,

    .start =             start_encoder,
    
    .write_video_frame = write_video_frame_encoder,
    
    /* Close it */

    .close =             close_encoder,
  };

bg_plugin_common_t * bg_singlepic_encoder_get()
  {
  return (bg_plugin_common_t*)(&encoder_plugin);
  }

bg_plugin_info_t * bg_singlepic_encoder_info(bg_plugin_registry_t * reg)
  {
  bg_plugin_info_t * ret;
  
  if(!bg_plugin_registry_get_num_plugins(reg, BG_PLUGIN_IMAGE_WRITER,
                                         BG_PLUGIN_FILE))
    return (bg_plugin_info_t *)0;
  
  ret = calloc(1, sizeof(*ret));

  ret->gettext_domain      = bg_strdup(ret->gettext_domain, encoder_plugin.common.gettext_domain);
  ret->gettext_directory   = bg_strdup(ret->gettext_directory,
                                       encoder_plugin.common.gettext_directory);
  
  
  ret->name      = bg_strdup(ret->name, encoder_plugin.common.name);
  ret->long_name = bg_strdup(ret->long_name, encoder_plugin.common.long_name);
  ret->description = bg_strdup(ret->description,
                               encoder_plugin.common.description);
  
  ret->extensions = get_extensions(reg, BG_PLUGIN_IMAGE_WRITER,
                                   BG_PLUGIN_FILE);
  ret->type     = encoder_plugin.common.type;
  ret->flags    = encoder_plugin.common.flags;
  ret->priority = encoder_plugin.common.priority;
  ret->parameters = create_encoder_parameters(reg);
  
  return ret;
  }

void * bg_singlepic_encoder_create(bg_plugin_registry_t * reg)
  {
  encoder_t * ret;
  ret = calloc(1, sizeof(*ret));

  ret->plugin_reg = reg;

  return ret;
  }
