#include <string.h>
#include <renderer.h>

struct bg_nle_video_compositor_s
  {
  int num_streams;
  
  struct
    {
    gavl_video_frame_t * frame;
    gavl_video_converter_t * cnv;
    int active;
    bg_nle_renderer_instream_video_t * s;
    } * streams;

  gavl_video_format_t format;
  float bg_color[4];
  bg_nle_outstream_t * os;
  int64_t pts;
  };

static void set_parameter(void * data, const char * name,
                          const bg_parameter_value_t * val)
  {
  bg_nle_video_compositor_t * c = data;
  if(!name)
    return;
  
  //  if(bg_gavl_video_set_parameter(&c->opt, name, val))
  //   return;
  
  if(!strcmp(name, "background"))
    {
    memcpy(c->bg_color, val->val_color, 4 * sizeof(float));
    }
  
  }

bg_nle_video_compositor_t *
bg_nle_video_compositor_create(bg_nle_outstream_t * os)
  {
  bg_nle_video_compositor_t * ret = calloc(1, sizeof(*ret));
  ret->os = os;
  return ret;
  }

void bg_nle_video_compositor_clear(bg_nle_video_compositor_t * c)
  {
  
  }

void bg_nle_video_compositor_add_stream(bg_nle_video_compositor_t * c,
                                        bg_nle_renderer_instream_video_t * s)
  {
  
  }

void
bg_nle_video_compositor_init(bg_nle_video_compositor_t * c,
                             gavl_video_format_t * format)
  {
  /* Initialize format */
  gavl_video_format_copy(format, &c->format);
  
  }

int bg_nle_video_compositor_read(bg_nle_video_compositor_t * c,
                                 gavl_video_frame_t * ret)
  {
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

