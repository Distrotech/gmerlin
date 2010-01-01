#include <math.h>
#include <string.h>

#include <renderer.h>

typedef struct stream_s stream_t;

typedef void (*composite_func_t)(stream_t * stream, gavl_video_frame_t * dst);

struct stream_s
  {
  gavl_video_frame_t * frame;
  gavl_video_frame_t * frame_cnv;
  gavl_video_converter_t * cnv;
  bg_nle_renderer_instream_video_t * s;
  bg_nle_track_t * t;

  gavl_video_format_t src_format;
  gavl_video_format_t composite_format;
  
  int id;
  int overlay_mode;
  composite_func_t func;

  float camera[3];
  float projector[3];

  float last_camera[3];
  float last_projector[3];
  
  gavl_rectangle_f_t src_rect;
  gavl_rectangle_i_t dst_rect;
  
  int do_convert;
  int rect_changed;

  gavl_overlay_blend_context_t * blend_context;
  gavl_overlay_t ovl;
  };

struct bg_nle_video_compositor_s
  {
  int num_streams;
  int streams_alloc;
  
  stream_t * streams;
  
  gavl_video_format_t format;
  float * bg_color;
  bg_nle_outstream_t * os;
  int64_t pts;
  bg_gavl_video_options_t * opt;
  };


bg_nle_video_compositor_t *
bg_nle_video_compositor_create(bg_nle_outstream_t * os, bg_gavl_video_options_t * opt,
                               const gavl_video_format_t * format, float * bg_color)
  {
  bg_nle_video_compositor_t * ret = calloc(1, sizeof(*ret));
  ret->os = os;
  ret->opt = opt;
  gavl_video_format_copy(&ret->format, format);
  ret->bg_color = bg_color;
  return ret;
  }

void bg_nle_video_compositor_clear(bg_nle_video_compositor_t * c)
  {
  
  }

static void composite_replace(stream_t * s, gavl_video_frame_t * dst)
  {
  if(s->do_convert)
    {
    if(s->frame)
      s->frame = gavl_video_frame_create(&s->src_format);
    
    bg_nle_renderer_instream_video_read(s->s, s->frame, s->id);
    gavl_video_convert(s->cnv, s->frame, dst);
    }
  else
    {
    bg_nle_renderer_instream_video_read(s->s, dst, s->id);
    }
  }

static void composite_blend(stream_t * s, gavl_video_frame_t * dst)
  {
  if(s->do_convert)
    {
    if(!s->frame)
      s->frame = gavl_video_frame_create(&s->src_format);

    if(!s->frame_cnv)
      s->frame_cnv = gavl_video_frame_create(&s->composite_format);
    
    bg_nle_renderer_instream_video_read(s->s, s->frame, s->id);
    gavl_video_convert(s->cnv, s->frame, s->frame_cnv);
    }
  else
    {
    if(!s->frame_cnv)
      s->frame_cnv = gavl_video_frame_create(&s->src_format);
    bg_nle_renderer_instream_video_read(s->s, s->frame_cnv, s->id);
    }
  s->ovl.frame = s->frame_cnv;
  
  }


void bg_nle_video_compositor_add_stream(bg_nle_video_compositor_t * c,
                                        bg_nle_renderer_instream_video_t * is,
                                        bg_nle_track_t * t)
  {
  stream_t * s;
  
  if(c->num_streams+1 > c->streams_alloc)
    {
    c->streams_alloc += 10;
    c->streams = realloc(c->streams,
                         c->streams_alloc * sizeof(*c->streams));
    memset(c->streams + c->num_streams, 0,
           (c->streams_alloc - c->num_streams) * sizeof(*c->streams));
    }
  
  s = &c->streams[c->num_streams];

  s->cnv = gavl_video_converter_create();
  gavl_video_options_copy(gavl_video_converter_get_options(s->cnv), c->opt->opt);
  
  s->t = t;
  s->s = is;
  s->id =
    bg_nle_renderer_instream_video_connect_output(is,
                                                  &s->src_format,
                                                  &s->overlay_mode);
  
  if(!gavl_pixelformat_has_alpha(s->src_format.pixelformat) &&
     s->overlay_mode == BG_NLE_OVERLAY_BLEND)
    s->overlay_mode = BG_NLE_OVERLAY_REPLACE;
  
  switch(s->overlay_mode)
    {
    case BG_NLE_OVERLAY_REPLACE:
      s->func = composite_replace;
      gavl_video_format_copy(&s->composite_format, &c->format);
      break;
    case BG_NLE_OVERLAY_BLEND:
      s->func = composite_blend;
      s->blend_context = gavl_overlay_blend_context_create();
      gavl_video_format_copy(&s->composite_format, &c->format);
      s->composite_format.pixelformat = s->src_format.pixelformat;
        

      gavl_video_options_copy(gavl_overlay_blend_context_get_options(s->blend_context),
                              c->opt->opt);
      
      gavl_overlay_blend_context_init(s->blend_context, &c->format, &s->composite_format);
      break;
    }
  s->rect_changed = 1;
  c->num_streams++;
  }

static void set_rectangle(float * values, gavl_rectangle_f_t * ret,
                          const gavl_video_format_t * format)
  {
  ret->w = values[2] * format->image_width;
  ret->h = values[2] * format->image_height;
  ret->x = 0.5 * (format->image_width - ret->w);
  ret->y = 0.5 * (format->image_height - ret->h);

  ret->x += values[0] * 0.005 * (format->image_height + ret->h);
  ret->y -= values[1] * 0.005 * (format->image_width + ret->w);
  
  }
  
int bg_nle_video_compositor_read(void * priv, gavl_video_frame_t* ret, int stream)
  {
  gavl_time_t pts_unscaled;
  bg_nle_video_compositor_t * c = priv;
  int j;
  int i = c->num_streams;
  
  stream_t * s;
  
  pts_unscaled = gavl_time_unscale(c->format.timescale, c->pts);
  gavl_video_frame_fill(ret, &c->format, c->bg_color);
  
  while(--i)
    {
    s = &c->streams[i];
    
    if(!bg_nle_renderer_instream_video_request(s->s,
                                               pts_unscaled, s->id, s->camera, s->projector))
      continue;
    
    /* Check if the rectangles changed */

    if(!s->rect_changed)
      {
      for(j = 0; j < 3; j++)
        {
        if((fabs(s->camera[j] - s->last_camera[j]) > 1.0e-5) ||
           (fabs(s->projector[j] - s->last_projector[j]) > 1.0e-5))
          {
          s->rect_changed = 1;
          break;
          }
        }
      }
    
    if(s->rect_changed)
      {
      gavl_rectangle_f_t tmp_rect;
      memcpy(s->last_camera, s->camera, 3 * sizeof(s->last_camera[0]));
      memcpy(s->last_projector, s->projector, 3 * sizeof(s->last_projector[0]));
      
      set_rectangle(s->camera, &s->src_rect, &s->composite_format);
      set_rectangle(s->projector, &tmp_rect, &c->format);
      gavl_rectangle_f_to_i(&s->dst_rect, &tmp_rect);
      
      gavl_rectangle_crop_to_format_scale(&s->src_rect, &s->dst_rect, &s->composite_format, &c->format);

      gavl_video_options_set_rectangles(gavl_video_converter_get_options(s->cnv),
                                        &s->src_rect, &s->dst_rect);
      
      s->do_convert = gavl_video_converter_init(s->cnv, &s->src_format, &s->composite_format);
      s->rect_changed = 0;
      }
    
    c->streams[i].func(&c->streams[i], ret);
    }
  
  ret->timestamp = c->pts;
  ret->duration = c->format.frame_duration;
  c->pts += c->format.frame_duration;
  
  return 1;
  }


int bg_nle_video_compositor_seek(bg_nle_video_compositor_t * c,
                                 int64_t time)
  {
  c->pts = gavl_time_scale(c->format.timescale, time + 10);
  c->pts /= c->format.frame_duration;
  c->pts *= c->format.frame_duration;
  return 1;
  }


void bg_nle_video_compositor_destroy(bg_nle_video_compositor_t * c)
     
  {
  free(c);
  }

