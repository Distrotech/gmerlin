/*****************************************************************
 
  transcoder.c
 
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
#include <string.h>

#include <pluginregistry.h>
#include <transcoder.h>
#include <utils.h>
#include <bggavl.h>

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
  int do_convert;

  int do_close; /* Output must be closed */

  char * output_filename;
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

  gavl_audio_converter_t * cnv;
  gavl_audio_frame_t * in_frame;
  gavl_audio_frame_t * out_frame;

  gavl_audio_format_t in_format;
  gavl_audio_format_t out_format;
    
  /* Set by set_parameter */
  bg_gavl_audio_options_t options;
  int64_t samples_to_read;
  int64_t samples_read;

  int64_t samples_written;
  
  int initialized;
  } audio_stream_t;

static void set_audio_parameter_general(void * data, char * name, bg_parameter_value_t * val)
  {
  audio_stream_t * stream;
  stream = (audio_stream_t*)data;
  
  if(!name)
    return;

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
  
  audio_stream_t * audio_streams;
  video_stream_t * video_streams;
  
  bg_transcoder_info_t transcoder_info;

  int state;
    
  bg_plugin_handle_t * in_handle;
  bg_input_plugin_t  * in_plugin;
  bg_track_info_t * track_info;

  /* Set by set_parameter */

  char * name;
  char * location;
  char * plugin;
  char * audio_encoder;
  char * video_encoder;
  
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

static void prepare_audio_stream(audio_stream_t * ret,
                                 bg_transcoder_track_audio_t * s,
                                 bg_plugin_handle_t * in_handle,
                                 int in_index)
  {
  ret->com.type = STREAM_TYPE_AUDIO;
  ret->com.in_handle = in_handle;
  ret->com.in_plugin = (bg_input_plugin_t*)(in_handle->plugin);
  /* Default options */
  
  /* Create converter */

  ret->cnv = gavl_audio_converter_create();
  ret->options.opt = gavl_audio_converter_get_options(ret->cnv);
  bg_gavl_audio_options_init(&(ret->options));

  
  /* Apply parameters */

  bg_cfg_section_apply(s->general_section,
                       bg_transcoder_track_audio_get_general_parameters(),
                       set_audio_parameter_general, ret);
  
  ret->com.in_index = in_index;
  
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

static void prepare_video_stream(video_stream_t * ret,
                                 bg_transcoder_track_video_t * s,
                                 bg_plugin_handle_t * in_handle,
                                 int in_index)
  {
  ret->com.type = STREAM_TYPE_VIDEO;
  ret->com.in_handle = in_handle;
  ret->com.in_plugin = (bg_input_plugin_t*)(in_handle->plugin);
    
  /* Create converter */

  ret->cnv = gavl_video_converter_create();

  ret->options.opt = gavl_video_converter_get_options(ret->cnv);

  /* Default options */

  bg_gavl_video_options_init(&(ret->options));
  
  /* Apply parameters */

  bg_cfg_section_apply(s->general_section,
                       bg_transcoder_track_video_get_general_parameters(),
                       set_video_parameter_general, ret);
  
  ret->com.in_index = in_index;

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


static void finalize_stream(stream_t * s, bg_plugin_handle_t * out_handle,
                            int out_index, int do_close)
  {
  s->status = STREAM_STATE_ON;
  s->do_close = do_close;
  s->out_index = out_index;
  s->out_handle = out_handle;
  bg_plugin_ref(s->out_handle);
  s->out_plugin = (bg_encoder_plugin_t*)(s->out_handle->plugin);
  }

static void finalize_audio_stream(audio_stream_t * ret,
                                  bg_transcoder_track_audio_t * s,
                                  bg_plugin_handle_t * out_handle,
                                  int out_index,
                                  bg_track_info_t * track_info, int do_close)
  {
  set_stream_param_struct_t st;

  finalize_stream(&(ret->com), out_handle, out_index, do_close);
  
  /* Get input format formats */
  
  gavl_audio_format_copy(&(ret->in_format),
                         &(track_info->audio_streams[ret->com.in_index].format));

  /* We set the frame size so we have roughly half second long audio chunks */
#if 1
  ret->in_format.samples_per_frame = gavl_time_to_samples(ret->in_format.samplerate,
                                                          GAVL_TIME_SCALE/2);
#endif

  bg_gavl_audio_options_set_format(&(ret->options),
                                   &(ret->in_format),
                                   &(ret->out_format));
  
  /* Add the audio stream */

  ret->com.out_plugin->add_audio_stream(out_handle->priv, &(ret->out_format));
  
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

  
  ret->com.out_plugin->get_audio_format(ret->com.out_handle->priv,
                                        ret->com.out_index,
                                        &(ret->out_format));

  /* Dump formats */
#if 0
  fprintf(stderr, "Input format:\n");
  gavl_audio_format_dump(&(ret->in_format));
  fprintf(stderr, "Output format:\n");
  gavl_audio_format_dump(&(ret->out_format));
  fprintf(stderr, "\n");
#endif
  /* Initialize converter */

  ret->com.do_convert = gavl_audio_converter_init(ret->cnv,
                                                  &(ret->in_format),
                                                  &(ret->out_format));

  /* Create frames */

  ret->in_frame = gavl_audio_frame_create(&(ret->in_format));
  
  ret->out_format.samples_per_frame =
    (ret->in_format.samples_per_frame * ret->out_format.samplerate) /
    ret->in_format.samplerate + 10;
  
  if(ret->com.do_convert)
    ret->out_frame = gavl_audio_frame_create(&(ret->out_format));
  
  }

static void finalize_video_stream(video_stream_t * ret,
                                  bg_transcoder_track_video_t * s,
                                  bg_plugin_handle_t * out_handle,
                                  int out_index,
                                  bg_track_info_t * track_info, int do_close)
  {
  set_stream_param_struct_t st;

  finalize_stream(&(ret->com), out_handle, out_index, do_close);
    
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
  /* Add the video stream */

  ret->com.out_plugin->add_video_stream(out_handle->priv,
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


  ret->com.out_plugin->get_video_format(ret->com.out_handle->priv,
                                           ret->com.out_index, &(ret->out_format));

  /* Dump formats */
#if 1
  fprintf(stderr, "Input format:\n");
  gavl_video_format_dump(&(ret->in_format));
  fprintf(stderr, "Output format:\n");
  gavl_video_format_dump(&(ret->out_format));
  fprintf(stderr, "\n");
#endif
  /* Initialize converter */
  
  ret->com.do_convert = gavl_video_converter_init(ret->cnv, 
                                                  &(ret->in_format),
                                                  &(ret->out_format));

  /* Check if we wanna convert the framerate */

  if(ret->in_format.free_framerate && !ret->out_format.free_framerate)
    ret->convert_framerate = 1;

  else if((int64_t)ret->in_format.frame_duration * (int64_t)ret->out_format.timescale !=
          (int64_t)ret->out_format.frame_duration * (int64_t)ret->in_format.timescale)
    ret->convert_framerate = 1;

  if(ret->convert_framerate)
    {
    fprintf(stderr, "Doing framerate conversion %5.2f (%s) -> %5.2f (%s)\n",
            (float)(ret->in_format.timescale) / (float)(ret->in_format.frame_duration),
            (ret->in_format.free_framerate ? "nonconstant" : "constant"),
            (float)(ret->out_format.timescale) / (float)(ret->out_format.frame_duration),
            (ret->out_format.free_framerate ? "nonconstant" : "constant"));
    }
  
  /* Create frames */

  ret->in_frame_1 = gavl_video_frame_create(&(ret->in_format));
  gavl_video_frame_clear(ret->in_frame_1, &(ret->in_format));
  
  ret->in_frame_1->time = GAVL_TIME_UNDEFINED;
  
  if(ret->convert_framerate)
    {
    ret->in_frame_2 = gavl_video_frame_create(&(ret->in_format));
    gavl_video_frame_clear(ret->in_frame_2, &(ret->in_format));
    }
  
  if(ret->com.do_convert)
    {
    ret->out_frame = gavl_video_frame_create(&(ret->out_format));
    gavl_video_frame_clear(ret->out_frame, &(ret->out_format));
    }
    
  }

static void audio_iteration(audio_stream_t*s, bg_transcoder_t * t)
  {
  int num_samples;
  int samples_decoded;
  
  //  fprintf(stderr, "Audio iteration\n");
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
  
  if(s->com.do_convert)
    {
    gavl_audio_convert(s->cnv, s->in_frame, s->out_frame);
    s->com.out_plugin->write_audio_frame(s->com.out_handle->priv,
                                         s->out_frame,
                                         s->com.out_index);
    s->samples_written += s->out_frame->valid_samples;
    }
  else
    {
    s->com.out_plugin->write_audio_frame(s->com.out_handle->priv,
                                         s->in_frame,
                                         s->com.out_index);
    s->samples_written += s->in_frame->valid_samples;
    }
  s->com.time = gavl_samples_to_time(s->out_format.samplerate,
                                     s->samples_written);
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
     (f->time >= t->end_time))
    return 0;
  
  /* Correct timestamps */

  if(s->start_time_scaled)
    {
    f->time_scaled -= s->start_time_scaled;
    if(f->time_scaled < 0)
      f->time_scaled = 0;
    f->time = gavl_samples_to_time(s->in_format.timescale, f->time_scaled);
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
    }
  
  //  fprintf(stderr, "Video iteration\n");
  
  if(s->convert_framerate)
    {
    next_time = gavl_frames_to_time(s->out_format.timescale,
                                    s->out_format.frame_duration,
                                    s->frames_written);


    if(s->in_frame_1->time == GAVL_TIME_UNDEFINED)
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

    while(s->in_frame_2->time < next_time)
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

    tmp_frame->time = next_time;
    tmp_frame->time_scaled = s->frames_written * s->out_format.frame_duration;
    
    if(s->com.do_convert)
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

    if(s->com.do_convert)
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
    s->com.time = s->in_frame_1->time;
    //    fprintf(stderr, "Video iteration %f\n", gavl_time_to_seconds(s->com.time));
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

bg_transcoder_t * bg_transcoder_create()
  {
  bg_transcoder_t * ret;
  ret = calloc(1, sizeof(*ret));
  ret->timer = gavl_timer_create();
  return ret;
  }

int bg_transcoder_init(bg_transcoder_t * ret,
                       bg_plugin_registry_t * plugin_reg,
                       bg_transcoder_track_t * track)
  {
  int i;
  int stream_index;
  int audio_to_video;
  int do_close;
  int num_audio_streams, num_video_streams;

  char * audio_encoder_name = (char*)0;
  char * video_encoder_name = (char*)0;
    
  bg_plugin_handle_t  * audio_encoder_handle = (bg_plugin_handle_t  *)0;
  bg_encoder_plugin_t * audio_encoder_plugin;
  
  bg_plugin_handle_t  * video_encoder_handle = (bg_plugin_handle_t  *)0;
  bg_encoder_plugin_t * video_encoder_plugin;

  bg_plugin_handle_t  * encoder_handle = (bg_plugin_handle_t  *)0;
  bg_encoder_plugin_t * encoder_plugin;
  
  const bg_plugin_info_t * plugin_info;

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

  //  fprintf(stderr, "done\n");
    
  //  fprintf(stderr, "Opening input with %s...", ret->location);

  /* Open input plugin */

  plugin_info = bg_plugin_find_by_name(plugin_reg, ret->plugin);
  if(!plugin_info)
    {
    ret->error_msg = bg_sprintf("Cannot find plugin %s", ret->plugin);
    ret->error_msg_ret = ret->error_msg;
    goto fail;
    }

  ret->in_handle = bg_plugin_load(plugin_reg, plugin_info);
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
  
  //  fprintf(stderr, "done\n");

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
    
  num_audio_streams = 0;
    
  for(i = 0; i < ret->num_audio_streams; i++)
    {
    //    fprintf(stderr, "Preparing audio stream %d...", i);

    prepare_audio_stream(&(ret->audio_streams[i]),
                         &(track->audio_streams[i]),
                         ret->in_handle, i);
    if(ret->audio_streams[i].com.action == STREAM_ACTION_TRANSCODE)
      num_audio_streams++;
    //    fprintf(stderr, "done\n");
    }

  num_video_streams = 0;
  
  for(i = 0; i < ret->num_video_streams; i++)
    {
    //    fprintf(stderr, "Preparing video stream %d...", i);
    prepare_video_stream(&(ret->video_streams[i]),
                         &(track->video_streams[i]),
                         ret->in_handle, i);
    if(ret->video_streams[i].com.action == STREAM_ACTION_TRANSCODE)
      num_video_streams++;
    //    fprintf(stderr, "done\n");
    }
  
  /* Start input plugin */

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
  
  /* Check for the encoding plugins */

  //  fprintf(stderr, "Checking for mode...");

  audio_encoder_name = bg_transcoder_track_get_audio_encoder(track);
  video_encoder_name = bg_transcoder_track_get_video_encoder(track);

  if(!strcmp(audio_encoder_name, video_encoder_name))
    audio_to_video = 1;
  else
    audio_to_video = 0;

  ret->separate_streams = 0;
  if(!audio_to_video && num_audio_streams && num_video_streams)
    ret->separate_streams = 1;
  
  /* Load audio and video plugins so we can check for the maximum
     stream numbers */

  if(!ret->separate_streams)
    {
    if(num_audio_streams)
      { /* Load audio plugin and check for max_audio_streams */

      if(audio_to_video)
        {
        video_encoder_handle = load_encoder_by_name(plugin_reg, video_encoder_name,
                                                    track->video_encoder_parameters,
                                                    track->video_encoder_section,
                                                    &video_encoder_plugin);
        if(!video_encoder_handle)
          goto fail;
        if((video_encoder_plugin->max_audio_streams >= 0) &&
           (num_audio_streams > video_encoder_plugin->max_audio_streams))
          ret->separate_streams = 1;
        }
      else
        {
        audio_encoder_handle = load_encoder_by_name(plugin_reg, audio_encoder_name,
                                                    track->audio_encoder_parameters,
                                                    track->audio_encoder_section,
                                                    &audio_encoder_plugin);
        if(!audio_encoder_handle)
          goto fail;

        if((audio_encoder_plugin->max_audio_streams >= 0) &&
           (num_audio_streams > audio_encoder_plugin->max_audio_streams))
          ret->separate_streams = 1;
        
        }
      }
    }

  //  fprintf(stderr, "done, ret->separate_streams: %d, audio_to_video: %d\n",
  //          ret->separate_streams, audio_to_video);
  
  if(!ret->separate_streams)
    {
    if(num_video_streams)
      { /* Load audio plugin and check for max_audio_streams */

      if(!video_encoder_handle)
        video_encoder_handle = load_encoder_by_name(plugin_reg, video_encoder_name,
                                                    track->video_encoder_parameters,
                                                    track->video_encoder_section,
                                                    &video_encoder_plugin);
      if((video_encoder_plugin->max_video_streams >= 0) &&
         (num_video_streams > video_encoder_plugin->max_video_streams))
        ret->separate_streams = 1;
      }
    }

  /* Finalize the streams */
    
  if(ret->separate_streams)
    {
    /* Open new files for each streams */

    for(i = 0; i < ret->num_audio_streams; i++)
      {
      if(ret->audio_streams[i].com.action != STREAM_ACTION_TRANSCODE)
        continue;
      if(audio_to_video)
        {
        if(!video_encoder_handle)
          {
          video_encoder_handle = load_encoder_by_name(plugin_reg, video_encoder_name,
                                                      track->video_encoder_parameters,
                                                      track->video_encoder_section,
                                                      &video_encoder_plugin);
          }
        encoder_handle = video_encoder_handle;
        encoder_plugin = video_encoder_plugin;
        }
      else
        {
        if(!audio_encoder_handle)
          {
          audio_encoder_handle = load_encoder_by_name(plugin_reg, audio_encoder_name,
                                                      track->audio_encoder_parameters,
                                                      track->audio_encoder_section,
                                                      &audio_encoder_plugin);
          }
        encoder_handle = audio_encoder_handle;
        encoder_plugin = audio_encoder_plugin;
        }

      ret->audio_streams[i].com.output_filename =
        bg_sprintf("%s/%s_audio_%02d%s", ret->output_directory, ret->name, i+1,
                   encoder_plugin->get_extension(encoder_handle->priv));

      if(!strcmp(ret->audio_streams[i].com.output_filename, ret->location))
        {
        ret->error_msg = bg_sprintf("Input and output are the same file");
        ret->error_msg_ret = ret->error_msg;
        bg_plugin_unref(encoder_handle);
        goto fail;
        }
      
      encoder_plugin->open(encoder_handle->priv,
                           ret->audio_streams[i].com.output_filename,
                           &(ret->metadata));
      
      finalize_audio_stream(&(ret->audio_streams[i]), &(track->audio_streams[i]),
                            encoder_handle, 0, ret->track_info, 1);

      if(encoder_plugin->start && !encoder_plugin->start(encoder_handle->priv))
        {
        ret->error_msg = bg_sprintf("Cannot setup %s", encoder_handle->info->long_name);
        ret->error_msg_ret = ret->error_msg;
        bg_plugin_unref(encoder_handle);
        goto fail;
        }
      
      bg_plugin_unref(encoder_handle);
      if(audio_to_video)
        video_encoder_handle = (bg_plugin_handle_t*)0;
      else
        audio_encoder_handle = (bg_plugin_handle_t*)0;
      }
    for(i = 0; i < ret->num_video_streams; i++)
      {
      if(!video_encoder_handle)
        {
        video_encoder_handle = load_encoder_by_name(plugin_reg, video_encoder_name,
                                                    track->video_encoder_parameters,
                                                    track->video_encoder_section,
                                                    &video_encoder_plugin);
        }
      
      ret->video_streams[i].com.output_filename =
        bg_sprintf("%s/%s_video_%02d%s", ret->output_directory, ret->name, i+1,
                   video_encoder_plugin->get_extension(video_encoder_handle->priv));

      if(!strcmp(ret->video_streams[i].com.output_filename, ret->location))
        {
        ret->error_msg = bg_sprintf("Input and output are the same file");
        ret->error_msg_ret = ret->error_msg;
        bg_plugin_unref(video_encoder_handle);
        goto fail;
        }
      
      video_encoder_plugin->open(video_encoder_handle->priv,
                                 ret->video_streams[i].com.output_filename,
                                 &(ret->metadata));

      finalize_video_stream(&(ret->video_streams[i]), &(track->video_streams[i]),
                            video_encoder_handle, 0, ret->track_info, 1);

      if(video_encoder_plugin->start && !video_encoder_plugin->start(video_encoder_handle->priv))
        {
        ret->error_msg = bg_sprintf("Cannot setup %s", video_encoder_handle->info->long_name);
        ret->error_msg_ret = ret->error_msg;
        bg_plugin_unref(video_encoder_handle);
        goto fail;
        }
      
      bg_plugin_unref(video_encoder_handle);
      video_encoder_handle = (bg_plugin_handle_t*)0;
      }
    }
  else
    {
    /* Put all streams into one file. Plugin is already loaded */

    stream_index = 0;
    
    if(audio_to_video || num_video_streams)
      {
      encoder_plugin = video_encoder_plugin;
      encoder_handle = video_encoder_handle;
      }
    else
      {
      encoder_plugin = audio_encoder_plugin;
      encoder_handle = audio_encoder_handle;
      }
    
    ret->output_filename = bg_sprintf("%s/%s%s", ret->output_directory, ret->name,
                                 encoder_plugin->get_extension(encoder_handle->priv));

    if(!strcmp(ret->output_filename, ret->location))
      {
      ret->error_msg = bg_sprintf("Input and output are the same file");
      ret->error_msg_ret = ret->error_msg;
      bg_plugin_unref(encoder_handle);
      goto fail;
      }
    
    encoder_plugin->open(encoder_handle->priv, ret->output_filename, &(ret->metadata));
    
    stream_index = 0;
    for(i = 0; i < ret->num_audio_streams; i++)
      {
      if(ret->audio_streams[i].com.action != STREAM_ACTION_TRANSCODE)
        continue;
      
      if((stream_index == num_audio_streams - 1) && !num_video_streams)
        do_close = 1;
      else
        do_close = 0;
      
      finalize_audio_stream(&(ret->audio_streams[i]), &(track->audio_streams[i]),
                            encoder_handle, stream_index, ret->track_info, do_close);
      stream_index++;
      }

    //    encoder_plugin = video_encoder_plugin;
    //    encoder_handle = video_encoder_handle;

    stream_index = 0;
    for(i = 0; i < ret->num_video_streams; i++)
      {
      if(ret->video_streams[i].com.action != STREAM_ACTION_TRANSCODE)
        continue;

      if(stream_index == num_video_streams - 1)
        do_close = 1;
      else
        do_close = 0;
      
      finalize_video_stream(&(ret->video_streams[i]), &(track->video_streams[i]),
                            encoder_handle, stream_index, ret->track_info, do_close);
      stream_index++;
      }

    if(encoder_plugin->start && !encoder_plugin->start(encoder_handle->priv))
      {
      ret->error_msg = bg_sprintf("Cannot setup %s", encoder_handle->info->long_name);
      ret->error_msg_ret = ret->error_msg;
      bg_plugin_unref(video_encoder_handle);
      goto fail;
      }
    
    bg_plugin_unref(encoder_handle);
    }

  /* Set duration */

  if(ret->set_start_time && ret->set_end_time)
    {
    ret->duration = ret->end_time - ret->start_time;
    }
  else if(ret->set_start_time)
    {
    ret->duration = ret->track_info->duration - ret->start_time;
    }
  else if(ret->set_end_time)
    {
    ret->duration = ret->end_time;
    }
  else
    {
    ret->duration = ret->track_info->duration;
    }
  
  /* Free stuff */
  
  free(audio_encoder_name);
  free(video_encoder_name);
  
  /* Start timer */

  gavl_timer_start(ret->timer);
  
  ret->state = TRANSCODER_STATE_RUNNING;
  
  return 1;

  fail:

  if(audio_encoder_name)
    free(audio_encoder_name);

  if(video_encoder_name)
    free(video_encoder_name);

  return 0;
  
  }

const bg_transcoder_info_t * bg_transcoder_get_info(bg_transcoder_t * t)
  {
  return &(t->transcoder_info);
  }

/*
 *  Do one iteration (Will be called as an idle function in the GUI main loop)
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
    t->state = TRANSCODER_STATE_FINISHED;
    //    fprintf(stderr, "finished\n");
    return 0;
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

  t->transcoder_info.percentage_done =
    gavl_time_to_seconds(t->time) /
    gavl_time_to_seconds(t->duration);
  
  if(t->transcoder_info.percentage_done < 0.0)
    t->transcoder_info.percentage_done = 0.0;
  if(t->transcoder_info.percentage_done > 1.0)
    t->transcoder_info.percentage_done = 1.0;
  if(t->transcoder_info.percentage_done == 0.0)
    remaining_seconds = 0.0;
  else
    {
    remaining_seconds = real_seconds *
      (1.0 / t->transcoder_info.percentage_done - 1.0);
    }
  
  t->transcoder_info.remaining_time =
    gavl_seconds_to_time(remaining_seconds);

  //  fprintf(stderr, "t->transcoder_info.remaining_time: %lld\n",
  //          t->transcoder_info.remaining_time);

  //  fprintf(stderr, "done\n");
  
  return 1;
  }

/* Cleanup streams */

#define FREE_STR(str) if(str)free(str);

static void cleanup_stream(stream_t * s, int do_delete)
  {
  if(s->action == STREAM_ACTION_FORGET)
    return;
      
  if((s->do_close) && (s->out_handle))
    s->out_plugin->close(s->out_handle->priv, do_delete);
  
  if(s->out_handle)
    bg_plugin_unref(s->out_handle);
  FREE_STR(s->output_filename);
  }

static void cleanup_audio_stream(audio_stream_t * s, int do_delete)
  {
  cleanup_stream(&(s->com), do_delete);
  
  /* Free all resources */

  if(s->in_frame)
    gavl_audio_frame_destroy(s->in_frame);
  if(s->out_frame)
    gavl_audio_frame_destroy(s->out_frame);
  if(s->cnv)
    gavl_audio_converter_destroy(s->cnv);
  }

static void cleanup_video_stream(video_stream_t * s, int do_delete)
  {
  cleanup_stream(&(s->com), do_delete);

  /* Free all resources */

  if(s->in_frame_1)
    gavl_video_frame_destroy(s->in_frame_1);
  if(s->in_frame_2)
    gavl_video_frame_destroy(s->in_frame_2);
  if(s->out_frame)
    gavl_video_frame_destroy(s->out_frame);
  if(s->cnv)
    gavl_video_converter_destroy(s->cnv);
  }

static void send_file(const char * name)
  {
  char * command;

  //  fprintf(stderr, "Sending file: %s\n", name);
  
  command = bg_sprintf("gmerlin_remote -add \"%s\"\n", name);
  system(command);
  free(command);
  }

static void send_finished(bg_transcoder_t * t)
  {
  int i;
  const char * filename;

  if(t->separate_streams)
    {
    for(i = 0; i < t->num_audio_streams; i++)
      {
      if(t->audio_streams[i].com.action == STREAM_ACTION_FORGET)
        continue;
      if(t->audio_streams[i].com.out_plugin->get_filename)
        filename =
          t->audio_streams[i].com.out_plugin->get_filename(t->audio_streams[i].com.out_handle->priv);
      else
        filename = t->audio_streams[i].com.output_filename;
      send_file(filename);
      }
    for(i = 0; i < t->num_video_streams; i++)
      {
      if(t->video_streams[i].com.action == STREAM_ACTION_FORGET)
        continue;
      if(t->video_streams[i].com.out_plugin->get_filename)
        filename =
          t->video_streams[i].com.out_plugin->get_filename(t->video_streams[i].com.out_handle->priv);
      else
        filename = t->video_streams[i].com.output_filename;
      send_file(filename);
      }
    }
  else
    {
    filename = (char*)0;

    for(i = 0; i < t->num_audio_streams; i++)
      {
      if(t->audio_streams[i].com.action == STREAM_ACTION_FORGET)
        continue;
      if(t->audio_streams[i].com.out_plugin->get_filename)
        filename =
          t->audio_streams[i].com.out_plugin->get_filename(t->audio_streams[i].com.out_handle->priv);
      }

    if(!filename)
      {
      for(i = 0; i < t->num_video_streams; i++)
        {
        if(t->video_streams[i].com.action == STREAM_ACTION_FORGET)
          continue;
        if(t->video_streams[i].com.out_plugin->get_filename)
          filename =
            t->video_streams[i].com.out_plugin->get_filename(t->video_streams[i].com.out_handle->priv);
        else
          filename = t->video_streams[i].com.output_filename;
        send_file(filename);
        }
      }
    if(!filename)
      filename = t->output_filename;
    send_file(filename);
    }
  }

void bg_transcoder_destroy(bg_transcoder_t * t)
  {
  int i;
  int do_delete;
  do_delete =
    ((t->state == TRANSCODER_STATE_RUNNING) && t->delete_incomplete && !t->is_url) ? 1 : 0;

  //  fprintf(stderr, "Do delete: %d\n", do_delete);

  /* Send created files to gmerlin */

  if((t->state != TRANSCODER_STATE_RUNNING) && (t->send_finished))
    {
    send_finished(t);
    }
  
  /* Cleanup streams */

  for(i = 0; i < t->num_audio_streams; i++)
    {
    cleanup_audio_stream(&(t->audio_streams[i]), do_delete);
    }
  for(i = 0; i < t->num_video_streams; i++)
    {
    cleanup_video_stream(&(t->video_streams[i]), do_delete);
    }
  if(t->audio_streams)
    free(t->audio_streams);
  if(t->video_streams)
    free(t->video_streams);

  /* Free metadata */

  bg_metadata_free(&(t->metadata));
  
  /* Close and destroy the input plugin */

  if(t->in_plugin->stop)
    t->in_plugin->stop(t->in_handle->priv);

  t->in_plugin->close(t->in_handle->priv);
  bg_plugin_unref(t->in_handle);

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
  
  free(t);
  }

const char * bg_transcoder_get_error(bg_transcoder_t * t)
  {
  return t->error_msg_ret;
  }

