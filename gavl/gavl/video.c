/*****************************************************************

  video.c

  Copyright (c) 2001 by Burkhard Plaum - plaum@ipf.uni-stuttgart.de

  http://gmerlin.sourceforge.net

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111, USA.

*****************************************************************/

#include <stdlib.h> /* calloc, free */

#include <string.h>

// #define DEBUG

#ifdef DEBUG
#include <stdio.h>  
#endif

#include "gavl.h"
#include "config.h"
#include "video.h"


static void copy_video_options(gavl_video_options_t * dst,
                               const gavl_video_options_t * src)
  {
  memcpy(dst, src, sizeof(*dst));
  }

void gavl_video_format_copy(gavl_video_format_t * dst,
                            const gavl_video_format_t * src)
  {
  memcpy(dst, src, sizeof(*dst));
  }

/***************************************************
 * Create and destroy video converters
 ***************************************************/

gavl_video_converter_t * gavl_video_converter_create()
  {
  gavl_video_converter_t * ret = calloc(1,sizeof(gavl_video_converter_t));

  ret->csp_context.options = &(ret->options);

  return ret;
  }

void gavl_video_converter_destroy(gavl_video_converter_t* cnv)
  {
  free(cnv);
  }

/***************************************************
 * Default Options
 ***************************************************/

void gavl_video_default_options(gavl_video_options_t * opt)
  {
  memset(opt, 0, sizeof(*opt));
  opt->accel_flags = GAVL_ACCEL_C;
  opt->crop_factor = 0.0;
  }

/***************************************************
 * Set formats, return FALSE if conversion cannot
 * be performed
 ***************************************************/

int gavl_video_converter_init(gavl_video_converter_t * cnv,
                              const gavl_video_options_t * options,
                              const gavl_video_format_t * input_format,
                              const gavl_video_format_t * output_format)
  {
  int ret = 0;
  /* Set the options */

  gavl_colorspace_t input_colorspace;
    
  copy_video_options(&(cnv->options), options);

  input_colorspace = input_format->colorspace;
  
  if(options->alpha_mode == GAVL_ALPHA_IGNORE)
    {
    switch(input_colorspace)
      {
      case GAVL_RGBA_32:
        input_colorspace = GAVL_RGB_32;
        break;
      default:
        break;
      }
    }
  
  if(input_colorspace != output_format->colorspace)
    {
    gavl_video_format_copy(&(cnv->csp_context.input_format),
                           input_format);

    gavl_video_format_copy(&(cnv->csp_context.output_format),
                           output_format);

    cnv->csp_context.input_format.colorspace = input_colorspace;
    
    cnv->csp_func = gavl_find_colorspace_converter(options,
                                                   input_colorspace,
                                                   output_format->colorspace,
                                                   input_format->frame_width,
                                                   input_format->frame_height);

    if(!cnv->csp_func)
      {
#ifdef DEBUG
      fprintf(stderr, "Found no conversion from %s to %s\n",
              gavl_colorspace_to_string(input_format->colorspace),
              gavl_colorspace_to_string(output_format->colorspace));
#endif
      return -1;
      }
#ifdef DEBUG
    fprintf(stderr, "Doing colorspace conversion from %s to %s\n",
            gavl_colorspace_to_string(input_format->colorspace),
            gavl_colorspace_to_string(output_format->colorspace));
    
#endif
    
    ret++;
    }
  else
    cnv->csp_func = (gavl_video_func_t)0;
  return ret;
  }

/***************************************************
 * Convert a frame
 ***************************************************/

void gavl_video_convert(gavl_video_converter_t * cnv,
                        gavl_video_frame_t * input_frame,
                        gavl_video_frame_t * output_frame)
  {
  if(cnv->csp_func)
    {
    cnv->csp_context.input_frame = input_frame;
    cnv->csp_context.output_frame = output_frame;
    cnv->csp_func(&(cnv->csp_context));
    }
  output_frame->time = input_frame->time;
  }
