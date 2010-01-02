#include <stdlib.h>
#include <string.h>
#include <renderer.h>

#include <gmerlin/filters.h>

typedef struct
  {
  void * frame;
  int64_t time;
  int64_t duration;
  } bg_nle_frame_cache_entry_t;


typedef struct
  {
  bg_nle_frame_cache_entry_t * entries;
  int num_entries;
  int entries_alloc;
  } bg_nle_frame_cache_t;

typedef struct
  {
  int scale;
  int64_t out_pts;
  int64_t in_pts;
  int64_t request_duration;
  } output_stream_t;

typedef struct
  {
  bg_nle_track_t * t;
  bg_nle_project_t * p;
  int num_output_streams;
  int output_streams_alloc;
  output_stream_t * output_streams;
  int cur_seg;
  int next_seg;
  bg_nle_file_cache_t * c;
  } common_t;

struct bg_nle_renderer_instream_audio_s
  {
  common_t com;
  gavl_audio_format_t format;
  bg_audio_filter_chain_t * fc;
  bg_gavl_audio_options_t opt;
  gavl_audio_converter_t * cnv;
  };

struct bg_nle_renderer_instream_video_s
  {
  common_t com;
  gavl_video_format_t format;
  bg_video_filter_chain_t * fc;
  bg_gavl_video_options_t opt;
  int overlay_mode;
  gavl_video_converter_t * cnv;
  };

static void init_common(common_t * com,  bg_nle_project_t * p,
                        bg_nle_track_t * t,
                        bg_nle_file_cache_t * c)
  {
  com->p = p;
  com->c = c;
  com->t = t;

  if(com->t->num_segments)
    {
    if(com->t->segments[0].dst_pos > 0)
      {
      com->cur_seg = -1;
      com->next_seg =  0;
      }
    else
      {
      com->cur_seg = 0;
      com->next_seg = com->t->num_segments > 1 ? 1 : -1;
      }
    }
  else
    {
    com->cur_seg = -1;
    com->next_seg = -1;
    }
  }


bg_nle_renderer_instream_audio_t *
bg_nle_renderer_instream_audio_create(bg_nle_project_t * p,
                                      bg_nle_track_t * t,
                                      const gavl_audio_options_t * opt, bg_nle_file_cache_t * c)
  {
  bg_nle_renderer_instream_audio_t * ret;
  ret = calloc(1, sizeof(*ret));

  init_common(&ret->com, p, t, c);
  
  ret->cnv = gavl_audio_converter_create();
  bg_gavl_audio_options_init(&ret->opt);
  
  gavl_audio_options_copy(ret->opt.opt, opt);
  
  ret->fc = bg_audio_filter_chain_create(&ret->opt, p->plugin_reg);
  
  return ret;
  }

bg_nle_renderer_instream_video_t *
bg_nle_renderer_instream_video_create(bg_nle_project_t * p,
                                      bg_nle_track_t * t,
                                      const gavl_video_options_t * opt,
                                      bg_nle_file_cache_t * c)
  {
  bg_nle_renderer_instream_video_t * ret;
  ret = calloc(1, sizeof(*ret));

  init_common(&ret->com, p, t, c);

  ret->cnv = gavl_video_converter_create();
  bg_gavl_video_options_init(&ret->opt);
  
  
  bg_gavl_video_options_init(&ret->opt);
  gavl_video_options_copy(ret->opt.opt, opt);

  ret->fc = bg_video_filter_chain_create(&ret->opt, p->plugin_reg);
  
  return ret;
  }

void bg_nle_renderer_instream_audio_destroy(bg_nle_renderer_instream_audio_t * s)
  {
  bg_gavl_audio_options_free(&s->opt);
  free(s);
  }

void bg_nle_renderer_instream_video_destroy(bg_nle_renderer_instream_video_t * s)
  {
  bg_gavl_video_options_free(&s->opt);
  free(s);
  }

static int request_common(common_t * com, int64_t time, int64_t duration, int stream)
  {
  output_stream_t * s = &com->output_streams[stream];
  if(s->out_pts != time)
    {
    /* Seeked */
    }
  if(com->cur_seg < 0)
    {
    if(com->next_seg < 0)
      return 0; /* EOS */
    
    //    com->t->segments[com->next_seg];
    }
  }

int bg_nle_renderer_instream_video_request(bg_nle_renderer_instream_video_t * s,
                                           int64_t time, int64_t duration,
                                           int stream, float * camera, float * projector)
  {
  if(request_common(&s->com, time, duration, stream))
    {
    camera[0] = 0.0;
    camera[1] = 0.0;
    camera[2] = 1.0;
    projector[0] = 0.0;
    projector[1] = 0.0;
    projector[2] = 1.0;
    return 1;
    }
  return 0;
  }

int bg_nle_renderer_instream_video_read(bg_nle_renderer_instream_video_t * s,
                                        gavl_video_frame_t * ret, int stream)
  {
  
  return 0;
  }

int bg_nle_renderer_instream_audio_request(bg_nle_renderer_instream_audio_t * s,
                                           int64_t time, int64_t duration, int stream)
  {
  return request_common(&s->com, time, duration, stream);
  }

int bg_nle_renderer_instream_audio_read(bg_nle_renderer_instream_audio_t * s,
                                        gavl_audio_frame_t * ret,
                                        int num_samples, int stream)
  {
  return 0;
  }

static int connect_output_common(common_t * com, int scale)
  {
  int ret;
  ret = com->num_output_streams;
  if(com->num_output_streams+1 > com->output_streams_alloc)
    {
    com->output_streams_alloc += 10;
    com->output_streams = realloc(com->output_streams,
                                com->output_streams_alloc * sizeof(*com->output_streams));

    memset(com->output_streams + com->num_output_streams,
           0, sizeof(*com->output_streams) *
           (com->output_streams_alloc - com->num_output_streams));
    }

  com->output_streams[com->num_output_streams].scale = scale;
  com->num_output_streams++;
  return ret;
  }

int
bg_nle_renderer_instream_video_connect_output(bg_nle_renderer_instream_video_t * s,
                                              gavl_video_format_t * format,
                                              int * overlay_mode, int scale)
  {
  gavl_video_format_copy(format, &s->format);
  *overlay_mode = s->overlay_mode;
  return connect_output_common(&s->com, scale);
  }

int
bg_nle_renderer_instream_audio_connect_output(bg_nle_renderer_instream_audio_t * s,
                                              gavl_audio_format_t * format, int scale)
  {
  gavl_audio_format_copy(format, &s->format);
  return connect_output_common(&s->com, scale);
  }
