/*****************************************************************
 
  ir_jpeg.c
 
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

#include <plugin.h>

#include <jpeglib.h>


#define PADD(i, size) i = ((i + size - 1) / size) * size

/* JPEG reader */

typedef struct
  {
  struct jpeg_decompress_struct cinfo;
  struct jpeg_error_mgr jerr;
 
  // For reading planar YUV images
   
  uint8_t ** yuv_rows[3];
  uint8_t *  rows_0[16];
  uint8_t *  rows_1[16];
  uint8_t *  rows_2[16];
  
  FILE * input;
  gavl_video_format_t format;
  } jpeg_t;

static
void * create_jpeg()
  {
  jpeg_t * ret;
  ret = calloc(1, sizeof(*ret));
  ret->cinfo.err = jpeg_std_error(&(ret->jerr));
  jpeg_create_decompress(&(ret->cinfo));

  ret->yuv_rows[0] = ret->rows_0;
  ret->yuv_rows[1] = ret->rows_1;
  ret->yuv_rows[2] = ret->rows_2;

  return ret;
  }

static void destroy_jpeg(void* priv)
  {
  jpeg_t * jpeg = (jpeg_t*)priv;
  jpeg_destroy_decompress(&(jpeg->cinfo));
  free(jpeg);
  }

/* Get available parameters */

// bg_parameter_info_t * (*get_parameters_jpeg)(void * priv);

/* Set configuration parameter */
    
// bg_parameter_func set_parameter;

/* OPTIONAL: Return a readable description of the last error */

// char * (*get_error)(void* priv);
static
int read_header_jpeg(void * priv, const char * filename,
                     gavl_video_format_t * format)
  {
  jpeg_t * jpeg = (jpeg_t*)priv;
  
  jpeg->input = fopen(filename, "rb");

  if(!jpeg->input)
    return 0;
  
  jpeg_stdio_src(&(jpeg->cinfo), jpeg->input);

  if(jpeg_read_header(&(jpeg->cinfo), TRUE) != JPEG_HEADER_OK)
    return 0;

  format->image_width = jpeg->cinfo.image_width;
  format->image_height = jpeg->cinfo.image_height;

  format->frame_width  = jpeg->cinfo.image_width;
  format->frame_height = jpeg->cinfo.image_height;
  format->pixel_width = 1;
  format->pixel_height = 1;
  
  /*
   *  Get the colorspace, we handle YUV 444, YUV 422, YUV 420 directly.
   *  All other formats are converted to RGB24
   */
  
  switch(jpeg->cinfo.jpeg_color_space)
    {
    case JCS_YCbCr:
      if((jpeg->cinfo.comp_info[0].h_samp_factor == 2) &&
         (jpeg->cinfo.comp_info[0].v_samp_factor == 2) &&
         (jpeg->cinfo.comp_info[1].h_samp_factor == 1) &&
         (jpeg->cinfo.comp_info[1].v_samp_factor == 1) &&
         (jpeg->cinfo.comp_info[2].h_samp_factor == 1) &&
         (jpeg->cinfo.comp_info[2].v_samp_factor == 1))
        {
        format->colorspace = GAVL_YUVJ_420_P;
        PADD(format->frame_width, 16);
        PADD(format->frame_height, 16);
        }
      else if((jpeg->cinfo.comp_info[0].h_samp_factor == 2) &&
              (jpeg->cinfo.comp_info[0].v_samp_factor == 1) &&
              (jpeg->cinfo.comp_info[1].h_samp_factor == 1) &&
              (jpeg->cinfo.comp_info[1].v_samp_factor == 1) &&
              (jpeg->cinfo.comp_info[2].h_samp_factor == 1) &&
              (jpeg->cinfo.comp_info[2].v_samp_factor == 1))
        {
        format->colorspace = GAVL_YUVJ_422_P;
        PADD(format->frame_width, 16);
        PADD(format->frame_height, 8);
        }
      else if((jpeg->cinfo.comp_info[0].h_samp_factor == 1) &&
              (jpeg->cinfo.comp_info[0].v_samp_factor == 1) &&
              (jpeg->cinfo.comp_info[1].h_samp_factor == 1) &&
              (jpeg->cinfo.comp_info[1].v_samp_factor == 1) &&
              (jpeg->cinfo.comp_info[2].h_samp_factor == 1) &&
              (jpeg->cinfo.comp_info[2].v_samp_factor == 1))
        {
        format->colorspace = GAVL_YUVJ_444_P;
        PADD(format->frame_width,  8);
        PADD(format->frame_height, 8);
        }
      else
        {
        format->colorspace = GAVL_RGB_24;
        }
      break;
    default:
      format->colorspace = GAVL_RGB_24;
    }
  gavl_video_format_copy(&(jpeg->format), format);
  return 1;
  }

static 
int read_image_jpeg(void * priv, gavl_video_frame_t * frame)
  {
  int i;
  int num_lines;
  jpeg_t * jpeg = (jpeg_t*)priv;

  if(!frame)
    {
    return 1;
    jpeg_abort_decompress(&jpeg->cinfo);
    }
  
  if(jpeg->format.colorspace != GAVL_RGB_24)
    jpeg->cinfo.raw_data_out = TRUE;
  
  jpeg_start_decompress(&jpeg->cinfo);
  
  switch(jpeg->format.colorspace)
    {
    case GAVL_RGB_24:
      while(jpeg->cinfo.output_scanline < jpeg->cinfo.output_height)
        {
        for(i = 0; i < 16; i++)
          {
          jpeg->rows_0[i] = frame->planes[0] + frame->strides[0] *
            (jpeg->cinfo.output_scanline + i);
          }
        num_lines = jpeg->cinfo.output_height - jpeg->cinfo.output_scanline;
        if(num_lines > 16)
          num_lines = 16;
        jpeg_read_scanlines(&(jpeg->cinfo),
                            (JSAMPLE**)(jpeg->rows_0), num_lines);
        }
      break;
    case GAVL_YUVJ_420_P:
      while(jpeg->cinfo.output_scanline < jpeg->cinfo.output_height)
        {
        for(i = 0; i < 16; i++)
          {
          jpeg->rows_0[i] = frame->planes[0] + frame->strides[0] *
            (jpeg->cinfo.output_scanline + i);
          }
        for(i = 0; i < 8; i++)
          {
          jpeg->rows_1[i] = frame->planes[1] + frame->strides[1] *
            (jpeg->cinfo.output_scanline/2 + i);
          jpeg->rows_2[i] = frame->planes[2] + frame->strides[2] *
            (jpeg->cinfo.output_scanline/2 + i);
          }
        num_lines = jpeg->cinfo.output_height - jpeg->cinfo.output_scanline;
        if(num_lines > 16)
          num_lines = 16;
        jpeg_read_raw_data(&(jpeg->cinfo), jpeg->yuv_rows, 16);
        }
      break;
    case GAVL_YUVJ_422_P:
    case GAVL_YUVJ_444_P:
      while(jpeg->cinfo.output_scanline < jpeg->cinfo.output_height)
        {
        for(i = 0; i < 8; i++)
          {
          jpeg->rows_0[i] = frame->planes[0] + frame->strides[0] *
            (jpeg->cinfo.output_scanline + i);
          jpeg->rows_1[i] = frame->planes[1] + frame->strides[1] *
            (jpeg->cinfo.output_scanline/2 + i);
          jpeg->rows_2[i] = frame->planes[2] + frame->strides[2] *
            (jpeg->cinfo.output_scanline/2 + i);
          }
        num_lines = jpeg->cinfo.output_height - jpeg->cinfo.output_scanline;
        if(num_lines > 8)
          num_lines = 8;
        jpeg_read_raw_data(&(jpeg->cinfo), jpeg->yuv_rows, 8);
        }
      break;
    default:
      fprintf(stderr, "Illegal colorspace\n");
      return 0;
    }
  jpeg_finish_decompress(&(jpeg->cinfo));
  fclose(jpeg->input);
  
  return 1;
  }

bg_image_reader_plugin_t the_plugin =
  {
    common:
    {
      name:          "ir_jpeg",
      long_name:     "JPEG loader",
      mimetypes:     (char*)0,
      extensions:    "jpeg jpg",
      type:          BG_PLUGIN_IMAGE_READER,
      flags:         BG_PLUGIN_FILE,
      priority:      BG_PLUGIN_PRIORITY_MAX,
      create:        create_jpeg,
      destroy:       destroy_jpeg,
      //      get_parameters: get_parameters_vorbis,
      //      set_parameter:  set_parameter_vorbis
    },
    read_header: read_header_jpeg,
    read_image:  read_image_jpeg,
  };

/* Include this into all plugin modules exactly once
   to let the plugin loader obtain the API version */
BG_GET_PLUGIN_API_VERSION;
