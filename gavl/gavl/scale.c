/*****************************************************************

  scale.c

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
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include <gavl/gavl.h>
#include <scale.h>
#include <accel.h>

// #define DUMP_TABLES

#define SCALE_ACTION_NOOP  0
#define SCALE_ACTION_COPY  1
#define SCALE_ACTION_SCALE 2

/* Public functions */

void gavl_video_scaler_destroy(gavl_video_scaler_t * s)
  {
  int i, j;

  gavl_video_frame_null(s->src);
  gavl_video_frame_null(s->dst);

  gavl_video_frame_destroy(s->src);
  gavl_video_frame_destroy(s->dst);

  for(i = 0; i < 2; i++)
    {
    for(j = 0; j < GAVL_MAX_PLANES; j++)
      {
      gavl_video_scale_context_cleanup(&(s->contexts[i][j]));
      }
    
    }
  
  
  free(s);  
  }

gavl_video_scaler_t * gavl_video_scaler_create()
  {
  gavl_video_scaler_t * ret;
  ret = calloc(1, sizeof(*ret));

  ret->src = gavl_video_frame_create((gavl_video_format_t*)0);
  ret->dst = gavl_video_frame_create((gavl_video_format_t*)0);

  gavl_video_options_set_defaults(&ret->opt);
  
  return ret;
  }

int gavl_video_scaler_init(gavl_video_scaler_t * scaler,
                           const gavl_video_format_t * src_format,
                           const gavl_video_format_t * dst_format)
  {
  gavl_rectangle_f_t src_rect;
  gavl_rectangle_i_t  dst_rect;
  gavl_video_options_t opt;

  gavl_scale_funcs_t funcs;
  int field, plane;
 
  int sub_h_out = 1, sub_v_out = 1;

  /* Copy options because we want to change them */

  gavl_video_options_copy(&opt, &(scaler->opt));
  
  /* Copy formats */
  
  gavl_video_format_copy(&(scaler->src_format), src_format);
  gavl_video_format_copy(&(scaler->dst_format), dst_format);
  
  /* Check if we have rectangles */

  if(!opt.have_rectangles)
    {
    gavl_rectangle_f_set_all(&(src_rect), src_format);
    gavl_rectangle_i_set_all(&(dst_rect), dst_format);
    gavl_video_options_set_rectangles(&opt, &(src_rect), &(dst_rect));
    }
  
  /* Check if we must deinterlace */

  if((opt.deinterlace_mode == GAVL_DEINTERLACE_SCALE) &&
     (((dst_format->interlace_mode == GAVL_INTERLACE_NONE) &&
       (src_format->interlace_mode != GAVL_INTERLACE_NONE)) ||
      (opt.conversion_flags & GAVL_FORCE_DEINTERLACE)))
    {
    scaler->deinterlace = 1;
    opt.src_rect.y /= 2.0;
    opt.src_rect.h /= 2.0;
    }
  /* Copy destination rectangle so we know, which subframe to take */
  gavl_rectangle_i_copy(&scaler->dst_rect, &opt.dst_rect);
  
#if 0  
  fprintf(stderr, "gavl_video_scaler_init:\n");
  gavl_rectangle_f_dump(&(scaler->src_rect));
  fprintf(stderr, "\n");
  gavl_rectangle_i_dump(&(scaler->dst_rect));
  fprintf(stderr, "\n");
#endif                      
  
  /* Crop source and destination rectangles to the formats */

  
  
  /* Align the destination rectangle to the output formtat */

  gavl_pixelformat_chroma_sub(scaler->dst_format.pixelformat, &sub_h_out, &sub_v_out);
  gavl_rectangle_i_align(&(opt.dst_rect), sub_h_out, sub_v_out);
  
#if 1
  fprintf(stderr, "Initializing scaler:\n");
  fprintf(stderr, "Src format:\n");
  gavl_video_format_dump(src_format);
  fprintf(stderr, "Dst format:\n");
  gavl_video_format_dump(dst_format);

  fprintf(stderr, "Src rectangle:\n");
  gavl_rectangle_f_dump(&opt.src_rect);
  fprintf(stderr, "\nDst rectangle:\n");
  gavl_rectangle_i_dump(&scaler->dst_rect);
  fprintf(stderr, "\n");
#endif
    
  /* Check how many planes we have */

  if((src_format->pixelformat == GAVL_YUY2) ||
     (src_format->pixelformat == GAVL_UYVY))
    scaler->num_planes = 3;
  else
    scaler->num_planes = gavl_pixelformat_num_planes(src_format->pixelformat);
  
  /* Check how many fields we must handle */
  if((src_format->interlace_mode != GAVL_INTERLACE_NONE) && !scaler->deinterlace)
    scaler->num_fields = 2;
  else
    scaler->num_fields = 1;

  if(scaler->num_fields == 2)
    {
    if(!scaler->src_field)
      scaler->src_field = gavl_video_frame_create(NULL);
    if(!scaler->dst_field)
      scaler->dst_field = gavl_video_frame_create(NULL);
    }
  else if(scaler->deinterlace)
    {
    if(!scaler->src_field)
      scaler->src_field = gavl_video_frame_create(NULL);
    }
  
#if 0
  fprintf(stderr, "Fields: %d, planes: %d\n",
          scaler->num_fields, scaler->num_planes);
#endif    

  /* Handle automatic mode selection */
  
  memset(&funcs, 0, sizeof(funcs));

  if(opt.scale_mode == GAVL_SCALE_AUTO)
    {
    if(opt.quality < 2)
      opt.scale_mode = GAVL_SCALE_NEAREST;
    else if(opt.quality <= 3)
      opt.scale_mode = GAVL_SCALE_BILINEAR;
    else
      opt.scale_mode = GAVL_SCALE_CUBIC_BSPLINE;
    }
  
  /* Get scale functions */
  
  switch(opt.scale_mode)
    {
    case GAVL_SCALE_AUTO:
      break;
    case GAVL_SCALE_NEAREST:
      gavl_init_scale_funcs_nearest_c(&funcs);
      break;
    case GAVL_SCALE_BILINEAR:
      gavl_init_scale_funcs_bilinear_c(&funcs);
      break;
    case GAVL_SCALE_QUADRATIC:
      gavl_init_scale_funcs_quadratic_c(&funcs);
      break;
    case GAVL_SCALE_CUBIC_BSPLINE:
    case GAVL_SCALE_CUBIC_MITCHELL:
    case GAVL_SCALE_CUBIC_CATMULL:
      gavl_init_scale_funcs_bicubic_c(&funcs);
      break;
    case GAVL_SCALE_SINC_LANCZOS:
      gavl_init_scale_funcs_generic_c(&funcs);
      break;
    }
  
  /* Now, initialize all fields and planes */

  if(scaler->deinterlace)
    {
    field = (scaler->opt.deinterlace_drop_mode == GAVL_DEINTERLACE_DROP_BOTTOM) ? 0 : 1;
    for(plane = 0; plane < scaler->num_planes; plane++)
      {
      gavl_video_scale_context_init(&(scaler->contexts[field][plane]),
                                    &opt,
                                    plane, src_format, dst_format, &funcs, 1, field);
      }
    
    }
  else
    {
    for(field = 0; field < scaler->num_fields; field++)
      {
      for(plane = 0; plane < scaler->num_planes; plane++)
        {
        gavl_video_scale_context_init(&(scaler->contexts[field][plane]),
                                      &opt,
                                      plane, src_format, dst_format, &funcs, 0, field);
        }
      }
    }
  return 1;
  }

gavl_video_options_t * gavl_video_scaler_get_options(gavl_video_scaler_t * s)
  {
  return &(s->opt);
  }

void gavl_video_scaler_scale(gavl_video_scaler_t * s,
                             gavl_video_frame_t * src,
                             gavl_video_frame_t * dst)
  {
  int i, field;
  /* Set the destination subframe */
  gavl_video_frame_get_subframe(s->dst_format.pixelformat, dst, s->dst, &(s->dst_rect));

  if(s->deinterlace)
    {
    field = (s->opt.deinterlace_drop_mode == GAVL_DEINTERLACE_DROP_BOTTOM) ? 0 : 1;
    gavl_video_frame_get_field(s->src_format.pixelformat, src, s->src_field, field);
    
    for(i = 0; i < s->num_planes; i++)
      gavl_video_scale_context_scale(&(s->contexts[field][i]), s->src_field, dst);
    }
  else if(s->num_fields == 2)
    {
    /* First field */
    gavl_video_frame_get_field(s->src_format.pixelformat, src,    s->src_field, 0);
    gavl_video_frame_get_field(s->dst_format.pixelformat, s->dst, s->dst_field, 0);
    for(i = 0; i < s->num_planes; i++)
      gavl_video_scale_context_scale(&(s->contexts[0][i]), s->src_field, s->dst_field);

    /* Second field */
    gavl_video_frame_get_field(s->src_format.pixelformat, src,    s->src_field, 1);
    gavl_video_frame_get_field(s->dst_format.pixelformat, s->dst, s->dst_field, 1);
    for(i = 0; i < s->num_planes; i++)
      gavl_video_scale_context_scale(&(s->contexts[1][i]), s->src_field, s->dst_field);
    }
  else
    {
    for(i = 0; i < s->num_planes; i++)
      {
      //      fprintf(stderr, "Scale %d, %p\n", i, &(s->contexts[0][i]));
      gavl_video_scale_context_scale(&(s->contexts[0][i]), src, s->dst);
      }
    }
  }
