#include <renderer.h>

struct bg_nle_video_compositor_s
  {
  int num_streams;
  
  struct
    {
    gavl_video_frame_t * frame;
    gavl_video_converter_t * cnv;
    int active;
    bg_nle_renderer_instream_t * s;
    } * streams;
  };

bg_nle_video_compositor_t *
bg_nle_video_compositor_create()
  {
  bg_nle_video_compositor_t * ret = calloc(1, sizeof(*ret));
  return ret;
  }

void bg_nle_video_compositor_clear(bg_nle_video_compositor_t * c)
  {
  
  }

void bg_nle_video_compositor_add_stream(bg_nle_video_compositor_t * c,
                                        bg_nle_renderer_instream_t * s)
  {
  
  }


int bg_nle_video_compositor_read(bg_nle_video_compositor_t * c,
                                 gavl_video_frame_t * ret)
  {
  int i = c->num_streams;

  while(--i)
    {
    
    }

  return 0;
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

