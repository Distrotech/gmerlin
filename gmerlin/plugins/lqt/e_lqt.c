#include <string.h>
#include <plugin.h>
#include <utils.h>

#include "lqt_common.h"

#define PARAM_AUDIO 0
#define PARAM_VIDEO 1

static bg_parameter_info_t parameters[] = 
  {
    {
      name:      "audio_codec",
      long_name: "Audio Codec",
    },
    {
      name:      "video_codec",
      long_name: "Video Codec",
    },
    { /* End of parameters */ }
  };

typedef struct
  {
  quicktime_t * file;
  bg_parameter_info_t * parameters;

  char * audio_codec_name;
  char * video_codec_name;
    
  } e_lqt_t;

static void * create_lqt()
  {
  e_lqt_t * ret = calloc(1, sizeof(*ret));
  return ret;
  }

static int open_lqt(void * data, const char * filename_base,
                    bg_metadata_t * metadata)
  {
  //  e_lqt_t * e = (e_lqt_t*)data;
  return 0;
  }

static void add_audio_stream_lqt(void * data, bg_audio_info_t * info)
  {
  //  e_lqt_t * e = (e_lqt_t*)data;
  }

static void add_video_stream_lqt(void * data, bg_video_info_t* info)
  {
  //  e_lqt_t * e = (e_lqt_t*)data;
  }

static void write_audio_frame_lqt(void * data, gavl_audio_frame_t* frame,
                                  int stream)
  {
  //  e_lqt_t * e = (e_lqt_t*)data;

  }

static void write_video_frame_lqt(void * data, gavl_video_frame_t* frame,
                                  int stream)
  {
  //  e_lqt_t * e = (e_lqt_t*)data;
  }

static void close_lqt(void * data)
  {
  //  e_lqt_t * e = (e_lqt_t*)data;
  }

static void destroy_lqt(void * data)
  {
  e_lqt_t * e = (e_lqt_t*)data;

  close_lqt(data);
    
  if(e->parameters)
    bg_parameter_info_destroy_array(e->parameters);
  free(e);
  }

static void create_parameters(e_lqt_t * e)
  {
  e->parameters = calloc(3, sizeof(bg_parameter_info_t));
  bg_parameter_info_copy(&(e->parameters[PARAM_AUDIO]), &(parameters[PARAM_AUDIO]));
  bg_parameter_info_copy(&(e->parameters[PARAM_VIDEO]), &(parameters[PARAM_VIDEO]));

  bg_lqt_create_codec_info(&(e->parameters[PARAM_AUDIO]),
                           1, 0, 1, 0);
  bg_lqt_create_codec_info(&(e->parameters[PARAM_VIDEO]),
                           0, 1, 1, 0);
  
  }

static bg_parameter_info_t * get_parameters_lqt(void * data)
  {
  e_lqt_t * e = (e_lqt_t*)data;
  
  if(!e->parameters)
    create_parameters(e);
  
  return e->parameters;
  }

static void set_parameter_lqt(void * data, char * name,
                              bg_parameter_value_t * val)
  {
  e_lqt_t * e = (e_lqt_t*)data;

  if(!name)
    return;

  if(!e->parameters)
    create_parameters(e);

  if(bg_lqt_set_parameter(name, val, &(e->parameters[PARAM_AUDIO])) ||
     bg_lqt_set_parameter(name, val, &(e->parameters[PARAM_VIDEO])))
    return;

  if(!strcmp(name, "audio_codec"))
    {
    e->audio_codec_name = bg_strdup(e->audio_codec_name, val->val_str);
    }
  else if(!strcmp(name, "video_codec"))
    {
    e->video_codec_name = bg_strdup(e->video_codec_name, val->val_str);
    }
  
  }

bg_encoder_plugin_t the_plugin =
  {
    common:
    {
      name:           "e_lqt",       /* Unique short name */
      long_name:      "Quicktime encoder",
      mimetypes:      NULL,
      extensions:     "mov",
      type:           BG_PLUGIN_ENCODER,
      flags:          BG_PLUGIN_FILE,
      create:         create_lqt,
      destroy:        destroy_lqt,
      get_parameters: get_parameters_lqt,
      set_parameter:  set_parameter_lqt,
    },

    open:              open_lqt,
    add_audio_stream:  add_audio_stream_lqt,
    add_video_stream:  add_video_stream_lqt,
    

    write_audio_frame: write_audio_frame_lqt,
    write_video_frame: write_video_frame_lqt,
    close:             close_lqt,
    
  };
