#include <string.h>
#include <plugin.h>
#include <utils.h>

#include "lqt_common.h"

#define PARAM_AUDIO 1
#define PARAM_VIDEO 3

static bg_parameter_info_t parameters[] = 
  {
    {
      name:      "audio",
      long_name: "Audio",
      type:      BG_PARAMETER_SECTION,
    },
    {
      name:      "audio_codecs",
      long_name: "Audio Codecs",
    },
    {
      name:      "video",
      long_name: "Video",
      type:      BG_PARAMETER_SECTION,
    },
    {
      name:      "video_codecs",
      long_name: "Video Codecs",
    },
    { /* End of parameters */ }
  };

typedef struct
  {
  quicktime_t * file;
  bg_parameter_info_t * parameters;

  char * audio_codec_string;
  char * video_codec_string;
  
  } i_lqt_t;

static void * create_lqt()
  {
  i_lqt_t * ret = calloc(1, sizeof(*ret));
  return ret;
  }

static void close_lqt(void * data)
  {
  //  i_lqt_t * e = (i_lqt_t*)data;
  }

static void destroy_lqt(void * data)
  {
  i_lqt_t * e = (i_lqt_t*)data;
  close_lqt(data);

  if(e->parameters)
    bg_parameter_info_destroy_array(e->parameters);
  
  free(e);
  }

static void create_parameters(i_lqt_t * e)
  {

  e->parameters = bg_parameter_info_copy_array(parameters);
    
  bg_lqt_create_codec_info(&(e->parameters[PARAM_AUDIO]),
                           1, 0, 0, 1);
  bg_lqt_create_codec_info(&(e->parameters[PARAM_VIDEO]),
                           0, 1, 0, 1);

  
  }

static bg_parameter_info_t * get_parameters_lqt(void * data)
  {
  i_lqt_t * e = (i_lqt_t*)data;
  
  if(!e->parameters)
    create_parameters(e);
  
  return e->parameters;
  }

static void set_parameter_lqt(void * data, char * name,
                              bg_parameter_value_t * val)
  {
  i_lqt_t * e = (i_lqt_t*)data;

  if(!name)
    return;

  if(!e->parameters)
    create_parameters(e);

  if(bg_lqt_set_parameter(name, val, &(e->parameters[PARAM_AUDIO])) ||
     bg_lqt_set_parameter(name, val, &(e->parameters[PARAM_VIDEO])))
    return;

  if(!strcmp(name, "audio_codecs"))
    {
    e->audio_codec_string = bg_strdup(e->audio_codec_string, val->val_str);
    }
  else if(!strcmp(name, "video_codecs"))
    {
    e->video_codec_string = bg_strdup(e->video_codec_string, val->val_str);
    }
  
  }

bg_input_plugin_t the_plugin =
  {
    common:
    {
      name:            "i_lqt",       /* Unique short name */
      long_name:       "Quicktime input plugin",
      mimetypes:       NULL,
      extensions:      "mov",
      type:            BG_PLUGIN_INPUT,
      flags:           BG_PLUGIN_FILE,
      create:          create_lqt,
      destroy:         destroy_lqt,
      get_parameters:  get_parameters_lqt,
      set_parameter:   set_parameter_lqt,
    },

    //    open:              open_lqt,
    //    get_track_info:    get_track_info,
    //    set_audio_stream:  set_audio_stream_lqt,
    //    set_video_stream:  set_audio_stream_lqt,
    //    start:             start_lqt,

    //    read_audio_samples: read_audio_samples_lqt,
    //    read_video_frame:   read_video_frame_lqt,
    //    seek:               seek_lqt,
    //    stop:               stop_lqt,
    close:              close_lqt
    
  };
