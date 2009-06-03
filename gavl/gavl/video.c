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

#define DEBUG

#ifdef DEBUG
#include <stdio.h>  
#endif

#include "gavl.h"
#include "config.h"
#include "video.h"


static void copy_video_options(gavl_video_options_t * dst,
                               const gavl_video_options_t * src)
  {
  dst->accel_flags = src->accel_flags;

  /* Conversion flags */
  
  dst->conversion_flags = src->conversion_flags;

  dst->crop_factor = src->crop_factor;
  
  /* Background color */

  dst->background_red   = src->background_red;
  dst->background_green = src->background_green;
  dst->background_blue  = src->background_blue;
  }

static void copy_video_format(gavl_video_format_t  * dst,
                              const gavl_video_format_t  * src)
  {
  dst->colorspace = src->colorspace;
  dst->width = src->width;
  dst->height = src->height;
  }

/***************************************************
 * Create and destroy video converters
 ***************************************************/

gavl_video_converter_t * gavl_create_video_converter()
  {
  gavl_video_converter_t * ret = calloc(1,sizeof(gavl_video_converter_t));

  ret->csp_context.options = &(ret->options);

  return ret;
  }

void gavl_destroy_video_converter(gavl_video_converter_t* cnv)
  {
  free(cnv);
  }

/***************************************************
 * Default Options
 ***************************************************/

void gavl_video_default_options(gavl_video_options_t * opt)
  {
  opt->background_red   = 0;
  opt->background_green = 0;
  opt->background_blue  = 0;
  opt->accel_flags = GAVL_ACCEL_C;
  opt->crop_factor = 0.0;
  opt->conversion_flags = 0;
  }

/***************************************************
 * Set formats, return FALSE if conversion cannot
 * be performed
 ***************************************************/

int gavl_video_init(gavl_video_converter_t * cnv,
                   const gavl_video_options_t * options,
                   const gavl_video_format_t * input_format,
                   const gavl_video_format_t * output_format)
  {
  int ret = 0;
  /* Set the options */

  copy_video_options(&(cnv->options), options);
  
  if(input_format->colorspace != output_format->colorspace)
    {
    copy_video_format(&(cnv->csp_context.input_format),
                      input_format);

    copy_video_format(&(cnv->csp_context.output_format),
                      output_format);

    cnv->csp_func = gavl_find_colorspace_converter(options,
                                                   input_format->colorspace,
                                                   output_format->colorspace,
                                                   input_format->width,
                                                   input_format->height);

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
  }
