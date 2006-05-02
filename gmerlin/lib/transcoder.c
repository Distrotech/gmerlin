/*****************************************************************
 
  transcoder.c
 
  Copyright (c) 2003-2006 by Burkhard Plaum - plaum@ipf.uni-stuttgart.de
 
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
#include <string.h>
#include <math.h>


#include <pluginregistry.h>
#include <log.h>
#include <msgqueue.h>
#include <transcoder.h>
#include <transcodermsg.h>
#include <utils.h>
#include <bggavl.h>

#define LOG_DOMAIN "transcoder"

#define STREAM_STATE_OFF      0
#define STREAM_STATE_ON       1
#define STREAM_STATE_FINISHED 2

#define STREAM_TYPE_AUDIO     0
#define STREAM_TYPE_VIDEO     1

#define STREAM_ACTION_FORGET    0
#define STREAM_ACTION_TRANSCODE 1

#define TRANSCODER_STATE_INIT     0
#define TRANSCODER_STATE_RUNNING  1
#define TRANSCODER_STATE_FINISHED 2

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

  bg_plugin_handle_t  * out_handle;
  bg_encoder_plugin_t * out_plugin;
  
  char * output_filename;

  int do_encode; /* Whether this stream should be really encoded */
  int do_decode; /* Whether this stream should be decoded */
  } stream_t;

static int set_stream_parameters_general(stream_t * s,
                                         char * name,
                                         bg_parameter_value_t * val)
  {
  if(!strcmp(name, "action"))
    {
    if(!strcmp(val->val_str, "transcode"))
      s->action = STREAM_ACTION_TRANSCODE;
    else
      s->action = STREAM_ACTION_FORGET;
    return 1;
    }
  return 0;
  }

typedef struct
  {
  stream_t com;

  int do_convert_in;
  int do_convert_out;
  
  gavl_audio_converter_t * cnv_in;
  gavl_audio_converter_t * cnv_out;
  gavl_audio_frame_t * in_frame;
  gavl_audio_frame_t * out_frame;
  gavl_audio_frame_t * pipe_frame;

  gavl_audio_format_t in_format;
  gavl_audio_format_t pipe_format;
  gavl_audio_format_t out_format;
    
  /* Set by set_parameter */
  bg_gavl_audio_options_t options;

  /* Do normalization */
  int normalize;
  
  int64_t samples_to_read; /* Samples to read     (in INPUT samplerate) */
  int64_t samples_read;    /* Samples read so far (in INPUT samplerate) */

  int64_t samples_written;
  
  int initialized;

  gavl_peak_detector_t * peak_detector;
  gavl_volume_control_t * volume_control;
  
  } audio_stream_t;

static void set_audio_parameter_general(void * data, char * name, bg_parameter_value_t * val)
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
  if(set_stream_parameters_general(&(stream->com),
                                   name, val))
    return;
  else if(bg_gavl_audio_set_parameter(&(stream->options), name, val))
    return;

  }

typedef struct
  {
  stream_t com;
  
  gavl_video_converter_t * cnv;
  int do_convert;
  
  gavl_video_frame_t * in_frame_1;
  gavl_video_frame_t * in_frame_2;
  
  gavl_video_frame_t * out_frame;
  

  gavl_video_format_t in_format;
  gavl_video_format_t out_format;

  int convert_framerate;

  int64_t frames_written;
    
  /* Set by set_parameter */

  bg_gavl_video_options_t options;

  
  /* Other stuff */

  int initialized;
  int64_t start_time_scaled;

  /* Whether 2-pass transcoding is requested */
  int twopass;
  char * stats_file;
  } video_stream_t;

#define SP_INT(s) if(!strcmp(name, # s)) \
    { \
    stream->s = val->val_i; \
    return; \
    }

static void set_video_parameter_general(void * data,
                                        char * name,
                                        bg_parameter_value_t * val)
  {
  video_stream_t * stream;
  stream = (video_stream_t*)data;
  
  if(!name)
    return;

  if(!strcmp(name, "twopass"))
    stream->twopass = val->val_i;
  
  if(set_stream_parameters_general(&(stream->com),
                                   name, val))
    return;

  else if(bg_gavl_video_set_parameter(&(stream->options),
                                 name, val))
    return;
  }

#undef SP_INT

struct bg_transcoder_s
  {
  int separate_streams;
  int num_audio_streams;
  int num_video_streams;

  int num_audio_streams_real;
  int num_video_streams_real;
  int audio_to_video;
    
  audio_stream_t * audio_streams;
  video_stream_t * video_streams;

  char * audio_encoder;
  char * video_encoder;
  
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
  int delete_incomplete;
  int send_finished;

  /* Timing stuff */

  gavl_timer_t * timer;
  gavl_time_t time;

  /* Duration of the section to be transcoded */
    
  gavl_time_t duration;
  
  /* Error handling */

  char * error_msg;
  const char * error_msg_ret;
  
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

  /* Postprocess only */
  int pp_only;
  
  };

static bg_parameter_info_t parameters[] =
  {
    {
      name:      "output_directory",
      long_name: "Output Directory",
      type:      BG_PARAMETER_DIRECTORY,
      val_default: { val_str: "." },
    },
    {
      name:        "delete_incomplete",
      long_name:   "Delete incomplete output files",
      type:        BG_PARAMETER_CHECKBUTTON,
      val_default: { val_i: 1 },
      help_string: "Delete the encoded file(s) file if you hit the stop button. \
This option will automatically be disabled, when the track is an URL",
    },
    {
      name:        "cleanup_pp",
      long_name:   "Clean up after postprocessing",
      type:        BG_PARAMETER_CHECKBUTTON,
      help_string: "Clean up all encoded files, which were postprocessed",
    },
    {
      name:        "send_finished",
      long_name:   "Send finished files to player",
      type:        BG_PARAMETER_CHECKBUTTON,
      val_default: { val_i: 1 },
    },
    { /* End of parameters */ }
  };

void bg_transcoder_set_parameter(void * data, char * name, bg_parameter_value_t * val)
  {
  bg_transcoder_t * w = (bg_transcoder_t*)data;
  
  if(!name)
    return;

  if(!strcmp(name, "output_directory"))
    {
    w->output_directory = bg_strdup(w->output_directory, val->val_str);
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

bg_parameter_info_t * bg_transcoder_get_parameters()
  {
  return parameters;
  }

/* Message stuff */

typedef struct
  {
  int id;
  int num;
  } message_num_t;

void set_message_num(bg_msg_t * msg, const void * data)
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
  int id;
  int index;
  const char * name;
  } message_file_t;

static void set_message_file(bg_msg_t * msg, const void * data)
  {
  message_file_t * m = (message_file_t *)data;

  bg_msg_set_id(msg, m->id);
  if(m->index >= 0)
    {
    bg_msg_set_arg_int(msg, 0, m->index);
    bg_msg_set_arg_string(msg, 1, m->name);
    }
  else
    bg_msg_set_arg_string(msg, 0, m->name);
  }
 

void bg_transcoder_send_msg_audio_file(bg_msg_queue_list_t * l,
                                int index,
                                const char * filename)
  {
  message_file_t m;
  m.id = BG_TRANSCODER_MSG_AUDIO_FILE;
  m.name = filename;
  m.index = index;
  bg_msg_queue_list_send(l,
                         set_message_file, &m);
  }

static void bg_transcoder_send_msg_video_file(bg_msg_queue_list_t * l,
                                int index,
                                const char * filename)
  {
  message_file_t m;
  m.id = BG_TRANSCODER_MSG_VIDEO_FILE;
  m.name = filename;
  m.index = index;
  bg_msg_queue_list_send(l,
                         set_message_file, &m);
  
  }

void bg_transcoder_send_msg_file(bg_msg_queue_list_t * l,
                          const char * filename)
  {
  message_file_t m;
  m.id = BG_TRANSCODER_MSG_FILE;
  m.name = filename;
  m.index = -1;
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
  bg_msg_queue_list_send(l,
                         set_message_finished, NULL);
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


static void init_audio_stream(audio_stream_t * ret,
                              bg_transcoder_track_audio_t * s,
                              int in_index)
  {
  ret->com.type = STREAM_TYPE_AUDIO;
  /* Default options */

  ret->volume_control = gavl_volume_control_create();
  ret->peak_detector = gavl_peak_detector_create();
  
  /* Create converter */
  
  ret->cnv_in  = gavl_audio_converter_create();
  ret->cnv_out = gavl_audio_converter_create();
  //  fprintf(stderr, "Options: %p\n", ret->options.opt);
  bg_gavl_audio_options_init(&(ret->options));

  
  /* Apply parameters */

  bg_cfg_section_apply(s->general_section,
                       bg_transcoder_track_audio_get_general_parameters(),
                       set_audio_parameter_general, ret);
  
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
    if(ret->com.action == STREAM_ACTION_TRANSCODE)
      ret->com.in_plugin->set_audio_stream(ret->com.in_handle->priv,
                                              ret->com.in_index,
                                              BG_STREAM_ACTION_DECODE);
    else
      ret->com.in_plugin->set_audio_stream(ret->com.in_handle->priv,
                                              ret->com.in_index,
                                              BG_STREAM_ACTION_OFF);
    }
  }


static void init_video_stream(video_stream_t * ret,
                              bg_transcoder_track_video_t * s,
                              int in_index)
  {
  ret->com.type = STREAM_TYPE_VIDEO;
    
  /* Create converter */

  ret->cnv = gavl_video_converter_create();


  /* Default options */

  bg_gavl_video_options_init(&(ret->options));
  
  /* Apply parameters */

  bg_cfg_section_apply(s->general_section,
                       bg_transcoder_track_video_get_general_parameters(),
                       set_video_parameter_general, ret);
  
  ret->com.in_index = in_index;

  
  }

static void start_video_stream_i(video_stream_t * ret, bg_plugin_handle_t * in_handle)
  {
  ret->com.in_handle = in_handle;
  ret->com.in_plugin = (bg_input_plugin_t*)(in_handle->plugin);
  
  /* Set stream */
  if(ret->com.in_plugin->set_video_stream)
    {
    if(ret->com.action == STREAM_ACTION_TRANSCODE)
      ret->com.in_plugin->set_video_stream(ret->com.in_handle->priv,
                                              ret->com.in_index,
                                              BG_STREAM_ACTION_DECODE);
    else
      ret->com.in_plugin->set_video_stream(ret->com.in_handle->priv,
                                     ret->com.in_index,
                                              BG_STREAM_ACTION_OFF);
    }
  }

typedef struct
  {
  void (*func)(void * data, int index, char * name, bg_parameter_value_t*val);
  void * data;
  int index;
  
  } set_stream_param_struct_t;

static void set_stream_param(void * priv, char * name, bg_parameter_value_t * val)
  {
  set_stream_param_struct_t * s;
  s = (set_stream_param_struct_t *)priv;
  s->func(s->data, s->index, name, val);
  }

static void set_stream_encoder(stream_t * s, bg_plugin_handle_t * out_handle)
  {
  s->out_handle = out_handle;
  s->out_plugin = (bg_encoder_plugin_t*)(s->out_handle->plugin);
  }

static void set_input_formats(bg_transcoder_t * ret)
  {
  int i;
  for(i = 0; i < ret->num_audio_streams; i++)
    {
    if(ret->audio_streams[i].com.do_decode)
      gavl_audio_format_copy(&(ret->audio_streams[i].in_format),
                             &(ret->track_info->audio_streams[ret->audio_streams[i].com.in_index].format));
    }
  for(i = 0; i < ret->num_video_streams; i++)
    {
    if(ret->video_streams[i].com.do_decode)
      gavl_video_format_copy(&(ret->video_streams[i].in_format),
                             &(ret->track_info->video_streams[ret->video_streams[i].com.in_index].format));
    }
  }

static void add_audio_stream(audio_stream_t * ret,
                             bg_transcoder_track_audio_t * s)
  {
  set_stream_param_struct_t st;
  ret->com.status = STREAM_STATE_ON;
  
  /* We set the frame size so we have roughly half second long audio chunks */
#if 1
  ret->in_format.samples_per_frame = gavl_time_to_samples(ret->in_format.samplerate,
                                                          GAVL_TIME_SCALE/2);
#endif

  bg_gavl_audio_options_set_format(&(ret->options),
                                   &(ret->in_format),
                                   &(ret->out_format));
  
  /* Add the audio stream */

  ret->com.out_index =
    ret->com.out_plugin->add_audio_stream(ret->com.out_handle->priv, &(ret->out_format));
  
  /* Apply parameters */

  if(s->encoder_parameters && s->encoder_section && ret->com.out_plugin->set_audio_parameter)
    {
    st.func =  ret->com.out_plugin->set_audio_parameter;
    st.data =  ret->com.out_handle->priv;
    st.index = ret->com.out_index;
    
    bg_cfg_section_apply(s->encoder_section,
                         s->encoder_parameters,
                         set_stream_param,
                         &st);
    }
  }


static void add_video_stream(video_stream_t * ret,
                             bg_transcoder_track_video_t * s,
                             bg_track_info_t * track_info)
  {
  set_stream_param_struct_t st;
  ret->com.status = STREAM_STATE_ON;

  
  /* Get Input format */

  gavl_video_format_copy(&(ret->in_format),
                         &(track_info->video_streams[ret->com.in_index].format));

  /* Set desired output format */
    
  gavl_video_format_copy(&(ret->out_format), &(ret->in_format));

  bg_gavl_video_options_set_framerate(&(ret->options),
                                      &(ret->in_format),
                                      &(ret->out_format));
  
  bg_gavl_video_options_set_framesize(&(ret->options),
                                      &(ret->in_format),
                                      &(ret->out_format));

  bg_gavl_video_options_set_interlace(&(ret->options),
                                      &(ret->in_format),
                                      &(ret->out_format));
  
  /* Add the video stream */

  ret->com.out_index = ret->com.out_plugin->add_video_stream(ret->com.out_handle->priv,
                                                             &(ret->out_format));
  
  /* Apply parameters */

  if(s->encoder_parameters && s->encoder_section &&
     ret->com.out_plugin->set_video_parameter)
    {
    st.func =  ret->com.out_plugin->set_video_parameter;
    st.data =  ret->com.out_handle->priv;
    st.index = ret->com.out_index;
    
    bg_cfg_section_apply(s->encoder_section,
                         s->encoder_parameters,
                         set_stream_param,
                         &st);
    }
  }

static int set_video_pass(bg_transcoder_t * t, int i)
  {
  video_stream_t * s;

  s = &(t->video_streams[i]);
  if(!s->twopass)
    return 1;
    
  if(!s->com.out_plugin->set_video_pass)
    {
    t->error_msg = bg_sprintf("Multipass encoding not supported by encoder plugin");
    t->error_msg_ret = t->error_msg;
    return 0;
    }

  if(!s->stats_file)
    {
    s->stats_file = bg_sprintf("%s/%s_video_%02d.stats", t->output_directory, t->name, i+1);
    bg_log(BG_LOG_INFO, LOG_DOMAIN, "Using statistics file: %s", s->stats_file);
    }
  
  if(!s->com.out_plugin->set_video_pass(s->com.out_handle->priv, s->com.out_index, t->pass, t->total_passes,
                                          s->stats_file))
    {
    t->error_msg = bg_sprintf("Multipass encoding not supported by codec");
    t->error_msg_ret = t->error_msg;
    return 0;
    }
  return 1;
  }

static void audio_iteration(audio_stream_t*s, bg_transcoder_t * t)
  {
  int num_samples;
  int samples_decoded;
  gavl_audio_frame_t * frame;
  
  //  fprintf(stderr, "Audio iteration\n");
  /* Get one frame worth of input data */

  if(!s->initialized)
    {
    if((t->start_time != GAVL_TIME_UNDEFINED) &&
       (t->end_time != GAVL_TIME_UNDEFINED) &&
       (t->end_time > t->start_time))
      {
      s->samples_to_read = gavl_time_to_samples(s->in_format.samplerate,
                                                t->end_time - t->start_time);
      
      }
    else if(t->end_time != GAVL_TIME_UNDEFINED)
      {
      s->samples_to_read = gavl_time_to_samples(s->in_format.samplerate,
                                                t->end_time);
      }
    else
      s->samples_to_read = 0; /* Zero == Infinite */

    s->initialized = 1;
    }

  if(s->samples_to_read &&
     (s->samples_read + s->in_format.samples_per_frame > s->samples_to_read))
    num_samples = s->samples_to_read - s->samples_read;
  else
    num_samples = s->in_format.samples_per_frame;
    
  samples_decoded = s->com.in_plugin->read_audio_samples(s->com.in_handle->priv,
                                                s->in_frame,
                                                s->com.in_index,
                                                num_samples);
  /* Nothing more to transcode */
  
  if(!samples_decoded)
    {
    s->com.status = STREAM_STATE_FINISHED;
    return;
    }

  s->samples_read += samples_decoded;

  /* Last samples */
  
  if((samples_decoded < num_samples) ||
     (s->samples_to_read && (s->samples_to_read <= s->samples_read)))
    {
    //    fprintf(stderr, "Audio stream finished\n");
    s->com.status = STREAM_STATE_FINISHED;
    }

  /* Convert and encode it */
  
  if(s->do_convert_in)
    {
    //    fprintf(stderr, "Converting %d samples, ", s->in_frame->valid_samples);
    gavl_audio_convert(s->cnv_in, s->in_frame, s->pipe_frame);
    //    fprintf(stderr, "got %d samples\n", s->out_frame->valid_samples);
    frame = s->pipe_frame;
    }
  else
    frame = s->in_frame;
  
  /* Update the time BEFORE we decide what top do with the frame */
  s->samples_written += frame->valid_samples;
  
  s->com.time = gavl_samples_to_time(s->out_format.samplerate,
                                     s->samples_written);
  
  /* Volume normalization */
  if(s->normalize)
    {
    if(t->pass == t->total_passes)
      {
      gavl_volume_control_apply(s->volume_control, frame);
      }
    else if((t->pass > 1) && (t->pass < t->total_passes))
      frame = (gavl_audio_frame_t*)0;
    }
  
  /* Output conversion */
  if(s->do_convert_out)
    {
    //    fprintf(stderr, "Converting %d samples, ", s->in_frame->valid_samples);
    gavl_audio_convert(s->cnv_out, frame, s->out_frame);
    //    fprintf(stderr, "got %d samples\n", s->out_frame->valid_samples);
    frame = s->out_frame;
    }

  /* Peak detection is done for the final frame */
  if(s->normalize)
    {
    if(t->pass == 1)
      {
      gavl_peak_detector_update(s->peak_detector, frame);
      frame = (gavl_audio_frame_t*)0;
      }
    }
  if(frame)
    s->com.out_plugin->write_audio_frame(s->com.out_handle->priv,
                                         frame,
                                         s->com.out_index);
  
  //  fprintf(stderr, "Audio iteration %f\n", gavl_time_to_seconds(s->com.time));
  }

static int decode_video_frame(video_stream_t * s, bg_transcoder_t * t,
                              gavl_video_frame_t * f, int idx)
  {
  int result;
  result = s->com.in_plugin->read_video_frame(s->com.in_handle->priv,
                                              f, idx);

  if(!result)
    return 0;
  
  /* Check for end of stream */
  
  if((t->end_time != GAVL_TIME_UNDEFINED) &&
     (gavl_time_unscale(s->in_format.timescale, f->time_scaled) >= t->end_time))
    return 0;
  
  /* Correct timestamps */

  if(s->start_time_scaled)
    {
    f->time_scaled -= s->start_time_scaled;
    if(f->time_scaled < 0)
      f->time_scaled = 0;
    }
  return 1;
  }


#define SWAP_FRAMES \
  tmp_frame=s->in_frame_1;\
  s->in_frame_1=s->in_frame_2;\
  s->in_frame_2=tmp_frame

static void video_iteration(video_stream_t * s, bg_transcoder_t * t)
  {
  gavl_time_t next_time;
  gavl_video_frame_t * tmp_frame;
  int result;

  if(!s->initialized)
    {
    if(t->start_time != GAVL_TIME_UNDEFINED)
      s->start_time_scaled = gavl_time_to_samples(s->in_format.timescale,
                                                  t->start_time);
    s->initialized = 1;
    }
  
  //  fprintf(stderr, "Video iteration\n");
  
  if(s->convert_framerate)
    {
    next_time = gavl_frames_to_time(s->out_format.timescale,
                                    s->out_format.frame_duration,
                                    s->frames_written);


    if(s->in_frame_1->time_scaled == GAVL_TIME_UNDEFINED)
      {
      /* Decode initial 2 frame(s) */
      result = decode_video_frame(s, t, s->in_frame_1, s->com.in_index);
      if(!result)
        {
        s->com.status = STREAM_STATE_FINISHED;
        return;
        }

      result = decode_video_frame(s, t, s->in_frame_2, s->com.in_index);
      if(!result)
        {
        s->com.status = STREAM_STATE_FINISHED;
        return;
        }
      }

    /* Decode frames until the time of in_frame_2 is
       larger than next_time */

    while(gavl_time_unscale(s->in_format.timescale, s->in_frame_2->time_scaled) < next_time)
      {
      SWAP_FRAMES;
      result = decode_video_frame(s, t, s->in_frame_2, s->com.in_index);
      if(!result)
        {
        s->com.status = STREAM_STATE_FINISHED;
        break;
        }
      }

    if(s->com.status == STREAM_STATE_FINISHED)
      tmp_frame = s->in_frame_2;
    else
      tmp_frame = s->in_frame_1;

    tmp_frame->time_scaled = s->frames_written * s->out_format.frame_duration;
    
    if(s->do_convert)
      {
      gavl_video_convert(s->cnv, tmp_frame, s->out_frame);
      s->com.out_plugin->write_video_frame(s->com.out_handle->priv,
                                           s->out_frame,
                                           s->com.out_index);
      }
    else
      {
      s->com.out_plugin->write_video_frame(s->com.out_handle->priv,
                                           tmp_frame,
                                           s->com.out_index);
      }

    s->com.time = next_time;
    s->frames_written++;
    }
  else
    {
    result = decode_video_frame(s, t, s->in_frame_1, s->com.in_index);
    
    if(!result)
      {
      s->com.status = STREAM_STATE_FINISHED;
      return;
      }

    if(s->do_convert)
      {
      gavl_video_convert(s->cnv, s->in_frame_1, s->out_frame);
      s->com.out_plugin->write_video_frame(s->com.out_handle->priv,
                                           s->out_frame,
                                           s->com.out_index);
      }
    else
      {
      s->com.out_plugin->write_video_frame(s->com.out_handle->priv,
                                           s->in_frame_1,
                                           s->com.out_index);
      }
    s->com.time = gavl_time_unscale(s->in_format.timescale, s->in_frame_1->time_scaled);
#if 0
    fprintf(stderr, "Video iteration, scale: %d, time_scaled: %lld -> %f\n",
            s->in_format.timescale,
            s->in_frame_1->time_scaled,
            gavl_time_to_seconds(s->com.time));
#endif
    s->frames_written++;
    }
  
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
set_parameter_general(void * data, char * name, bg_parameter_value_t * val)
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
    
    return;
    }
  SP_STR(name);
  SP_STR(location);
  SP_STR(plugin);
  SP_STR(audio_encoder);
  SP_STR(video_encoder);
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

static bg_plugin_handle_t * load_encoder_by_name(bg_plugin_registry_t * plugin_reg,
                                                const char * name,
                                                bg_parameter_info_t * parameter_info,
                                                bg_cfg_section_t * section,
                                                bg_encoder_plugin_t ** encoder)
  {
  bg_plugin_handle_t * ret;
  const bg_plugin_info_t * info;

  info = bg_plugin_find_by_name(plugin_reg, name);
  if(!info)
    {
    return (bg_plugin_handle_t*)0;
    }
  ret = bg_plugin_load(plugin_reg, info);

  if(parameter_info && section && ret->plugin->set_parameter)
    {
    bg_cfg_section_apply(section,
                         parameter_info,
                         ret->plugin->set_parameter,
                         ret->priv);
    }
  if(encoder)
    *encoder = (bg_encoder_plugin_t*)(ret->plugin);
  return ret;
  }

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
  pthread_mutex_init(&(ret->stop_mutex),  (pthread_mutexattr_t *)0);
  return ret;
  }

static void send_init_messages(bg_transcoder_t * t)
  {
  int i;
  char * tmp_string;

  if(t->pp_only)
    {
    tmp_string = bg_sprintf("Postprocessing %s", t->location);
    }
  else if(t->total_passes > 1)
    {
    tmp_string = bg_sprintf("Transcoding %s [Track %d, Pass %d/%d]",
                            t->location, t->track+1, t->pass, t->total_passes);
    }
  else
    {
    tmp_string = bg_sprintf("Transcoding %s [Track %d]", t->location, t->track+1);
    }
  bg_transcoder_send_msg_start(t->message_queues, tmp_string);
  bg_log(BG_LOG_INFO, LOG_DOMAIN, "%s", tmp_string);
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
        bg_transcoder_send_msg_audio_format(t->message_queues, i, &(t->audio_streams[i].in_format),
                                            &(t->audio_streams[i].out_format));
      
        tmp_string = bg_audio_format_to_string(&(t->audio_streams[i].in_format), 1);
        bg_log(BG_LOG_INFO, LOG_DOMAIN, "Audio stream %d input format:\n%s",
               i+1, tmp_string);
        free(tmp_string);

        tmp_string = bg_audio_format_to_string(&(t->audio_streams[i].out_format), 1);
        bg_log(BG_LOG_INFO, LOG_DOMAIN, "Audio stream %d output format:\n%s",
               i+1, tmp_string);
        free(tmp_string);
        }
      }
    bg_transcoder_send_msg_num_video_streams(t->message_queues, t->num_audio_streams);
    for(i = 0; i < t->num_video_streams; i++)
      {
      if(t->video_streams[i].com.do_decode)
        {
        bg_transcoder_send_msg_video_format(t->message_queues, i, &(t->video_streams[i].in_format),
                                            &(t->video_streams[i].out_format));
        tmp_string = bg_video_format_to_string(&(t->video_streams[i].in_format), 0);
        bg_log(BG_LOG_INFO, LOG_DOMAIN, "Video stream %d input format:\n%s",
               i+1, tmp_string);
        free(tmp_string);

        tmp_string = bg_video_format_to_string(&(t->video_streams[i].out_format), 0);
        bg_log(BG_LOG_INFO, LOG_DOMAIN, "Video stream %d output format:\n%s",
               i+1, tmp_string);
        free(tmp_string);
        }
      }
    }
  }

static void send_file(const char * name)
  {
  char * command;

  //  fprintf(stderr, "Sending file: %s\n", name);
  
  command = bg_sprintf("gmerlin_remote -add \"%s\"\n", name);

  //  fprintf(stderr, "Send file command: \"%s\"\n", command);

  system(command);
  free(command);
  }

static void send_file_messages(bg_transcoder_t * t)
  {
  int i;

  if(t->separate_streams)
    {
    for(i = 0; i < t->num_audio_streams; i++)
      {
      if(t->audio_streams[i].com.do_encode)
        {
        if(t->audio_streams[i].com.output_filename)
          {
          bg_transcoder_send_msg_audio_file(t->message_queues, i,
                                            t->audio_streams[i].com.output_filename);
          bg_log(BG_LOG_INFO, LOG_DOMAIN, "Audio stream %d -> file: %s",
                 i+1, t->audio_streams[i].com.output_filename);
          if(t->send_finished)
            send_file(t->audio_streams[i].com.output_filename);
          }
        }
      }
    for(i = 0; i < t->num_video_streams; i++)
      {
      if(t->video_streams[i].com.do_encode)
        {
        if(t->video_streams[i].com.output_filename)
          {
          bg_transcoder_send_msg_video_file(t->message_queues,
                                            i, t->video_streams[i].com.output_filename);
          bg_log(BG_LOG_INFO, LOG_DOMAIN, "Video stream %d -> %s", i+1,
                 t->video_streams[i].com.output_filename);
          if(t->send_finished)
            send_file(t->video_streams[i].com.output_filename);
          }
        }
      }
    }
  else if(t->output_filename)
    {
    bg_transcoder_send_msg_file(t->message_queues, t->output_filename);
    bg_log(BG_LOG_INFO, LOG_DOMAIN, "Output file: %s", t->output_filename);
    if(t->send_finished)
      send_file(t->output_filename);
    }
  }

static int open_input(bg_transcoder_t * ret)
  {
  const bg_plugin_info_t * plugin_info;
  
  plugin_info = bg_plugin_find_by_name(ret->plugin_reg, ret->plugin);
  if(!plugin_info)
    {
    ret->error_msg = bg_sprintf("Cannot find plugin %s", ret->plugin);
    ret->error_msg_ret = ret->error_msg;
    goto fail;
    }

  ret->in_handle = bg_plugin_load(ret->plugin_reg, plugin_info);
  if(!ret->in_handle)
    {
    ret->error_msg = bg_sprintf("Cannot open plugin %s", ret->plugin);
    ret->error_msg_ret = ret->error_msg;
    goto fail;
    }

  ret->in_plugin = (bg_input_plugin_t*)(ret->in_handle->plugin);

  if(!ret->in_plugin->open(ret->in_handle->priv, ret->location))
    {
    ret->error_msg = bg_sprintf("Cannot open %s with plugin %s",
                                  ret->location, ret->plugin);
    ret->error_msg_ret = ret->error_msg;
    goto fail;
    }

  if(bg_string_is_url(ret->location))
    ret->is_url = 1;
  
  //  fprintf(stderr, "done\n");

  
  /* Select track and get track info */

  //  fprintf(stderr, "Selecting track...");
    
  if(ret->in_plugin->get_num_tracks &&
     (ret->track >= ret->in_plugin->get_num_tracks(ret->in_handle->priv)))
    {
    ret->error_msg = bg_sprintf("Invalid track number %d", ret->track);
    ret->error_msg_ret = ret->error_msg;
    goto fail;
    }

  ret->track_info = ret->in_plugin->get_track_info(ret->in_handle->priv,
                                                   ret->track);

  if(ret->in_plugin->set_track)
    {
    ret->in_plugin->set_track(ret->in_handle->priv, ret->track);
    }
  return 1;
  fail:
  return 0;
  
  }
                       
static void create_streams(bg_transcoder_t * ret,
                           bg_transcoder_track_t * track)
  {
  int i;
  /* Allocate streams */

  ret->num_audio_streams = ret->track_info->num_audio_streams;
  if(ret->num_audio_streams)
    ret->audio_streams = calloc(ret->num_audio_streams,
                                sizeof(*(ret->audio_streams)));
  
  ret->num_video_streams = ret->track_info->num_video_streams;

  if(ret->num_video_streams)
    ret->video_streams = calloc(ret->num_video_streams,
                                sizeof(*(ret->video_streams)));
  
  /* Prepare streams */
    
  ret->num_audio_streams_real = 0;
    
  for(i = 0; i < ret->num_audio_streams; i++)
    {
    //    fprintf(stderr, "Preparing audio stream %d...", i);

    init_audio_stream(&(ret->audio_streams[i]),
                      &(track->audio_streams[i]),
                      i);
    if(ret->audio_streams[i].com.action == STREAM_ACTION_TRANSCODE)
      ret->num_audio_streams_real++;
    //    fprintf(stderr, "done\n");
    }

  ret->num_video_streams_real = 0;
  
  for(i = 0; i < ret->num_video_streams; i++)
    {
    //    fprintf(stderr, "Preparing video stream %d...", i);
    init_video_stream(&(ret->video_streams[i]),
                      &(track->video_streams[i]),
                      i);
    if(ret->video_streams[i].com.action == STREAM_ACTION_TRANSCODE)
      ret->num_video_streams_real++;
    //    fprintf(stderr, "done\n");
    }
  }

static int start_input(bg_transcoder_t * ret)
  {
  int i;
  for(i = 0; i < ret->num_audio_streams; i++)
    {
    if(ret->audio_streams[i].com.do_decode)
      {
      start_audio_stream_i(&(ret->audio_streams[i]),
                           ret->in_handle);
      }
    }
  for(i = 0; i < ret->num_video_streams; i++)
    {
    if(ret->video_streams[i].com.do_decode)
      {
      start_video_stream_i(&(ret->video_streams[i]),
                           ret->in_handle);
      }
    }
  
  if(ret->in_plugin->start)
    ret->in_plugin->start(ret->in_handle->priv);

  /* Check if we must seek to the start position */

  if(ret->start_time != GAVL_TIME_UNDEFINED)
    {
    if(!ret->in_plugin->seek)
      {
      ret->error_msg = bg_sprintf("Cannot seek to start point");
      ret->error_msg_ret = ret->error_msg;
      goto fail;
      }
    ret->in_plugin->seek(ret->in_handle->priv, &(ret->start_time));

    /* This happens, if the decoder reached EOF during the seek */

    if(ret->start_time == GAVL_TIME_UNDEFINED)
      {
      ret->error_msg = bg_sprintf("Cannot seek to start point");
      ret->error_msg_ret = ret->error_msg;
      goto fail;
      }
    }

  /* Check, if the user entered bullshit */

  if((ret->start_time != GAVL_TIME_UNDEFINED) &&
     (ret->end_time != GAVL_TIME_UNDEFINED) &&
     (ret->end_time < ret->start_time))
    {
    ret->error_msg = bg_sprintf("End time if before start time");
    ret->error_msg_ret = ret->error_msg;
    goto fail;
    }
  

  return 1;
  fail:
  return 0;
  }

int check_separate(bg_transcoder_t * ret)
  {
  const bg_plugin_info_t * info;
  
  ret->separate_streams = 0;
  if(!ret->audio_to_video &&
     ret->num_audio_streams_real &&
     ret->num_video_streams_real)
    ret->separate_streams = 1;

  /* all passes except the last one will have separate streams */
  if(!ret->separate_streams && (ret->pass < ret->total_passes))
    {
    ret->separate_streams = 1;
    }
  
  /* Get audio and video plugin infos so we can check for the maximum
     stream numbers */

  if(!ret->separate_streams)
    {
    if(ret->num_audio_streams_real)
      { /* Load audio plugin and check for max_audio_streams */
      if(ret->audio_to_video)
        {
        info = bg_plugin_find_by_name(ret->plugin_reg, ret->video_encoder);
        if(!info)
          goto fail;
        if((info->max_audio_streams >= 0) &&
           (ret->num_audio_streams_real > info->max_audio_streams))
          ret->separate_streams = 1;
        }
      else
        {
        info = bg_plugin_find_by_name(ret->plugin_reg, ret->audio_encoder);
        if(!info)
          goto fail;

        if((info->max_audio_streams >= 0) &&
           (ret->num_audio_streams_real > info->max_audio_streams))
          ret->separate_streams = 1;
        }
      }
    }

  if(!ret->separate_streams)
    {
    if(ret->num_video_streams_real)
      { /* Load video plugin and check for max_audio_streams */
      info = bg_plugin_find_by_name(ret->plugin_reg, ret->video_encoder);
      if((info->max_video_streams >= 0) &&
         (ret->num_video_streams_real > info->max_video_streams))
        ret->separate_streams = 1;
      }
    }
  
  return 1;
  fail:
  return 0;
  
  }


void check_passes(bg_transcoder_t * ret)
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

void setup_pass(bg_transcoder_t * ret)
  {
  int i;
  for(i = 0; i < ret->num_audio_streams; i++)
    {
    if(ret->audio_streams[i].com.action != STREAM_ACTION_TRANSCODE)
      {
      ret->audio_streams[i].com.do_decode = 0;
      ret->audio_streams[i].com.do_encode = 0;
      }
    else if(ret->pass == ret->total_passes)
      {
      ret->audio_streams[i].com.do_decode = 1;
      ret->audio_streams[i].com.do_encode = 1;
      }
    else if((ret->pass == 1) && (ret->audio_streams[i].normalize))
      {
      ret->audio_streams[i].com.do_decode = 1;
      ret->audio_streams[i].com.do_encode = 0;
      }
    else
      {
      ret->audio_streams[i].com.do_decode = 0;
      ret->audio_streams[i].com.do_encode = 0;
      }
    if(!ret->audio_streams[i].com.do_decode)
      ret->audio_streams[i].com.status = STREAM_STATE_OFF;
    else
      ret->audio_streams[i].com.status = STREAM_STATE_ON;
    }
  
  for(i = 0; i < ret->num_video_streams; i++)
    {
    if(ret->video_streams[i].com.action != STREAM_ACTION_TRANSCODE)
      {
      ret->video_streams[i].com.do_decode = 0;
      ret->video_streams[i].com.do_encode = 0;
      }
    else if((ret->pass == ret->total_passes) || ret->video_streams[i].twopass)
      {
      ret->video_streams[i].com.do_decode = 1;
      ret->video_streams[i].com.do_encode = 1;
      }
    else
      {
      ret->video_streams[i].com.do_decode = 0;
      ret->video_streams[i].com.do_encode = 0;
      }

    if(!ret->video_streams[i].com.do_decode)
      ret->video_streams[i].com.status = STREAM_STATE_OFF;
    else
      ret->video_streams[i].com.status = STREAM_STATE_ON;
    }
  }

static int open_encoder(bg_transcoder_t * ret, bg_plugin_handle_t  * encoder_handle,
                        bg_encoder_plugin_t * encoder_plugin, char ** filename)
  {
  const char * new_filename;
  
  if(!strcmp(*filename, ret->location))
    {
    ret->error_msg = bg_sprintf("Input and output are the same file");
    ret->error_msg_ret = ret->error_msg;
    bg_plugin_unref(encoder_handle);
    return 0;
    }

  if(encoder_plugin->open(encoder_handle->priv, *filename,  &(ret->metadata)))
    {
    if((ret->pass == ret->total_passes) && encoder_plugin->get_filename)
      {
      new_filename =
        encoder_plugin->get_filename(encoder_handle->priv);
      *filename = bg_strdup(*filename, new_filename);
      }
    return 1;
    }
  else
    {
    if(encoder_plugin->common.get_error)
      ret->error_msg = bg_sprintf("Could not open %s: %s", encoder_handle->info->long_name,
                                  encoder_plugin->common.get_error(encoder_handle->priv));
    else
      ret->error_msg = bg_sprintf("Could not open %s: Unknown error", encoder_handle->info->long_name);
    ret->error_msg_ret = ret->error_msg;
    bg_log(BG_LOG_ERROR, LOG_DOMAIN, ret->error_msg);
    return 0;
    }
  }

static int start_encoder(bg_transcoder_t * ret, bg_plugin_handle_t  * encoder_handle,
                         bg_encoder_plugin_t * encoder_plugin)
  {
  if(encoder_plugin->start && !encoder_plugin->start(encoder_handle->priv))
    {
    ret->error_msg = bg_sprintf("Cannot setup %s", encoder_handle->info->long_name);
    ret->error_msg_ret = ret->error_msg;
    bg_plugin_unref(encoder_handle);
    return 0;
    }
  return 1;
  }

static int init_encoders(bg_transcoder_t * ret)
  {
  int i;

  char * encoder_name;
  bg_plugin_handle_t  * encoder_handle = (bg_plugin_handle_t  *)0;
  bg_encoder_plugin_t * encoder_plugin;

  if(ret->separate_streams)
    {
    /* Open new files for each streams */

    encoder_name = ret->audio_to_video ? ret->video_encoder : ret->audio_encoder;
    for(i = 0; i < ret->num_audio_streams; i++)
      {
      if(!ret->audio_streams[i].com.do_decode) /* If we don't encode we still need to open the
                                                 plugin to get the final output format */
        continue;
      encoder_handle = load_encoder_by_name(ret->plugin_reg, encoder_name,
                                            ret->transcoder_track->video_encoder_parameters,
                                            ret->transcoder_track->video_encoder_section,
                                            &encoder_plugin);
      if(!ret->audio_streams[i].com.output_filename)
        ret->audio_streams[i].com.output_filename =
          bg_sprintf("%s/%s_audio_%02d%s", ret->output_directory, ret->name, i+1,
                     encoder_plugin->get_extension(encoder_handle->priv));
      
      if(!open_encoder(ret, encoder_handle,
                       encoder_plugin, &ret->audio_streams[i].com.output_filename))
        goto fail;

      set_stream_encoder(&(ret->audio_streams[i].com), encoder_handle);
      
      add_audio_stream(&(ret->audio_streams[i]), &(ret->transcoder_track->audio_streams[i]));
      
      if(!start_encoder(ret, encoder_handle, encoder_plugin))
        goto fail;
      
      }
    for(i = 0; i < ret->num_video_streams; i++)
      {
      if(!ret->video_streams[i].com.do_decode)/* If we don't encode we still need to open the
                                                 plugin to get the final output format */
        continue;
      encoder_handle = load_encoder_by_name(ret->plugin_reg, ret->video_encoder,
                                            ret->transcoder_track->video_encoder_parameters,
                                            ret->transcoder_track->video_encoder_section,
                                            &encoder_plugin);
      if(!ret->video_streams[i].com.output_filename)
        ret->video_streams[i].com.output_filename =
          bg_sprintf("%s/%s_video_%02d%s", ret->output_directory, ret->name, i+1,
                     encoder_plugin->get_extension(encoder_handle->priv));
      
      if(!open_encoder(ret, encoder_handle,
                       encoder_plugin, &ret->video_streams[i].com.output_filename))
        goto fail;

      set_stream_encoder(&(ret->video_streams[i].com), encoder_handle);
      add_video_stream(&(ret->video_streams[i]), &(ret->transcoder_track->video_streams[i]), ret->track_info);

      if(!set_video_pass(ret, i))
        goto fail;
      
      if(!start_encoder(ret, encoder_handle, encoder_plugin))
        goto fail;
      }
    }
  else
    {
    /* Put all streams into one file. */
    
    if(ret->audio_to_video || ret->num_video_streams_real)
      {
      ret->out_handle = load_encoder_by_name(ret->plugin_reg, ret->video_encoder,
                                             ret->transcoder_track->video_encoder_parameters,
                                             ret->transcoder_track->video_encoder_section,
                                             &encoder_plugin);
      }
    else
      {
      ret->out_handle = load_encoder_by_name(ret->plugin_reg, ret->audio_encoder,
                                             ret->transcoder_track->video_encoder_parameters,
                                             ret->transcoder_track->video_encoder_section,
                                             &encoder_plugin);
      }

    if(!ret->output_filename)
      ret->output_filename = bg_sprintf("%s/%s%s", ret->output_directory, ret->name,
                                        encoder_plugin->get_extension(ret->out_handle->priv));
    
    if(!open_encoder(ret, ret->out_handle,
                     encoder_plugin, &ret->output_filename))
      goto fail;
    
    for(i = 0; i < ret->num_audio_streams; i++)
      {
      if(!ret->audio_streams[i].com.do_decode)/* If we don't encode we still need to open the
                                                 plugin to get the final output format */
        continue;
      set_stream_encoder(&(ret->audio_streams[i].com), ret->out_handle);
      add_audio_stream(&(ret->audio_streams[i]), &(ret->transcoder_track->audio_streams[i]));
      }

    for(i = 0; i < ret->num_video_streams; i++)
      {
      if(!ret->video_streams[i].com.do_decode)/* If we don't encode we still need to open the
                                                 plugin to get the final output format */
        continue;
      set_stream_encoder(&(ret->video_streams[i].com), ret->out_handle);
      add_video_stream(&(ret->video_streams[i]), &(ret->transcoder_track->video_streams[i]), ret->track_info);

      if(!set_video_pass(ret, i))
        goto fail;
      }
    if(!start_encoder(ret, ret->out_handle, encoder_plugin))
      goto fail;
    }
  return 1;
  fail:
  return 0;
  }

static void close_encoders(bg_transcoder_t * ret, int do_delete)
  {
  int i;
  stream_t * s;
  bg_encoder_plugin_t * encoder_plugin;

  if(ret->pp_only)
    return;

  if(ret->separate_streams)
    {
    for(i = 0; i < ret->num_audio_streams; i++)
      {
      s = &ret->audio_streams[i].com; 
      if(s->out_handle)
        {
        s->out_plugin->close(s->out_handle->priv, do_delete);
        bg_plugin_unref(s->out_handle);
        s->out_handle = (bg_plugin_handle_t*)0;
        }
      }
    for(i = 0; i < ret->num_video_streams; i++)
      {
      s = &ret->video_streams[i].com; 
      if(s->out_handle)
        {
        s->out_plugin->close(s->out_handle->priv, do_delete);
        bg_plugin_unref(s->out_handle);
        s->out_handle = (bg_plugin_handle_t*)0;
        }
      }
    }
  else
    {
    /* Put all streams into one file. */
    encoder_plugin = (bg_encoder_plugin_t*)(ret->out_handle->plugin);
    encoder_plugin->close(ret->out_handle->priv, do_delete);
    bg_plugin_unref(ret->out_handle);
    ret->out_handle = (bg_plugin_handle_t*)0;

    for(i = 0; i < ret->num_audio_streams; i++)
      {
      s = &ret->audio_streams[i].com; 
      s->out_handle = (bg_plugin_handle_t*)0;
      }
    for(i = 0; i < ret->num_video_streams; i++)
      {
      s = &ret->video_streams[i].com; 
      s->out_handle = (bg_plugin_handle_t*)0;
      }
    }
  }



static void init_audio_converter(audio_stream_t * ret)
  {
  gavl_audio_options_t * opt;
  ret->com.out_plugin->get_audio_format(ret->com.out_handle->priv,
                                        ret->com.out_index,
                                        &(ret->out_format));

  ret->out_format.samples_per_frame =
    (ret->in_format.samples_per_frame * ret->out_format.samplerate) /
    ret->in_format.samplerate + 10;

  gavl_audio_format_copy(&ret->pipe_format, &ret->out_format);
  if(ret->options.force_float)
    ret->pipe_format.sample_format = GAVL_SAMPLE_FLOAT;
  
  /* Initialize input converter */

  opt = gavl_audio_converter_get_options(ret->cnv_in);
  gavl_audio_options_copy(opt, ret->options.opt);
  
  ret->do_convert_in = gavl_audio_converter_init(ret->cnv_in,
                                                     &(ret->in_format),
                                                     &(ret->pipe_format));

  //  if(ret->do_convert_in)
  //    fprintf(stderr, "**** Doing Input conversion\n");
  
  opt = gavl_audio_converter_get_options(ret->cnv_out);
  gavl_audio_options_copy(opt, ret->options.opt);
  
  ret->do_convert_out = gavl_audio_converter_init(ret->cnv_out,
                                                  &(ret->pipe_format),
                                                  &(ret->out_format));
  //  if(ret->do_convert_out)
  //    fprintf(stderr, "**** Doing Output conversion\n");
  
  /* Create frames (could be left from the previous pass) */

  if(!ret->in_frame)
    ret->in_frame = gavl_audio_frame_create(&(ret->in_format));
  
  if(ret->do_convert_in && !ret->pipe_frame)
    ret->pipe_frame = gavl_audio_frame_create(&(ret->pipe_format));

  if(ret->do_convert_out && !ret->out_frame)
    ret->out_frame = gavl_audio_frame_create(&(ret->out_format));
  }


static void init_video_converter(video_stream_t * ret)
  {
  gavl_video_options_t * opt;
  ret->com.out_plugin->get_video_format(ret->com.out_handle->priv,
                                           ret->com.out_index, &(ret->out_format));

  bg_gavl_video_options_set_rectangles(&(ret->options),
                                       &(ret->in_format),
                                       &(ret->out_format));

  
  /* Initialize converter */

  opt = gavl_video_converter_get_options(ret->cnv);
  gavl_video_options_copy(opt, ret->options.opt);
  
  ret->do_convert = gavl_video_converter_init(ret->cnv, 
                                                  &(ret->in_format),
                                                  &(ret->out_format));

  /* Check if we wanna convert the framerate */

  if((ret->in_format.framerate_mode != GAVL_FRAMERATE_CONSTANT) && (ret->out_format.framerate_mode == GAVL_FRAMERATE_CONSTANT))
    ret->convert_framerate = 1;

  else if((int64_t)ret->in_format.frame_duration * (int64_t)ret->out_format.timescale !=
          (int64_t)ret->out_format.frame_duration * (int64_t)ret->in_format.timescale)
    ret->convert_framerate = 1;

  if(ret->convert_framerate)
    {
    fprintf(stderr, "Doing framerate conversion %5.2f (%s) -> %5.2f (%s)\n",
            (float)(ret->in_format.timescale) / (float)(ret->in_format.frame_duration),
            (ret->in_format.framerate_mode == GAVL_FRAMERATE_VARIABLE ? "nonconstant" : "constant"),
            (float)(ret->out_format.timescale) / (float)(ret->out_format.frame_duration),
            (ret->out_format.framerate_mode == GAVL_FRAMERATE_VARIABLE ? "nonconstant" : "constant"));
    }
  
  /* Create frames */
  if(!ret->in_frame_1)
    ret->in_frame_1 = gavl_video_frame_create(&(ret->in_format));
  gavl_video_frame_clear(ret->in_frame_1, &(ret->in_format));
  
  ret->in_frame_1->time_scaled = GAVL_TIME_UNDEFINED;
  
  if(ret->convert_framerate)
    {
    if(!ret->in_frame_2)
      ret->in_frame_2 = gavl_video_frame_create(&(ret->in_format));
    gavl_video_frame_clear(ret->in_frame_2, &(ret->in_format));
    }
  
  if(ret->do_convert)
    {
    if(!ret->out_frame)
      ret->out_frame = gavl_video_frame_create(&(ret->out_format));
    gavl_video_frame_clear(ret->out_frame, &(ret->out_format));
    }
  }

static void init_converters(bg_transcoder_t * ret)
  {
  int i;
  for(i = 0; i < ret->num_audio_streams; i++)
    {
    if(!ret->audio_streams[i].com.do_decode)
      continue;
    init_audio_converter(&(ret->audio_streams[i]));
    }
  for(i = 0; i < ret->num_video_streams; i++)
    {
    if(!ret->video_streams[i].com.do_decode)
      continue;
    init_video_converter(&(ret->video_streams[i]));
    }
  }

static void init_normalize(bg_transcoder_t * ret)
  {
  int i;
  double min, max, absolute;
  double volume_dB;
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
                                  &min, &max);
      absolute = max > -min ? max : -min;

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
  //  fprintf(stderr, "Setting global parameters...");
  
  /* Set general parameter */

  bg_cfg_section_apply(track->general_section,
                       track->general_parameters,
                       set_parameter_general,
                       ret);

  /* Set Metadata */

  bg_cfg_section_apply(track->metadata_section,
                       track->metadata_parameters,
                       bg_metadata_set_parameter,
                       &(ret->metadata));

  /* Postprocess only: Send messages and return */
  if(ret->pp_only)
    {
    ret->output_filename = bg_strdup(ret->output_filename, ret->location);

    send_init_messages(ret);
    ret->state = STREAM_STATE_FINISHED;
    return 1;
    }
  
  //  fprintf(stderr, "done\n");
    
  //  fprintf(stderr, "Opening input with %s...", ret->location);

  /* Open input plugin */
  if(!open_input(ret))
    goto fail;
  
  //  fprintf(stderr, "done\n");

  create_streams(ret, track);

  /* Set first transcoding pass */
  check_passes(ret);
  ret->pass = 1;
  setup_pass(ret);
  
  /* Start input plugin */
  
  if(!start_input(ret))
    goto fail;

  set_input_formats(ret);
  
  /* Check for the encoding plugins */

  //  fprintf(stderr, "Checking for mode...");
  
  if(!strcmp(ret->audio_encoder, ret->video_encoder))
    ret->audio_to_video = 1;
  else
    ret->audio_to_video = 0;
    
  if(!check_separate(ret))
    goto fail;
  
  /* Set up the streams in the encoders */
  if(!init_encoders(ret))
    goto fail;
  
  /* Set formats */
  init_converters(ret);

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
  FREE_STR(s->output_filename);
  }

static void cleanup_audio_stream(audio_stream_t * s)
  {
  cleanup_stream(&(s->com));
  
  /* Free all resources */

  if(s->in_frame)
    gavl_audio_frame_destroy(s->in_frame);
  if(s->out_frame)
    gavl_audio_frame_destroy(s->out_frame);
  if(s->cnv_in)
    gavl_audio_converter_destroy(s->cnv_in);
  if(s->cnv_out)
    gavl_audio_converter_destroy(s->cnv_out);

  if(s->volume_control)
    gavl_volume_control_destroy(s->volume_control);
  if(s->peak_detector)
    gavl_peak_detector_destroy(s->peak_detector);
  
  bg_gavl_audio_options_free(&s->options);
  }

static void cleanup_video_stream(video_stream_t * s)
  {
  cleanup_stream(&(s->com));

  /* Free all resources */

  if(s->in_frame_1)
    gavl_video_frame_destroy(s->in_frame_1);
  if(s->in_frame_2)
    gavl_video_frame_destroy(s->in_frame_2);
  if(s->out_frame)
    gavl_video_frame_destroy(s->out_frame);
  if(s->cnv)
    gavl_video_converter_destroy(s->cnv);

  /* Delete stats file */
  if(s->stats_file)
    {
    //    remove(s->stats_file);
    free(s->stats_file);
    }
  bg_gavl_video_options_free(&s->options);
  }



static void reset_streams(bg_transcoder_t * t)
  {
  int i;
  for(i = 0; i < t->num_audio_streams; i++)
    {
    t->audio_streams[i].samples_written = 0;
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
  t->in_handle = (bg_plugin_handle_t*)0;
  }

/* Switch to next pass */

static void next_pass(bg_transcoder_t * t)
  {
  char * tmp_string;

  close_encoders(t, 1);
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
  
  /* Decide, which stream will be en/decoded*/
  setup_pass(t);

  start_input(t);

  /* Some streams don't have this already */
  set_input_formats(t);
  
  /* Check for the encoding plugins */
  check_separate(t);
  init_encoders(t);

  /* Set formats */
  init_converters(t);

  /* Init normalizing */
  init_normalize(t);
  
  /* Send message */
  tmp_string = bg_sprintf("Transcoding %s [Track %d, Pass %d/%d]", t->location, t->track+1, t->pass, t->total_passes);
  bg_transcoder_send_msg_start(t->message_queues, tmp_string);
  bg_log(BG_LOG_INFO, LOG_DOMAIN, "%s", tmp_string);
  free(tmp_string);

  gavl_timer_start(t->timer);
  }

/*
 *  Do one iteration (Will be called as an idle function in the GUI main loop or by bg_transcoder_run())
 *  If return value is FALSE, we are done
 */

int bg_transcoder_iteration(bg_transcoder_t * t)
  {
  int i;
  gavl_time_t time;
  stream_t * stream = (stream_t*)0;

  gavl_time_t real_time;
  double real_seconds;

  double remaining_seconds;
  
  int done = 1;

  if(t->pp_only)
    {
    t->state = TRANSCODER_STATE_FINISHED;
    //    fprintf(stderr, "finished\n");
    bg_transcoder_send_msg_finished(t->message_queues);
    return 0;
    }
  
  //  fprintf(stderr, "bg_transcoder_iteration...");
  
  time = GAVL_TIME_MAX;
  
  /* Find the stream with the smallest time */
  
  for(i = 0; i < t->num_audio_streams; i++)
    {
    if(t->audio_streams[i].com.status != STREAM_STATE_ON)
      continue;
    done = 0;
    if(t->audio_streams[i].com.time < time)
      {
      time = t->audio_streams[i].com.time;
      stream = &(t->audio_streams[i].com);
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
      stream = &(t->video_streams[i].com);
      }
    }
  
  if(done)
    {
    if(t->pass < t->total_passes)
      {
      next_pass(t);
      return 1;
      }
    else
      {
      t->state = TRANSCODER_STATE_FINISHED;
      //    fprintf(stderr, "finished\n");
      bg_transcoder_send_msg_finished(t->message_queues);
      return 0;
      }
    }
  
  /* Do the actual transcoding */

  if(stream->type == STREAM_TYPE_AUDIO)
    audio_iteration((audio_stream_t*)stream, t);
  else if(stream->type == STREAM_TYPE_VIDEO)
    video_iteration((video_stream_t*)stream, t);

  if(stream->time > t->time)
    t->time = stream->time;
  
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
  
  //  fprintf(stderr, "t->transcoder_info.remaining_time: %lld\n",
  //          t->transcoder_info.remaining_time);

  //  fprintf(stderr, "done\n");
  
  return 1;
  }



void bg_transcoder_destroy(bg_transcoder_t * t)
  {
  int i;
  int do_delete;
  do_delete =
    ((t->state == TRANSCODER_STATE_RUNNING) && t->delete_incomplete &&
     !t->is_url) ? 1 : 0;

  if(t->state == TRANSCODER_STATE_INIT)
    do_delete = 1;

  //  fprintf(stderr, "Do delete: %d\n", do_delete);
  
  /* Close all encoders so the files are finished */

  close_encoders(t, do_delete);
  
  /* Send created files to gmerlin */

  if((t->state != TRANSCODER_STATE_RUNNING) && !do_delete)
    {
    send_file_messages(t);
    }
  
  /* Cleanup streams */

  for(i = 0; i < t->num_video_streams; i++)
    {
    if((t->video_streams[i].com.action != STREAM_ACTION_FORGET) && !do_delete)
      bg_log(BG_LOG_INFO, LOG_DOMAIN,
             "Video stream %d: Transcoded %lld frames", i+1,
             t->video_streams[i].frames_written);
    cleanup_video_stream(&(t->video_streams[i]));
    }
  for(i = 0; i < t->num_audio_streams; i++)
    {
    if((t->audio_streams[i].com.action != STREAM_ACTION_FORGET) && !do_delete)
      bg_log(BG_LOG_INFO, LOG_DOMAIN,
             "Audio stream %d: Transcoded %lld samples", i+1,
             t->audio_streams[i].samples_written);
    cleanup_audio_stream(&(t->audio_streams[i]));
    }

    
  if(t->audio_streams)
    free(t->audio_streams);
  if(t->video_streams)
    free(t->video_streams);

  /* Free metadata */

  bg_metadata_free(&(t->metadata));
  
  /* Close and destroy the input plugin */

  close_input(t);
  
  /* Free rest */

  FREE_STR(t->name);
  FREE_STR(t->location);
  FREE_STR(t->plugin);
  FREE_STR(t->audio_encoder);
  FREE_STR(t->video_encoder);
  FREE_STR(t->output_directory);
  FREE_STR(t->output_filename);
  
  gavl_timer_destroy(t->timer);
  if(t->error_msg)
    free(t->error_msg);

  bg_msg_queue_list_destroy(t->message_queues);
  pthread_mutex_destroy(&(t->stop_mutex));

  
  free(t);
  }

const char * bg_transcoder_get_error(bg_transcoder_t * t)
  {
  return t->error_msg_ret;
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
  pthread_create(&(t->thread), (pthread_attr_t*)0, thread_func, t);
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

