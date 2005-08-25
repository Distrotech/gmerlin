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

#include <gmerlin/plugin.h>
#include <gmerlin/utils.h>
#include <gmerlin_encoders.h>
#include <yuv4mpeg.h>

typedef struct
  {
  gavl_video_format_t format;
  char * filename;
  int fd;
  FILE * file;

  char * error_msg;
  uint8_t * tmp_planes[4];

  /* For checking if an incoming frame can be read without memcpying */
  int strides[4]; 
  
  int chroma_mode;  

  y4m_stream_info_t si;
  y4m_frame_info_t fi;
  
  y4m_cb_writer_t writer;

  gavl_video_frame_t * frame;
  
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

static ssize_t write_func(void * data, const void *buf, size_t len)
  {
  size_t result;
  result = fwrite(buf, 1, len, (FILE*)(data));
  
  if(result != len)
    return -len;
  return 0;
  }


static int open_y4m(void * data, const char * filename,
                    bg_metadata_t * metadata)
  {
  e_y4m_t * e = (e_y4m_t*)data;
  e->file = fopen(filename, "w");
  if(!e->file)
    return 0;
  /* Set up writer */
  e->writer.data  = e->file;
  e->writer.write = write_func;
  return 1;
  }

static void add_video_stream_y4m(void * data, gavl_video_format_t* format)
  {
  e_y4m_t * e = (e_y4m_t*)data;
  gavl_video_format_copy(&(e->format), format);
  }

static void get_video_format_y4m(void * data, int stream,
                                 gavl_video_format_t * ret)
  {
  e_y4m_t * e = (e_y4m_t*)data;

  gavl_video_format_copy(ret, &(e->format));
  }

static int start_y4m(void * data)
  {
  int i;
  y4m_ratio_t r;
  e_y4m_t * e = (e_y4m_t*)data;

  /* Set up the stream- and frame header */
  y4m_init_stream_info(&(e->si));
  y4m_init_frame_info(&(e->fi));

  y4m_si_set_width(&(e->si), e->format.image_width);
  y4m_si_set_height(&(e->si), e->format.image_height);

  switch(e->format.interlace_mode)
    {
    case GAVL_INTERLACE_TOP_FIRST:
      i = Y4M_ILACE_TOP_FIRST;
      break;
    case GAVL_INTERLACE_BOTTOM_FIRST:
      i = Y4M_ILACE_BOTTOM_FIRST;
      break;
    default:
      i = Y4M_ILACE_NONE;
      break;
    }
  y4m_si_set_interlace(&(e->si), i);

  r.n = e->format.timescale;
  r.d = e->format.frame_duration;
  
  y4m_si_set_framerate(&(e->si), r);

  r.n = e->format.pixel_width;
  r.d = e->format.pixel_height;

  y4m_si_set_sampleaspect(&(e->si), r);
  y4m_si_set_chroma(&(e->si), e->chroma_mode);

  /* Now, it's time to write the stream header */

  fprintf(stderr, "Writing stream header....");
  if(y4m_write_stream_header_cb(&(e->writer), &(e->si)) != Y4M_OK)
    {
    fprintf(stderr, "Writing stream header failed\n");
    return 0;
    }
  fprintf(stderr, "done\n");
  return 1;
  }

static void convert_yuva4444(uint8_t ** dst, uint8_t ** src, int width, int height, int stride)
  {
  int i, j;
  uint8_t *y, *u, *v, *a, *s;
  
  y = dst[0];
  u = dst[1];
  v = dst[2];
  a = dst[3];

  s = src[0];
  
  for(i = 0; i < height; i++)
    {
    s = src[0] + i * stride;
    for(j = 0; j < width; j++)
      {
      *(y++) = *(s++);
      *(u++) = *(s++);
      *(v++) = *(s++);
      *(a++) = *(s++);
      }
    }
  }

static void write_video_frame_y4m(void * data,
                                  gavl_video_frame_t* frame,
                                  int stream)
  {
  e_y4m_t * e = (e_y4m_t*)data;

  /* Check for YUVA4444 */
  if(e->format.pixelformat == GAVL_YUVA_32)
    {
    convert_yuva4444(e->tmp_planes, frame->planes,
                     e->format.image_width,
                     e->format.image_height,
                     frame->strides[0]);
    
    }
  else
    {
    if((frame->strides[0] == e->strides[0]) &&
       (frame->strides[1] == e->strides[1]) &&
       (frame->strides[2] == e->strides[2]) &&
       (frame->strides[3] == e->strides[3]))
      y4m_write_frame_cb(&(e->writer), &(e->si), &(e->fi), frame->planes);
    else
      {
      if(!e->frame)
        e->frame = gavl_video_frame_create_nopadd(&(e->format));
      gavl_video_frame_copy(&(e->format), e->frame, frame);
      y4m_write_frame_cb(&(e->writer), &(e->si), &(e->fi), e->frame->planes);
      }
    }
  
  }

static void close_y4m(void * data, int do_delete)
  {
  e_y4m_t * e = (e_y4m_t*)data;
  fclose(e->file);
  }

static void destroy_y4m(void * data)
  {
  e_y4m_t * e = (e_y4m_t*)data;

  y4m_fini_stream_info(&(e->si));
  y4m_fini_frame_info(&(e->fi));
  
  if(e->tmp_planes[0])
    free(e->tmp_planes[0]);
  if(e->frame)
    gavl_video_frame_destroy(e->frame);
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
    if(e->chroma_mode == -1)
      {
      if(gavl_pixelformat_has_alpha(e->format.pixelformat))
        {
        e->chroma_mode = Y4M_CHROMA_444ALPHA;
        }
      else
        {
        gavl_pixelformat_chroma_sub(e->format.pixelformat, &sub_h, &sub_v);
        /* 4:2:2 */
        if((sub_h == 2) && (sub_v == 1))
          e->chroma_mode = Y4M_CHROMA_422;
        
        /* 4:1:1 */
        else if((sub_h == 4) && (sub_v == 1))
          e->chroma_mode = Y4M_CHROMA_411;
        
        /* 4:2:0 */
        else if((sub_h == 2) && (sub_v == 2))
          {
          switch(e->format.chroma_placement)
            {
            case GAVL_CHROMA_PLACEMENT_DEFAULT:
              e->chroma_mode = Y4M_CHROMA_420JPEG;
              break;
            case GAVL_CHROMA_PLACEMENT_MPEG2:
              e->chroma_mode = Y4M_CHROMA_420MPEG2;
              break;
            case GAVL_CHROMA_PLACEMENT_DVPAL:
              e->chroma_mode = Y4M_CHROMA_420PALDV;
              break;
            }
          }
        else
          e->chroma_mode = Y4M_CHROMA_444;
        }
      }
    
    switch(e->chroma_mode)
      {
      case Y4M_CHROMA_420JPEG:
        e->format.pixelformat = GAVL_YUV_420_P;
        e->format.chroma_placement = GAVL_CHROMA_PLACEMENT_DEFAULT;
        break;
      case Y4M_CHROMA_420MPEG2:
        e->format.pixelformat = GAVL_YUV_420_P;
        e->format.chroma_placement = GAVL_CHROMA_PLACEMENT_MPEG2;
        break;
      case Y4M_CHROMA_420PALDV:
        e->format.pixelformat = GAVL_YUV_420_P;
        e->format.chroma_placement = GAVL_CHROMA_PLACEMENT_DVPAL;
        break;
      case Y4M_CHROMA_444:
        e->format.pixelformat = GAVL_YUV_444_P;
        break;
      case Y4M_CHROMA_422:
        e->format.pixelformat = GAVL_YUV_422_P;
        break;
      case Y4M_CHROMA_411:
        e->format.pixelformat = GAVL_YUV_411_P;
        break;
      case Y4M_CHROMA_MONO:
        /* Monochrome isn't supported by gavl, we choose the format with the
           smallest chroma planes to save memory */
        e->format.pixelformat = GAVL_YUV_410_P;
        break;
      case Y4M_CHROMA_444ALPHA:
        /* Must be converted to packed */
        e->format.pixelformat = GAVL_YUVA_32;
        e->tmp_planes[0] = malloc(e->format.image_width *
                                  e->format.image_height * 4);
        
        e->tmp_planes[1] = e->tmp_planes[0] +
          e->format.image_width * e->format.image_height;
        e->tmp_planes[2] = e->tmp_planes[1] +
          e->format.image_width * e->format.image_height;
        e->tmp_planes[3] = e->tmp_planes[2] +
          e->format.image_width * e->format.image_height;
        break;
      }
    return;
    }
  
  if(!strcmp(name, "chroma_mode"))
    {
    SET_ENUM("auto",     e->chroma_mode, -1);
    SET_ENUM("420jpeg",  e->chroma_mode, Y4M_CHROMA_420JPEG);
    SET_ENUM("420mpeg2", e->chroma_mode, Y4M_CHROMA_420MPEG2);
    SET_ENUM("420paldv", e->chroma_mode, Y4M_CHROMA_420PALDV);
    SET_ENUM("444",      e->chroma_mode, Y4M_CHROMA_444);
    SET_ENUM("422",      e->chroma_mode, Y4M_CHROMA_422);
    SET_ENUM("411",      e->chroma_mode, Y4M_CHROMA_411);
    SET_ENUM("mono",     e->chroma_mode, Y4M_CHROMA_MONO);
    SET_ENUM("yuva4444", e->chroma_mode, Y4M_CHROMA_444ALPHA);
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
