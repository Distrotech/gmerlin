/*****************************************************************
 
  iw_jpeg.c
 
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
#include <stdio.h>
#include <string.h>

#include <plugin.h>
#include <utils.h>

#include <jpeglib.h>

#define PADD(i, size) i = ((i + size - 1) / size) * size

/* JPEG writer */

typedef struct
  {
  char * filename;
  struct jpeg_compress_struct cinfo;
  struct jpeg_error_mgr jerr;
 
  // For writing planar YUV images
   
  uint8_t ** yuv_rows[3];
  uint8_t *  rows_0[16];
  uint8_t *  rows_1[16];
  uint8_t *  rows_2[16];
  
  FILE * output;
  gavl_video_format_t format;

  int quality;
  } jpeg_t;

void * create_jpeg()
  {
  jpeg_t * ret;
  ret = calloc(1, sizeof(*ret));
  ret->cinfo.err = jpeg_std_error(&(ret->jerr));
  jpeg_create_compress(&(ret->cinfo));

  ret->yuv_rows[0] = ret->rows_0;
  ret->yuv_rows[1] = ret->rows_1;
  ret->yuv_rows[2] = ret->rows_2;

  return ret;
  }

void destroy_jpeg(void * priv)
  {
  jpeg_t * jpeg = (jpeg_t*)priv;
  jpeg_destroy_compress(&(jpeg->cinfo));
  free(jpeg);
  }

int write_header_jpeg(void * priv, const char * filename_base,
                      gavl_video_format_t * format)
  {
  jpeg_t * jpeg = (jpeg_t*)priv;
  
  jpeg->filename = bg_sprintf("%s.jpg", filename_base);
  jpeg->output = fopen(jpeg->filename, "wb");
  if(!jpeg->output)
    return 0;
  
  jpeg_stdio_dest(&(jpeg->cinfo), jpeg->output);

  jpeg->cinfo.image_width  = format->image_width;
  jpeg->cinfo.image_height = format->image_height;

  /* We always have 3 components */

  jpeg->cinfo.input_components = 3;

  /* Set defaults */

  jpeg_set_defaults(&(jpeg->cinfo));
  
  /* Adjust colorspaces */

  format->colorspace = jpeg->format.colorspace;

  jpeg_set_colorspace(&(jpeg->cinfo), JCS_YCbCr);
  jpeg->cinfo.raw_data_in = TRUE;
  
  switch(format->colorspace)
    {
    case GAVL_YUVJ_420_P:
      jpeg->cinfo.comp_info[0].h_samp_factor = 2;
      jpeg->cinfo.comp_info[0].v_samp_factor = 2;

      jpeg->cinfo.comp_info[1].h_samp_factor = 1;
      jpeg->cinfo.comp_info[1].v_samp_factor = 1;

      jpeg->cinfo.comp_info[2].h_samp_factor = 1;
      jpeg->cinfo.comp_info[2].v_samp_factor = 1;

      PADD(format->frame_width, 16);
      PADD(format->frame_height, 16);
            
      break;
    case GAVL_YUVJ_422_P:
      jpeg->cinfo.comp_info[0].h_samp_factor = 2;
      jpeg->cinfo.comp_info[0].v_samp_factor = 1;
      
      jpeg->cinfo.comp_info[1].h_samp_factor = 1;
      jpeg->cinfo.comp_info[1].v_samp_factor = 1;

      jpeg->cinfo.comp_info[2].h_samp_factor = 1;
      jpeg->cinfo.comp_info[2].v_samp_factor = 1;

      PADD(format->frame_width, 16);
      PADD(format->frame_height, 8);
            
      break;
    case GAVL_YUVJ_444_P:
      jpeg->cinfo.comp_info[0].h_samp_factor = 1;
      jpeg->cinfo.comp_info[0].v_samp_factor = 1;
      
      jpeg->cinfo.comp_info[1].h_samp_factor = 1;
      jpeg->cinfo.comp_info[1].v_samp_factor = 1;

      jpeg->cinfo.comp_info[2].h_samp_factor = 1;
      jpeg->cinfo.comp_info[2].v_samp_factor = 1;

      PADD(format->frame_width,  8);
      PADD(format->frame_height, 8);
      
      break;
    default:
      fprintf(stderr, "Illegal colorspace: %s\n",
              gavl_colorspace_to_string(jpeg->format.colorspace));      
      break;
    }

  /* Set compression parameters */

  jpeg_set_quality(&jpeg->cinfo, jpeg->quality, TRUE);
  jpeg_start_compress(&jpeg->cinfo, TRUE);

  gavl_video_format_copy(&(jpeg->format), format);
  
  return 1;
  }

int write_image_jpeg(void * priv, gavl_video_frame_t * frame)
  {
  int i;
  int num_lines;
  jpeg_t * jpeg = (jpeg_t*)priv;

  switch(jpeg->format.colorspace)
    {
    case GAVL_YUVJ_420_P:
      while(jpeg->cinfo.next_scanline < jpeg->cinfo.image_height)
        {
        for(i = 0; i < 16; i++)
          {
          jpeg->rows_0[i] = frame->planes[0] + frame->strides[0] *
            (jpeg->cinfo.next_scanline + i);
          }
        for(i = 0; i < 8; i++)
          {
          jpeg->rows_1[i] = frame->planes[1] + frame->strides[1] *
            (jpeg->cinfo.next_scanline/2 + i);
          jpeg->rows_2[i] = frame->planes[2] + frame->strides[2] *
            (jpeg->cinfo.next_scanline/2 + i);
          }
        num_lines = jpeg->cinfo.image_height - jpeg->cinfo.next_scanline;
        if(num_lines > 16)
          num_lines = 16;
        jpeg_write_raw_data(&(jpeg->cinfo), jpeg->yuv_rows, 16);
        }
      break;
    case GAVL_YUVJ_422_P:
    case GAVL_YUVJ_444_P:
      while(jpeg->cinfo.next_scanline < jpeg->cinfo.image_height)
        {
        for(i = 0; i < 8; i++)
          {
          jpeg->rows_0[i] = frame->planes[0] + frame->strides[0] *
            (jpeg->cinfo.next_scanline + i);
          jpeg->rows_1[i] = frame->planes[1] + frame->strides[1] *
            (jpeg->cinfo.next_scanline/2 + i);
          jpeg->rows_2[i] = frame->planes[2] + frame->strides[2] *
            (jpeg->cinfo.next_scanline/2 + i);
          }
        num_lines = jpeg->cinfo.image_height - jpeg->cinfo.next_scanline;
        if(num_lines > 8)
          num_lines = 8;
        jpeg_write_raw_data(&(jpeg->cinfo), jpeg->yuv_rows, 8);
        }
      break;
    default:
      fprintf(stderr, "Illegal colorspace\n");
      return 0;
    }
  jpeg_finish_compress(&(jpeg->cinfo));
  fclose(jpeg->output);
  free(jpeg->filename);
  return 1;
  }

/* Configuration stuff */

static bg_parameter_info_t parameters[] =
  {
    {
      name:        "quality",
      long_name:   "Quality",
      type:        BG_PARAMETER_SLIDER_INT,
      val_min:     { val_i: 0 },
      val_max:     { val_i: 100 },
      val_default: { val_i: 95 },
    },
    {
      name:               "chroma_sampling",
      long_name:          "Chroma Sampling",
      type:               BG_PARAMETER_STRINGLIST,
      val_default:        { val_str: "4:2:0" },
      options: (char*[]) { "4:2:0",
                           "4:2:2",
                           "4:4:4",
                           (char*)0 },
      
    },
    { /* End of parameters */ }
  };

static bg_parameter_info_t * get_parameters_jpeg(void * p)
  {
  return parameters;
  }

static void set_parameter_jpeg(void * p, char * name,
                               bg_parameter_value_t * val)
  {
  jpeg_t * jpeg;
  jpeg = (jpeg_t *)p;
  
  if(!name)
    return;

  if(!strcmp(name, "quality"))
    jpeg->quality = val->val_i;
  else if(!strcmp(name, "chroma_sampling"))
    {
    if(!strcmp(val->val_str, "4:2:0"))
      {
      jpeg->format.colorspace = GAVL_YUVJ_420_P;
      }
    else if(!strcmp(val->val_str, "4:2:2"))
      {
      jpeg->format.colorspace = GAVL_YUVJ_422_P;
      }
    else if(!strcmp(val->val_str, "4:4:4"))
      {
      jpeg->format.colorspace = GAVL_YUVJ_444_P;
      }
    }
  }

bg_image_writer_plugin_t the_plugin =
  {
    common:
    {
      name:           "iw_jpeg",
      long_name:      "JPEG writer",
      mimetypes:      (char*)0,
      extensions:     "jpeg jpg",
      type:           BG_PLUGIN_IMAGE_WRITER,
      flags:          BG_PLUGIN_FILE,
      create:         create_jpeg,
      destroy:        destroy_jpeg,
      get_parameters: get_parameters_jpeg,
      set_parameter:  set_parameter_jpeg
    },
    write_header: write_header_jpeg,
    write_image:  write_image_jpeg,
  };
