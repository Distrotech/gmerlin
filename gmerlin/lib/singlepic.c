/*****************************************************************
 
  singlepic.c
 
  Copyright (c) 2003-2004 by Burkhard Plaum - plaum@ipf.uni-stuttgart.de
 
  http://gmerlin.sourceforge.net
 
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
 
  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111, USA.
 
*****************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <cfg_registry.h>
#include <pluginregistry.h>
#include <config.h>
#include <utils.h>

char * bg_singlepic_ouput_name = "e_singlepic";
char * bg_singlepic_input_name = "i_singlepic";

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

static void set_plugin_parameter(bg_parameter_info_t * ret,
                                 bg_plugin_registry_t * reg,
                                 uint32_t type_mask, uint32_t flag_mask)
  {
  int num_plugins, i;
  const bg_plugin_info_t * info;
  bg_plugin_handle_t * h;

  num_plugins =
    bg_plugin_registry_get_num_plugins(reg, type_mask, flag_mask);

  ret->multi_names      = calloc(num_plugins + 1, sizeof(*ret->multi_names));
  ret->multi_labels     = calloc(num_plugins + 1, sizeof(*ret->multi_labels));
  ret->multi_parameters = calloc(num_plugins + 1,
                                 sizeof(*ret->multi_parameters));
  
  for(i = 0; i < num_plugins; i++)
    {
    info = bg_plugin_find_by_index(reg, i,
                                   type_mask, flag_mask);
    ret->multi_names[i] = bg_strdup(NULL, info->name);

    if(!i) /* First plugin is the default one */
      {
      ret->val_default.val_str = bg_strdup(NULL, info->name);
      }

    ret->multi_labels[i] = bg_strdup(NULL, info->long_name);

    h = bg_plugin_load(reg, info);
    if(h->plugin->get_parameters)
      {
      ret->multi_parameters[i] =
        bg_parameter_info_copy_array(h->plugin->get_parameters(h->priv));
      }
    bg_plugin_unref(h);
    }
  }

/* Input stuff */

static bg_parameter_info_t parameters_input[] =
  {
    {
      name:      "timescale",
      long_name: "Timescale",
      type:      BG_PARAMETER_INT,
      val_min:     { val_i: 1 },
      val_max:     { val_i: 100000 },
      val_default: { val_i: 25 }
    },
    {
      name:      "frame_duration",
      long_name: "Frame duration",
      type:      BG_PARAMETER_INT,
      val_min:     { val_i: 1 },
      val_max:     { val_i: 100000 },
      val_default: { val_i: 1 }
    },
  };

typedef struct
  {
  bg_track_info_t track_info;
  bg_parameter_info_t * parameters;
  bg_plugin_registry_t * plugin_reg;
  
  int timescale;
  int frame_duration;

  char * template;
  int64_t frame_start;
  int64_t frame_end;
  int64_t current_frame;

  bg_plugin_handle_t * handle;
  bg_image_reader_plugin_t * image_reader;

  char * filename_buffer;
  int header_read;

  bg_stream_action_t action;
  } input_t;

static bg_parameter_info_t * get_parameters_input(void * priv)
  {
  input_t * inp = (input_t *)priv;
  inp->parameters =
    calloc(sizeof(parameters_input)/sizeof(parameters_input[0])+1,
           sizeof(*inp->parameters));
  
  bg_parameter_info_copy(&inp->parameters[0], &parameters_input[0]);
  bg_parameter_info_copy(&inp->parameters[1], &parameters_input[1]);

  return inp->parameters;
  }

static void set_parameter_input(void * priv, char * name,
                                bg_parameter_value_t * val)
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
  
  }

static int open_input(void * priv, const char * arg)
  {
  struct stat stat_buf;
  char * tmp_string;
  const char * filename;
  const char * pos;
  const char * pos_start;
  const char * pos_end;
  
  input_t * inp = (input_t *)priv;

  filename = arg;

  /* Check if the first file exists */
  
  if(stat(filename, &stat_buf))
    return 0;
    
  
  /* First of all, check if there is a plugin for this format */
  
  if(!bg_plugin_find_by_filename(inp->plugin_reg, filename,
                                 BG_PLUGIN_IMAGE_READER))
    return 0;
  
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
  //  fprintf(stderr, "Frames: %lld .. %lld, template: %s\n",
  //         inp->frame_start, inp->frame_end, inp->template);

  /* Create stream */
    
  inp->track_info.num_video_streams = 1;
  inp->track_info.video_streams =
    calloc(1, sizeof(*inp->track_info.video_streams));
  
  inp->track_info.duration = gavl_frames_to_time(inp->timescale,
                                                 inp->frame_duration,
                                                 inp->frame_end -
                                                 inp->frame_start);

  /* Get track name */

  bg_set_track_name_default(&(inp->track_info), arg);
  
  inp->track_info.seekable = 1;
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

static void start_input(void * priv)
  {
  const bg_plugin_info_t * info;
  
  input_t * inp = (input_t *)priv;

  if(inp->action != BG_STREAM_ACTION_DECODE)
    return;
  
  inp->track_info.video_streams[0].description =
    bg_strdup(NULL, "Single images\n");

  /* Load plugin */

  info = bg_plugin_find_by_filename(inp->plugin_reg, inp->template,
                                    BG_PLUGIN_IMAGE_READER);

  inp->handle = bg_plugin_load(inp->plugin_reg, info);
  inp->image_reader = (bg_image_reader_plugin_t*)inp->handle->plugin;

  inp->current_frame = inp->frame_start;

  sprintf(inp->filename_buffer, inp->template, inp->current_frame);
  
  if(!inp->image_reader->read_header(inp->handle->priv,
                                     inp->filename_buffer,
                                     &(inp->track_info.video_streams[0].format)))
    return;

  inp->header_read = 1;
  inp->track_info.video_streams[0].format.timescale = inp->timescale;
  inp->track_info.video_streams[0].format.frame_duration = inp->frame_duration;
  inp->track_info.video_streams[0].format.free_framerate = 0;
  }

static int read_video_frame_input(void * priv, gavl_video_frame_t* f,
                                  int stream)
  {
  gavl_video_format_t format;
  input_t * inp = (input_t *)priv;

  if(inp->current_frame == inp->frame_end)
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
    return 0;
  if(f)
    {
    f->time_scaled = (inp->current_frame - inp->frame_start) * inp->frame_duration;
    f->time = gavl_frames_to_time(inp->timescale, inp->frame_duration,
                                  inp->current_frame - inp->frame_start);
    }
  inp->header_read = 0;
  inp->current_frame++;
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
  /* Unload the plugin */

  if(inp->action != BG_STREAM_ACTION_DECODE)
    return;
  bg_plugin_unref(inp->handle);
  inp->handle = NULL;
  inp->image_reader = NULL;
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
  }

static void destroy_input(void* priv)
  {
  input_t * inp = (input_t *)priv;

  close_input(priv);
  
  if(inp->parameters)
    bg_parameter_info_destroy_array(inp->parameters);
  free(priv);
  }

static bg_input_plugin_t input_plugin =
  {
    common:
    {
      name:           "i_singlepic",
      long_name:      "Singlepicture input plugin",
      extensions:     NULL, /* Filled in later */
      type:           BG_PLUGIN_INPUT,
      flags:          BG_PLUGIN_FILE,
      priority:       5,
      create:         NULL,
      destroy:        destroy_input,
      get_parameters: get_parameters_input,
      set_parameter:  set_parameter_input
    },
    open:          open_input,

    //    get_num_tracks: bg_avdec_get_num_tracks,

    get_track_info: get_track_info_input,
    /* Set streams */
    set_video_stream:      set_video_stream_input,
    /*
     *  Start decoding.
     *  Track info is the track, which should be played.
     *  The plugin must take care of the "active" fields
     *  in the stream infos to check out, which streams are to be decoded
     */
    start:                 start_input,
    /* Read one video frame (returns FALSE on EOF) */
    read_video_frame:      read_video_frame_input,
    /*
     *  Do percentage seeking (can be NULL)
     *  Media streams are supposed to be seekable, if this
     *  function is non-NULL AND the duration field of the track info
     *  is > 0
     */
    seek: seek_input,
    /* Stop playback, close all decoders */
    stop: stop_input,
    close: close_input,
  };

bg_plugin_common_t * bg_singlepic_input_get()
  {
  return (bg_plugin_common_t*)(&input_plugin);
  }

bg_plugin_info_t * bg_singlepic_input_info(bg_plugin_registry_t * reg)
  {
  bg_plugin_info_t * ret;
  
  if(!bg_plugin_registry_get_num_plugins(reg, BG_PLUGIN_IMAGE_READER,
                                         BG_PLUGIN_FILE))
    return (bg_plugin_info_t *)0;
  
  ret = calloc(1, sizeof(*ret));
  
  ret->name      = bg_strdup(ret->name, input_plugin.common.name);
  ret->long_name = bg_strdup(ret->long_name, input_plugin.common.long_name);
  ret->extensions = get_extensions(reg, BG_PLUGIN_IMAGE_READER,
                                   BG_PLUGIN_FILE);
  ret->type  =  input_plugin.common.type;
  ret->flags =  input_plugin.common.flags;
  
  
  return ret;
  }

void * bg_singlepic_input_create(bg_plugin_registry_t * reg)
  {
  input_t * ret;
  ret = calloc(1, sizeof(*ret));

  ret->plugin_reg = reg;

  return ret;
  }

/* Encoder stuff */

static bg_parameter_info_t parameters_encoder[] =
  {
    {
      name:        "plugin",
      long_name:   "Plugin",
      type:        BG_PARAMETER_MULTI_MENU,
    },
    {
      name:        "frame_digits",
      long_name:   "Framenumber digits",
      type:        BG_PARAMETER_INT,
      val_min:     { val_i: 1 },
      val_max:     { val_i: 9 },
      val_default: { val_i: 4 },
    },
    {
      name:        "frame_offset",
      long_name:   "Framenumber offset",
      type:        BG_PARAMETER_INT,
      val_min:     { val_i: 0 },
      val_max:     { val_i: 1000000 },
      val_default: { val_i: 0 },
    }
  };

typedef struct
  {
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

#if 0
bg_plugin_info_t * bg_singlepic_encoder_info(bg_plugin_registry_t * reg)
  {
  bg_plugin_info_t * ret;
  
  if(!bg_plugin_registry_get_num_plugins(reg, BG_PLUGIN_IMAGE_WRITER,
                                         BG_PLUGIN_FILE))
    {
    fprintf(stderr, "No singlepicture encoding possible\n");
    return (bg_plugin_info_t *)0;
    }
  ret = calloc(1, sizeof(*ret));
  
  ret->name = bg_strdup(ret->name, bg_singlepic_input_name);
  ret->long_name = "Single picture encoder";
  ret->extensions = get_extensions(reg, BG_PLUGIN_IMAGE_WRITER,
                                   BG_PLUGIN_FILE);
  return ret;
  }
#endif

static bg_parameter_info_t * get_parameters_encoder(void * priv)
  {
  int i;
  encoder_t * enc = (encoder_t *)priv;
  
  if(!enc->parameters)
    {
    enc->parameters =
      calloc(sizeof(parameters_encoder)/sizeof(parameters_encoder[0])+1,
             sizeof(*enc->parameters));
    
    for(i = 0; i < sizeof(parameters_encoder)/sizeof(parameters_encoder[0]); i++)
      {
      bg_parameter_info_copy(&enc->parameters[i], &parameters_encoder[i]);
      }
    set_plugin_parameter(&enc->parameters[0],
                         enc->plugin_reg, BG_PLUGIN_IMAGE_WRITER, BG_PLUGIN_FILE);
    }
  return enc->parameters;
  }

#if 0
void * bg_singlepic_encoder_create(bg_plugin_registry_t * reg)
  {
  encoder_t * ret;
  ret = calloc(1, sizeof(*ret));
  ret->reg = reg;
  return ret;
  }
#endif

static void set_parameter_encoder(void * priv, char * name, 
                                  bg_parameter_value_t * val)
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

    e->extension_mask = bg_sprintf("-%%0%dlld%s", e->frame_digits, plugin_extension);
    e->extension      = bg_sprintf(e->extension_mask, (int64_t)e->frame_offset);

    fprintf(stderr, "Extension mask: %s, extension: %s\n",
            e->extension_mask,e->extension);
    }
  return e->extension;
  }

static int open_encoder(void * data, const char * filename,
                        bg_metadata_t * metadata)
  {
  encoder_t * e;
  int filename_len;
  
  e = (encoder_t *)data;

  /* Create the final mask */

  e->mask = bg_strdup(e->mask, filename);

  filename_len = strlen(filename);
  
  e->mask[filename_len - strlen(get_extension_encoder(data))] = '\0';

  e->mask = bg_strcat(e->mask, e->extension_mask);

  fprintf(stderr, "Mask: %s\n", e->mask);

  e->filename_buffer = malloc(filename_len+1);

  e->frame_counter = e->frame_offset;
  return 0;
  }


static int write_frame_header(encoder_t * e)
  {
  e->have_header = 1;

  /* Create filename */

  sprintf(e->filename_buffer, e->mask, e->frame_counter);

  //  fprintf(stderr, "Write header...");
  
  e->image_writer->write_header(e->plugin_handle->priv,
                                e->filename_buffer,
                                &(e->format));
  //  fprintf(stderr, "done\n");

  e->frame_counter++;
  return 1;
  }


static void add_video_stream_encoder(void * data, gavl_video_format_t * format)
  {
  encoder_t * e;
  e = (encoder_t *)data;

  gavl_video_format_copy(&(e->format), format);

  /* Write image header so we know the format */
  
  write_frame_header(e);
  }

static void get_video_format_encoder(void * data, int stream,
                                     gavl_video_format_t * format)
  {
  encoder_t * e;

  e = (encoder_t *)data;
  gavl_video_format_copy(format, &(e->format));
  }

static void write_video_frame_encoder(void * data, gavl_video_frame_t * frame,int stream)
  {
  encoder_t * e;

  e = (encoder_t *)data;

  if(!e->have_header)
    {
    write_frame_header(e);
    }
  //  fprintf(stderr, "Write image...");
  e->image_writer->write_image(e->plugin_handle->priv, frame);
  //  fprintf(stderr, "done\n");
  e->have_header = 0;
  }

#define STR_FREE(s) if(s){free(s);s=(char*)0;}

static void close_encoder(void * data, int do_delete)
  {
  int64_t i;
  
  encoder_t * e;
  e = (encoder_t *)data;

  if(do_delete)
    {
    for(i = e->frame_offset; i < e->frame_counter; i++)
      {
      sprintf(e->filename_buffer, e->mask, i);
      fprintf(stderr, "Removing %s\n", e->filename_buffer);
      remove(e->filename_buffer);
      }
    }
  
  STR_FREE(e->extension);
  STR_FREE(e->extension_mask);
  STR_FREE(e->mask);
  STR_FREE(e->filename_buffer);

  if(e->plugin_handle)
    {
    bg_plugin_unref(e->plugin_handle);
    e->plugin_handle = (bg_plugin_handle_t*)0;
    }
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
    common:
    {
      name:           "e_singlepic",
      long_name:      "Singlepicture encoder",
      extensions:     NULL, /* Filled in later */
      type:           BG_PLUGIN_ENCODER_VIDEO,
      flags:          BG_PLUGIN_FILE,
      priority:       5,
      create:         NULL,
      destroy:        destroy_encoder,
      get_parameters: get_parameters_encoder,
      set_parameter:  set_parameter_encoder
    },
    
    /* Maximum number of audio/video streams. -1 means infinite */
    
    max_audio_streams: 0,
    max_video_streams: 1,

    get_extension:     get_extension_encoder,
    
    /* Open a file, filename base is without extension, which
       will be added by the plugin */
        
    open: open_encoder,

    add_video_stream: add_video_stream_encoder,
    
    get_video_format:    get_video_format_encoder,
    
    write_video_frame: write_video_frame_encoder,
    
    /* Close it */

    close: close_encoder,
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
  
  ret->name      = bg_strdup(ret->name, encoder_plugin.common.name);
  ret->long_name = bg_strdup(ret->long_name, encoder_plugin.common.long_name);
  ret->extensions = get_extensions(reg, BG_PLUGIN_IMAGE_READER,
                                   BG_PLUGIN_FILE);
  ret->type  =  encoder_plugin.common.type;
  ret->flags =  encoder_plugin.common.flags;
  
  
  return ret;
  }

void * bg_singlepic_encoder_create(bg_plugin_registry_t * reg)
  {
  encoder_t * ret;
  ret = calloc(1, sizeof(*ret));

  ret->plugin_reg = reg;

  return ret;
  }
