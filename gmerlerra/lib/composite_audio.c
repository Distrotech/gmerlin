#include <string.h>
#include <renderer.h>

typedef struct stream_s stream_t;

typedef void (*composite_func_t)(stream_t * stream,
                                 gavl_audio_frame_t * src, gavl_audio_frame_t * dst);

struct stream_s
  {
  gavl_audio_frame_t * frame;
  gavl_audio_frame_t * frame_cnv;
  gavl_audio_converter_t * cnv;
  bg_nle_renderer_instream_audio_t * s;
  bg_nle_track_t * t;
  
  gavl_audio_format_t in_format;
  gavl_audio_format_t *out_format;
  int id;
  composite_func_t func;
  int overlay_mode;
  int do_convert;
  };

struct bg_nle_audio_compositor_s
  {
  int num_streams;
  int streams_alloc;
  
  stream_t * streams;
  bg_nle_outstream_t * os;
  bg_gavl_audio_options_t * opt;
  gavl_audio_format_t format;
  };

static void composite_replace(stream_t * stream,
                              gavl_audio_frame_t * src, gavl_audio_frame_t * dst)
  {
  gavl_audio_frame_copy(stream->out_format, dst, src, 0, 0, src->valid_samples, src->valid_samples);
  }

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

  c->streams[c->num_streams].id =
    bg_nle_renderer_instream_audio_connect_output(s,
                                                  &c->streams[c->num_streams].in_format,
                                                  &c->streams[c->num_streams].overlay_mode,
                                                  c->format.samplerate);
  
  c->streams[c->num_streams].out_format = &c->format;
  
  c->streams[c->num_streams].cnv = gavl_audio_converter_create();
  gavl_audio_options_copy(gavl_audio_converter_get_options(c->streams[c->num_streams].cnv), c->opt->opt);
  
  c->streams[c->num_streams].do_convert =
    gavl_audio_converter_init(c->streams[c->num_streams].cnv,
                              &c->streams[c->num_streams].in_format, &c->format);

  c->streams[c->num_streams].frame = gavl_audio_frame_create(&c->streams[c->num_streams].in_format);
  if(c->streams[c->num_streams].do_convert)
    c->streams[c->num_streams].frame_cnv = gavl_audio_frame_create(&c->format);

  switch(c->streams[c->num_streams].overlay_mode)
    {
    case BG_NLE_OVERLAY_REPLACE:
      c->streams[c->num_streams].func = composite_replace;
      break;
    case BG_NLE_OVERLAY_ADD:
      break;
    }
  
  c->num_streams++;
  }

int bg_nle_audio_compositor_read(void * priv, gavl_audio_frame_t* ret, int stream,
                                 int num_samples)
  {
  bg_nle_audio_compositor_t * c = priv;
  stream_t * s;

  int i = c->num_streams;

  gavl_audio_frame_mute(ret, &c->format);
  
  while(--i)
    {
    s = &c->streams[i];
    }
  
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

