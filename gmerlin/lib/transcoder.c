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

  gavl_audio_options_t opt;
  
  /* Set by set_parameter */

  int fixed_samplerate;
  int samplerate;
  
  int fixed_channel_setup;
  gavl_channel_setup_t channel_setup;

  int64_t samples_written;
  
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

  if(!strcmp(name, "conversion_quality"))
    {
    stream->opt.quality = val->val_i;
    }
  else if(!strcmp(name, "fixed_samplerate"))
    {
    stream->fixed_samplerate = val->val_i;
    }
  else if(!strcmp(name, "samplerate"))
    {
    stream->samplerate = val->val_i;
    }
  else if(!strcmp(name, "fixed_channel_setup"))
    {
    stream->fixed_channel_setup = val->val_i;
    }
  else if(!strcmp(name, "channel_setup"))
    {
    if(!strcmp(val->val_str, "Mono"))
      stream->channel_setup = GAVL_CHANNEL_MONO;
    else if(!strcmp(val->val_str, "Stereo"))
      stream->channel_setup = GAVL_CHANNEL_STEREO;
    else if(!strcmp(val->val_str, "3 Front"))
      stream->channel_setup = GAVL_CHANNEL_3F;
    else if(!strcmp(val->val_str, "2 Front 1 Rear"))
      stream->channel_setup = GAVL_CHANNEL_2F1R;
    else if(!strcmp(val->val_str, "3 Front 1 Rear"))
      stream->channel_setup = GAVL_CHANNEL_3F1R;
    else if(!strcmp(val->val_str, "2 Front 2 Rear"))
      stream->channel_setup = GAVL_CHANNEL_2F2R;
    else if(!strcmp(val->val_str, "3 Front 2 Rear"))
      stream->channel_setup = GAVL_CHANNEL_3F2R;
    }

  else if(!strcmp(name, "front_to_rear"))
    {
    stream->opt.conversion_flags &= ~GAVL_AUDIO_FRONT_TO_REAR_MASK;
    
    if(!strcmp(val->val_str, "Copy"))
      {
      stream->opt.conversion_flags |= GAVL_AUDIO_FRONT_TO_REAR_COPY;
      }
    else if(!strcmp(val->val_str, "Mute"))
      {
      stream->opt.conversion_flags |= GAVL_AUDIO_FRONT_TO_REAR_MUTE;
      }
    else if(!strcmp(val->val_str, "Diff"))
      {
      stream->opt.conversion_flags |= GAVL_AUDIO_FRONT_TO_REAR_DIFF;
      }
    }

  else if(!strcmp(name, "stereo_to_mono"))
    {
    stream->opt.conversion_flags &= ~GAVL_AUDIO_STEREO_TO_MONO_MASK;
                                                                                                        
    if(!strcmp(val->val_str, "Choose left"))
      {
      stream->opt.conversion_flags |= GAVL_AUDIO_STEREO_TO_MONO_LEFT;
      }
    else if(!strcmp(val->val_str, "Choose right"))
      {
      stream->opt.conversion_flags |= GAVL_AUDIO_STEREO_TO_MONO_RIGHT;
      }
    else if(!strcmp(val->val_str, "Mix"))
      {
      stream->opt.conversion_flags |= GAVL_AUDIO_STEREO_TO_MONO_MIX;
      }
    }

  }

typedef struct
  {
  stream_t com;
  
  gavl_video_converter_t * cnv;
  gavl_video_frame_t * in_frame_1;
  gavl_video_frame_t * in_frame_2;
  
  gavl_video_frame_t * out_frame;
  
  gavl_video_options_t opt;

  gavl_video_format_t in_format;
  gavl_video_format_t out_format;

  int convert_framerate;

  int64_t frames_written;
    
  /* Set by set_parameter */

  int fixed_framerate;
  int frame_duration;
  int timescale;

  
  } video_stream_t;

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

  if(!strcmp(name, "conversion_quality"))
    {
    stream->opt.quality = val->val_i;
    }
  else if(!strcmp(name, "fixed_framerate"))
    {
    stream->fixed_framerate = val->val_i;
    }
  else if(!strcmp(name, "frame_duration"))
    {
    stream->frame_duration = val->val_i;
    }
  else if(!strcmp(name, "timescale"))
    {
    stream->timescale = val->val_i;
    }
  }

struct bg_transcoder_s
  {
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

  bg_metadata_t metadata;

  /* General configuration stuff */

  char * output_directory;
  int delete_incomplete;

  /* Timing stuff */

  gavl_timer_t * timer;
  gavl_time_t time;

  /* Error handling */

  char * error_msg;
  const char * error_msg_ret;
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

  gavl_audio_default_options(&(ret->opt));

  /* Create converter */

  ret->cnv = gavl_audio_converter_create();
  
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

  /* Default options */

  gavl_video_default_options(&(ret->opt));

  /* Create converter */

  ret->cnv = gavl_video_converter_create();
  
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
  gavl_audio_format_copy(&(ret->out_format), &(ret->in_format));
    
  /* Adjust format */
  
  if(ret->fixed_samplerate)
    {
    ret->out_format.samplerate = ret->samplerate;
    }
  if(ret->fixed_channel_setup)
    {
    ret->out_format.channel_setup = ret->channel_setup;
    ret->out_format.channel_locations[0] = GAVL_CHID_NONE;
    gavl_set_channel_setup(&(ret->out_format));
    }
  
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

  fprintf(stderr, "Input format:\n");
  gavl_audio_format_dump(&(ret->in_format));
  fprintf(stderr, "Output format:\n");
  gavl_audio_format_dump(&(ret->out_format));
  fprintf(stderr, "\n");
  
  /* Initialize converter */

  ret->com.do_convert = gavl_audio_converter_init(ret->cnv, &(ret->opt),
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

  if(ret->fixed_framerate)
    {
    ret->out_format.frame_duration = ret->frame_duration;
    ret->out_format.timescale = ret->timescale;
    ret->out_format.free_framerate = 0;
    }
  
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

  fprintf(stderr, "Input format:\n");
  gavl_video_format_dump(&(ret->in_format));
  fprintf(stderr, "Output format:\n");
  gavl_video_format_dump(&(ret->out_format));
  fprintf(stderr, "\n");
  
  /* Initialize converter */
  
  ret->com.do_convert = gavl_video_converter_init(ret->cnv, &(ret->opt),
                                                  &(ret->in_format),
                                                  &(ret->out_format));

  /* Check if we wanna convert the framerate */

  if(ret->in_format.free_framerate && !ret->out_format.free_framerate)
    ret->convert_framerate = 1;

  else if((int64_t)ret->in_format.frame_duration * (int64_t)ret->out_format.timescale !=
          (int64_t)ret->out_format.frame_duration * (int64_t)ret->in_format.timescale)
    ret->convert_framerate = 1;
  
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

static void audio_iteration(audio_stream_t*s)
  {
  int result;
  //  fprintf(stderr, "Audio iteration\n");
  /* Get one frame worth of input data */

  result = s->com.in_plugin->read_audio_samples(s->com.in_handle->priv,
                                                s->in_frame,
                                                s->com.in_index,
                                                s->in_format.samples_per_frame);
  if(!result)
    {
    s->com.status = STREAM_STATE_FINISHED;
    return;
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
  //  fprintf(stderr, "Audio iteration 1 %d\n", s->in_format.samples_per_frame);
  }

#define SWAP_FRAMES \
  tmp_frame=s->in_frame_1;\
  s->in_frame_1=s->in_frame_2;\
  s->in_frame_2=tmp_frame


static void video_iteration(video_stream_t * s)
  {
  gavl_time_t next_time;
  gavl_video_frame_t * tmp_frame;
  int result;

  //  fprintf(stderr, "Video iteration\n");
  
  if(s->convert_framerate)
    {
    next_time = gavl_frames_to_time(s->out_format.timescale,
                                    s->out_format.frame_duration,
                                    s->frames_written);


    if(s->in_frame_1->time == GAVL_TIME_UNDEFINED)
      {
      /* Decode initial 2 frame(s) */
      result = s->com.in_plugin->read_video_frame(s->com.in_handle->priv,
                                                  s->in_frame_1,
                                                  s->com.in_index);
      if(!result)
        {
        s->com.status = STREAM_STATE_FINISHED;
        return;
        }

      result = s->com.in_plugin->read_video_frame(s->com.in_handle->priv,
                                                  s->in_frame_2,
                                                  s->com.in_index);
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
      result = s->com.in_plugin->read_video_frame(s->com.in_handle->priv,
                                                  s->in_frame_1,
                                                  s->com.in_index);
      
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
    result = s->com.in_plugin->read_video_frame(s->com.in_handle->priv,
                                                s->in_frame_1,
                                                s->com.in_index);
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
    }

static void
set_parameter_general(void * data, char * name, bg_parameter_value_t * val)
  {
  bg_transcoder_t * t;
  t = (bg_transcoder_t *)data;

  if(!name)
    return;

  SP_STR(name);
  SP_STR(location);
  SP_STR(plugin);
  SP_STR(audio_encoder);
  SP_STR(video_encoder);
  SP_INT(track);

  }

#undef SP_INT
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
  int separate_streams;
  int do_close;
  int num_audio_streams, num_video_streams;

  char * audio_encoder_name = (char*)0;
  char * video_encoder_name = (char*)0;
  
  char * output_filename = (char*)0;
  
  bg_plugin_handle_t  * audio_encoder_handle = (bg_plugin_handle_t  *)0;
  bg_encoder_plugin_t * audio_encoder_plugin;
  
  bg_plugin_handle_t  * video_encoder_handle = (bg_plugin_handle_t  *)0;
  bg_encoder_plugin_t * video_encoder_plugin;

  bg_plugin_handle_t  * encoder_handle = (bg_plugin_handle_t  *)0;
  bg_encoder_plugin_t * encoder_plugin;
  
  const bg_plugin_info_t * plugin_info;

  fprintf(stderr, "Setting global parameters...");
  
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

  fprintf(stderr, "done\n");
    
  fprintf(stderr, "Opening input...");

  /* Open input plugin */

  plugin_info = bg_plugin_find_by_name(plugin_reg, ret->plugin);
  if(!plugin_info)
    {
    fprintf(stderr, "Cannot find plugin %s\n", ret->plugin);
    goto fail;
    }

  ret->in_handle = bg_plugin_load(plugin_reg, plugin_info);
  if(!ret->in_handle)
    {
    fprintf(stderr, "Cannot open plugin %s\n", ret->plugin);
    goto fail;
    }

  ret->in_plugin = (bg_input_plugin_t*)(ret->in_handle->plugin);

  if(!ret->in_plugin->open(ret->in_handle->priv, ret->location))
    {
    fprintf(stderr, "Cannot open %s with plugin %s\n",
            ret->location, ret->plugin);
    goto fail;
    }

  fprintf(stderr, "done\n");

  
  /* Select track and get track info */

  fprintf(stderr, "Selecting track...");
    
  if(ret->in_plugin->get_num_tracks &&
     (ret->track >= ret->in_plugin->get_num_tracks(ret->in_handle->priv)))
    {
    fprintf(stderr, "Invalid track number %d\n", ret->track);
    goto fail;
    }

  ret->track_info = ret->in_plugin->get_track_info(ret->in_handle->priv,
                                                   ret->track);

  if(ret->in_plugin->set_track)
    {
    ret->in_plugin->set_track(ret->in_handle->priv, ret->track);
    }
  
  fprintf(stderr, "done\n");

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
    fprintf(stderr, "Preparing audio stream %d...", i);

    prepare_audio_stream(&(ret->audio_streams[i]),
                         &(track->audio_streams[i]),
                         ret->in_handle, i);
    if(ret->audio_streams[i].com.action == STREAM_ACTION_TRANSCODE)
      num_audio_streams++;
    fprintf(stderr, "done\n");
    }

  num_video_streams = 0;
  
  for(i = 0; i < ret->num_video_streams; i++)
    {
    fprintf(stderr, "Preparing video stream %d...", i);
    prepare_video_stream(&(ret->video_streams[i]),
                         &(track->video_streams[i]),
                         ret->in_handle, i);
    if(ret->video_streams[i].com.action == STREAM_ACTION_TRANSCODE)
      num_video_streams++;
    fprintf(stderr, "done\n");
    }
  
  /* Start input plugin */

  if(ret->in_plugin->start)
    ret->in_plugin->start(ret->in_handle->priv);

  /* Check for the encoding plugins */

  fprintf(stderr, "Checking for mode...");

  audio_encoder_name = bg_transcoder_track_get_audio_encoder(track);
  video_encoder_name = bg_transcoder_track_get_video_encoder(track);

  if(!strcmp(audio_encoder_name, video_encoder_name))
    audio_to_video = 1;
  else
    audio_to_video = 0;

  separate_streams = 0;
  if(!audio_to_video && num_audio_streams && num_video_streams)
    separate_streams = 1;
  
  /* Load audio and video plugins so we can check for the maximum
     stream numbers */

  if(!separate_streams)
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
          separate_streams = 1;
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
          separate_streams = 1;
        
        }
      }
    }

  fprintf(stderr, "done, separate_streams: %d, audio_to_video: %d\n",
          separate_streams, audio_to_video);
  
  if(!separate_streams)
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
        separate_streams = 1;
      }
    }

  /* Finalize the streams */
    
  if(separate_streams)
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

      output_filename =
        bg_sprintf("%s/%s_audio_%02d%s", ret->output_directory, ret->name, i+1,
                   encoder_plugin->get_extension(encoder_handle->priv));

      if(!strcmp(output_filename, ret->location))
        {
        ret->error_msg = bg_sprintf("Input and output are the same file");
        ret->error_msg_ret = ret->error_msg;
        free(output_filename);
        bg_plugin_unref(encoder_handle);
        goto fail;
        }
      
      encoder_plugin->open(encoder_handle->priv, output_filename, &(ret->metadata));
      free(output_filename);
      
      finalize_audio_stream(&(ret->audio_streams[i]), &(track->audio_streams[i]),
                            encoder_handle, 0, ret->track_info, 1);

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
      
      output_filename =
        bg_sprintf("%s/%s_video_%02d%s", ret->output_directory, ret->name, i+1,
                   video_encoder_plugin->get_extension(video_encoder_handle->priv));

      if(!strcmp(output_filename, ret->location))
        {
        ret->error_msg = bg_sprintf("Input and output are the same file");
        ret->error_msg_ret = ret->error_msg;
        free(output_filename);
        bg_plugin_unref(video_encoder_handle);
        goto fail;
        }
      
      video_encoder_plugin->open(video_encoder_handle->priv, output_filename, &(ret->metadata));
      free(output_filename);

      finalize_video_stream(&(ret->video_streams[i]), &(track->video_streams[i]),
                            video_encoder_handle, 0, ret->track_info, 1);
      
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
    
    output_filename = bg_sprintf("%s/%s%s", ret->output_directory, ret->name,
                                 encoder_plugin->get_extension(encoder_handle->priv));

    if(!strcmp(output_filename, ret->location))
      {
      ret->error_msg = bg_sprintf("Input and output are the same file");
      ret->error_msg_ret = ret->error_msg;
      free(output_filename);
      bg_plugin_unref(encoder_handle);
      goto fail;
      }
    
    encoder_plugin->open(encoder_handle->priv, output_filename, &(ret->metadata));
    free(output_filename);
    
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

    encoder_plugin = video_encoder_plugin;
    encoder_handle = video_encoder_handle;

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
    bg_plugin_unref(encoder_handle);
    }
  
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
  stream_t * stream;

  gavl_time_t real_time;
  double real_seconds;

  double remaining_seconds;
  
  int done = 1;
    
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
    return 0;
    }
  
  /* Do the actual transcoding */

  if(stream->type == STREAM_TYPE_AUDIO)
    audio_iteration((audio_stream_t*)stream);
  else if(stream->type == STREAM_TYPE_VIDEO)
    video_iteration((video_stream_t*)stream);

  if(stream->time > t->time)
    t->time = stream->time;
  
  /* Update status */

  real_time = gavl_timer_get(t->timer);
  real_seconds = gavl_time_to_seconds(real_time);

  t->transcoder_info.percentage_done =
    gavl_time_to_seconds(t->time) /
    gavl_time_to_seconds(t->track_info->duration);

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
  
  return 1;
  }

/* Cleanup streams */

static void cleanup_stream(stream_t * s, int do_delete)
  {
  if(s->action == STREAM_ACTION_FORGET)
    return;
      
  if((s->do_close) && (s->out_handle))
    {
    s->out_plugin->close(s->out_handle->priv, do_delete);
    }
  if(s->out_handle)
    bg_plugin_unref(s->out_handle);
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

#define FREE_STR(str) if(str)free(str);

void bg_transcoder_destroy(bg_transcoder_t * t)
  {
  int i;
  int do_delete;
  do_delete =
    ((t->state == TRANSCODER_STATE_RUNNING) && t->delete_incomplete) ? 1 : 0;

  fprintf(stderr, "Do delete: %d\n", do_delete);
    
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

  t->in_plugin->close(t->in_handle->priv);
  bg_plugin_unref(t->in_handle);

  /* Free rest */

  FREE_STR(t->name);
  FREE_STR(t->location);
  FREE_STR(t->plugin);
  FREE_STR(t->audio_encoder);
  FREE_STR(t->video_encoder);
  FREE_STR(t->output_directory);

  gavl_timer_destroy(t->timer);
  if(t->error_msg)
    free(t->error_msg);


  
  free(t);
  }

const char * bg_transcoder_get_error(bg_transcoder_t * t)
  {
  return t->error_msg_ret;
  }

