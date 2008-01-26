#include <gmerlin_effectv.h>

void bg_effectv_connect_input_port(void * priv,
                                   bg_read_video_func_t func,
                                   void * data, int stream, int port)
  {
  bg_effectv_plugin_t * vp;
  vp = (bg_effectv_plugin_t *)priv;
  
  if(!port)
    {
    vp->read_func = func;
    vp->read_data = data;
    vp->read_stream = stream;
    }
  }

void bg_effectv_set_input_format(void * priv, gavl_video_format_t * format, int port)
  {
  bg_effectv_plugin_t * vp;
  vp = (bg_effectv_plugin_t *)priv;
  
  if(port)
    return;

  /* TODO: bswap for big endian */
  format->pixelformat = GAVL_BGR_32;

  if(vp->started)
    {
    vp->e->stop(vp->e);
    vp->started = 0;
    }
  
  gavl_video_format_copy(&vp->format, format);

  vp->e->video_width = vp->format.image_width;
  vp->e->video_height = vp->format.image_height;
  vp->e->video_area = vp->format.image_width * vp->format.image_height;

  vp->e->start(vp->e);
  vp->started = 1;
  
  if(vp->in_frame)
    {
    gavl_video_frame_destroy(vp->in_frame);
    vp->in_frame = (gavl_video_frame_t*)0;
    }
  if(vp->out_frame)
    {
    gavl_video_frame_destroy(vp->out_frame);
    vp->out_frame = (gavl_video_frame_t*)0;
    }
  }


void * bg_effectv_create(effectRegisterFunc * f, int flags)
  {
  bg_effectv_plugin_t * ret;
  ret = calloc(1, sizeof(*ret));
  ret->e = f();
  ret->flags = flags;
  return ret;
  }

void bg_effectv_destroy(void*priv)
  {
  bg_effectv_plugin_t * p;
  p = (bg_effectv_plugin_t *)priv;
  
  if(p->e)
    {
    if(p->e->stop)              p->e->stop(p->e);
    if(p->e->priv)              free(p->e->priv);
    if(p->e->yuv2rgb)           free(p->e->yuv2rgb);
    if(p->e->rgb2yuv)           free(p->e->rgb2yuv);
    if(p->e->stretching_buffer) free(p->e->stretching_buffer);
    if(p->e->background)        free(p->e->background);
    if(p->e->diff)              free(p->e->diff);
    if(p->e->diff2)             free(p->e->diff2);
    }
  free(p);
  }

void bg_effectv_get_output_format(void * priv, gavl_video_format_t * format)
  {
  bg_effectv_plugin_t * vp;
  vp = (bg_effectv_plugin_t *)priv;
  gavl_video_format_copy(format, &vp->format);
  }

int bg_effectv_read_video(void * priv, gavl_video_frame_t * frame, int stream)
  {
  bg_effectv_plugin_t * vp;
  vp = (bg_effectv_plugin_t *)priv;

  if(!vp->in_frame)
    {
    vp->in_frame = gavl_video_frame_create_nopad(&vp->format);
    gavl_video_frame_clear(vp->in_frame, &vp->format);
    }
  if(!vp->read_func(vp->read_data, vp->in_frame, vp->read_stream))
    return 0;

  /* Frame not padded, good */
  if((frame->strides[0] == vp->format.image_width * 4) &&
     !(vp->flags & BG_EFFECTV_REUSE_OUTPUT))
    {
    vp->e->draw(vp->e, (RGB32*)vp->in_frame->planes[0],
                (RGB32*)frame->planes[0]);
    }
  else
    {
    if(!vp->out_frame)
      {
      vp->out_frame = gavl_video_frame_create_nopad(&vp->format);
      gavl_video_frame_clear(vp->in_frame, &vp->format);
      }
    vp->e->draw(vp->e, (RGB32*)vp->in_frame->planes[0],
                (RGB32*)vp->out_frame->planes[0]);
    gavl_video_frame_copy(&vp->format, frame, vp->out_frame);
    }
  frame->timestamp = vp->in_frame->timestamp;
  frame->duration = vp->in_frame->duration;
  return 1;
  }
