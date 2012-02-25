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


#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <math.h>

// mkdir()
#include <sys/stat.h> 
#include <sys/types.h>

#include <config.h>
#include <gmerlin/translation.h>

#include <gmerlin/pluginregistry.h>
#include <gmerlin/log.h>
#include <gmerlin/msgqueue.h>
#include <gmerlin/transcoder.h>
#include <gmerlin/transcodermsg.h>
#include <gmerlin/utils.h>
#include <gmerlin/bggavl.h>

#include <gmerlin/textrenderer.h>
#include <gmerlin/converters.h>
#include <gmerlin/filters.h>
#include <gmerlin/encoder.h>

#include <gmerlin/subprocess.h>

// #define DUMP_VIDEO_TIMESTAMPS
// #define DUMP_AUDIO_TIMESTAMPS

#define LOG_DOMAIN "transcoder"

#define STREAM_STATE_OFF      0
#define STREAM_STATE_ON       1
#define STREAM_STATE_FINISHED 2

#define STREAM_TYPE_AUDIO            0
#define STREAM_TYPE_VIDEO            1
#define STREAM_TYPE_SUBTITLE_TEXT    2
#define STREAM_TYPE_SUBTITLE_OVERLAY 3

#define STREAM_ACTION_FORGET    0
#define STREAM_ACTION_TRANSCODE 1
#define STREAM_ACTION_COPY      2

/* The followings are for subtitles only */

#define STREAM_ACTION_BLEND           3
/* The following is only for text subtitles and means, that they'll be
   converted to graphical overlays using the text renderer */
#define STREAM_ACTION_TRANSCODE_OVERLAY  4

#define TRANSCODER_STATE_INIT     0
#define TRANSCODER_STATE_RUNNING  1
#define TRANSCODER_STATE_FINISHED 2
#define TRANSCODER_STATE_ERROR    3

/* Language utilities */

static void copy_language(char * dst, const char * src)
  {
  if(src)
    strncpy(dst, src, 3);
  else
    *dst = '\0';
  }

static char * get_language(char * in_lang, char * lang, int force)
  {
  if(in_lang)
    return force ? lang : in_lang;
  return lang;
  }

typedef struct subtitle_stream_s subtitle_stream_t;

typedef struct
  {
  
  int status;
  int type;

  int action;
  
  int in_index;
  int out_index;
  gavl_time_t time;

  bg_plugin_handle_t * in_handle;
  bg_input_plugin_t  * in_plugin;
  
  int do_encode; /* Whether this stream should be really encoded */
  int do_decode; /* Whether this stream should be decoded */
  int do_copy;   /* Whether this stream should be copied */
  
  gavl_compression_info_t ci;
  gavl_packet_t packet;
  
  } stream_t;

static int set_stream_parameters_general(stream_t * s,
                                         const char * name,
                                         const bg_parameter_value_t * val)
  {
  if(!strcmp(name, "action"))
    {
    if(!strcmp(val->val_str, "transcode"))
      s->action = STREAM_ACTION_TRANSCODE;
    else if(!strcmp(val->val_str, "copy"))
      s->action = STREAM_ACTION_COPY;
    else if(!strcmp(val->val_str, "transcode_overlay"))
      s->action = STREAM_ACTION_TRANSCODE_OVERLAY;
    else if(!strcmp(val->val_str, "blend"))
      s->action = STREAM_ACTION_BLEND;
    else
      s->action = STREAM_ACTION_FORGET;
    return 1;
    }
  return 0;
  }

typedef struct
  {
  stream_t com;

  // int do_convert_in;
  int do_convert_out;
  
  gavl_audio_converter_t * cnv_out;

  bg_audio_filter_chain_t * fc;
  
  gavl_audio_frame_t * out_frame;
  gavl_audio_frame_t * pipe_frame;
  
  gavl_audio_format_t in_format;
  gavl_audio_format_t pipe_format;
  gavl_audio_format_t out_format;
  
  bg_read_audio_func_t in_func;
  void * in_data;
  int in_stream;
  
  /* Set by set_parameter */
  bg_gavl_audio_options_t options;

  /* Do normalization */
  int normalize;
  
  int64_t samples_to_read; /* Samples to read     (in OUTPUT samplerate) */
  int64_t samples_read;    /* Samples read so far (in OUTPUT samplerate) */
  
  int initialized;

  gavl_peak_detector_t * peak_detector;
  gavl_volume_control_t * volume_control;

  char in_language[4];
  char out_language[4];
  int force_language;
  } audio_stream_t;

static void set_audio_parameter_general(void * data, const char * name,
                                        const bg_parameter_value_t * val)
  {
  audio_stream_t * stream;
  stream = (audio_stream_t*)data;
  
  if(!name)
    return;

  if(!strcmp(name, "normalize"))
    {
    stream->normalize = val->val_i;
    return;
    }
  if(!strcmp(name, "in_language"))
    {
    copy_language(stream->in_language, val->val_str);
    return;
    }
  if(!strcmp(name, "language"))
    {
    copy_language(stream->out_language, val->val_str);
    return;
    }
  if(!strcmp(name, "force_language"))
    {
    stream->force_language = val->val_i;
    return;
    }
  if(set_stream_parameters_general(&stream->com,
                                   name, val))
    return;
  else if(bg_gavl_audio_set_parameter(&stream->options, name, val))
    return;

  }

typedef struct
  {
  stream_t com;
  
  bg_video_filter_chain_t * fc;
  
  gavl_video_frame_t * frame;
  
  gavl_video_format_t in_format;
  gavl_video_format_t out_format;

  int64_t frames_written;
    
  /* Set by set_parameter */

  bg_gavl_video_options_t options;
  
  /* Data source */
  bg_read_video_func_t in_func;
  void * in_data;
  int in_stream;
  
  /* Other stuff */

  int initialized;
  int64_t start_time_scaled;

  /* Whether 2-pass transcoding is requested */
  int twopass;
  char * stats_file;

  /* Subtitle streams for blending */
  int num_subtitle_streams;
  subtitle_stream_t ** subtitle_streams;
  
  int b_frames_seen;
  int flush_b_frames;
  
  } video_stream_t;

static void set_video_parameter_general(void * data,
                                        const char * name,
                                        const bg_parameter_value_t * val)
  {
  video_stream_t * stream;
  stream = (video_stream_t*)data;
  
  if(!name)
    return;

  if(!strcmp(name, "twopass"))
    stream->twopass = val->val_i;
  
  if(set_stream_parameters_general(&stream->com,
                                   name, val))
    return;

  else if(bg_gavl_video_set_parameter(&stream->options,
                                 name, val))
    return;
  }

struct subtitle_stream_s
  {
  stream_t com;
  gavl_video_converter_t * cnv;
  int do_convert;

  gavl_overlay_t ovl1, ovl2;

  gavl_overlay_t * current_ovl;
  gavl_overlay_t * next_ovl;

  int has_current;
  int has_next;
  
  gavl_overlay_blend_context_t * blend_context;
  int video_stream;

  gavl_video_format_t in_format;
  gavl_video_format_t out_format;
  
  int do_blend; /* Set by check_video_blend() */
  
  int eof; /* Set STREAM finished not before the last subtitle expired */

  char in_language[4];
  char out_language[4];
  int force_language;
  };

typedef struct
  {
  subtitle_stream_t com;
  bg_text_renderer_t * textrenderer;

  char * text;
  int text_alloc;

  gavl_time_t subtitle_start;
  gavl_time_t subtitle_duration;

  } subtitle_text_stream_t;

static void set_subtitle_parameter_general(void * data,
                                           const char * name,
                                           const bg_parameter_value_t * val)
  {
  subtitle_stream_t * stream;
  stream = (subtitle_stream_t*)data;

  if(!name)
    return;

  if(!strcmp(name, "video_stream"))
    stream->video_stream = val->val_i-1;
  if(!strcmp(name, "in_language"))
    {
    copy_language(stream->in_language, val->val_str);
    return;
    }
  if(!strcmp(name, "language"))
    {
    copy_language(stream->out_language, val->val_str);
    return;
    }
  if(!strcmp(name, "force_language"))
    {
    stream->force_language = val->val_i;
    return;
    }

  if(set_stream_parameters_general(&stream->com,
                                   name, val))
    return;
  }

struct bg_transcoder_s
  {
  int num_audio_streams;
  int num_video_streams;

  int num_subtitle_text_streams;
  int num_subtitle_overlay_streams;

  int num_audio_streams_real;
  int num_video_streams_real;

  int num_subtitle_text_streams_real;
  int num_subtitle_overlay_streams_real;
      
  audio_stream_t * audio_streams;
  video_stream_t * video_streams;

  subtitle_text_stream_t * subtitle_text_streams;
  subtitle_stream_t      * subtitle_overlay_streams;
  
  float percentage_done;
  gavl_time_t remaining_time; /* Remaining time (Transcoding time, NOT track time!!!) */
  double last_seconds;
    
  int state;
    
  bg_plugin_handle_t * in_handle;
  bg_input_plugin_t  * in_plugin;
  bg_track_info_t * track_info;

  bg_plugin_handle_t * out_handle;

  
  /* Set by set_parameter */

  char * name;
  char * location;
  char * plugin;
  
  int track;

  gavl_time_t start_time;
  gavl_time_t end_time;
  int set_start_time;
  int set_end_time;
  
  bg_metadata_t metadata;
  
  /* General configuration stuff */

  char * output_directory;
  char * output_path;
  char * subdir;
  
  int delete_incomplete;
  int send_finished;

  /* Timing stuff */

  gavl_timer_t * timer;
  gavl_time_t time;

  /* Duration of the section to be transcoded */
    
  gavl_time_t duration;
  
  char * output_filename;

  int is_url;

  /* Message queues */
  bg_msg_queue_list_t * message_queues;

  
  /* Multi threading stuff */
  pthread_t thread;

  int do_stop;
  pthread_mutex_t stop_mutex;

  /* Multipass */

  int total_passes;
  int pass;
  bg_plugin_registry_t * plugin_reg;

  /* Track we are created from */
  bg_transcoder_track_t * transcoder_track;

  /* Encoder frontend */
  bg_encoder_t * enc;
  
  /* Postprocess only */
  int pp_only;
  
  bg_encoder_callbacks_t cb;

  char ** output_files;
  int num_output_files;
  };

static void add_output_file(bg_transcoder_t * t, const char * filename)
  {
  t->output_files = realloc(t->output_files,
                            (t->num_output_files+2) * sizeof(*t->output_files));

  memset(t->output_files + t->num_output_files, 0, 2 * sizeof(*t->output_files));

  t->output_files[t->num_output_files] = bg_strdup(NULL, filename);
  t->num_output_files++;
  }

static void free_output_files(bg_transcoder_t * t)
  {
  int i = 0;
  
  for(i = 0; i < t->num_output_files; i++)
    free(t->output_files[i]);

  if(t->output_files)
    free(t->output_files);
  }

static void log_transcoding_time(bg_transcoder_t * t)
  {
  gavl_time_t transcoding_time;
  char time_str[GAVL_TIME_STRING_LEN];

  transcoding_time = gavl_timer_get(t->timer);
  gavl_time_prettyprint(transcoding_time, time_str);
  bg_log(BG_LOG_INFO, LOG_DOMAIN, "Transcoding took %s (%.2f %% of realtime duration)",
         time_str,
         100.0 * gavl_time_to_seconds(transcoding_time) /
         gavl_time_to_seconds(t->duration));
  }

static const bg_parameter_info_t parameters[] =
  {
    {
      .name =      "output_path",
      .long_name = TRS("Output Directory"),
      .type =      BG_PARAMETER_DIRECTORY,
      .val_default = { .val_str = "." },
    },
    {
      .name =        "delete_incomplete",
      .long_name =   TRS("Delete incomplete output files"),
      .type =        BG_PARAMETER_CHECKBUTTON,
      .val_default = { .val_i = 1 },
      .help_string = TRS("Delete the encoded files if you hit the stop button. \
This option will automatically be disabled, when the track is an URL"),
    },
    {
      .name =        "cleanup_pp",
      .long_name =   TRS("Clean up after postprocessing"),
      .type =        BG_PARAMETER_CHECKBUTTON,
      .help_string = TRS("Clean up all encoded files, which were postprocessed"),
    },
    {
      .name =        "send_finished",
      .long_name =   TRS("Send finished files to player"),
      .type =        BG_PARAMETER_CHECKBUTTON,
      .val_default = { .val_i = 1 },
    },
    { /* End of parameters */ }
  };

void bg_transcoder_set_parameter(void * data, const char * name, const bg_parameter_value_t * val)
  {
  bg_transcoder_t * w = (bg_transcoder_t*)data;
  
  if(!name)
    return;

  if(!strcmp(name, "output_path"))
    {
    w->output_path = bg_strdup(w->output_path, val->val_str);
    }
  else if(!strcmp(name, "delete_incomplete"))
    {
    w->delete_incomplete = val->val_i;
    }
  else if(!strcmp(name, "send_finished"))
    {
    w->send_finished = val->val_i;
    }
  }

const bg_parameter_info_t * bg_transcoder_get_parameters()
  {
  return parameters;
  }

/* Message stuff */

typedef struct
  {
  int id;
  int num;
  } message_num_t;

static void set_message_num(bg_msg_t * msg, const void * data)
  {
  message_num_t * num = (message_num_t *)data;
  bg_msg_set_id(msg, num->id);
  bg_msg_set_arg_int(msg, 0, num->num);
  }

void bg_transcoder_send_msg_num_audio_streams(bg_msg_queue_list_t * l,
                                                     int num)
  {
  message_num_t n;
  n.id = BG_TRANSCODER_MSG_NUM_AUDIO_STREAMS;
  n.num = num;
  bg_msg_queue_list_send(l,
                         set_message_num, &n);
  }

void bg_transcoder_send_msg_num_video_streams(bg_msg_queue_list_t * l,
                                                     int num)
  {
  message_num_t n;
  n.id = BG_TRANSCODER_MSG_NUM_VIDEO_STREAMS;
  n.num = num;
  bg_msg_queue_list_send(l,
                         set_message_num, &n);
  }


typedef struct
  {
  int id;
  int index;
  gavl_audio_format_t * ifmt;
  gavl_audio_format_t * ofmt;
  } message_af_t;

static void set_message_audio_format(bg_msg_t * msg, const void * data)
  {
  message_af_t * m = (message_af_t *)data;

  bg_msg_set_id(msg, m->id);
  bg_msg_set_arg_int(msg, 0, m->index);
  bg_msg_set_arg_audio_format(msg, 1, m->ifmt);
  bg_msg_set_arg_audio_format(msg, 2, m->ofmt);
  }

void bg_transcoder_send_msg_audio_format(bg_msg_queue_list_t * l,
                                         int index,
                                         gavl_audio_format_t * input_format,
                                         gavl_audio_format_t * output_format)
  {
  message_af_t m;
  m.id = BG_TRANSCODER_MSG_AUDIO_FORMAT;
  m.index = index;
  m.ifmt = input_format;
  m.ofmt = output_format;
  bg_msg_queue_list_send(l,
                         set_message_audio_format, &m);
  }

typedef struct
  {
  int id;
  int index;
  gavl_video_format_t * ifmt;
  gavl_video_format_t * ofmt;
  } message_vf_t;

static void set_message_video_format(bg_msg_t * msg, const void * data)
  {
  message_vf_t * m = (message_vf_t *)data;

  bg_msg_set_id(msg, m->id);
  bg_msg_set_arg_int(msg, 0, m->index);
  bg_msg_set_arg_video_format(msg, 1, m->ifmt);
  bg_msg_set_arg_video_format(msg, 2, m->ofmt);
  }


void bg_transcoder_send_msg_video_format(bg_msg_queue_list_t * l,
                                  int index,
                                  gavl_video_format_t * input_format,
                                  gavl_video_format_t * output_format)
  {
  message_vf_t m;
  m.id = BG_TRANSCODER_MSG_VIDEO_FORMAT;
  m.index = index;
  m.ifmt = input_format;
  m.ofmt = output_format;
  bg_msg_queue_list_send(l,
                         set_message_video_format, &m);
  }

typedef struct
  {
  const char * name;
  int pp_only;
  } message_file_t;

static void set_message_file(bg_msg_t * msg, const void * data)
  {
  message_file_t * m = (message_file_t *)data;

  bg_msg_set_id(msg, BG_TRANSCODER_MSG_FILE);
  
  bg_msg_set_arg_string(msg, 0, m->name);
  bg_msg_set_arg_int(msg, 1, m->pp_only);
  }

void bg_transcoder_send_msg_file(bg_msg_queue_list_t * l,
                                 const char * filename, int pp_only)
  {
  message_file_t m;
  m.name = filename;
  m.pp_only = pp_only;
  bg_msg_queue_list_send(l,
                         set_message_file, &m);
  
  }

typedef struct
  {
  float perc;
  gavl_time_t rem;
  } message_progress_t;

static void set_message_progress(bg_msg_t * msg, const void * data)
  {
  message_progress_t * m = (message_progress_t *)data;
  bg_msg_set_id(msg, BG_TRANSCODER_MSG_PROGRESS);
  bg_msg_set_arg_float(msg, 0, m->perc);
  bg_msg_set_arg_time(msg, 1, m->rem);
  }

void bg_transcoder_send_msg_progress(bg_msg_queue_list_t * l,
                                            float percentage_done,
                                            gavl_time_t remaining_time)
  {
  message_progress_t n;
  n.perc = percentage_done;
  n.rem = remaining_time;
  bg_msg_queue_list_send(l,
                         set_message_progress, &n);
  }

static void set_message_finished(bg_msg_t * msg, const void * data)
  {
  bg_msg_set_id(msg, BG_TRANSCODER_MSG_FINISHED);
  }


void bg_transcoder_send_msg_finished(bg_msg_queue_list_t * l)
  {
  bg_msg_queue_list_send(l, set_message_finished, NULL);
  }

static void set_message_start(bg_msg_t * msg, const void * data)
  {
  bg_msg_set_id(msg, BG_TRANSCODER_MSG_START);
  bg_msg_set_arg_string(msg, 0, (char*)data);
  }

void bg_transcoder_send_msg_start(bg_msg_queue_list_t * l, char * what)
  {
  bg_msg_queue_list_send(l,
                         set_message_start, what);
  }

static void set_message_error(bg_msg_t * msg, const void * data)
  {
  bg_msg_set_id(msg, BG_TRANSCODER_MSG_ERROR);
  }

void bg_transcoder_send_msg_error(bg_msg_queue_list_t * l)
  {
  bg_msg_queue_list_send(l, set_message_error, NULL);
  }


static void set_message_metadata(bg_msg_t * msg, const void * data)
  {
  bg_msg_set_id(msg, BG_TRANSCODER_MSG_METADATA);
  bg_msg_set_arg_metadata(msg, 0, (const bg_metadata_t*)data);
  }

void bg_transcoder_send_msg_metadata(bg_msg_queue_list_t * l, bg_metadata_t * m)
  {
  bg_msg_queue_list_send(l,
                         set_message_metadata, m);
  }


/* */

static int decode_video_frame(void * priv, gavl_video_frame_t * f, int stream)
  {
  video_stream_t * s;
  bg_transcoder_t * t;
  int result;

  t = (bg_transcoder_t *)priv;
  s = &t->video_streams[stream];
  
  result = s->com.in_plugin->read_video(s->com.in_handle->priv,
                                        f, s->com.in_index);

  if(!result)
    return 0;

#ifdef DUMP_VIDEO_TIMESTAMPS
  bg_debug("Input timestamp (video): %"PRId64" Offset: %"PRId64"\n",
           f->timestamp, s->start_time_scaled);
#endif
  
  /* Check for end of stream */
  
  if((t->end_time != GAVL_TIME_UNDEFINED) &&
     (gavl_time_unscale(s->in_format.timescale, f->timestamp) >= t->end_time))
    return 0;
  
  /* Correct timestamps */
  
  if(s->start_time_scaled)
    {
    f->timestamp -= s->start_time_scaled;
    if(f->timestamp < 0)
      f->timestamp = 0;
    }
  return 1;
  }

static void init_audio_stream(audio_stream_t * ret,
                              bg_transcoder_track_audio_t * s,
                              int in_index, bg_plugin_registry_t * plugin_reg)
  {
  ret->com.type = STREAM_TYPE_AUDIO;
  /* Default options */

  ret->volume_control = gavl_volume_control_create();
  ret->peak_detector = gavl_peak_detector_create();
  
  /* Create converter */
  
  bg_gavl_audio_options_init(&ret->options);
  ret->cnv_out = gavl_audio_converter_create();
  ret->fc = bg_audio_filter_chain_create(&ret->options, plugin_reg);
  
  /* Apply parameters */

  bg_cfg_section_apply(s->general_section,
                       bg_transcoder_track_audio_get_general_parameters(),
                       set_audio_parameter_general, ret);

  bg_cfg_section_apply(s->filter_section,
                       s->filter_parameters,
                       bg_audio_filter_chain_set_parameter, ret->fc);
  
  ret->com.in_index = in_index;
  }

static void start_audio_stream_i(audio_stream_t * ret,
                                 bg_plugin_handle_t * in_handle)
  {
  ret->com.in_handle = in_handle;
  ret->com.in_plugin = (bg_input_plugin_t*)(in_handle->plugin);
  
  /* Set stream */

  if(ret->com.in_plugin->set_audio_stream)
    {
    if(ret->com.do_decode)
      ret->com.in_plugin->set_audio_stream(ret->com.in_handle->priv,
                                              ret->com.in_index,
                                              BG_STREAM_ACTION_DECODE);
    else if(ret->com.do_copy)
      ret->com.in_plugin->set_audio_stream(ret->com.in_handle->priv,
                                              ret->com.in_index,
                                              BG_STREAM_ACTION_READRAW);
    else
      ret->com.in_plugin->set_audio_stream(ret->com.in_handle->priv,
                                              ret->com.in_index,
                                              BG_STREAM_ACTION_OFF);
    }
  }


static void init_video_stream(video_stream_t * ret,
                              bg_transcoder_track_video_t * s,
                              int in_index, bg_plugin_registry_t * plugin_reg)
  {
  ret->com.type = STREAM_TYPE_VIDEO;

  /* Default options */
  bg_gavl_video_options_init(&ret->options);
  
  /* Create converter */

  ret->fc  = bg_video_filter_chain_create(&ret->options, plugin_reg);
  
  
  /* Apply parameters */

  bg_cfg_section_apply(s->general_section,
                       bg_transcoder_track_video_get_general_parameters(),
                       set_video_parameter_general, ret);

  bg_cfg_section_apply(s->filter_section,
                       s->filter_parameters,
                       bg_video_filter_chain_set_parameter, ret->fc);
  
  ret->com.in_index = in_index;
  }

static void start_video_stream_i(video_stream_t * ret, bg_plugin_handle_t * in_handle)
  {
  ret->com.in_handle = in_handle;
  ret->com.in_plugin = (bg_input_plugin_t*)(in_handle->plugin);
  
  /* Set stream */
  if(ret->com.in_plugin->set_video_stream)
    {
    if(ret->com.do_decode)
      ret->com.in_plugin->set_video_stream(ret->com.in_handle->priv,
                                           ret->com.in_index,
                                           BG_STREAM_ACTION_DECODE);
    else if(ret->com.do_copy)
      ret->com.in_plugin->set_video_stream(ret->com.in_handle->priv,
                                           ret->com.in_index,
                                           BG_STREAM_ACTION_READRAW);
    else
      ret->com.in_plugin->set_video_stream(ret->com.in_handle->priv,
                                     ret->com.in_index,
                                              BG_STREAM_ACTION_OFF);
    }
  }


static void init_subtitle_overlay_stream(subtitle_stream_t * ret,
                                         bg_transcoder_track_subtitle_overlay_t * s)
  {
  ret->com.type = STREAM_TYPE_SUBTITLE_OVERLAY;
  
  /* Apply parameters */
  
  bg_cfg_section_apply(s->general_section,
                       s->general_parameters,
                       set_subtitle_parameter_general, ret);
  ret->com.in_index = s->in_index;
  
  if(ret->com.action == STREAM_ACTION_BLEND)
    {
    ret->blend_context = gavl_overlay_blend_context_create();
    }

  ret->cnv = gavl_video_converter_create();
  }

static void init_subtitle_text_stream(subtitle_text_stream_t * ret,
                                      bg_transcoder_track_subtitle_text_t * s)
  {
  ret->com.com.type = STREAM_TYPE_SUBTITLE_TEXT;
  
  /* Apply parameters */
  
  bg_cfg_section_apply(s->general_section,
                       s->general_parameters,
                       set_subtitle_parameter_general, ret);
  ret->com.com.in_index = s->in_index;

  if(ret->com.com.action == STREAM_ACTION_BLEND)
    {
    ret->com.blend_context = gavl_overlay_blend_context_create();
    
    ret->textrenderer = bg_text_renderer_create();
    bg_cfg_section_apply(s->textrenderer_section,
                         bg_text_renderer_get_parameters(),
                         bg_text_renderer_set_parameter,
                         ret->textrenderer);
    }
  else if(ret->com.com.action == STREAM_ACTION_TRANSCODE_OVERLAY)
    {
    ret->textrenderer = bg_text_renderer_create();
    bg_cfg_section_apply(s->textrenderer_section,
                         bg_text_renderer_get_parameters(),
                         bg_text_renderer_set_parameter,
                         ret->textrenderer);
    ret->com.cnv = gavl_video_converter_create();
    }
  }

static void start_subtitle_stream_i(subtitle_stream_t * ret,
                                    bg_plugin_handle_t * in_handle)
  {
  ret->com.in_handle = in_handle;
  ret->com.in_plugin = (bg_input_plugin_t*)(in_handle->plugin);
  
  /* Set stream */
  if(ret->com.in_plugin->set_subtitle_stream)
    {
    if(ret->com.action != STREAM_ACTION_FORGET)
      ret->com.in_plugin->set_subtitle_stream(ret->com.in_handle->priv,
                                              ret->com.in_index,
                                              BG_STREAM_ACTION_DECODE);
    else
      ret->com.in_plugin->set_subtitle_stream(ret->com.in_handle->priv,
                                              ret->com.in_index,
                                              BG_STREAM_ACTION_OFF);
    }
  }

static void set_input_formats(bg_transcoder_t * ret)
  {
  int i;
  for(i = 0; i < ret->num_audio_streams; i++)
    {
    if(ret->audio_streams[i].com.do_decode || ret->audio_streams[i].com.do_copy)
      gavl_audio_format_copy(&ret->audio_streams[i].in_format,
                             &ret->track_info->audio_streams[ret->audio_streams[i].com.in_index].format);
    }
  for(i = 0; i < ret->num_video_streams; i++)
    {
    if(ret->video_streams[i].com.do_decode || ret->video_streams[i].com.do_copy)
      gavl_video_format_copy(&ret->video_streams[i].in_format,
                             &ret->track_info->video_streams[ret->video_streams[i].com.in_index].format);
    }
  for(i = 0; i < ret->num_subtitle_overlay_streams; i++)
    {
    if(ret->subtitle_overlay_streams[i].com.do_decode)
      gavl_video_format_copy(&ret->subtitle_overlay_streams[i].in_format,
                             &ret->track_info->subtitle_streams[ret->subtitle_overlay_streams[i].com.in_index].format);
    }
  for(i = 0; i < ret->num_subtitle_text_streams; i++)
    {
    if(ret->subtitle_text_streams[i].com.com.do_decode)
      gavl_video_format_copy(&ret->subtitle_text_streams[i].com.in_format,
                             &ret->track_info->subtitle_streams[ret->subtitle_text_streams[i].com.com.in_index].format);
    }
  }

static void add_audio_stream(audio_stream_t * ret,
                             bg_transcoder_t * t)
  {
  char * language;
  
  ret->in_func = ret->com.in_plugin->read_audio;
  ret->in_data = ret->com.in_handle->priv;
  ret->in_stream = ret->com.in_index;
  
  /* We set the frame size so we have roughly half second long audio chunks */
  ret->in_format.samples_per_frame = gavl_time_to_samples(ret->in_format.samplerate,
                                                          GAVL_TIME_SCALE/2);
  
  bg_audio_filter_chain_connect_input(ret->fc,
                                      ret->in_func,
                                      ret->in_data,
                                      ret->in_stream);
  
  bg_audio_filter_chain_init(ret->fc, &ret->in_format, &ret->pipe_format);

  ret->pipe_format.samples_per_frame = gavl_time_to_samples(ret->pipe_format.samplerate,
                                                            GAVL_TIME_SCALE/2);
  
  ret->in_func = bg_audio_filter_chain_read;
  ret->in_data = ret->fc;
  ret->in_stream = 0;
  
  gavl_audio_format_copy(&ret->out_format,
                         &ret->pipe_format);
  
  /* Decide language */
  language = get_language(ret->in_language,
                          ret->out_language,
                          ret->force_language);
  
  /* Add the audio stream */

  ret->com.out_index =
    bg_encoder_add_audio_stream(t->enc,
                                language,
                                &ret->out_format,
                                ret->com.in_index);
  }

static void add_audio_stream_compressed(audio_stream_t * ret,
                                        bg_transcoder_t * t)
  {
  char * language;
  /* Decide language */
  language = get_language(ret->in_language,
                          ret->out_language,
                          ret->force_language);
  
  /* Add the audio stream */
  
  ret->com.out_index =
    bg_encoder_add_audio_stream_compressed(t->enc,
                                           language,
                                           &ret->in_format,
                                           &ret->com.ci,
                                           ret->com.in_index);
  
  }


static void add_subtitle_text_stream(subtitle_text_stream_t * ret,
                                     bg_transcoder_track_subtitle_text_t * s,
                                     bg_transcoder_t * t)
  {
  char * language;
  if(ret->com.com.action == STREAM_ACTION_TRANSCODE)
    {
    /* Decide language */
    language = get_language(ret->com.in_language,
                            ret->com.out_language,
                            ret->com.force_language);

    gavl_video_format_copy(&ret->com.out_format,
                           &ret->com.in_format);

    
    /* TODO: timescale might get changed by the encoder!!! */
    ret->com.com.out_index =
      bg_encoder_add_subtitle_text_stream(t->enc,
                                          language, ret->com.out_format.timescale,
                                          ret->com.com.in_index);
    
    }
  else if(ret->com.com.action == STREAM_ACTION_TRANSCODE_OVERLAY)
    {
    /* Get the video format for overlay encoding. This is a bit nasty, since we have no idea yet,
       how the format will look like (the video output format isn't known by now). We'll copy the
       format, which was passed to the video encoder and hope that the overlay encoder will choose
       a proper pixelformat for us. Then, we pass the same format as *frame* format to the
       textrenderer */

    if(t->video_streams[ret->com.video_stream].com.do_encode)
      {
      gavl_video_format_copy(&ret->com.out_format,
                             &t->video_streams[ret->com.video_stream].out_format);
      }
    else
      {
      /* Video stream won't get encoded: Use video format associatd with the text subtitle stream */
      gavl_video_format_copy(&ret->com.out_format, &ret->com.in_format);
      }

    /* Decide language */
    language = get_language(ret->com.in_language,
                            ret->com.out_language,
                            ret->com.force_language);
    
    ret->com.com.out_index =
      bg_encoder_add_subtitle_overlay_stream(t->enc,
                                             language,
                                             &ret->com.out_format, ret->com.com.in_index,
                                             BG_STREAM_SUBTITLE_TEXT);
    }
  }

static void add_subtitle_overlay_stream(subtitle_stream_t * ret,
                                        bg_transcoder_track_subtitle_overlay_t * s,
                                        bg_transcoder_t * t)
  {
  char * language;
  
  gavl_video_format_copy(&ret->out_format, &ret->in_format);

  /* Decide language */

  language = get_language(ret->in_language,
                          ret->out_language,
                          ret->force_language);
  
  ret->com.out_index =
    bg_encoder_add_subtitle_overlay_stream(t->enc,
                                           language,
                                           &ret->out_format, ret->com.in_index,
                                           BG_STREAM_SUBTITLE_OVERLAY);
  }

static void add_video_stream(video_stream_t * ret,
                             bg_transcoder_t * t)
  {
  ret->in_func = decode_video_frame;
  ret->in_data = t;
  ret->in_stream = ret->com.in_index;

  bg_video_filter_chain_connect_input(ret->fc,
                                      ret->in_func, ret->in_data, ret->in_stream);
  
  bg_video_filter_chain_init(ret->fc, &ret->in_format,
                             &ret->out_format);
  ret->in_func   = bg_video_filter_chain_read;
  ret->in_data   = ret->fc;
  ret->in_stream = 0;
  
  /* Add the video stream */

  ret->com.out_index =
    bg_encoder_add_video_stream(t->enc,
                                &ret->out_format,
                                ret->com.in_index);
  }

static void add_video_stream_compressed(video_stream_t * ret,
                                        bg_transcoder_t * t)
  {
  /* Add the video stream */

  ret->com.out_index =
    bg_encoder_add_video_stream_compressed(t->enc,
                                           &ret->in_format,
                                           &ret->com.ci,
                                           ret->com.in_index);
  }

static int set_video_pass(bg_transcoder_t * t, int i)
  {
  video_stream_t * s;

  s = &t->video_streams[i];
  if(!s->twopass)
    return 1;
  
  if(!s->stats_file)
    {
    s->stats_file = bg_sprintf("%s/%s_video_%02d.stats", t->output_directory, t->name, i+1);
    bg_log(BG_LOG_INFO, LOG_DOMAIN, "Using statistics file: %s", s->stats_file);
    }

  bg_encoder_set_video_pass(t->enc,
                            s->com.out_index, t->pass, t->total_passes,
                            s->stats_file);
  return 1;
  }

static int audio_iteration(audio_stream_t*s, bg_transcoder_t * t)
  {
  int ret = 1;
  int num_samples;
  int samples_decoded;
  gavl_audio_frame_t * frame;
  
  /* Get one frame worth of input data */

  if(!s->initialized)
    {
    if((t->start_time != GAVL_TIME_UNDEFINED) &&
       (t->end_time != GAVL_TIME_UNDEFINED) &&
       (t->end_time > t->start_time))
      {
      s->samples_to_read = gavl_time_to_samples(s->out_format.samplerate,
                                                t->end_time - t->start_time);
      
      }
    else if(t->end_time != GAVL_TIME_UNDEFINED)
      {
      s->samples_to_read = gavl_time_to_samples(s->out_format.samplerate,
                                                t->end_time);
      }
    else
      s->samples_to_read = 0; /* Zero == Infinite */

    s->initialized = 1;
    }

  if(s->com.do_copy)
    {
    if(!s->com.in_plugin->read_audio_packet(s->com.in_handle->priv,
                                            s->com.in_index,
                                            &s->com.packet))
      {
      /* EOF */
      s->com.status = STREAM_STATE_FINISHED;
      return ret;
      }
    ret = bg_encoder_write_audio_packet(t->enc, &s->com.packet,
                                        s->com.out_index); 
    s->samples_read += s->com.packet.duration;
    if(s->samples_to_read && (s->samples_to_read <= s->samples_read))
      s->com.status = STREAM_STATE_FINISHED;

    s->com.time = gavl_samples_to_time(s->in_format.samplerate,
                                       s->samples_read);
    }
  else
    {
    if(s->samples_to_read &&
       (s->samples_read + s->out_format.samples_per_frame > s->samples_to_read))
      num_samples = s->samples_to_read - s->samples_read;
    else
      num_samples = s->out_format.samples_per_frame;

    if(s->do_convert_out)
      frame = s->pipe_frame;
    else
      frame = s->out_frame;
  
    samples_decoded = s->in_func(s->in_data, frame, s->in_stream,
                                 num_samples);
    /* Nothing more to transcode */
  
    if(!samples_decoded)
      {
      s->com.status = STREAM_STATE_FINISHED;
      return ret;
      }
    
    /* Volume normalization */
    if(s->normalize)
      {
      if(t->pass == t->total_passes)
        {
        gavl_volume_control_apply(s->volume_control, frame);
        }
      else if((t->pass > 1) && (t->pass < t->total_passes))
        frame = NULL;
      }
  
    /* Output conversion */
    if(s->do_convert_out)
      {
      gavl_audio_convert(s->cnv_out, s->pipe_frame, s->out_frame);
      frame = s->out_frame;
      }

    /* Update sample counter before the frame is set to NULL
       after peak-detection */
    
    s->samples_read += frame->valid_samples;
        
    /* Peak detection is done for the final frame */
    if(s->normalize)
      {
      if(t->pass == 1)
        {
        gavl_peak_detector_update(s->peak_detector, frame);
        frame = NULL;
        }
      }
    if(frame)
      {
#ifdef DUMP_AUDIO_TIMESTAMPS
      bg_debug("Output timestamp (audio): %"PRId64"\n",
               frame->timestamp);
#endif
      ret = bg_encoder_write_audio_frame(t->enc,
                                         frame,
                                         s->com.out_index);
      }

    s->com.time = gavl_samples_to_time(s->out_format.samplerate,
                                       s->samples_read);

    }
  
  /* Last samples */
  
  if(s->samples_to_read && (s->samples_to_read <= s->samples_read))
    s->com.status = STREAM_STATE_FINISHED;
  
  if(!ret)
    bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Encoding audio failed");
  return ret;
  }


static void correct_subtitle_timestamp(subtitle_stream_t * s,
                                       int64_t * start,
                                       int64_t * duration,
                                       bg_transcoder_t * t)
  {
  /* Correct timestamps */

  if(t->start_time != GAVL_TIME_UNDEFINED)
    {
    *start -= gavl_time_scale(s->in_format.timescale, t->start_time);
    if(*start < 0)
      {
      *duration += *start;
      if(*duration < 0)
        *duration = 0;
      *start = 0;
      }
    }
  /* Rescale */
  *start = gavl_time_rescale(s->in_format.timescale, s->out_format.timescale,
                             *start);
  *duration = gavl_time_rescale(s->in_format.timescale, s->out_format.timescale,
                                *duration);
  }

static int decode_subtitle_overlay(subtitle_stream_t * s, bg_transcoder_t * t,
                                   gavl_overlay_t * ovl)
  {
  int result;
  subtitle_text_stream_t * st;

  result = s->com.in_plugin->has_subtitle(s->com.in_handle->priv, s->com.in_index);
  if(!result)
    return 0;
  
  if(s->com.type == STREAM_TYPE_SUBTITLE_TEXT)
    {
    st = (subtitle_text_stream_t*)s;
    result = s->com.in_plugin->read_subtitle_text(s->com.in_handle->priv,
                                                  &st->text, &st->text_alloc,
                                                  &st->subtitle_start,
                                                  &st->subtitle_duration,
                                                  s->com.in_index);
    

    if(!result || ((t->end_time != GAVL_TIME_UNDEFINED) &&
                   (st->subtitle_start >= t->end_time)))
      {
      s->eof = 1;
      return 0;
      }

    correct_subtitle_timestamp(&st->com, &st->subtitle_start,
                               &st->subtitle_duration, t);
    
    ovl->frame->timestamp     = st->subtitle_start;
    ovl->frame->duration = st->subtitle_duration;
    
    bg_text_renderer_render(st->textrenderer, st->text, ovl);
    }
  else
    {
    result = s->com.in_plugin->read_subtitle_overlay(s->com.in_handle->priv,
                                                     ovl, s->com.in_index);
    
    if(!result || ((t->end_time != GAVL_TIME_UNDEFINED) &&
                   (ovl->frame->timestamp >= t->end_time)))
      {
      s->eof = 1;
      return 0;
      }
    correct_subtitle_timestamp(s, &ovl->frame->timestamp,
                               &ovl->frame->duration, t);
    }
  return 1;
  }

static int decode_subtitle_text(subtitle_text_stream_t * s, bg_transcoder_t * t)
  {
  int result;
  result =
    s->com.com.in_plugin->has_subtitle(s->com.com.in_handle->priv,
                                       s->com.com.in_index);
  if(!result)
    return 0;

  result = s->com.com.in_plugin->read_subtitle_text(s->com.com.in_handle->priv,
                                                    &s->text, &s->text_alloc,
                                                    &s->subtitle_start,
                                                    &s->subtitle_duration,
                                                    s->com.com.in_index);
  
  if(!result || ((t->end_time != GAVL_TIME_UNDEFINED) &&
                 (s->subtitle_start >= t->end_time)))
    {
    s->com.eof = 1;
    return 0;
    }

  correct_subtitle_timestamp(&s->com, &s->subtitle_start,
                             &s->subtitle_duration, t);
  
  return 1;
  }


static int check_video_blend(video_stream_t * vs,
                             bg_transcoder_t * t, int64_t time)
  {
  gavl_overlay_t * tmp_ovl;
  int i;
  subtitle_stream_t * ss;
  int ret = 0;
  int current_changed = 0;
  
  for(i = 0; i < vs->num_subtitle_streams; i++)
    {
    ss = vs->subtitle_streams[i];
    
    if(ss->com.status != STREAM_STATE_ON)
      continue;

    
    /* Check if the overlay expired */
    if(ss->has_current)
      {
      if(bg_overlay_too_old(time,
                            ss->current_ovl->frame->timestamp,
                            ss->current_ovl->frame->duration))
        {
        tmp_ovl = ss->current_ovl;
        ss->current_ovl = ss->next_ovl;
        ss->next_ovl = tmp_ovl;

        ss->has_current = ss->has_next;
        ss->has_next = 0;
        
        if(ss->has_current)
          current_changed = 1;
        }
      }
    
    /* Check if the next overlay replaces the current one */
    if(ss->has_next)
      {
      if(!bg_overlay_too_new(time,
                             ss->next_ovl->frame->timestamp))
        {
        tmp_ovl = ss->current_ovl;
        ss->current_ovl = ss->next_ovl;
        ss->next_ovl = tmp_ovl;

        ss->has_current = 1;
        
        ss->has_next = 0;
        
        if(bg_overlay_too_old(time,
                              ss->current_ovl->frame->timestamp,
                              ss->current_ovl->frame->duration))
          ss->has_current = 0;
        else
          current_changed = 1;
        }
      }

    if(!ss->has_current && !ss->eof)
      {
      if(decode_subtitle_overlay(ss, t, ss->current_ovl))
        {
        ss->has_current = 1;
        current_changed = 1;
        }
      else
        continue;
      }
    if(!ss->has_next && !ss->eof)
      {
      if(decode_subtitle_overlay(ss, t, ss->next_ovl))
        ss->has_next = 1;
      }
    
    if(ss->has_current &&
       !bg_overlay_too_new(time,
                           ss->current_ovl->frame->timestamp))
      {
      ss->do_blend = 1;
      ret = 1;

      }
    else
      ss->do_blend = 0;

    if(current_changed)
      gavl_overlay_blend_context_set_overlay(ss->blend_context,
                                             ss->current_ovl);

    if(!ss->has_current && !ss->has_next &&
       ss->eof)
      ss->com.status = STREAM_STATE_FINISHED;
    }
  return ret;
  }

#define SWAP_FRAMES \
  tmp_frame=s->in_frame_1;\
  s->in_frame_1=s->in_frame_2;\
  s->in_frame_2=tmp_frame

static int video_iteration(video_stream_t * s, bg_transcoder_t * t)
  {
  int ret = 1;
  int i;
  int result;
  
  if(!s->initialized)
    {
    if(t->start_time != GAVL_TIME_UNDEFINED)
      {
      s->start_time_scaled = gavl_time_scale(s->in_format.timescale,
                                             t->start_time);
      //      bg_video_converter_reset(s->cnv, s->start_time_scaled);
      }
    s->initialized = 1;
    }

  if(s->com.do_copy)
    {
    if(!s->com.in_plugin->read_video_packet(s->com.in_handle->priv,
                                            s->com.in_index,
                                            &s->com.packet))
      {
      /* EOF */
      s->com.status = STREAM_STATE_FINISHED;
      return ret;
      }

    if((s->com.packet.flags & GAVL_PACKET_TYPE_MASK) != GAVL_PACKET_TYPE_B)
      {
      if(s->flush_b_frames)
        {
        s->com.status = STREAM_STATE_FINISHED;
        return ret;
        }
      s->com.time = gavl_time_unscale(s->in_format.timescale,
                                      s->com.packet.pts + s->com.packet.duration);
      
      if((t->end_time != GAVL_TIME_UNDEFINED) &&
         (s->com.time >= t->end_time))
        {
        if(s->b_frames_seen)
          s->flush_b_frames = 1;
        else
          {
          s->com.status = STREAM_STATE_FINISHED;
          return ret;
          }
        }
      }
    else
      {
      s->b_frames_seen = 1;
      }
    
    bg_encoder_write_video_packet(t->enc, &s->com.packet,
                                  s->com.out_index); 
    }
  else
    {
    result = s->in_func(s->in_data, s->frame, s->in_stream);
    if(!result)
      {
      s->com.status = STREAM_STATE_FINISHED;

      /* Set this also for all attached subtitle streams */
      for(i = 0; i < s->num_subtitle_streams; i++)
        s->subtitle_streams[i]->com.status = STREAM_STATE_FINISHED;
      return ret;
      }

    s->com.time = gavl_time_unscale(s->out_format.timescale,
                                    s->frame->timestamp + s->frame->duration);

    if((t->end_time != GAVL_TIME_UNDEFINED) &&
       (s->com.time >= t->end_time))
      s->com.status = STREAM_STATE_FINISHED;
    
    //  if(check_video_blend(s, t, s->com.time))
    if(check_video_blend(s, t, s->frame->timestamp))
      {
      for(i = 0; i < s->num_subtitle_streams; i++)
        {
        if(s->subtitle_streams[i]->do_blend)
          {
          gavl_overlay_blend(s->subtitle_streams[i]->blend_context,
                             s->frame);
          }
        }
      }

#ifdef DUMP_VIDEO_TIMESTAMPS
    bg_debug("Output timestamp (video): %"PRId64"\n", s->frame->timestamp);
#endif
  
    ret = bg_encoder_write_video_frame(t->enc,
                                       s->frame,
                                       s->com.out_index);
    }
  
  
  if(!ret)
    bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Encoding video failed");
  
  s->frames_written++;
  return ret;
  }

/* Time offset of 0.5 seconds means, that we encode subtitles maximum
   0.5 seconds before the subtitle should appear. This is only interesting
   for formats, which don't allow random access to subtitles */

#define SUBTITLE_TIME_OFFSET (GAVL_TIME_SCALE/2)


static int subtitle_iteration(bg_transcoder_t * t)
  {
  int i, ret = 1;
  subtitle_text_stream_t * st;
  subtitle_stream_t      * ss;
  video_stream_t         * vs;
  gavl_video_frame_t * tmp_frame;
  
  for(i = 0; i < t->num_subtitle_text_streams; i++)
    {
    st = &t->subtitle_text_streams[i];

    if(!st->com.com.do_encode)
      continue;
    
    if(st->com.eof)
      {
      st->com.com.status = STREAM_STATE_FINISHED;
      continue;
      }
    
    
    /* Check for decoding */
    if(!st->com.has_current)
      {
      if(st->com.com.action == STREAM_ACTION_TRANSCODE)
        {
        st->com.has_current = decode_subtitle_text(st, t);
        }
      else if(st->com.com.action == STREAM_ACTION_TRANSCODE_OVERLAY)
        {
        st->com.has_current = decode_subtitle_overlay((subtitle_stream_t*)st, t, &st->com.ovl1);
        if(st->com.has_current && st->com.do_convert)
          {
          gavl_video_convert(st->com.cnv, st->com.ovl1.frame, st->com.ovl2.frame);
          tmp_frame = st->com.ovl1.frame;
          st->com.ovl1.frame = st->com.ovl2.frame;
          st->com.ovl2.frame = tmp_frame;
          }
        }
      }

    /* Check for encoding */
    if(st->com.has_current)
      {
      vs = &t->video_streams[st->com.video_stream];
      if(!vs->com.do_encode || (st->subtitle_start - vs->com.time < SUBTITLE_TIME_OFFSET))
        {
        if(st->com.com.action == STREAM_ACTION_TRANSCODE)
          {
          ret =
            bg_encoder_write_subtitle_text(t->enc,
                                           st->text, st->subtitle_start,
                                           st->subtitle_duration,
                                           st->com.com.out_index);
          
          if(st->subtitle_start > t->time)
            t->time = st->subtitle_start;
          }
        else if(st->com.com.action == STREAM_ACTION_TRANSCODE_OVERLAY)
          {
          ret =
            bg_encoder_write_subtitle_overlay(t->enc,
                                              &st->com.ovl1,
                                              st->com.com.out_index);
          if(st->com.ovl1.frame->timestamp > t->time)
            t->time = st->com.ovl1.frame->timestamp;
          }
        st->com.has_current = 0;
        }
      }
    if(!ret)
      bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Encoding subtitles failed");
    }
  
  if(!ret)
    return ret;
  
  for(i = 0; i < t->num_subtitle_overlay_streams; i++)
    {
    ss = &t->subtitle_overlay_streams[i];

    if(!ss->com.do_encode)
      continue;
    
    if(ss->eof)
      {
      ss->com.status = STREAM_STATE_FINISHED;
      continue;
      }
    
    
    /* Check for decoding */
    if(!ss->has_current)
      {
      ss->has_current = decode_subtitle_overlay(ss, t, &ss->ovl1);
      if(ss->has_current && ss->do_convert)
        {
        gavl_video_convert(ss->cnv, ss->ovl1.frame, ss->ovl2.frame);
        tmp_frame = ss->ovl1.frame;
        ss->ovl1.frame = ss->ovl2.frame;
        ss->ovl2.frame = tmp_frame;
        }
      }

    /* Check for encoding */
    if(ss->has_current)
      {
      vs = &t->video_streams[ss->video_stream];
      if(!vs->com.do_encode || (ss->ovl1.frame->timestamp - vs->com.time < SUBTITLE_TIME_OFFSET))
        {
        ret = bg_encoder_write_subtitle_overlay(t->enc,
                                                &ss->ovl1,
                                                ss->com.out_index);
        if(ss->ovl1.frame->timestamp > t->time)
          t->time = ss->ovl1.frame->timestamp;
        ss->has_current = 0;
        }
      }

    if(!ret)
      {
      bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Encoding subtitles failed");
      break;
      }
    }
  return ret;
  }

/* Parameter passing for the Transcoder */

#define SP_STR(s) if(!strcmp(name, # s))   \
    { \
    t->s = bg_strdup(t->s, val->val_str);  \
    return; \
    }

#define SP_INT(s) if(!strcmp(name, # s)) \
    { \
    t->s = val->val_i; \
    return; \
    }

#define SP_TIME(s) if(!strcmp(name, # s)) \
    { \
    t->s = val->val_time; \
    return; \
    }

static void
set_parameter_general(void * data, const char * name,
                      const bg_parameter_value_t * val)
  {
  int i, name_len;
  bg_transcoder_t * t;
  t = (bg_transcoder_t *)data;

  if(!name)
    {
    /* Set start and end times */

    if(!t->set_start_time)
      t->start_time = GAVL_TIME_UNDEFINED;

    if(!t->set_end_time)
      t->end_time = GAVL_TIME_UNDEFINED;

    /* Replace all '/' by '-' */

    if(t->name)
      {
      name_len = strlen(t->name);
      for(i = 0; i < name_len; i++)
        {
        if(t->name[i] == '/')
          t->name[i] = '-';
        }
      }

    /* Create the subdirectory if necessary */
    if(t->subdir)
      {
      t->output_directory = bg_sprintf("%s/%s", t->output_path, t->subdir);

      if(mkdir(t->output_directory,
               S_IRUSR|S_IWUSR|S_IXUSR|S_IRGRP|S_IXGRP|S_IROTH|S_IWOTH) == -1)
        {
        if(errno != EEXIST)
          {
          bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Cannot create directory %s: %s, using default",
                 t->output_directory, strerror(errno));
          t->output_directory = bg_strdup(t->output_directory, t->output_path);
          }
        }
      else
        bg_log(BG_LOG_INFO, LOG_DOMAIN, "Created directory %s", t->output_directory);
      }
    else
      t->output_directory = bg_strdup(t->output_directory, t->output_path);
    
    return;
    }
  SP_STR(name);
  SP_STR(location);
  SP_STR(plugin);
  SP_STR(subdir);
  
  SP_INT(track);
  
  
  SP_INT(set_start_time);
  SP_INT(set_end_time);
  SP_TIME(start_time);
  SP_TIME(end_time);
  SP_INT(pp_only);
  }

#undef SP_INT
#undef SP_TIME
#undef SP_STR


void bg_transcoder_add_message_queue(bg_transcoder_t * t,
                                     bg_msg_queue_t * message_queue)
  {
  bg_msg_queue_list_add(t->message_queues, message_queue);
  }


bg_transcoder_t * bg_transcoder_create()
  {
  bg_transcoder_t * ret;
  ret = calloc(1, sizeof(*ret));
  ret->timer = gavl_timer_create();
  ret->message_queues = bg_msg_queue_list_create();
  pthread_mutex_init(&ret->stop_mutex,  NULL);
  return ret;
  }

static void send_init_messages(bg_transcoder_t * t)
  {
  int i;
  char * tmp_string;

  if(t->pp_only)
    {
    tmp_string = bg_sprintf(TR("Postprocessing %s"), t->location);
    }
  else if(t->total_passes > 1)
    {
    tmp_string = bg_sprintf(TR("Transcoding %s [Track %d, Pass %d/%d]"),
                            t->location, t->track+1, t->pass, t->total_passes);
    }
  else
    {
    tmp_string = bg_sprintf(TR("Transcoding %s [Track %d]"), t->location, t->track+1);
    }
  bg_transcoder_send_msg_start(t->message_queues, tmp_string);
  bg_log_notranslate(BG_LOG_INFO, LOG_DOMAIN, "%s", tmp_string);
  free(tmp_string);
    
  bg_transcoder_send_msg_metadata(t->message_queues, &t->metadata);
  
  tmp_string = bg_metadata_to_string(&t->metadata, 1);
  if(tmp_string)
    {
    bg_log(BG_LOG_INFO, LOG_DOMAIN, "Metadata:\n%s", tmp_string);
    free(tmp_string);
    }

  if(!t->pp_only)
    {
    bg_transcoder_send_msg_num_audio_streams(t->message_queues, t->num_audio_streams);
    for(i = 0; i < t->num_audio_streams; i++)
      {
      if(t->audio_streams[i].com.do_decode)
        {
        bg_transcoder_send_msg_audio_format(t->message_queues, i,
                                            &t->audio_streams[i].in_format,
                                            &t->audio_streams[i].out_format);
      
        tmp_string = bg_audio_format_to_string(&t->audio_streams[i].in_format,
                                               1);
        bg_log(BG_LOG_INFO, LOG_DOMAIN, "Audio stream %d input format:\n%s",
               i+1, tmp_string);
        free(tmp_string);

        tmp_string = bg_audio_format_to_string(&t->audio_streams[i].out_format,
                                               1);
        bg_log(BG_LOG_INFO, LOG_DOMAIN, "Audio stream %d output format:\n%s",
               i+1, tmp_string);
        free(tmp_string);
        }
      else if(t->audio_streams[i].com.do_copy)
        {
        tmp_string = bg_audio_format_to_string(&t->audio_streams[i].in_format,
                                               0);
        bg_log(BG_LOG_INFO, LOG_DOMAIN, "Audio stream %d format:\n%s",
               i+1, tmp_string);
        free(tmp_string);
        }
      }
    bg_transcoder_send_msg_num_video_streams(t->message_queues, t->num_video_streams);
    for(i = 0; i < t->num_video_streams; i++)
      {
      if(t->video_streams[i].com.do_decode)
        {
        bg_transcoder_send_msg_video_format(t->message_queues, i,
                                            &t->video_streams[i].in_format,
                                            &t->video_streams[i].out_format);
        tmp_string = bg_video_format_to_string(&t->video_streams[i].in_format,
                                               0);
        bg_log(BG_LOG_INFO, LOG_DOMAIN, "Video stream %d input format:\n%s",
               i+1, tmp_string);
        free(tmp_string);

        tmp_string = bg_video_format_to_string(&t->video_streams[i].out_format, 0);
        bg_log(BG_LOG_INFO, LOG_DOMAIN, "Video stream %d output format:\n%s",
               i+1, tmp_string);
        free(tmp_string);
        }
      else if(t->video_streams[i].com.do_copy)
        {
        tmp_string = bg_video_format_to_string(&t->video_streams[i].in_format, 0);
        bg_log(BG_LOG_INFO, LOG_DOMAIN, "Video stream %d format:\n%s",
               i+1, tmp_string);
        free(tmp_string);
        }
      }
    for(i = 0; i < t->num_subtitle_text_streams; i++)
      {
      if(t->subtitle_text_streams[i].com.com.do_decode)
        {
        switch(t->subtitle_text_streams[i].com.com.action)
          {
          case STREAM_ACTION_BLEND:
            bg_log(BG_LOG_INFO, LOG_DOMAIN, "Text subtitle stream %d: Blending onto video stream %d",
                   i+1, t->subtitle_text_streams[i].com.video_stream);
            break;
          case STREAM_ACTION_TRANSCODE:
          case STREAM_ACTION_TRANSCODE_OVERLAY:
            bg_log(BG_LOG_INFO, LOG_DOMAIN, "Text subtitle stream %d: Exporting to file", i+1);
            break;
          }
        }
      
      }

    for(i = 0; i < t->num_subtitle_overlay_streams; i++)
      {
      if(t->subtitle_overlay_streams[i].com.do_decode)
        {
        switch(t->subtitle_overlay_streams[i].com.action)
          {
          case STREAM_ACTION_BLEND:
            bg_log(BG_LOG_INFO, LOG_DOMAIN, "Text subtitle stream %d: Blending onto video stream %d",
                   i+1, t->subtitle_overlay_streams[i].video_stream);
            break;
          case STREAM_ACTION_TRANSCODE:
          case STREAM_ACTION_TRANSCODE_OVERLAY:
            bg_log(BG_LOG_INFO, LOG_DOMAIN, "Text subtitle stream %d: Exporting to file", i+1);
            break;
          }
        }
      
      }


    }
  }

static void send_file(const char * name)
  {
  char * command;

  
  command = bg_sprintf("gmerlin_remote -add \"%s\"\n", name);


  bg_system(command);
  free(command);
  }

static void send_file_messages(bg_transcoder_t * t)
  {
  int i;

  for(i = 0; i < t->num_output_files; i++)
    {
    bg_transcoder_send_msg_file(t->message_queues,
                                t->output_files[i],
                                t->pp_only);
          
    if(t->send_finished)
      send_file(t->output_files[i]);
    }
  }

static int open_input(bg_transcoder_t * ret)
  {
  const bg_plugin_info_t * plugin_info;
  
  plugin_info = bg_plugin_find_by_name(ret->plugin_reg, ret->plugin);
  if(!plugin_info)
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Cannot find plugin %s", ret->plugin);
    goto fail;
    }

  ret->in_handle = bg_plugin_load(ret->plugin_reg, plugin_info);
  if(!ret->in_handle)
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Cannot open plugin %s", ret->plugin);
    goto fail;
    }

  ret->in_plugin = (bg_input_plugin_t*)(ret->in_handle->plugin);

  if(ret->in_plugin->common.set_parameter &&
     ret->in_plugin->common.get_parameters &&
     ret->transcoder_track->input_section)
    {
    bg_cfg_section_apply(ret->transcoder_track->input_section,
                         ret->in_plugin->common.get_parameters(ret->in_handle->priv),
                         ret->in_plugin->common.set_parameter, ret->in_handle->priv);
    }
  
  if(!ret->in_plugin->open(ret->in_handle->priv, ret->location))
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Cannot open %s with plugin %s",
                                  ret->location, ret->plugin);
    goto fail;
    }

  if(bg_string_is_url(ret->location))
    ret->is_url = 1;
  

  
  /* Select track and get track info */

    
  if(ret->in_plugin->get_num_tracks &&
     (ret->track >= ret->in_plugin->get_num_tracks(ret->in_handle->priv)))
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Invalid track number %d", ret->track);
    goto fail;
    }

  ret->track_info = ret->in_plugin->get_track_info(ret->in_handle->priv,
                                                   ret->track);

  if(ret->in_plugin->set_track)
    ret->in_plugin->set_track(ret->in_handle->priv, ret->track);
  
  return 1;
  fail:
  return 0;
  
  }

static void check_compressed(bg_transcoder_t * ret)
  {
  int i, j;
  
  for(i = 0; i < ret->num_audio_streams; i++)
    {
    if(!(ret->audio_streams[i].com.action == STREAM_ACTION_COPY))
      continue;

    /* Check if we can read compressed data */
    if(!ret->in_plugin->get_audio_compression_info ||
       !ret->in_plugin->get_audio_compression_info(ret->in_handle->priv,
                                                   i, &ret->audio_streams[i].com.ci))
      {
      bg_log(BG_LOG_WARNING, LOG_DOMAIN, "Audio stream %d cannot be read compressed", i+1);
      ret->audio_streams[i].com.action = STREAM_ACTION_TRANSCODE;
      continue;
      }
    
    /* Check if we can write compressed data */
    if(!bg_encoder_writes_compressed_audio(ret->enc,
                                           &ret->track_info->audio_streams[i].format,
                                           &ret->audio_streams[i].com.ci))
      {
      bg_log(BG_LOG_WARNING, LOG_DOMAIN, "Audio stream %d cannot be written compressed", i+1);
      ret->audio_streams[i].com.action = STREAM_ACTION_TRANSCODE;
      continue;
      }

    bg_log(BG_LOG_INFO, LOG_DOMAIN, "Copying compressed audio stream %d", i+1);
    //    bg_dprintf("Copying compressed audio stream %d\n", i+1);
    //    gavl_compression_info_dump(&ret->audio_streams[i].com.ci);
    }
  for(i = 0; i < ret->num_video_streams; i++)
    {
    if(ret->video_streams[i].com.action != STREAM_ACTION_COPY)
      continue;
    
    /* Check if we can read compressed data */
    if(!ret->in_plugin->get_video_compression_info ||
       !ret->in_plugin->get_video_compression_info(ret->in_handle->priv,
                                                   i, &ret->video_streams[i].com.ci))
      {
      bg_log(BG_LOG_WARNING, LOG_DOMAIN, "Video stream %d cannot be read compressed", i+1);
      ret->video_streams[i].com.action = STREAM_ACTION_TRANSCODE;
      continue;
      }
    
    /* Check if we need to blend text subtitles */
    for(j = 0; j < ret->num_subtitle_text_streams; j++)
      {
      if((ret->subtitle_text_streams[i].com.com.action == STREAM_ACTION_BLEND) &&
         (ret->subtitle_text_streams[i].com.video_stream == i))
        {
        bg_log(BG_LOG_WARNING, LOG_DOMAIN,
               "Not copying video stream %d: Will blend subtitles", i+1);
        ret->video_streams[i].com.action = STREAM_ACTION_TRANSCODE;
        break;
        }
      }
    if(ret->video_streams[i].com.action != STREAM_ACTION_COPY)
      continue;
    
    /* Check if we need to blend overlay subtitles */
    for(j = 0; j < ret->num_subtitle_overlay_streams; j++)
      {
      if((ret->subtitle_overlay_streams[i].com.action == STREAM_ACTION_BLEND) &&
         (ret->subtitle_overlay_streams[i].video_stream == i))
        {
        bg_log(BG_LOG_WARNING, LOG_DOMAIN,
               "Not copying video stream %d: Will blend subtitles", i+1);
        ret->video_streams[i].com.action = STREAM_ACTION_TRANSCODE;
        }
      }
    if(ret->video_streams[i].com.action != STREAM_ACTION_COPY)
      continue;


    /* Check if we can write compressed data */
    if(!bg_encoder_writes_compressed_video(ret->enc,
                                           &ret->track_info->video_streams[i].format,
                                           &ret->video_streams[i].com.ci))
      {
      bg_log(BG_LOG_WARNING, LOG_DOMAIN, "Video stream %d cannot be written compressed", i+1);
      ret->video_streams[i].com.action = STREAM_ACTION_TRANSCODE;
      continue;
      }

    bg_log(BG_LOG_INFO, LOG_DOMAIN, "Copying compressed video stream %d", i+1);
    //    bg_dprintf("Copying compressed video stream %d\n", i+1);
    //    gavl_compression_info_dump(&ret->video_streams[i].com.ci);
    }
  }

static void create_streams(bg_transcoder_t * ret,
                           bg_transcoder_track_t * track)
  {
  int i;
  /* Allocate streams */

  ret->num_audio_streams = track->num_audio_streams;
  if(ret->num_audio_streams)
    ret->audio_streams = calloc(ret->num_audio_streams,
                                sizeof(*(ret->audio_streams)));
  
  ret->num_video_streams = track->num_video_streams;

  if(ret->num_video_streams)
    ret->video_streams = calloc(ret->num_video_streams,
                                sizeof(*(ret->video_streams)));

  ret->num_subtitle_text_streams = track->num_subtitle_text_streams;

  if(ret->num_subtitle_text_streams)
    ret->subtitle_text_streams = calloc(ret->num_subtitle_text_streams,
                                sizeof(*(ret->subtitle_text_streams)));

  ret->num_subtitle_overlay_streams = track->num_subtitle_overlay_streams;

  if(ret->num_subtitle_overlay_streams)
    ret->subtitle_overlay_streams = calloc(ret->num_subtitle_overlay_streams,
                                sizeof(*(ret->subtitle_overlay_streams)));
  
  /* Prepare streams */
    
  ret->num_audio_streams_real = 0;
    
  for(i = 0; i < ret->num_audio_streams; i++)
    {

    init_audio_stream(&ret->audio_streams[i],
                      &track->audio_streams[i],
                      i, ret->plugin_reg);
    if(ret->audio_streams[i].com.action == STREAM_ACTION_TRANSCODE)
      ret->num_audio_streams_real++;
    }

  ret->num_video_streams_real = 0;
  
  for(i = 0; i < ret->num_video_streams; i++)
    {
    init_video_stream(&ret->video_streams[i],
                      &track->video_streams[i],
                      i, ret->plugin_reg);
    if(ret->video_streams[i].com.action == STREAM_ACTION_TRANSCODE)
      ret->num_video_streams_real++;
    }

  ret->num_subtitle_text_streams_real = 0;
  ret->num_subtitle_overlay_streams_real = 0;
  
  for(i = 0; i < ret->num_subtitle_text_streams; i++)
    {
    init_subtitle_text_stream(&ret->subtitle_text_streams[i],
                              &track->subtitle_text_streams[i]);

    if(ret->subtitle_text_streams[i].com.com.action == STREAM_ACTION_TRANSCODE)
      ret->num_subtitle_text_streams_real++;
    else if(ret->subtitle_text_streams[i].com.com.action == STREAM_ACTION_TRANSCODE_OVERLAY)
      ret->num_subtitle_overlay_streams_real++;
    }
  
  for(i = 0; i < ret->num_subtitle_overlay_streams; i++)
    {
    init_subtitle_overlay_stream(&ret->subtitle_overlay_streams[i],
                                 &track->subtitle_overlay_streams[i]);
    if(ret->subtitle_overlay_streams[i].com.action == STREAM_ACTION_TRANSCODE)
      ret->num_subtitle_overlay_streams_real++;
    }
  }

static int start_input(bg_transcoder_t * ret)
  {
  int i;
  for(i = 0; i < ret->num_audio_streams; i++)
    {
    if(ret->audio_streams[i].com.do_decode || ret->audio_streams[i].com.do_copy)
      {
      start_audio_stream_i(&ret->audio_streams[i],
                           ret->in_handle);
      }
    }
  for(i = 0; i < ret->num_video_streams; i++)
    {
    if(ret->video_streams[i].com.do_decode || ret->video_streams[i].com.do_copy)
      {
      start_video_stream_i(&ret->video_streams[i],
                           ret->in_handle);
      }
    }
  for(i = 0; i < ret->num_subtitle_text_streams; i++)
    {
    if(ret->subtitle_text_streams[i].com.com.do_decode)
      {
      start_subtitle_stream_i((subtitle_stream_t*)(&ret->subtitle_text_streams[i]),
                              ret->in_handle);
      }
    }
  for(i = 0; i < ret->num_subtitle_overlay_streams; i++)
    {
    if(ret->subtitle_overlay_streams[i].com.do_decode)
      {
      start_subtitle_stream_i(&ret->subtitle_overlay_streams[i],
                              ret->in_handle);
      }
    }
  
  if(ret->in_plugin->start)
    {
    if(!ret->in_plugin->start(ret->in_handle->priv))
      {
      bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Starting input plugin failed");
      goto fail;
      }
    }
  /* Check if we must seek to the start position */

  if(ret->start_time != GAVL_TIME_UNDEFINED)
    {
    if(!ret->in_plugin->seek)
      {
      bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Cannot seek to start point");
      goto fail;
      }
    ret->in_plugin->seek(ret->in_handle->priv, &ret->start_time, GAVL_TIME_SCALE);

    /* This happens, if the decoder reached EOF during the seek */

    if(ret->start_time == GAVL_TIME_UNDEFINED)
      {
      bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Cannot seek to start point");
      goto fail;
      }
    }

  /* Check, if the user entered bullshit */

  if((ret->start_time != GAVL_TIME_UNDEFINED) &&
     (ret->end_time != GAVL_TIME_UNDEFINED) &&
     (ret->end_time < ret->start_time))
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN, "End time if before start time");
    goto fail;
    }
  

  return 1;
  fail:
  return 0;
  }

static void check_passes(bg_transcoder_t * ret)
  {
  int i;
  ret->total_passes = 1;

  for(i = 0; i < ret->num_audio_streams; i++)
    {
    if((ret->audio_streams[i].com.action == STREAM_ACTION_TRANSCODE) &&
       ret->audio_streams[i].normalize)
      {
      ret->total_passes = 2;
      return;
      }
    }

  for(i = 0; i < ret->num_video_streams; i++)
    {
    if((ret->video_streams[i].com.action == STREAM_ACTION_TRANSCODE) &&
       ret->video_streams[i].twopass)
      {
      ret->total_passes = 2;
      return;
      }
    }
  }

static int setup_pass(bg_transcoder_t * ret)
  {
  int i;
  int result = 0;
  
  for(i = 0; i < ret->num_audio_streams; i++)
    {
    /* Reset the samples already decoded */
    ret->audio_streams[i].samples_read = 0;

    if(ret->audio_streams[i].com.action == STREAM_ACTION_COPY)
      {
      if(ret->pass == ret->total_passes)
        {
        ret->audio_streams[i].com.do_copy = 1;
        result = 1;
        }
      else
        ret->audio_streams[i].com.do_copy = 0;
      }
    else if(ret->audio_streams[i].com.action != STREAM_ACTION_TRANSCODE)
      {
      ret->audio_streams[i].com.do_decode = 0;
      ret->audio_streams[i].com.do_encode = 0;
      }
    else if(ret->pass == ret->total_passes)
      {
      ret->audio_streams[i].com.do_decode = 1;
      ret->audio_streams[i].com.do_encode = 1;
      result = 1;
      }
    else if((ret->pass == 1) && (ret->audio_streams[i].normalize))
      {
      ret->audio_streams[i].com.do_decode = 1;
      ret->audio_streams[i].com.do_encode = 0;
      result = 1;
      }
    else
      {
      ret->audio_streams[i].com.do_decode = 0;
      ret->audio_streams[i].com.do_encode = 0;
      }
    if(ret->audio_streams[i].com.do_decode ||
       ret->audio_streams[i].com.do_copy)
      ret->audio_streams[i].com.status = STREAM_STATE_ON;
    else
      ret->audio_streams[i].com.status = STREAM_STATE_OFF;

    }
  
  for(i = 0; i < ret->num_video_streams; i++)
    {
    if(ret->video_streams[i].com.action == STREAM_ACTION_COPY)
      {
      if(ret->pass == ret->total_passes)
        {
        ret->video_streams[i].com.do_copy = 1;
        result = 1;
        }
      else
        ret->video_streams[i].com.do_copy = 0;
      }
    else if(ret->video_streams[i].com.action != STREAM_ACTION_TRANSCODE)
      {
      ret->video_streams[i].com.do_decode = 0;
      ret->video_streams[i].com.do_encode = 0;
      }
    else if((ret->pass == ret->total_passes) || ret->video_streams[i].twopass)
      {
      ret->video_streams[i].com.do_decode = 1;
      ret->video_streams[i].com.do_encode = 1;
      result = 1;
      }
    else
      {
      ret->video_streams[i].com.do_decode = 0;
      ret->video_streams[i].com.do_encode = 0;
      }

    if(ret->video_streams[i].com.do_decode ||
       ret->video_streams[i].com.do_copy)
      ret->video_streams[i].com.status = STREAM_STATE_ON;
    else
      ret->video_streams[i].com.status = STREAM_STATE_OFF;

    }
  
  /* Subtitles */

  for(i = 0; i < ret->num_subtitle_text_streams; i++)
    {
    switch(ret->subtitle_text_streams[i].com.com.action)
      {
      case STREAM_ACTION_FORGET:
        ret->subtitle_text_streams[i].com.com.do_decode = 0;
        ret->subtitle_text_streams[i].com.com.do_encode = 0;
        break;
      case STREAM_ACTION_BLEND:
        if((ret->video_streams[ret->subtitle_text_streams[i].com.video_stream].twopass) ||
           (ret->pass == ret->total_passes))
          {
          ret->subtitle_text_streams[i].com.com.do_decode = 1;
          ret->subtitle_text_streams[i].com.com.do_encode = 0;
          }
        else
          {
          ret->subtitle_text_streams[i].com.com.do_decode = 0;
          ret->subtitle_text_streams[i].com.com.do_encode = 0;
          }
        result = 1;
        break;
      case STREAM_ACTION_TRANSCODE:
      case STREAM_ACTION_TRANSCODE_OVERLAY:
        if(ret->pass == ret->total_passes)
          {
          ret->subtitle_text_streams[i].com.com.do_decode = 1;
          ret->subtitle_text_streams[i].com.com.do_encode = 1;
          result = 1;
          }
        else
          {
          ret->subtitle_text_streams[i].com.com.do_decode = 0;
          ret->subtitle_text_streams[i].com.com.do_encode = 0;
          }
        break;
      default:
        bg_log(BG_LOG_WARNING, LOG_DOMAIN, "Subtitle stream cannot be handled");
        ret->subtitle_text_streams[i].com.com.do_decode = 0;
        ret->subtitle_text_streams[i].com.com.do_encode = 0;
        ret->subtitle_text_streams[i].com.com.action = STREAM_ACTION_FORGET;
        break;
      }
    
    if(!ret->subtitle_text_streams[i].com.com.do_decode)
      ret->subtitle_text_streams[i].com.com.status = STREAM_STATE_OFF;
    else
      ret->subtitle_text_streams[i].com.com.status = STREAM_STATE_ON;
    }

  for(i = 0; i < ret->num_subtitle_overlay_streams; i++)
    {
    switch(ret->subtitle_overlay_streams[i].com.action)
      {
      case STREAM_ACTION_FORGET:
        ret->subtitle_overlay_streams[i].com.do_decode = 0;
        ret->subtitle_overlay_streams[i].com.do_encode = 0;
        break;
      case STREAM_ACTION_BLEND:
        if(ret->video_streams[ret->subtitle_overlay_streams[i].video_stream].com.do_encode)
          {
          ret->subtitle_overlay_streams[i].com.do_decode = 1;
          ret->subtitle_overlay_streams[i].com.do_encode = 1;
          result = 1;
          }
        else
          {
          ret->subtitle_overlay_streams[i].com.do_decode = 0;
          ret->subtitle_overlay_streams[i].com.do_encode = 0;
          }
        break;
      case STREAM_ACTION_TRANSCODE:
        if(ret->pass == ret->total_passes)
          {
          ret->subtitle_overlay_streams[i].com.do_decode = 1;
          ret->subtitle_overlay_streams[i].com.do_encode = 1;
          result = 1;
          }
        else
          {
          ret->subtitle_overlay_streams[i].com.do_decode = 0;
          ret->subtitle_overlay_streams[i].com.do_encode = 0;
          }
        break;
      default:
        bg_log(BG_LOG_WARNING, LOG_DOMAIN, "Subtitle stream cannot be handled");
        ret->subtitle_overlay_streams[i].com.do_decode = 0;
        ret->subtitle_overlay_streams[i].com.do_encode = 0;
        ret->subtitle_overlay_streams[i].com.action = STREAM_ACTION_FORGET;
        break;
      }
    
    if(!ret->subtitle_overlay_streams[i].com.do_decode)
      ret->subtitle_overlay_streams[i].com.status = STREAM_STATE_OFF;
    else
      ret->subtitle_overlay_streams[i].com.status = STREAM_STATE_ON;
    }
  return result;
  }

static int create_output_file_cb(void * priv, const char * filename)
  {
  bg_transcoder_t * t = priv;

  if(!strcmp(filename, t->location))
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Input and output are the same file");
    return 0;
    }
  
  if(t->pass == t->total_passes)
    add_output_file(t, filename);
  
  return 1;
  }

static void create_encoder(bg_transcoder_t * ret)
  {
  ret->enc = bg_encoder_create(ret->plugin_reg,
                               NULL,
                               ret->transcoder_track,
                               BG_STREAM_AUDIO |
                               BG_STREAM_VIDEO |
                               BG_STREAM_SUBTITLE_TEXT |
                               BG_STREAM_SUBTITLE_OVERLAY,
                               BG_PLUGIN_FILE);
  }

static int init_encoder(bg_transcoder_t * ret)
  {
  int i;
  char * tmp_string;
  
  ret->cb.create_output_file = create_output_file_cb;
  ret->cb.data = ret;
  
  bg_encoder_set_callbacks(ret->enc, &ret->cb);
  
  tmp_string = bg_sprintf("%s/%s", ret->output_directory, ret->name);
  
  bg_encoder_open(ret->enc, tmp_string,
                  &ret->metadata,
                  ret->transcoder_track->chapter_list);
  free(tmp_string);
  
  for(i = 0; i < ret->num_audio_streams; i++)
    {
    if(ret->audio_streams[i].com.do_decode) /* If we don't encode we still need to open the
                                                plugin to get the final output format */
      add_audio_stream(&ret->audio_streams[i], ret);
    else if(ret->audio_streams[i].com.do_copy)
      add_audio_stream_compressed(&ret->audio_streams[i], ret);
    }
  
  /* Video streams: Must be added before the subtitle streams, because we need to know
     the output format (at least the output size) of the stream */
  for(i = 0; i < ret->num_video_streams; i++)
    {
    if(ret->video_streams[i].com.do_decode)/* If we don't encode we still need to open the
                                               plugin to get the final output format */
      {
      add_video_stream(&ret->video_streams[i], ret);
      set_video_pass(ret, i);
      }
    else if(ret->video_streams[i].com.do_copy)
      add_video_stream_compressed(&ret->video_streams[i], ret);
    
    }
    
  for(i = 0; i < ret->num_subtitle_text_streams; i++)
    {
    if(!ret->subtitle_text_streams[i].com.com.do_encode)
      continue;
    add_subtitle_text_stream(&ret->subtitle_text_streams[i],
                             &ret->transcoder_track->subtitle_text_streams[i], ret);
    }

  for(i = 0; i < ret->num_subtitle_overlay_streams; i++)
    {
    if(!ret->subtitle_overlay_streams[i].com.do_encode)
      continue;
    add_subtitle_overlay_stream(&ret->subtitle_overlay_streams[i],
                                &ret->transcoder_track->subtitle_overlay_streams[i], ret);
    
    }
  
  return bg_encoder_start(ret->enc);
  }

static int init_audio_converter(audio_stream_t * ret, bg_transcoder_t * t)
  {
  gavl_audio_options_t * opt;
  bg_encoder_get_audio_format(t->enc,
                              ret->com.out_index,
                              &ret->out_format);

  gavl_audio_format_copy(&ret->pipe_format, &ret->out_format);

  if(ret->options.force_format != GAVL_SAMPLE_NONE)
    ret->pipe_format.sample_format = ret->options.force_format;

  bg_audio_filter_chain_set_out_format(ret->fc, &ret->pipe_format);
  
  /* Initialize output converter */
  
  opt = gavl_audio_converter_get_options(ret->cnv_out);
  gavl_audio_options_copy(opt, ret->options.opt);
  
  ret->do_convert_out = gavl_audio_converter_init(ret->cnv_out,
                                                  &ret->pipe_format,
                                                  &ret->out_format);
  
  /* Create frames (could be left from the previous pass) */
  
  if(ret->do_convert_out && !ret->pipe_frame)
    ret->pipe_frame = gavl_audio_frame_create(&ret->pipe_format);
  
  if(!ret->out_frame)
    ret->out_frame = gavl_audio_frame_create(&ret->out_format);
  return 1;
  }


static int init_video_converter(video_stream_t * ret, bg_transcoder_t * t)
  {
  bg_encoder_get_video_format(t->enc,
                              ret->com.out_index, &ret->out_format);
  
  /* Initialize converter */

  bg_video_filter_chain_set_out_format(ret->fc, &ret->out_format);
  
  /* Create frames */
  if(!ret->frame)
    ret->frame = gavl_video_frame_create(&ret->out_format);
  gavl_video_frame_clear(ret->frame, &ret->out_format);
  
  return 1;
  }

static void subtitle_init_blend(subtitle_stream_t * ss, video_stream_t * vs)
  {
  subtitle_text_stream_t * sst;
  vs->subtitle_streams =
    realloc(vs->subtitle_streams,
            sizeof(*(vs->subtitle_streams))*(vs->num_subtitle_streams+1));
  vs->subtitle_streams[vs->num_subtitle_streams] = ss;
  vs->num_subtitle_streams++;

  /* Check whether to initialize the text renderer */
  if(ss->com.type == STREAM_TYPE_SUBTITLE_TEXT)
    {
    sst = (subtitle_text_stream_t*)ss;
    bg_text_renderer_init(sst->textrenderer,
                          &vs->out_format,
                          &ss->in_format);
    }
  if(!ss->ovl1.frame) ss->ovl1.frame = gavl_video_frame_create(&ss->in_format);
  if(!ss->ovl2.frame) ss->ovl2.frame = gavl_video_frame_create(&ss->in_format);

  ss->current_ovl = &ss->ovl1;
  ss->next_ovl    = &ss->ovl2;

  
  gavl_overlay_blend_context_init(ss->blend_context,
                                  &vs->out_format, &ss->in_format);
  gavl_video_format_copy(&ss->out_format, &vs->out_format);
  }

static void subtitle_init_encode_text(subtitle_text_stream_t * ss,
                                      bg_transcoder_t * t)
  {
  /* Nothing to do here for now */
  bg_encoder_get_subtitle_text_timescale(t->enc,
                                         ss->com.com.out_index,
                                         &ss->com.out_format.timescale);
  }

static void subtitle_init_encode_overlay(subtitle_stream_t * ss,
                                         bg_transcoder_t * t)
  {
  subtitle_text_stream_t * sst;

  bg_encoder_get_subtitle_overlay_format(t->enc,
                                         ss->com.out_index,
                                         &ss->out_format);
    
  /* Check whether to initialize the text renderer */
  if(ss->com.type == STREAM_TYPE_SUBTITLE_TEXT)
    {
    sst = (subtitle_text_stream_t*)ss;
    bg_text_renderer_init(sst->textrenderer,
                          &ss->out_format,
                          &ss->in_format);
    
    //    gavl_video_format_dump(&ss->in_format);
    //    gavl_video_format_dump(&ss->out_format);
    }
  
  ss->do_convert = gavl_video_converter_init(ss->cnv, &ss->in_format,
                                             &ss->out_format);
  
  if(!ss->ovl1.frame)
    ss->ovl1.frame = gavl_video_frame_create(&ss->in_format);

  if(ss->do_convert)
    {
    if(!ss->ovl2.frame)
      ss->ovl2.frame = gavl_video_frame_create(&ss->out_format);
    }
  }

static int init_converters(bg_transcoder_t * ret)
  {
  int i;
  for(i = 0; i < ret->num_audio_streams; i++)
    {
    if(!ret->audio_streams[i].com.do_decode)
      continue;
    if(!init_audio_converter(&ret->audio_streams[i], ret))
      return 0;
    }
  for(i = 0; i < ret->num_video_streams; i++)
    {
    if(!ret->video_streams[i].com.do_decode)
      continue;

    if(!init_video_converter(&ret->video_streams[i], ret))
      return 0;
    
    if(ret->video_streams[i].subtitle_streams)
      {
      free(ret->video_streams[i].subtitle_streams);
      ret->video_streams[i].subtitle_streams = NULL;
      ret->video_streams[i].num_subtitle_streams = 0;
      }
    }

  for(i = 0; i < ret->num_subtitle_text_streams; i++)
    {
    if(!ret->subtitle_text_streams[i].com.com.do_decode)
      continue;

    switch(ret->subtitle_text_streams[i].com.com.action)
      {
      case STREAM_ACTION_BLEND:
        subtitle_init_blend((subtitle_stream_t*)(&ret->subtitle_text_streams[i]),
                            &ret->video_streams[ret->subtitle_text_streams[i].com.video_stream]);
        break;
      case STREAM_ACTION_TRANSCODE:
        subtitle_init_encode_text((&ret->subtitle_text_streams[i]), ret);
        break;
      case STREAM_ACTION_TRANSCODE_OVERLAY:
        subtitle_init_encode_overlay((subtitle_stream_t*)&ret->subtitle_text_streams[i], ret);
        break;
      }
    }

  for(i = 0; i < ret->num_subtitle_overlay_streams; i++)
    {
    if(!ret->subtitle_overlay_streams[i].com.do_decode)
      continue;

    switch(ret->subtitle_overlay_streams[i].com.action)
      {
      case STREAM_ACTION_BLEND:
        subtitle_init_blend(&ret->subtitle_overlay_streams[i],
                            &ret->video_streams[ret->subtitle_overlay_streams[i].video_stream]);
        break;
      case STREAM_ACTION_TRANSCODE:
        subtitle_init_encode_overlay(&ret->subtitle_overlay_streams[i], ret);
        break;
      }
    }
  return 1;
  }

static void init_normalize(bg_transcoder_t * ret)
  {
  int i;
  double absolute;
  double volume_dB = 0.0;
  for(i = 0; i < ret->num_audio_streams; i++)
    {
    if(!ret->audio_streams[i].normalize)
      continue;

    if(ret->pass == 1)
      {
      gavl_peak_detector_set_format(ret->audio_streams[i].peak_detector,
                                    &ret->audio_streams[i].out_format);
      }
    else if(ret->pass == ret->total_passes)
      {
      gavl_volume_control_set_format(ret->audio_streams[i].volume_control,
                                     &ret->audio_streams[i].pipe_format);
      /* Set the volume */
      gavl_peak_detector_get_peak(ret->audio_streams[i].peak_detector,
                                  NULL, NULL, &absolute);
      if(absolute == 0.0)
        {
        gavl_volume_control_set_volume(ret->audio_streams[i].volume_control, volume_dB);
        bg_log(BG_LOG_WARNING, LOG_DOMAIN, "Zero peaks detected (silent file?). Disabling normalization.");
        }
      else
        {
        volume_dB = - 20.0 * log10(absolute);
        gavl_volume_control_set_volume(ret->audio_streams[i].volume_control, volume_dB);
        bg_log(BG_LOG_INFO, LOG_DOMAIN, "Correcting volume by %.2f dB", volume_dB);
        }
      }
    }
  }

int bg_transcoder_init(bg_transcoder_t * ret,
                       bg_plugin_registry_t * plugin_reg,
                       bg_transcoder_track_t * track)
  {

  ret->plugin_reg = plugin_reg;
  ret->transcoder_track = track;

  /* Initialize encoder info */
  
  
  /* Set general parameter */

  bg_cfg_section_apply(track->general_section,
                       track->general_parameters,
                       set_parameter_general,
                       ret);

  /* Set Metadata */

  bg_cfg_section_apply(track->metadata_section,
                       track->metadata_parameters,
                       bg_metadata_set_parameter,
                       &ret->metadata);

  /* Postprocess only: Send messages and return */
  if(ret->pp_only)
    {
    ret->output_filename = bg_strdup(ret->output_filename, ret->location);
    
    send_init_messages(ret);
    ret->state = STREAM_STATE_FINISHED;
    return 1;
    }
  
  
  /* Open input plugin */
  if(!open_input(ret))
    goto fail;

  create_encoder(ret);
  
  create_streams(ret, track);

  check_compressed(ret);
  
  /* Check how many passes we must do */
  check_passes(ret);
  ret->pass = 1;

  /* Set up this pass */
  if(!setup_pass(ret))
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN, "No stream to encode");
    goto fail;
    }
  
  /* Start input plugin */
  
  if(!start_input(ret))
    goto fail;

  set_input_formats(ret);
  
  
  /* Set up the streams in the encoders */
  if(!init_encoder(ret))
    goto fail;
  
  /* Set formats */
  if(!init_converters(ret))
    goto fail;

  /* Init normalizing */
  init_normalize(ret);
  
  /* Send messages */
  send_init_messages(ret);  
  
  /* Set duration */

  if(ret->set_start_time && ret->set_end_time)
    ret->duration = ret->end_time - ret->start_time;
  else if(ret->set_start_time)
    ret->duration = ret->track_info->duration - ret->start_time;
  else if(ret->set_end_time)
    ret->duration = ret->end_time;
  else
    ret->duration = ret->track_info->duration;
  
  /* Start timer */

  gavl_timer_start(ret->timer);
  
  ret->state = TRANSCODER_STATE_RUNNING;
  
  return 1;

  fail:


  return 0;
  
  }

#define FREE_STR(str) if(str)free(str);

static void cleanup_stream(stream_t * s)
  {
  gavl_compression_info_free(&s->ci);
  gavl_packet_free(&s->packet);
  }

static void cleanup_audio_stream(audio_stream_t * s)
  {
  cleanup_stream(&s->com);
  
  /* Free all resources */

  if(s->out_frame)
    gavl_audio_frame_destroy(s->out_frame);
  if(s->pipe_frame)
    gavl_audio_frame_destroy(s->pipe_frame);
  if(s->cnv_out)
    gavl_audio_converter_destroy(s->cnv_out);
  if(s->fc)
    bg_audio_filter_chain_destroy(s->fc);
  if(s->volume_control)
    gavl_volume_control_destroy(s->volume_control);
  if(s->peak_detector)
    gavl_peak_detector_destroy(s->peak_detector);
  
  bg_gavl_audio_options_free(&s->options);
  }

static void cleanup_video_stream(video_stream_t * s)
  {
  cleanup_stream(&s->com);

  /* Free all resources */

  if(s->frame)
    gavl_video_frame_destroy(s->frame);
  if(s->fc)
    bg_video_filter_chain_destroy(s->fc);
  
  if(s->subtitle_streams)
    free(s->subtitle_streams);
  
  /* Delete stats file */
  if(s->stats_file)
    {
    remove(s->stats_file);
    free(s->stats_file);
    }
  bg_gavl_video_options_free(&s->options);
  }

static void cleanup_subtitle_stream(subtitle_stream_t * s)
  {
  subtitle_text_stream_t * ts;
  cleanup_stream(&s->com);
  if(s->ovl1.frame)
    gavl_video_frame_destroy(s->ovl1.frame);
  if(s->ovl2.frame)
    gavl_video_frame_destroy(s->ovl2.frame);
  if(s->blend_context)
    gavl_overlay_blend_context_destroy(s->blend_context);
  if(s->com.type == STREAM_TYPE_SUBTITLE_TEXT)
    {
    ts = (subtitle_text_stream_t*)s;
    if(ts->textrenderer) bg_text_renderer_destroy(ts->textrenderer);
    }
  }

static void reset_streams(bg_transcoder_t * t)
  {
  int i;
  for(i = 0; i < t->num_audio_streams; i++)
    {
    t->audio_streams[i].samples_read = 0;
    t->audio_streams[i].com.time = 0;
    }
  for(i = 0; i < t->num_video_streams; i++)
    {
    t->video_streams[i].frames_written = 0;
    t->video_streams[i].com.time = 0;
    }
  }

static void close_input(bg_transcoder_t * t)
  {
  if(t->pp_only)
    return;
  
  if(t->in_plugin->stop)
    t->in_plugin->stop(t->in_handle->priv);

  t->in_plugin->close(t->in_handle->priv);
  bg_plugin_unref(t->in_handle);
  t->in_handle = NULL;
  }

/* Switch to next pass */

static void next_pass(bg_transcoder_t * t)
  {
  char * tmp_string;

  bg_encoder_destroy(t->enc, 1);
  
  close_input(t);
  t->pass++;
  
  /* Reset stream times */
  reset_streams(t);

  gavl_timer_stop(t->timer);
  gavl_timer_set(t->timer, 0);
  t->percentage_done = 0.0;
  t->time = 0;
  t->last_seconds = 0.0;
  
  open_input(t);
  create_encoder(t);
  
  /* Decide, which stream will be en/decoded*/
  setup_pass(t);

  start_input(t);

  /* Some streams don't have this already */
  set_input_formats(t);
  
  /* Initialize encoding plugins */
  init_encoder(t);

  /* Set formats */
  init_converters(t);

  /* Init normalizing */
  init_normalize(t);
  
  /* Send message */
  tmp_string = bg_sprintf(TR("Transcoding %s [Track %d, Pass %d/%d]"), t->location, t->track+1, t->pass, t->total_passes);
  bg_transcoder_send_msg_start(t->message_queues, tmp_string);
  bg_log_notranslate(BG_LOG_INFO, LOG_DOMAIN, "%s", tmp_string);
  free(tmp_string);

  gavl_timer_start(t->timer);
  }

/*
 *  Do one iteration.
 *  If return value is FALSE, we are done
 */

int bg_transcoder_iteration(bg_transcoder_t * t)
  {
  int i;
  gavl_time_t time;
  stream_t * stream = NULL;

  gavl_time_t real_time;
  double real_seconds;

  double remaining_seconds;
  
  int done = 1;

  if(t->pp_only)
    {
    t->state = TRANSCODER_STATE_FINISHED;
    bg_transcoder_send_msg_finished(t->message_queues);
    log_transcoding_time(t);
    return 0;
    }
  
  
  time = GAVL_TIME_MAX;
  
  /* Find the stream with the smallest time */
  
  for(i = 0; i < t->num_audio_streams; i++)
    {
    /* Check for the most urgent audio/video stream */
    if(t->audio_streams[i].com.status != STREAM_STATE_ON)
      continue;
    done = 0;
    if(t->audio_streams[i].com.time < time)
      {
      time = t->audio_streams[i].com.time;
      stream = &t->audio_streams[i].com;
      }
    }
  for(i = 0; i < t->num_video_streams; i++)
    {
    if(t->video_streams[i].com.status != STREAM_STATE_ON)
      continue;
    done = 0;
    
    if(t->video_streams[i].com.time < time)
      {
      time = t->video_streams[i].com.time;
      stream = &t->video_streams[i].com;
      }
    }

  for(i = 0; i < t->num_subtitle_text_streams; i++)
    {
    if(t->subtitle_text_streams[i].com.com.status != STREAM_STATE_ON)
      continue;
    done = 0;
    }
  for(i = 0; i < t->num_subtitle_overlay_streams; i++)
    {
    if(t->subtitle_overlay_streams[i].com.status != STREAM_STATE_ON)
      continue;
    done = 0;
    }
  
  if(done)
    {
    if(t->pass < t->total_passes)
      {
      log_transcoding_time(t);
      
      next_pass(t);
      return 1;
      }
    else
      {
      t->state = TRANSCODER_STATE_FINISHED;
      bg_transcoder_send_msg_finished(t->message_queues);
      log_transcoding_time(t);
      return 0;
      }
    }
  
  /* Do the actual transcoding */
  /* Subtitle iteration must always be done */
  if(!subtitle_iteration(t))
    {
    t->state = TRANSCODER_STATE_ERROR;
    bg_transcoder_send_msg_error(t->message_queues);
    return 0;
    }


  if(stream)
    {
    if(stream->type == STREAM_TYPE_AUDIO)
      {
      if(!audio_iteration((audio_stream_t*)stream, t))
        {
        t->state = TRANSCODER_STATE_ERROR;
        bg_transcoder_send_msg_error(t->message_queues);
        return 0;
        }
      }
    
    else if(stream->type == STREAM_TYPE_VIDEO)
      {
      if(!video_iteration((video_stream_t*)stream, t))
        {
        t->state = TRANSCODER_STATE_ERROR;
        bg_transcoder_send_msg_error(t->message_queues);
        return 0;
        }
      }
    if(stream->time > t->time)
      t->time = stream->time;
    
    }

  
  /* Update status */

  real_time = gavl_timer_get(t->timer);
  real_seconds = gavl_time_to_seconds(real_time);

  t->percentage_done =
    gavl_time_to_seconds(t->time) /
    gavl_time_to_seconds(t->duration);
  
  if(t->percentage_done < 0.0)
    t->percentage_done = 0.0;
  if(t->percentage_done > 1.0)
    t->percentage_done = 1.0;
  if(t->percentage_done == 0.0)
    remaining_seconds = 0.0;
  else
    {
    remaining_seconds = real_seconds *
      (1.0 / t->percentage_done - 1.0);
    }
  
  t->remaining_time =
    gavl_seconds_to_time(remaining_seconds);

  if(real_seconds - t->last_seconds > 1.0)
    {
    bg_transcoder_send_msg_progress(t->message_queues, t->percentage_done, t->remaining_time);
    t->last_seconds = real_seconds;
    }
  
  
  return 1;
  }



void bg_transcoder_destroy(bg_transcoder_t * t)
  {
  int i;
  int do_delete = 0;
  char tmp_string[128];
  
  if((t->state == TRANSCODER_STATE_RUNNING) && t->delete_incomplete &&
     !t->is_url)
    do_delete = 1;
  else if(t->state == TRANSCODER_STATE_INIT)
    do_delete = 1;
  else if(t->state == TRANSCODER_STATE_ERROR)
    do_delete = 1;
  
  /* Close all encoders so the files are finished */

  if(t->enc)
    bg_encoder_destroy(t->enc, do_delete);
  
  /* Send created files to gmerlin */

  if((t->state != TRANSCODER_STATE_RUNNING) && !do_delete)
    {
    send_file_messages(t);
    }
  
  free_output_files(t);
  
  /* Cleanup streams */

  for(i = 0; i < t->num_video_streams; i++)
    {
    if((t->video_streams[i].com.action != STREAM_ACTION_FORGET) && !do_delete)
      {
      sprintf(tmp_string, "%" PRId64, t->video_streams[i].frames_written);
      bg_log(BG_LOG_INFO, LOG_DOMAIN,
             "Video stream %d: Transcoded %s frames", i+1,
             tmp_string);
      }
    cleanup_video_stream(&t->video_streams[i]);
    }
  for(i = 0; i < t->num_audio_streams; i++)
    {
    if((t->audio_streams[i].com.action != STREAM_ACTION_FORGET) && !do_delete)
      {
      sprintf(tmp_string, "%" PRId64, t->audio_streams[i].samples_read);
      bg_log(BG_LOG_INFO, LOG_DOMAIN,
             "Audio stream %d: Transcoded %s samples", i+1,
             tmp_string);
      }
    cleanup_audio_stream(&t->audio_streams[i]);
    }

  for(i = 0; i < t->num_subtitle_text_streams; i++)
    {
    cleanup_subtitle_stream((subtitle_stream_t*)(&t->subtitle_text_streams[i]));
    }
  for(i = 0; i < t->num_subtitle_overlay_streams; i++)
    {
    cleanup_subtitle_stream(&t->subtitle_overlay_streams[i]);
    }
      
  if(t->audio_streams)
    free(t->audio_streams);
  if(t->video_streams)
    free(t->video_streams);
  if(t->subtitle_text_streams)
    free(t->subtitle_text_streams);
  if(t->subtitle_overlay_streams)
    free(t->subtitle_overlay_streams);
  
  /* Free metadata */

  bg_metadata_free(&t->metadata);
  
  /* Close and destroy the input plugin */

  close_input(t);
  
  /* Free rest */

  FREE_STR(t->name);
  FREE_STR(t->location);
  FREE_STR(t->plugin);
  FREE_STR(t->output_directory);
  FREE_STR(t->output_path);
  FREE_STR(t->subdir);

  FREE_STR(t->output_filename);
  
  gavl_timer_destroy(t->timer);

  bg_msg_queue_list_destroy(t->message_queues);
  pthread_mutex_destroy(&t->stop_mutex);

  
  free(t);
  }


static void * thread_func(void * data)
  {
  int do_stop;
  bg_transcoder_t * t = (bg_transcoder_t*)data;
  while(bg_transcoder_iteration(t))
    {
    pthread_mutex_lock(&t->stop_mutex);
    do_stop = t->do_stop;
    pthread_mutex_unlock(&t->stop_mutex);
    if(do_stop)
      break;
    }
  
  return NULL;
  }

void bg_transcoder_run(bg_transcoder_t * t)
  {
  pthread_create(&t->thread, NULL, thread_func, t);
  }

void bg_transcoder_stop(bg_transcoder_t * t)
  {
  pthread_mutex_lock(&t->stop_mutex);
  t->do_stop = 1;
  pthread_mutex_unlock(&t->stop_mutex);
  }

void bg_transcoder_finish(bg_transcoder_t * t)
  {
  pthread_join(t->thread, NULL);
  }

