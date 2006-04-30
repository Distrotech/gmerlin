/*****************************************************************

  e_yuv4mpeg.c

  Copyright (c) 2005 by Burkhard Plaum - plaum@ipf.uni-stuttgart.de

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

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <gmerlin/plugin.h>
#include <gmerlin/utils.h>
#include <gmerlin_encoders.h>
#include <yuv4mpeg.h>
#include "y4m_common.h"

typedef struct
  {
  char * error_msg;
  bg_y4m_common_t com;
  char * filename;
  } e_y4m_t;

static void * create_y4m()
  {
  e_y4m_t * ret = calloc(1, sizeof(*ret));
  return ret;
  }

static const char * get_error_y4m(void * priv)
  {
  e_y4m_t * y4m;
  y4m = (e_y4m_t*)priv;
  return y4m->error_msg;
  }

static const char * extension_y4m  = ".y4m";

static const char * get_extension_y4m(void * data)
  {
  //  e_y4m_t * e = (e_y4m_t*)data;
  return extension_y4m;
  }



static int open_y4m(void * data, const char * filename,
                    bg_metadata_t * metadata)
  {
  e_y4m_t * e = (e_y4m_t*)data;
  e->com.fd = open(filename, O_WRONLY | O_CREAT);
  if(e->com.fd == -1)
    return 0;

  /* Copy filename for later reusal */
  e->filename = bg_strdup(e->filename, filename);
  
  return 1;
  }

static int add_video_stream_y4m(void * data, gavl_video_format_t* format)
  {
  e_y4m_t * e = (e_y4m_t*)data;
  gavl_video_format_copy(&(e->com.format), format);
  return 0;
  }

static void get_video_format_y4m(void * data, int stream,
                                 gavl_video_format_t * ret)
  {
  e_y4m_t * e = (e_y4m_t*)data;

  gavl_video_format_copy(ret, &(e->com.format));
  }

static int start_y4m(void * data)
  {
  int result;
  e_y4m_t * e = (e_y4m_t*)data;
  result = bg_y4m_write_header(&e->com);
  return result;
  }


static void write_video_frame_y4m(void * data,
                                  gavl_video_frame_t* frame,
                                  int stream)
  {
  e_y4m_t * e = (e_y4m_t*)data;
  bg_y4m_write_frame(&(e->com), frame);
  }

static void close_y4m(void * data, int do_delete)
  {
  e_y4m_t * e = (e_y4m_t*)data;
  close(e->com.fd);
  if(do_delete)
    remove(e->filename);
  }

static void destroy_y4m(void * data)
  {
  e_y4m_t * e = (e_y4m_t*)data;
  bg_y4m_cleanup(&e->com);

  if(e->error_msg)
    free(e->error_msg);
  if(e->filename)
    free(e->filename);
  
  free(e);
  }

/* Global parameters */

#if 0
static bg_parmeter_info_t video_parameters[] =
  {
    {
      
    }
  };


static bg_parameter_info_t * get_parameters_y4m(void * data)
  {
  return parameters;
  }

static void set_parameter_y4m(void * data, char * name,
                              bg_parameter_value_t * val)
  {

  }
#endif

/* Per stream parameters */

static bg_parameter_info_t video_parameters[] =
  {
    {
      name:        "chroma_mode",
      long_name:   "Chroma mode",
      type:        BG_PARAMETER_STRINGLIST,
      val_default: { val_str: "auto" },
      multi_names: (char*[]){ "auto",
                              "420jpeg",
                              "420mpeg2",
                              "420paldv",
                              "444",
                              "422",
                              "411",
                              "mono",
                              "yuva4444",
                              (char*)0 },
      multi_labels: (char*[]){ "Auto",
                               "4:2:0 (MPEG-1/JPEG)",
                               "4:2:0 (MPEG-2)",
                               "4:2:0 (PAL DV)",
                               "4:4:4",
                               "4:2:2",
                               "4:1:1",
                               "Greyscale",
                               "4:4:4:4 (YUVA)",
                               (char*)0 },
      help_string: "Set the chroma mode of the output file. Auto means to take the format most similar to the source."
    },
    { /* End of parameters */ }
  };

static bg_parameter_info_t * get_video_parameters_y4m(void * data)
  {
  return video_parameters;
  }

#define SET_ENUM(str, dst, v) if(!strcmp(val->val_str, str)) dst = v

static void set_video_parameter_y4m(void * data, int stream, char * name,
                                    bg_parameter_value_t * val)
  {
  int sub_h, sub_v;
  e_y4m_t * e = (e_y4m_t*)data;
  if(!name)
    {
    /* Detect chroma mode from input format */
    if(e->com.chroma_mode == -1)
      {
      if(gavl_pixelformat_has_alpha(e->com.format.pixelformat))
        {
        e->com.chroma_mode = Y4M_CHROMA_444ALPHA;
        }
      else
        {
        gavl_pixelformat_chroma_sub(e->com.format.pixelformat, &sub_h, &sub_v);
        /* 4:2:2 */
        if((sub_h == 2) && (sub_v == 1))
          e->com.chroma_mode = Y4M_CHROMA_422;
        
        /* 4:1:1 */
        else if((sub_h == 4) && (sub_v == 1))
          e->com.chroma_mode = Y4M_CHROMA_411;
        
        /* 4:2:0 */
        else if((sub_h == 2) && (sub_v == 2))
          {
          switch(e->com.format.chroma_placement)
            {
            case GAVL_CHROMA_PLACEMENT_DEFAULT:
              e->com.chroma_mode = Y4M_CHROMA_420JPEG;
              break;
            case GAVL_CHROMA_PLACEMENT_MPEG2:
              e->com.chroma_mode = Y4M_CHROMA_420MPEG2;
              break;
            case GAVL_CHROMA_PLACEMENT_DVPAL:
              e->com.chroma_mode = Y4M_CHROMA_420PALDV;
              break;
            }
          }
        else
          e->com.chroma_mode = Y4M_CHROMA_444;
        }
      }
    bg_y4m_set_pixelformat(&e->com);
    return;
    }
  
  if(!strcmp(name, "chroma_mode"))
    {
    SET_ENUM("auto",     e->com.chroma_mode, -1);
    SET_ENUM("420jpeg",  e->com.chroma_mode, Y4M_CHROMA_420JPEG);
    SET_ENUM("420mpeg2", e->com.chroma_mode, Y4M_CHROMA_420MPEG2);
    SET_ENUM("420paldv", e->com.chroma_mode, Y4M_CHROMA_420PALDV);
    SET_ENUM("444",      e->com.chroma_mode, Y4M_CHROMA_444);
    SET_ENUM("422",      e->com.chroma_mode, Y4M_CHROMA_422);
    SET_ENUM("411",      e->com.chroma_mode, Y4M_CHROMA_411);
    SET_ENUM("mono",     e->com.chroma_mode, Y4M_CHROMA_MONO);
    SET_ENUM("yuva4444", e->com.chroma_mode, Y4M_CHROMA_444ALPHA);
    }
    
  }

#undef SET_ENUM

bg_encoder_plugin_t the_plugin =
  {
    common:
    {
      name:           "e_y4m",       /* Unique short name */
      long_name:      "yuv4mpeg2 encoder",
      mimetypes:      NULL,
      extensions:     "y4m",
      type:           BG_PLUGIN_ENCODER_VIDEO,
      flags:          BG_PLUGIN_FILE,
      priority:       BG_PLUGIN_PRIORITY_MAX,
      create:         create_y4m,
      destroy:        destroy_y4m,
      //      get_parameters: get_parameters_y4m,
      //      set_parameter:  set_parameter_y4m,
      get_error:      get_error_y4m,
    },

    max_audio_streams:  0,
    max_video_streams:  1,

    get_video_parameters: get_video_parameters_y4m,

    get_extension:        get_extension_y4m,

    open:                 open_y4m,

    add_video_stream:     add_video_stream_y4m,

    set_video_parameter:  set_video_parameter_y4m,

    get_video_format:     get_video_format_y4m,

    start:                start_y4m,

    write_video_frame: write_video_frame_y4m,
    close:             close_y4m,
    
  };

/* Include this into all plugin modules exactly once
   to let the plugin loader obtain the API version */
BG_GET_PLUGIN_API_VERSION;
