
#include <plugin.h>
#include <utils.h>
#include <subprocess.h>


typedef struct
  {
  bg_subprocess_t * proc;  
  bg_track_info_t track_info;
  } i_mikmod_t;


static void * create_mikmod()
  {
  i_mikmod_t * ret = calloc(1, sizeof(*ret));
  return ret;
  }

static int open_mikmod(void * data, const char * arg)
  {
  //  i_mikmod_t * e = (i_mikmod_t*)data;
  return 0;
  }

static int get_num_tracks_mikmod(void * data)
  {
  return 1;
  }

static bg_track_info_t * get_track_info_mikmod(void * data, int track)
  {
  i_mikmod_t * e = (i_mikmod_t*)data;
  return &(e->track_info);
  }

/* Read one audio frame (returns FALSE on EOF) */
static  
int read_audio_samples_mikmod(void * data, gavl_audio_frame_t * f, int stream,
                          int num_samples)
  {
  //  i_mikmod_t * e = (i_mikmod_t*)data;

  //  mikmod_gavl_decode_audio(e->file, e->audio_streams[stream].quicktime_index,
  //                        f, num_samples);
  //  fprintf(stderr, "read %d samples\n", f->valid_samples);
  return f->valid_samples;
  }

static void close_mikmod(void * data)
  {
  //  i_mikmod_t * e = (i_mikmod_t*)data;
  }

static void destroy_mikmod(void * data)
  {
  }

/* Configuration */

static bg_parameter_info_t parameters[] = 
  {
    { /* End of parameters */ }
  };

static bg_parameter_info_t * get_parameters_mikmod(void * data)
  {
  return parameters;
  }

static void set_parameter_mikmod(void * data, char * name,
                              bg_parameter_value_t * val)
  {

  }

bg_input_plugin_t the_plugin =
  {
    common:
    {
      name:            "i_mikmod",       /* Unique short name */
      long_name:       "mikmod input plugin",
      mimetypes:       NULL,
      extensions:      "",
      type:            BG_PLUGIN_INPUT,
      flags:           BG_PLUGIN_FILE,
      priority:        5,
      create:          create_mikmod,
      destroy:         destroy_mikmod,
      get_parameters:  get_parameters_mikmod,
      set_parameter:   set_parameter_mikmod,
    },
    
    open:              open_mikmod,
    get_num_tracks:    get_num_tracks_mikmod,
    get_track_info:    get_track_info_mikmod,
    
    read_audio_samples: read_audio_samples_mikmod,
    close:              close_mikmod
    
  };

/* Include this into all plugin modules exactly once
   to let the plugin loader obtain the API version */
BG_GET_PLUGIN_API_VERSION;
