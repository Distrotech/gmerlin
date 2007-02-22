/*****************************************************************
 
  fv_equalizer.c
 
  Copyright (c) 2007 by Burkhard Plaum - plaum@ipf.uni-stuttgart.de
 
  http://gmerlin.sourceforge.net
 
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
 
  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111, USA.
 
*****************************************************************/

/*
 *  Based on vf_eq.c (by Richard Felker) and
 *  vf_hue.c (by Michael Niedermayer) from MPlayer
 *
 *  Added support for packed YUV formats and "in place"
 *  conversion.
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

#include <config.h>
#include <translation.h>
#include <plugin.h>
#include <utils.h>
#include <log.h>


#define LOG_DOMAIN "fv_equalizer"

typedef struct
  {
  int brightness;
  int contrast;
  float hue;
  float saturation;

  /* Offsets and advance */
  int offset[3];
  int advance[2];
  
  bg_read_video_func_t read_func;
  void * read_data;
  int read_stream;

  int chroma_width;
  int chroma_height;
  
  gavl_video_format_t format;
  
  void (*process_bc)(unsigned char *dest, int dstride,
                     int w, int h, int brightness, int contrast, int advance);
  
  void (*process_sh)(uint8_t *udst, uint8_t *vdst,
                     int dststride,
                     int w, int h, float hue, float sat, int advance);
  
  } equalizer_priv_t;

static void process_sh_C(uint8_t *udst, uint8_t *vdst,
                         int dststride, 
                         int w, int h, float hue, float sat, int advance)
  {
  int i, index;
  const int s= rint(sin(hue) * (1<<16) * sat);
  const int c= rint(cos(hue) * (1<<16) * sat);
  
  while (h--)
    {
    index = 0;
    for (i = 0; i<w; i++)
      {
      const int u= udst[index] - 128;
      const int v= vdst[index] - 128;
      int32_t new_u= (c*u - s*v + (1<<15) + (128<<16))>>16;
      int32_t new_v= (s*u + c*v + (1<<15) + (128<<16))>>16;
      if(new_u & 0xFFFF00) new_u= (-new_u)>>31;
      if(new_v & 0xFFFF00) new_v= (-new_v)>>31;
      udst[index]= new_u;
      vdst[index]= new_v;
      index += advance;
      }
    udst += dststride;
    vdst += dststride;
    }
  }

static void process_bc_C(unsigned char *dest, int dstride,
                         int w, int h, int brightness, int contrast, int advance)
  {
  int i, index;
  int pel;
  
  contrast = ((contrast+100)*256*256)/100;
  brightness = ((brightness+100)*511)/200-128 - contrast/512;
  
  while (h--)
    {
    index = 0;
    for (i = w; i; i--)
      {
      pel = ((dest[index] * contrast)>>16) + brightness;
      if(pel&768) pel = (-pel)>>31;
      dest[index] = pel;
      index += advance;
      }
    dest += dstride;
    }
  }

static void * create_equalizer()
  {
  equalizer_priv_t * ret;
  ret = calloc(1, sizeof(*ret));
  return ret;
  }

static void destroy_equalizer(void * priv)
  {
  equalizer_priv_t * vp;
  vp = (equalizer_priv_t *)priv;
  free(vp);
  }

static bg_parameter_info_t parameters[] =
  {
    {
      gettext_domain: PACKAGE,
      gettext_directory: LOCALE_DIR,
      name: "brightness",
      long_name: TRS("Brightness"),
      type: BG_PARAMETER_SLIDER_FLOAT,
      flags: BG_PARAMETER_SYNC,
      val_min: { val_f: -10.0 },
      val_max: { val_f:  10.0 },
      val_default: { val_f:  0.0 },
      num_digits: 1,
    },
    {
      name: "contrast",
      long_name: TRS("Contrast"),
      type: BG_PARAMETER_SLIDER_FLOAT,
      flags: BG_PARAMETER_SYNC,
      val_min: { val_f: -10.0 },
      val_max: { val_f:  10.0 },
      val_default: { val_f:  0.0 },
      num_digits: 1,
    },
    {
      name: "saturation",
      long_name: TRS("Saturation"),
      type: BG_PARAMETER_SLIDER_FLOAT,
      flags: BG_PARAMETER_SYNC,
      val_min: { val_f: -10.0 },
      val_max: { val_f:  10.0 },
      val_default: { val_f:  0.0 },
      num_digits: 1,
    },
    {
      name: "hue",
      long_name: TRS("Hue"),
      type: BG_PARAMETER_SLIDER_FLOAT,
      flags: BG_PARAMETER_SYNC,
      val_min: { val_f: -180.0 },
      val_max: { val_f:  180.0 },
      val_default: { val_f:  0.0 },
      num_digits: 1,
    },
    { /* End of parameters */ },
  };

static bg_parameter_info_t * get_parameters_equalizer(void * priv)
  {
  return parameters;
  }

static void set_parameter_equalizer(void * priv, char * name, bg_parameter_value_t * val)
  {
  equalizer_priv_t * vp;
  vp = (equalizer_priv_t *)priv;

  if(!name)
    return;

  if(!strcmp(name, "brightness"))
    vp->brightness = (int)(10*val->val_f);
  else if(!strcmp(name, "contrast"))
    vp->contrast = (int)(10*val->val_f);
  else if(!strcmp(name, "saturation"))
    vp->saturation = (val->val_f + 10.0)/10.0;
  else if(!strcmp(name, "hue"))
    vp->hue = val->val_f * M_PI / 180.0;
  }

static void connect_input_port_equalizer(void * priv,
                                         bg_read_video_func_t func,
                                         void * data, int stream, int port)
  {
  equalizer_priv_t * vp;
  vp = (equalizer_priv_t *)priv;

  if(!port)
    {
    vp->read_func = func;
    vp->read_data = data;
    vp->read_stream = stream;
    }
  
  }

static void set_input_format_equalizer(void * priv, gavl_video_format_t * format, int port)
  {
  int sub_h, sub_v;
  equalizer_priv_t * vp;
  vp = (equalizer_priv_t *)priv;

  if(!port)
    {
    switch(format->pixelformat)
      {
      case GAVL_RGB_15:
      case GAVL_BGR_15:
      case GAVL_RGB_16:
      case GAVL_BGR_16:
      case GAVL_RGB_24:
      case GAVL_BGR_24:
      case GAVL_RGB_32:
      case GAVL_BGR_32:
      case GAVL_YUV_444_P:
      case GAVL_YUV_444_P_16:
      case GAVL_RGB_48:
      case GAVL_RGB_FLOAT:
        format->pixelformat = GAVL_YUV_444_P;
        vp->advance[0] = 1;
        vp->advance[1] = 1;
        vp->offset[0]  = 0;
        vp->offset[1]  = 0;
        vp->offset[2]  = 0;
        break;
      case GAVL_RGBA_32:
      case GAVL_YUVA_32:
      case GAVL_RGBA_64:
      case GAVL_RGBA_FLOAT:
        format->pixelformat = GAVL_YUVA_32;
        vp->advance[0] = 4;
        vp->advance[1] = 4;
        vp->offset[0]  = 0;
        vp->offset[1]  = 1;
        vp->offset[2]  = 2;
        break;
      case GAVL_YUY2:
        vp->advance[0] = 2;
        vp->advance[1] = 4;
        vp->offset[0]  = 0;
        vp->offset[1]  = 1;
        vp->offset[2]  = 3;
        break;
      case GAVL_UYVY:
        vp->advance[0] = 2;
        vp->advance[1] = 4;
        vp->offset[0]  = 1;
        vp->offset[1]  = 0;
        vp->offset[2]  = 2;
        break;
      case GAVL_YUV_420_P:
      case GAVL_YUV_410_P:
      case GAVL_YUV_411_P:
      case GAVL_YUVJ_420_P:
      case GAVL_YUVJ_422_P:
      case GAVL_YUVJ_444_P:
        vp->advance[0] = 1;
        vp->advance[1] = 1;
        vp->offset[0]  = 0;
        vp->offset[1]  = 0;
        vp->offset[2]  = 0;
        break;
      case GAVL_YUV_422_P:
      case GAVL_YUV_422_P_16:
        format->pixelformat = GAVL_YUV_422_P;
        vp->advance[0] = 1;
        vp->advance[1] = 1;
        vp->offset[0]  = 0;
        vp->offset[1]  = 0;
        vp->offset[2]  = 0;
        break;
      case GAVL_PIXELFORMAT_NONE:
        break;
      }
    gavl_video_format_copy(&vp->format, format);
    }
  vp->process_bc = process_bc_C;
  vp->process_sh = process_sh_C;

  gavl_pixelformat_chroma_sub(format->pixelformat, &sub_h, &sub_v);

  vp->chroma_width  = format->image_width / sub_h;
  vp->chroma_height = format->image_height / sub_v;
  }

static void get_output_format_equalizer(void * priv, gavl_video_format_t * format)
  {
  equalizer_priv_t * vp;
  vp = (equalizer_priv_t *)priv;
  gavl_video_format_copy(format, &vp->format);
  }

static int read_video_equalizer(void * priv, gavl_video_frame_t * frame, int stream)
  {
  equalizer_priv_t * vp;
  vp = (equalizer_priv_t *)priv;
  
  if(!vp->read_func(vp->read_data, frame, vp->read_stream))
    return 0;

  /* Do the conversion */

  if((vp->contrast != 0.0) || (vp->brightness != 0.0))
    {
    vp->process_bc(frame->planes[0] + vp->offset[0], frame->strides[0],
                   vp->format.image_width,
                   vp->format.image_height,
                   vp->brightness, vp->contrast, vp->advance[0]);
    }
  
  if((vp->hue != 0.0) || (vp->saturation != 1.0))
    {
    if(gavl_pixelformat_is_planar(vp->format.pixelformat))
      {
      vp->process_sh(frame->planes[1] + vp->offset[1],
                     frame->planes[2] + vp->offset[2],
                     frame->strides[1],
                     vp->chroma_width,
                     vp->chroma_height,
                     vp->hue, vp->saturation, vp->advance[1]);
      }
    else
      {
      vp->process_sh(frame->planes[0] + vp->offset[1],
                     frame->planes[0] + vp->offset[2],
                     frame->strides[0],
                     vp->chroma_width,
                     vp->chroma_height,
                     vp->hue, vp->saturation, vp->advance[1]);
      }
    }
  
  return 1;
  }

bg_fv_plugin_t the_plugin = 
  {
    common:
    {
      BG_LOCALE,
      name:      "fv_equalizer",
      long_name: TRS("Video equalizer"),
      description: TRS("Control hue, saturation, contrast and brightness. Based on the vf_eq and vf_hue from the MPlayer project."),
      type:     BG_PLUGIN_FILTER_VIDEO,
      flags:    BG_PLUGIN_FILTER_1,
      create:   create_equalizer,
      destroy:   destroy_equalizer,
      get_parameters:   get_parameters_equalizer,
      set_parameter:    set_parameter_equalizer,
      priority:         1,
    },
    
    connect_input_port: connect_input_port_equalizer,
    
    set_input_format: set_input_format_equalizer,
    get_output_format: get_output_format_equalizer,

    read_video: read_video_equalizer,
    
  };

/* Include this into all plugin modules exactly once
   to let the plugin loader obtain the API version */
BG_GET_PLUGIN_API_VERSION;
