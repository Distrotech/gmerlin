#include <stdlib.h>
#include <string.h>
#include <renderer.h>
#include <framecache.h>

#include <gmerlin/filters.h>

typedef struct
  {
  int scale;
  int64_t out_pts;
  int64_t in_pts;
  int64_t request_duration;
  int cur_seg;
  int next_seg;
  int64_t seq; /* Sequence number for the frame cache */
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
  bg_nle_file_handle_t * file;

  /* In segment timescale */
  int64_t in_pts;
  int64_t in_pts_end;
  
  int64_t fc_pts;
  int64_t fc_pts_end;
  
  } common_t;

struct bg_nle_renderer_instream_audio_s
  {
  common_t com;
  gavl_audio_format_t format;
  gavl_audio_format_t *in_format;

  bg_audio_filter_chain_t * fc;
  bg_gavl_audio_options_t opt;
  gavl_audio_converter_t * cnv;

  bg_nle_frame_cache_t * cache;

  gavl_audio_frame_t * frame;
  int overlay_mode;
  };

struct bg_nle_renderer_instream_video_s
  {
  common_t com;
  gavl_video_format_t format;
  gavl_video_format_t *in_format;
  bg_video_filter_chain_t * fc;
  bg_gavl_video_options_t opt;
  int overlay_mode;
  gavl_video_converter_t * cnv;

  bg_nle_frame_cache_t * cache;
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

/* Callbacks for reading from plugins */

static int load_file(common_t * com, bg_nle_track_type_t type)
  {
  int64_t t;
  if(com->cur_seg < 0)
    return 0;
  
  com->file = bg_nle_file_cache_load(com->c, com->t->segments[com->cur_seg].file_id,
                                     type,
                                     com->t->segments[com->cur_seg].stream);
  if(!com->file)
    return 0;

  t = com->t->segments[com->cur_seg].src_pos;

  com->file->plugin->seek(com->file->h->priv,
                          &t,
                          com->t->segments[com->cur_seg].scale);
  com->in_pts = t;
  com->in_pts_end = com->in_pts + gavl_time_scale(com->t->segments[com->cur_seg].scale,
                                                  com->t->segments[com->cur_seg].len);

  /* TODO: This breaks if the filterchain changes the timescale */
  com->fc_pts     = com->t->segments[com->cur_seg].dst_pos;
  com->fc_pts_end = com->t->segments[com->cur_seg].dst_pos + com->t->segments[com->cur_seg].len;
  }

static int read_audio_from_plugin(void * priv, gavl_audio_frame_t* frame, int stream,
                                  int num_samples)
  {
  bg_nle_renderer_instream_audio_t * s = priv;
  int result;
  int samples_to_read;
  
  gavl_audio_frame_mute_samples(frame, s->in_format, num_samples);
  samples_to_read = num_samples;

  if(s->com.in_pts + samples_to_read > s->com.in_pts_end)
    samples_to_read = s->com.in_pts_end - s->com.in_pts;
  
  result = s->com.file->plugin->read_audio(s->com.file->h->priv, frame, s->com.file->stream, samples_to_read);

  /* Fake more samples if the filter chain has a latency */
  frame->valid_samples = num_samples;
  
  s->com.in_pts += num_samples;
  return frame->valid_samples;
  }

static int read_video_from_plugin(void * priv, gavl_video_frame_t* frame, int stream)
  {
  bg_nle_renderer_instream_video_t * s = priv;
  int result;
  
  if(!s->com.file)
    {
    if(!load_file(&s->com, BG_NLE_TRACK_VIDEO))
      return 0;
    s->in_format = &s->com.file->ti->video_streams[s->com.file->stream].format;
    bg_video_filter_chain_set_input_format(s->fc, s->in_format);
    }
  result = s->com.file->plugin->read_video(s->com.file->h->priv, frame, s->com.file->stream);

  return result;
  }

/* Callbacks for reading from filter chains */

static void get_audio_frame(void * data, bg_nle_frame_cache_entry_t * e)
  {
  bg_nle_renderer_instream_audio_t * s = data;
  int samples_read = 0;
  int samples_to_read;
  
  if(!e->frame)
    e->frame = gavl_audio_frame_create(&s->format);

  /* TODO: Filter automation */

  gavl_audio_frame_mute(e->frame, &s->format);

  e->time = s->com.fc_pts;
  
  while(samples_read < s->format.samples_per_frame)
    {
    samples_to_read = s->format.samples_per_frame - samples_read;

    if(s->com.fc_pts + samples_to_read > s->com.fc_pts_end)
      samples_to_read = s->com.fc_pts_end - s->com.fc_pts;
    
    if(s->com.cur_seg >= 0)
      {
      bg_audio_filter_chain_read(s->fc, s->frame, 0, samples_to_read);
      gavl_audio_frame_copy(&s->format, e->frame, s->frame,
                            samples_read, // dst_pos
                            0,            // src_pos
                            s->format.samples_per_frame - samples_read, // dst_size
                            s->frame->valid_samples);  // src_size
      }
    samples_read += samples_to_read;
    s->com.fc_pts += samples_to_read;
    
    /* Update segments */

    if(s->com.fc_pts >= s->com.fc_pts_end)
      {
      if(s->com.cur_seg >= 0) /* Check for segment end */
        {
        s->com.cur_seg = -1;

        /* Unload file */
        bg_nle_file_cache_unload(s->com.c, s->com.file);
        s->com.file = NULL;
        }
      
      if(s->com.cur_seg < 0) /* Check for segment start */
        {
        if((s->com.next_seg >= 0) && (s->com.t->segments[s->com.next_seg].dst_pos <= s->com.fc_pts))
          {
          s->com.cur_seg = s->com.next_seg;
          if(s->com.cur_seg < s->com.t->num_segments - 1)
            s->com.next_seg = s->com.cur_seg+1;
          
          if(!load_file(&s->com, BG_NLE_TRACK_AUDIO))
            return;
          s->in_format = &s->com.file->ti->audio_streams[s->com.file->stream].format;
          bg_audio_filter_chain_set_input_format(s->fc, s->in_format);
          }
        }
      
      }
    }

  e->duration = s->com.fc_pts - e->time;
  }

static void free_audio_frame(void * data, bg_nle_frame_cache_entry_t * e)
  {
  if(e->frame)
    gavl_audio_frame_destroy(e->frame);
  }

static void get_video_frame(void * data, bg_nle_frame_cache_entry_t * e)
  {
  bg_nle_renderer_instream_video_t * s = data;
  if(!e->frame)
    e->frame = gavl_video_frame_create(&s->format);

  /* TODO: Filter automation */

  if((s->com.cur_seg >= 0) && (s->com.in_pts >= s->com.in_pts_end))
    {
    
    }
  
  
  
  }

static void free_video_frame(void * data, bg_nle_frame_cache_entry_t * e)
  {
  if(e->frame)
    gavl_video_frame_destroy(e->frame);
  }

static const gavl_audio_format_t default_audio_format =
  {
    .samplerate = 48000,
    .num_channels = 2,
    .interleave_mode = GAVL_INTERLEAVE_NONE,
    .sample_format = GAVL_SAMPLE_FLOAT,
    .channel_locations = { GAVL_CHID_FRONT_LEFT, GAVL_CHID_FRONT_RIGHT },
  };

static const gavl_video_format_t default_video_format =
  {
    .image_width = 640,
    .image_height = 480,
    .frame_width = 640,
    .frame_height = 480,
    .pixel_width = 1,
    .pixel_height = 1,
    .pixelformat = GAVL_RGB_24,
    .timescale = 25,
    .frame_duration = 1,
  };

bg_nle_renderer_instream_audio_t *
bg_nle_renderer_instream_audio_create(bg_nle_project_t * p,
                                      bg_nle_track_t * t,
                                      const gavl_audio_options_t * opt,
                                      bg_nle_file_cache_t * c)
  {
  bg_nle_renderer_instream_audio_t * ret;
  
  ret = calloc(1, sizeof(*ret));

  init_common(&ret->com, p, t, c);
  
  ret->cnv = gavl_audio_converter_create();
  bg_gavl_audio_options_init(&ret->opt);
  
  gavl_audio_options_copy(ret->opt.opt, opt);
  
  ret->fc = bg_audio_filter_chain_create(&ret->opt, p->plugin_reg);
  bg_audio_filter_chain_connect_input(ret->fc, read_audio_from_plugin, ret, 0);

  if(ret->com.cur_seg >= 0)
    {
    if(!load_file(&ret->com, BG_NLE_TRACK_AUDIO))
      return NULL;
    ret->in_format =
      &ret->com.file->ti->audio_streams[ret->com.file->stream].format;
    bg_audio_filter_chain_init(ret->fc, ret->in_format, &ret->format);
    }
  else
    {
    /* Use default format to initialize the chain */
    bg_audio_filter_chain_init(ret->fc, &default_audio_format, &ret->format);
    }
  
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
  bg_video_filter_chain_connect_input(ret->fc, read_video_from_plugin, ret, 0);

  if(ret->com.cur_seg >= 0)
    {
    if(!load_file(&ret->com, BG_NLE_TRACK_VIDEO))
      return NULL;
    ret->in_format = &ret->com.file->ti->video_streams[ret->com.file->stream].format;
    bg_video_filter_chain_init(ret->fc, ret->in_format, &ret->format);
    }
  else
    {
    /* Use default format to initialize the chain */
    bg_video_filter_chain_init(ret->fc, &default_video_format, &ret->format);
    }
  
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
  int ret;
  if(s->out_pts != time)
    {
    /* Seeked */
    }

  /* Check for segment end */
  if((s->cur_seg >= 0) &&
     gavl_time_unscale(s->scale, time) >
     com->t->segments[s->cur_seg].dst_pos + com->t->segments[s->cur_seg].len)
    s->cur_seg = -1;
  
  s->request_duration = duration;
  
  if(s->cur_seg < 0) /* Check for segment end */
    {
    if(s->next_seg < 0)
      ret = 0; /* EOS */

    else if(gavl_time_unscale(s->scale, time) >=
            com->t->segments[s->next_seg].dst_pos)
      {
      /* Switch to next segment immediately */
      s->cur_seg = s->next_seg;
      if(s->cur_seg < com->t->num_segments-1)
        s->next_seg = s->cur_seg+1;
      else
        s->next_seg = -1;
      ret = 1;
      }
    else if(gavl_time_unscale(s->scale, time + duration) >
            com->t->segments[s->next_seg].dst_pos)
      {
      /* Next segment is about to come during that time */
      ret = 1;
      }
    else
      ret = 0;
    }
  else
    ret = 1;
  
  if(!ret)
    {
    s->out_pts += s->request_duration;
    }
  return ret;
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

int bg_nle_renderer_instream_audio_request(bg_nle_renderer_instream_audio_t * s,
                                           int64_t time, int64_t duration, int stream)
  {
  return request_common(&s->com, time, duration, stream);
  }

int bg_nle_renderer_instream_video_read(bg_nle_renderer_instream_video_t * s,
                                        gavl_video_frame_t * ret, int stream)
  {
  bg_nle_frame_cache_entry_t * e;
  output_stream_t * os = &s->com.output_streams[stream];

  if(os->cur_seg < 0)
    {
    os->cur_seg = os->next_seg;
    if(os->cur_seg < s->com.t->num_segments-1)
      os->next_seg = os->cur_seg+1;
    else
      os->next_seg = -1;
    }

  while(1)
    {
    e = bg_nle_frame_cache_get(s->cache, os->seq);
    
    }

  os->out_pts += os->request_duration;

  return 1;
  }

int bg_nle_renderer_instream_audio_read(bg_nle_renderer_instream_audio_t * s,
                                        gavl_audio_frame_t * ret,
                                        int stream)
  {
  int samples_read = 0;
  bg_nle_frame_cache_entry_t * e;
  output_stream_t * os = &s->com.output_streams[stream];

  gavl_audio_frame_mute_samples(ret, &s->format, os->request_duration);
  
  os->out_pts += os->request_duration;
  
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
                                              gavl_audio_format_t * format, int * overlay_mode,
                                              int scale)
  {
  *overlay_mode = s->overlay_mode;
  gavl_audio_format_copy(format, &s->format);
  return connect_output_common(&s->com, scale);
  }
