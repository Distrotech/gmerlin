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

/***************************************************
 * Create and destroy video converters
 ***************************************************/

gavl_video_converter_t * gavl_video_converter_create()
  {
  gavl_video_converter_t * ret = calloc(1,sizeof(gavl_video_converter_t));
  return ret;
  }

static void video_converter_cleanup(gavl_video_converter_t* cnv)
  {
  gavl_video_convert_context_t * ctx;
  while(cnv->first_context)
    {
    ctx = cnv->first_context->next;
    
    if(cnv->first_context->scaler)
      gavl_video_scaler_destroy(cnv->first_context->scaler);
    if(cnv->first_context->output_frame && cnv->first_context->next)
      gavl_video_frame_destroy(cnv->first_context->output_frame);
    free(cnv->first_context);
    cnv->first_context = ctx;
    }
  cnv->last_context = (gavl_video_convert_context_t*)0;
  cnv->num_contexts = 0;
  }

void gavl_video_converter_destroy(gavl_video_converter_t* cnv)
  {
  video_converter_cleanup(cnv);
  free(cnv);
  }

/* Add a context to the converter */

static gavl_video_convert_context_t *
add_context(gavl_video_converter_t * cnv,
            const gavl_video_format_t * input_format,
            const gavl_video_format_t * output_format)
  {
  gavl_video_convert_context_t * ctx;
  ctx = calloc(1, sizeof(*ctx));
  ctx->options = &(cnv->options);
  gavl_video_format_copy(&(ctx->input_format),
                         input_format);
  
  gavl_video_format_copy(&(ctx->output_format),
                         output_format);
  
  if(cnv->last_context)
    {
    cnv->last_context->next = ctx;
    cnv->last_context = cnv->last_context->next;
    }
  else
    {
    cnv->first_context = ctx;
    cnv->last_context = ctx;
    }
  cnv->num_contexts++;
  return ctx;
  }

int add_context_csp(gavl_video_converter_t * cnv,
                     const gavl_video_format_t * input_format,
                     const gavl_video_format_t * output_format)
  {
  gavl_video_convert_context_t * ctx;
  ctx = add_context(cnv, input_format, output_format);

  ctx->func = gavl_find_colorspace_converter(&(cnv->options),
                                             input_format->colorspace,
                                             output_format->colorspace,
                                             input_format->frame_width,
                                             input_format->frame_height);

  if(!ctx->func)
    {
#ifdef DEBUG
    fprintf(stderr, "Found no conversion from %s to %s\n",
            gavl_colorspace_to_string(input_format->colorspace),
            gavl_colorspace_to_string(output_format->colorspace));
#endif
    return 0;
    }
#ifdef DEBUG
  fprintf(stderr, "Doing colorspace conversion from %s to %s\n",
          gavl_colorspace_to_string(input_format->colorspace),
          gavl_colorspace_to_string(output_format->colorspace));
  
#endif
  return 1;
  }

static void scale_func(gavl_video_convert_context_t * ctx)
  {
  gavl_video_scaler_scale(ctx->scaler,
                          ctx->input_frame,
                          ctx->output_frame);
  }

void add_context_scale(gavl_video_converter_t * cnv,
                       const gavl_video_format_t * input_format,
                       const gavl_video_format_t * output_format)
  {
  gavl_video_options_t * scaler_options;
  
  gavl_video_convert_context_t * ctx;
  ctx = add_context(cnv, input_format, output_format);

  ctx->scaler = gavl_video_scaler_create();

  scaler_options = gavl_video_scaler_get_options(ctx->scaler);

  gavl_video_options_copy(scaler_options, &(cnv->options));
#if 0
  fprintf(stderr, "gavl_video_scaler_init:\n");
  fprintf(stderr, "src_format:\n");
  gavl_video_format_dump(input_format);
  fprintf(stderr, "dst_format:\n");
  gavl_video_format_dump(output_format);

  fprintf(stderr, "src_rectangle:\n");
  gavl_rectangle_dump(&(cnv->options.src_rect));

  fprintf(stderr, "dst_rectangle:\n");
  gavl_rectangle_dump(&(cnv->options.dst_rect));
#endif
  
  gavl_video_scaler_init(ctx->scaler,
                         input_format->colorspace,
                         &(cnv->options.src_rect),
                         &(cnv->options.dst_rect),
                         input_format,
                         output_format);
  
  ctx->func = scale_func;
  
  
  }

int gavl_video_converter_init(gavl_video_converter_t * cnv,
                              const gavl_video_format_t * input_format,
                              const gavl_video_format_t * output_format)
  {
  gavl_video_convert_context_t * tmp_ctx;
  
  
  int do_csp = 0;
  int do_scale = 0;

  int in_sub;
  int out_sub;

  int sub_h;
  int sub_v;

  gavl_video_format_t tmp_format;
  gavl_video_format_t tmp_format1;

  video_converter_cleanup(cnv);
  
  gavl_video_format_copy(&tmp_format, input_format);
    
  /* Adjust colorspace */
    
  if(gavl_colorspace_has_alpha(tmp_format.colorspace) &&
     !gavl_colorspace_has_alpha(output_format->colorspace))
    { 
    if(cnv->options.alpha_mode == GAVL_ALPHA_IGNORE)
      {
      switch(tmp_format.colorspace)
        {
        case GAVL_RGBA_32:
          tmp_format.colorspace = GAVL_RGB_32;
          break;
        default:
          break;
        }
      }
    }

  /* Check for colorspace conversion */

  if(tmp_format.colorspace != output_format->colorspace)
    {
    do_csp = 1;
    }
  
  if(cnv->options.src_rect.w || 
     (tmp_format.image_width != output_format->image_width) ||
     (tmp_format.image_height != output_format->image_height) ||
     (tmp_format.pixel_width != output_format->pixel_width) ||
     (tmp_format.pixel_height != output_format->pixel_height))
    {
    do_scale = 1;
    }
  
  
  if(do_csp && do_scale)
    {
    /* For qualities below 3, we scale in the colorspace with the
       smaller subsampling */

    gavl_colorspace_chroma_sub(tmp_format.colorspace, &sub_h, &sub_v);
    in_sub = sub_h * sub_v;

    gavl_colorspace_chroma_sub(output_format->colorspace, &sub_h, &sub_v);
    out_sub = sub_h * sub_v;

    /* csp then scale */
    if(((in_sub < out_sub) && cnv->options.quality < 3) ||
       ((in_sub >= out_sub) && cnv->options.quality >= 3))
      {
      /* csp */
      
      gavl_video_format_copy(&tmp_format1, &tmp_format);

      tmp_format1.colorspace = output_format->colorspace;
      if(!add_context_csp(cnv, &tmp_format, &tmp_format1))
        return -1;
      
      gavl_video_format_copy(&tmp_format, &tmp_format1);

      /* scale */

      gavl_video_format_copy(&tmp_format1, &tmp_format);

      tmp_format1.image_width  = output_format->image_width;
      tmp_format1.image_height = output_format->image_height;

      tmp_format1.pixel_width  = output_format->pixel_width;
      tmp_format1.pixel_height = output_format->pixel_height;
      
      add_context_csp(cnv, &tmp_format, &tmp_format1);

      gavl_video_format_copy(&tmp_format, &tmp_format1);
      
      }
    /* scale then csp */
    else
      {
      /* scale */

      gavl_video_format_copy(&tmp_format1, &tmp_format);

      tmp_format1.image_width  = output_format->image_width;
      tmp_format1.image_height = output_format->image_height;

      tmp_format1.pixel_width  = output_format->pixel_width;
      tmp_format1.pixel_height = output_format->pixel_height;
      
      add_context_csp(cnv, &tmp_format, &tmp_format1);

      gavl_video_format_copy(&tmp_format, &tmp_format1);

      /* csp */
      
      gavl_video_format_copy(&tmp_format1, &tmp_format);

      tmp_format1.colorspace = output_format->colorspace;
      if(!add_context_csp(cnv, &tmp_format, &tmp_format1))
        return -1;

      gavl_video_format_copy(&tmp_format, &tmp_format1);
      }
    
    }

  else if(do_csp)
    {
    if(!add_context_csp(cnv, &tmp_format,
                        output_format))
      return -1;
    }

  else if(do_scale)
    {
    add_context_scale(cnv, &tmp_format,
                      output_format);
    }

  /* Now, create temporary frames for the contexts */

  tmp_ctx = cnv->first_context;
  while(tmp_ctx && tmp_ctx->next)
    {
    tmp_ctx->output_frame =
      gavl_video_frame_create(&(tmp_ctx->output_format));
    tmp_ctx->next->input_frame = tmp_ctx->output_frame;
    tmp_ctx = tmp_ctx->next;
    }
  return cnv->num_contexts;
  }

gavl_video_options_t *
gavl_video_converter_get_options(gavl_video_converter_t * cnv)
  {
  return &(cnv->options);
  }

/***************************************************
 * Convert a frame
 ***************************************************/

void gavl_video_convert(gavl_video_converter_t * cnv,
                        gavl_video_frame_t * input_frame,
                        gavl_video_frame_t * output_frame)
  {
  gavl_video_convert_context_t * tmp_ctx;
  
  cnv->first_context->input_frame = input_frame;
  cnv->last_context->output_frame = output_frame;

  tmp_ctx = cnv->first_context;

  while(tmp_ctx)
    {
    tmp_ctx->func(tmp_ctx);
    tmp_ctx = tmp_ctx->next;
    }

  output_frame->time = input_frame->time;
  output_frame->time_scaled = input_frame->time_scaled;
  output_frame->duration_scaled = input_frame->duration_scaled;
  }
