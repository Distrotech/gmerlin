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

/* Format definitions */

#define FORMAT_QUICKTIME            0
#define FORMAT_QUICKTIME_STREAMABLE 1
#define FORMAT_AVI                  2

static bg_parameter_info_t stream_parameters[] = 
  {
    {
      name:      "codec",
      long_name: "Codec",
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
  
  int format; /* See defines above */

  int num_video_streams;
  int num_audio_streams;
    
  struct
    {
    gavl_audio_format_t format;
    int codec_index;
    } * audio_streams;

  struct
    {
    gavl_video_format_t format;
    uint8_t ** rows;
    int codec_index;
    } * video_streams;
  
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

  if(e->format == FORMAT_AVI)
    e->filename = bg_sprintf("%s.avi", filename_base);
  else if(e->format == FORMAT_QUICKTIME)
    e->filename = bg_sprintf("%s.mov", filename_base);
  else if(e->format == FORMAT_QUICKTIME_STREAMABLE)
    e->filename = bg_sprintf("%s.mov.tmp", filename_base);
  
  e->file = quicktime_open(e->filename, 0, 1);

  /* Set metadata */

  if(metadata->copyright)
    quicktime_set_copyright(e->file, metadata->copyright);
  if(metadata->title)
    quicktime_set_name(e->file, metadata->title);
  if(metadata->comment)
    quicktime_set_info(e->file, metadata->comment);
    
  return 0;
  }

static void add_audio_stream_lqt(void * data, gavl_audio_format_t * format)
  {
  e_lqt_t * e = (e_lqt_t*)data;

  e->audio_streams =
    realloc(e->audio_streams,
            (e->num_audio_streams+1)*sizeof(*(e->audio_streams)));
  memset(&(e->audio_streams[e->num_audio_streams]), 0,
         sizeof(*(e->audio_streams)));
  gavl_audio_format_copy(&(e->audio_streams[e->num_audio_streams].format),
                         format);
  
  e->num_audio_streams++;
  }

static void add_video_stream_lqt(void * data, gavl_video_format_t* format)
  {
  e_lqt_t * e = (e_lqt_t*)data;

  e->video_streams =
    realloc(e->video_streams,
            (e->num_video_streams+1)*sizeof(*(e->video_streams)));
  memset(&(e->video_streams[e->num_video_streams]), 0,
         sizeof(*(e->video_streams)));
  
  gavl_video_format_copy(&(e->video_streams[e->num_video_streams].format),
                         format);
  e->num_video_streams++;
  
  }

static void get_audio_format_lqt(void * data, int stream, gavl_audio_format_t * ret)
  {
  e_lqt_t * e = (e_lqt_t*)data;

  gavl_audio_format_copy(ret, &(e->audio_streams[stream].format));
  
  }
  
static void get_video_format_lqt(void * data, int stream, gavl_video_format_t * ret)
  {
  e_lqt_t * e = (e_lqt_t*)data;
  
  gavl_video_format_copy(ret, &(e->video_streams[stream].format));
  }


static void write_audio_frame_lqt(void * data, gavl_audio_frame_t* frame,
                                  int stream)
  {
  e_lqt_t * e = (e_lqt_t*)data;

  lqt_encode_audio_track(e->file, (int16_t**)0, frame->channels.f, frame->valid_samples,
                         stream);
  
  }

static void write_video_frame_lqt(void * data, gavl_video_frame_t* frame,
                                  int stream)
  {
  int i;
  uint8_t ** rows;
  
  e_lqt_t * e = (e_lqt_t*)data;

  lqt_set_row_span(e->file, stream, frame->strides[0]);
  lqt_set_row_span_uv(e->file, stream, frame->strides[1]);
  
  if(e->video_streams[stream].rows)
    {
    for(i = 0; i < e->video_streams[stream].format.image_height; i++)
      e->video_streams[stream].rows[i] = frame->planes[0] + i * frame->strides[0];
    rows = e->video_streams[stream].rows;
    }
  else
    rows = frame->planes;

  /* TODO: lqt_encode_video */
  
  quicktime_encode_video(e->file, rows, stream);
  }

static void close_lqt(void * data, int do_delete)
  {
  char * filename_final, *pos;
  e_lqt_t * e = (e_lqt_t*)data;

  if(!e->file)
    return;

  fprintf(stderr, "close_lqt\n");
  
  quicktime_close(e->file);
  e->file = (quicktime_t*)0;
  
  if(do_delete)
    remove(e->filename);

  else if(e->format == FORMAT_QUICKTIME_STREAMABLE)
    {
    filename_final = bg_strdup((char*)0, e->filename);
    pos = strrchr(filename_final, '.');
    *pos = '\0';
    fprintf(stderr, "Making streamable....");
    quicktime_make_streamable(e->filename, filename_final);
    fprintf(stderr, "done\n");
    remove(e->filename);
    free(filename_final);
    }
  
  if(e->filename)
    {
    free(e->filename);
    e->filename = (char*)0;
    }
  if(e->audio_streams)
    {
    free(e->audio_streams);
    e->audio_streams = NULL;
    }
  if(e->video_streams)
    {
    free(e->video_streams);
    e->video_streams = NULL;
    }
  e->num_audio_streams = 0;
  e->num_video_streams = 0;
  
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

  bg_parameter_info_copy(&(e->audio_parameters[0]), &(stream_parameters[0]));
  bg_parameter_info_copy(&(e->video_parameters[0]), &(stream_parameters[0]));
  
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
      multi_names:    (char*[]) { "quicktime", "quicktime_s", "avi", (char*)0 },
      multi_labels:   (char*[]) { "Quicktime", "Quicktime (streamable)", "AVI", (char*)0 },
      val_default: { val_str: "quicktime" },
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
  e_lqt_t * e = (e_lqt_t*)data;
  if(!name)
    return;

  else if(!strcmp(name, "format"))
    {
    if(!strcmp(val->val_str, "quicktime"))
      e->format = FORMAT_QUICKTIME;
    else if(!strcmp(val->val_str, "quicktime_s"))
      e->format = FORMAT_QUICKTIME_STREAMABLE;
    else if(!strcmp(val->val_str, "avi"))
      e->format = FORMAT_AVI;
    }
  
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
  e_lqt_t * e = (e_lqt_t*)data;
  lqt_codec_info_t ** info;
    
  if(!name)
    return;

  if(!strcmp(name, "codec"))
    {
    /* Now we can add the stream */

    info = lqt_find_audio_codec_by_name(val->val_str);
    
    lqt_add_audio_track(e->file, e->audio_streams[stream].format.num_channels,
                        e->audio_streams[stream].format.samplerate, 16,
                        *info);

    e->audio_streams[stream].format.interleave_mode = GAVL_INTERLEAVE_NONE;
    e->audio_streams[stream].format.sample_format = GAVL_SAMPLE_FLOAT;
    
    lqt_destroy_codec_info(info);
    }
  else
    {
    fprintf(stderr, "set_audio_parameter_lqt %s\n", name);
    }
  
#if 0  
  if(bg_lqt_set_parameter(name, val, &(e->parameters[PARAM_AUDIO])) ||
     bg_lqt_set_parameter(name, val, &(e->parameters[PARAM_VIDEO])))
    return;
#endif
  }

static void set_video_parameter_lqt(void * data, int stream, char * name,
                                    bg_parameter_value_t * val)
  {
  e_lqt_t * e = (e_lqt_t*)data;
  int quicktime_colormodel;
  
  lqt_codec_info_t ** info;
    
  if(!name)
    return;

  if(!strcmp(name, "codec"))
    {
    /* Now we can add the stream */

    info = lqt_find_video_codec_by_name(val->val_str);
    
    lqt_add_video_track(e->file, e->video_streams[stream].format.image_width,
                        e->video_streams[stream].format.image_height,
                        e->video_streams[stream].format.frame_duration,
                        e->video_streams[stream].format.timescale,
                        *info);
    lqt_destroy_codec_info(info);

    quicktime_colormodel = lqt_get_best_colormodel(e->file, stream,
                                                   bg_lqt_supported_colormodels);
    e->video_streams[stream].format.colorspace =
      bg_lqt_get_gavl_colorspace(quicktime_colormodel);
    lqt_set_cmodel(e->file, stream, quicktime_colormodel);

    if(!gavl_colorspace_is_planar(e->video_streams[stream].format.colorspace))
      e->video_streams[stream].rows =
        malloc(e->video_streams[stream].format.image_height * 
               sizeof(*(e->video_streams[e->num_audio_streams].rows)));
    }
  else
    {
    fprintf(stderr, "set_video_parameter_lqt %s\n", name);
    }
  

#if 0  
  if(bg_lqt_set_parameter(name, val, &(e->parameters[PARAM_AUDIO])) ||
     bg_lqt_set_parameter(name, val, &(e->parameters[PARAM_VIDEO])))
    return;
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
    
    open:                 open_lqt,

    add_audio_stream:     add_audio_stream_lqt,
    add_video_stream:     add_video_stream_lqt,
        
    set_audio_parameter:  set_audio_parameter_lqt,
    set_video_parameter:  set_video_parameter_lqt,

    get_audio_format:     get_audio_format_lqt,
    get_video_format:     get_video_format_lqt,
    
    
    write_audio_frame: write_audio_frame_lqt,
    write_video_frame: write_video_frame_lqt,
    close:             close_lqt,
    
  };
