/*****************************************************************
 
  e_lqt.c
 
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

#include <string.h>
#include <plugin.h>
#include <utils.h>

#include "lqt_common.h"

#define PARAM_AUDIO 0
#define PARAM_VIDEO 1

static bg_parameter_info_t audio_parameters[] = 
  {
    {
      name:      "audio_codec",
      long_name: "Audio Codec",
    },
  };


static bg_parameter_info_t video_parameters[] = 
  {
    {
      name:      "video_codec",
      long_name: "Video Codec",
    },
  };

typedef struct
  {
  char * filename;
  quicktime_t * file;
  //  bg_parameter_info_t * parameters;

  char * audio_codec_name;
  char * video_codec_name;

  bg_parameter_info_t * audio_parameters;
  bg_parameter_info_t * video_parameters;
  
  
  } e_lqt_t;

static void * create_lqt()
  {
  e_lqt_t * ret = calloc(1, sizeof(*ret));
  return ret;
  }

static int open_lqt(void * data, const char * filename_base,
                    bg_metadata_t * metadata)
  {
  e_lqt_t * e = (e_lqt_t*)data;
  e->filename = bg_sprintf("%s.mov", filename_base);
  
  return 0;
  }

#if 0
static void add_audio_stream_lqt(void * data, bg_audio_info_t * info)
  {
  //  e_lqt_t * e = (e_lqt_t*)data;
  }

static void add_video_stream_lqt(void * data, bg_video_info_t* info)
  {
  //  e_lqt_t * e = (e_lqt_t*)data;
  }
#endif

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

static void close_lqt(void * data, int do_delete)
  {
  e_lqt_t * e = (e_lqt_t*)data;

  if(!e->file)
    return;
  quicktime_close(e->file);
  e->file = (quicktime_t*)0;
  
  if(do_delete)
    remove(e->filename);
  if(e->filename)
    free(e->filename);
  }

static void destroy_lqt(void * data)
  {
  e_lqt_t * e = (e_lqt_t*)data;

  close_lqt(data, 1);
  
  if(e->audio_parameters)
    bg_parameter_info_destroy_array(e->audio_parameters);
  if(e->video_parameters)
    bg_parameter_info_destroy_array(e->video_parameters);
  free(e);
  }

static void create_parameters(e_lqt_t * e)
  {
  e->audio_parameters = calloc(2, sizeof(*(e->audio_parameters)));
  e->video_parameters = calloc(2, sizeof(*(e->video_parameters)));

  bg_parameter_info_copy(&(e->audio_parameters[0]), &(audio_parameters[0]));
  bg_parameter_info_copy(&(e->video_parameters[0]), &(video_parameters[0]));
  
  bg_lqt_create_codec_info(&(e->audio_parameters[0]),
                           1, 0, 1, 0);
  bg_lqt_create_codec_info(&(e->video_parameters[0]),
                           0, 1, 1, 0);
  
  }

static bg_parameter_info_t common_parameters[] =
  {
    {
      name:      "format",
      long_name: "Format",
      type:      BG_PARAMETER_STRINGLIST,
      multi_names:   (char*[]) { "Quicktime", "Quicktime (streamable)", "AVI", (char*)0 },
      val_default: { val_str: "Quicktime" },
    },
    { /* End of parameters */ }
  };

static bg_parameter_info_t * get_parameters_lqt(void * data)
  {
  return common_parameters;
  }

static void set_parameter_lqt(void * data, char * name,
                              bg_parameter_value_t * val)
  {
  
  }

static bg_parameter_info_t * get_audio_parameters_lqt(void * data)
  {
  e_lqt_t * e = (e_lqt_t*)data;
  
  if(!e->audio_parameters)
    create_parameters(e);
  
  return e->audio_parameters;
  }

static bg_parameter_info_t * get_video_parameters_lqt(void * data)
  {
  e_lqt_t * e = (e_lqt_t*)data;
  
  if(!e->video_parameters)
    create_parameters(e);
  
  return e->video_parameters;
  }

static void set_audio_parameter_lqt(void * data, int stream, char * name,
                                    bg_parameter_value_t * val)
  {
#if 0
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
#endif
  }

static void set_video_parameter_lqt(void * data, int stream, char * name,
                                    bg_parameter_value_t * val)
  {
#if 0
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
#endif
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

    max_audio_streams: -1,
    max_video_streams: -1,

    get_audio_parameters: get_audio_parameters_lqt,
    get_video_parameters: get_video_parameters_lqt,

    set_audio_parameter:  set_audio_parameter_lqt,
    set_video_parameter:  set_video_parameter_lqt,
    
    open:              open_lqt,
    
    write_audio_frame: write_audio_frame_lqt,
    write_video_frame: write_video_frame_lqt,
    close:             close_lqt,
    
  };
