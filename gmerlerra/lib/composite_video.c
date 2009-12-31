#include <string.h>
#include <renderer.h>

struct bg_nle_video_compositor_s
  {
  int num_streams;
  int streams_alloc;
  
  struct
    {
    gavl_video_frame_t * frame;
    gavl_video_converter_t * cnv;
    bg_nle_renderer_instream_video_t * s;
    bg_nle_track_t * t;
    } * streams;

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

void bg_nle_video_compositor_add_stream(bg_nle_video_compositor_t * c,
                                        bg_nle_renderer_instream_video_t * s,
                                        bg_nle_track_t * t)
  {
  if(c->num_streams+1 > c->streams_alloc)
    {
    c->streams_alloc += 10;
    c->streams = realloc(c->streams,
                         c->streams_alloc * sizeof(*c->streams));
    memset(c->streams + c->num_streams, 0,
           (c->streams_alloc - c->num_streams) * sizeof(*c->streams));
    }
  c->streams[c->num_streams].t = t;
  c->streams[c->num_streams].s = s;
  c->num_streams++;
  }


int bg_nle_video_compositor_read(void * priv, gavl_video_frame_t* ret, int stream)
  {
  bg_nle_video_compositor_t * c = priv;
  int i = c->num_streams;

  gavl_video_frame_fill(ret, &c->format, c->bg_color);
  
  while(--i)
    {
    
    }

  ret->timestamp = c->pts;
  c->pts += c->format.frame_duration;
  
  return 1;
  }


int bg_nle_video_compositor_seek(bg_nle_video_compositor_t * c,
                                 int64_t time)
  {
  return 0;
  }


void bg_nle_video_compositor_destroy(bg_nle_video_compositor_t * c)
     
  {
  free(c);
  }

