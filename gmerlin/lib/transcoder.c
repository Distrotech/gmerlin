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

#define STREAM_STATUS_OFF      0
#define STREAM_STATUS_ON       1
#define STREAM_STATUS_FINISHED 2

#define STREAM_AUDIO           0
#define STREAM_VIDEO           1

#define STREAM_ACTION_TRANSCODE 0
#define STREAM_ACTION_FORGET    1

typedef struct
  {
  int status;
  int type;

  int action;
    
  int input_index;
  int output_index;
  gavl_time_t time;

  bg_plugin_handle_t * input_handle;
  bg_input_plugin_t  * input_plugin;

  bg_plugin_handle_t  * output_handle;
  bg_encoder_plugin_t * output_plugin;
  int do_convert;
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

  gavl_audio_options_t opt;
  
  /* Set by set_parameter */

  int fixed_samplerate;
  int samplerate;
  
  int fixed_channel_setup;
  gavl_channel_setup_t channel_setup;
  
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
  gavl_video_frame_t * in_frame;
  gavl_video_frame_t * out_frame;
  
  gavl_video_options_t opt;

  /* Set by set_parameter */

  int fixed_framerate;
  int frame_duration;
  int timescale;
  
  } video_stream_t;

static void set_video_parameter_general(void * data, char * name, bg_parameter_value_t * val)
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
  
  bg_transcoder_status_t status;
  
  bg_plugin_handle_t * input_handle;
  bg_input_plugin_t  * input_plugin;
  bg_track_info_t * track_info;

  /* Set by set_parameter */

  char * name;
  char * location;
  char * plugin;
  char * audio_encoder;
  char * video_encoder;
  
  int track;

  bg_metadata_t metadata;
  
  };

static void prepare_audio_stream(audio_stream_t * ret, bg_transcoder_track_audio_t * s,
                                 bg_plugin_handle_t * input_handle,
                                 int input_index)
  {
  ret->com.input_handle = input_handle;
  ret->com.input_plugin = (bg_input_plugin_t*)(input_handle->plugin);

  /* Default options */

  gavl_audio_default_options(&(ret->opt));
    
  /* Apply parameters */

  bg_cfg_section_apply(s->general_section, bg_transcoder_track_audio_get_general_parameters(),
                       set_audio_parameter_general, ret);

  ret->com.input_index = input_index;
  
  /* Set stream */

  if(ret->com.input_plugin->set_audio_stream)
    {
    if(ret->com.action == STREAM_ACTION_TRANSCODE)
      ret->com.input_plugin->set_audio_stream(ret->com.input_handle->priv,
                                              ret->com.input_index, BG_STREAM_ACTION_DECODE);
    else
      ret->com.input_plugin->set_audio_stream(ret->com.input_handle->priv,
                                     ret->com.input_index, BG_STREAM_ACTION_OFF);
    }
  
  }

static void finalize_audio_stream(audio_stream_t * ret, bg_transcoder_track_audio_t * s,
                                  bg_plugin_handle_t * output_handle,
                                  int output_index, bg_track_info_t * track_info)
  {
  
  }

static void prepare_video_stream(video_stream_t * ret, bg_transcoder_track_video_t * s,
                                 bg_plugin_handle_t * input_handle,
                                 int input_index)
  {
  ret->com.input_handle = input_handle;
  ret->com.input_plugin = (bg_input_plugin_t*)(input_handle->plugin);

  /* Default options */

  gavl_video_default_options(&(ret->opt));
  
  /* Apply parameters */

  bg_cfg_section_apply(s->general_section, bg_transcoder_track_video_get_general_parameters(),
                       set_video_parameter_general, ret);
  
  ret->com.input_index = input_index;

  /* Set stream */

  if(ret->com.input_plugin->set_video_stream)
    {
    if(ret->com.action == STREAM_ACTION_TRANSCODE)
      ret->com.input_plugin->set_video_stream(ret->com.input_handle->priv,
                                              ret->com.input_index, BG_STREAM_ACTION_DECODE);
    else
      ret->com.input_plugin->set_video_stream(ret->com.input_handle->priv,
                                     ret->com.input_index, BG_STREAM_ACTION_OFF);
    }

  
  }

static void finalize_video_stream(audio_stream_t * ret, bg_transcoder_track_video_t * s,
                                  bg_plugin_handle_t * output_handle,
                                  int output_index, bg_track_info_t * track_info)
  {
  
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

static void set_parameter_general(void * data, char * name, bg_parameter_value_t * val)
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

bg_transcoder_t *
bg_transcoder_create(bg_plugin_registry_t * plugin_reg, bg_transcoder_track_t * track)
  {
  int i;
  bg_plugin_handle_t * input_handle;
  bg_input_plugin_t * input_plugin;

  bg_plugin_handle_t * encoder_handle;
  bg_encoder_plugin_t * encoder_plugin;
  
  const bg_plugin_info_t * plugin_info;
  
  bg_transcoder_t * ret;
  ret = calloc(1, sizeof(*ret));

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

  /* Open input plugin */

  plugin_info = bg_plugin_find_by_name(plugin_reg, ret->plugin);
  if(!plugin_info)
    {
    fprintf(stderr, "Cannot find plugin %s\n", ret->plugin);
    goto fail;
    }

  input_handle = bg_plugin_load(plugin_reg, plugin_info);

  if(!input_handle)
    {
    fprintf(stderr, "Cannot open plugin %s\n", ret->plugin);
    goto fail;
    }

  input_plugin = (bg_input_plugin_t*)(input_handle->plugin);

  if(!input_plugin->open(input_handle->priv, ret->location))
    {
    fprintf(stderr, "Cannot open %s with plugin %s\n", ret->location, ret->plugin);
    goto fail;
    }

  /* Select track and get track info */
    
  if(ret->track >= input_plugin->get_num_tracks(input_handle->priv))
    {
    fprintf(stderr, "Invalid track number %d\n", ret->track);
    goto fail;
    }

  ret->track_info = input_plugin->get_track_info(input_handle->priv, ret->track);

  if(input_plugin->set_track)
    {
    input_plugin->set_track(input_handle->priv, ret->track);
    }

  /* Allocate streams */

  ret->num_audio_streams = ret->track_info->num_audio_streams;
  ret->audio_streams = calloc(ret->num_audio_streams, sizeof(*(ret->audio_streams)));

  ret->num_video_streams = ret->track_info->num_video_streams;
  ret->video_streams = calloc(ret->num_video_streams, sizeof(*(ret->video_streams)));

  /* Prepare streams */

  for(i = 0; i < ret->num_audio_streams; i++)
    {
    prepare_audio_stream(&(ret->audio_streams[i]), &(track->audio_streams[i]),
                         input_handle, i);
    }
  for(i = 0; i < ret->num_video_streams; i++)
    {
    prepare_video_stream(&(ret->video_streams[i]), &(track->video_streams[i]),
                         input_handle, i);
    }
  
  /* Start input plugin */

  if(input_plugin->start)
    input_plugin->start(input_handle->priv);
  
  /* Now, finalize the streams */
  
  for(i = 0; i < ret->num_audio_streams; i++)
    {
    
    }
  for(i = 0; i < ret->num_video_streams; i++)
    {
    
    }
  
  
  return ret;

  fail:
  free(ret);
  return (bg_transcoder_t*)0;
  
  }

const bg_transcoder_status_t * bg_transcoder_get_status(bg_transcoder_t * t)
  {
  return &(t->status);
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

  time = 0;
  
  /* Find the stream with the smallest time */
  
  if(t->audio_streams)
    {
    time = t->audio_streams->com.time;
    stream = &(t->audio_streams->com);
    }
  else if(t->video_streams)
    {
    time = t->video_streams->com.time;
    stream = &(t->video_streams->com);
    }

  for(i = 0; i < t->num_audio_streams; i++)
    {
    if(t->audio_streams[i].com.time < time)
      {
      time = t->audio_streams[i].com.time;
      stream = &(t->audio_streams[i].com);
      }
    }
  for(i = 0; i < t->num_video_streams; i++)
    {
    if(t->video_streams[i].com.time < time)
      {
      time = t->video_streams[i].com.time;
      stream = &(t->video_streams[i].com);
      }
    }

  /* Do the transcoding */

  if(stream->type == STREAM_AUDIO)
    {
        
    }
  
  return 1;
  }
