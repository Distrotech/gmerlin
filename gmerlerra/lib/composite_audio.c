#include <string.h>
#include <renderer.h>

struct bg_nle_audio_compositor_s
  {
  int num_streams;
  int streams_alloc;
  
  struct
    {
    gavl_audio_frame_t * frame;
    gavl_audio_converter_t * cnv;
    bg_nle_renderer_instream_audio_t * s;
    bg_nle_track_t * t;
    } * streams;
  bg_nle_outstream_t * os;
  bg_gavl_audio_options_t * opt;
  gavl_audio_format_t format;
  };

bg_nle_audio_compositor_t *
bg_nle_audio_compositor_create(bg_nle_outstream_t * os,
                               bg_gavl_audio_options_t * opt,
                               const gavl_audio_format_t * format)
  {
  bg_nle_audio_compositor_t * ret = calloc(1, sizeof(*ret));
  ret->os = os;
  ret->opt = opt;
  gavl_audio_format_copy(&ret->format, format);
  return ret;
  }

void bg_nle_audio_compositor_clear(bg_nle_audio_compositor_t * c)
  {
  
  }

void bg_nle_audio_compositor_add_stream(bg_nle_audio_compositor_t * c,
                                        bg_nle_renderer_instream_audio_t * s,
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


int bg_nle_audio_compositor_read(void * priv, gavl_audio_frame_t* frame, int stream,
                                 int num_samples)
  {
  bg_nle_audio_compositor_t * c = priv;
  return 0;
  }


int bg_nle_audio_compositor_seek(bg_nle_audio_compositor_t * c,
                                 int64_t time)
  {
  return 1;
  }


void bg_nle_audio_compositor_destroy(bg_nle_audio_compositor_t * c)
     
  {
  free(c);
  }

