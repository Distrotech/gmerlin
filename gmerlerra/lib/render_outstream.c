
#include <stdlib.h>
#include <renderer.h>

struct bg_nle_renderer_outstream_s
  {
  bg_nle_outstream_t * s;
  bg_nle_project_t * p;
  
  bg_nle_audio_compositor_t * ac;
  bg_nle_video_compositor_t * vc;
  };

bg_nle_renderer_outstream_t *
bg_nle_renderer_outstream_create(bg_nle_project_t * p,
                                 bg_nle_outstream_t * s)
  {
  bg_nle_renderer_outstream_t * ret;
  ret = calloc(1, sizeof(*ret));
  ret->p = p;
  ret->s = s;
  
  switch(s->type)
    {
    case BG_NLE_TRACK_AUDIO:
      ret->ac = bg_nle_audio_compositor_create();
      break;
    case BG_NLE_TRACK_VIDEO:
      ret->vc = bg_nle_video_compositor_create();
      break;
    case BG_NLE_TRACK_NONE:
      break;
    }
  
  return ret;
  }

void bg_nle_renderer_outstream_destroy(bg_nle_renderer_outstream_t * s)
  {
  if(s->ac) bg_nle_audio_compositor_destroy(s->ac);
  if(s->vc) bg_nle_video_compositor_destroy(s->vc);
  free(s);
  }

int bg_nle_renderer_outstream_read_video(bg_nle_renderer_outstream_t * s,
                                        gavl_video_frame_t * ret)
  {
  return bg_nle_video_compositor_read(s->vc, ret);
  }

int bg_nle_renderer_outstream_read_audio(bg_nle_renderer_outstream_t * s,
                                        gavl_audio_frame_t * ret,
                                        int num_samples)
  {
  return bg_nle_audio_compositor_read(s->ac, ret, num_samples);
  }

void bg_nle_renderer_outstream_seek(bg_nle_renderer_outstream_t * s,
                                    int64_t time)
  {
  if(s->vc)
    bg_nle_video_compositor_seek(s->vc, time);
  else if(s->ac)
    bg_nle_audio_compositor_seek(s->ac, time);
  }

