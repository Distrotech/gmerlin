#include <converters.h>

#include <config.h>
#include <translation.h>


#include <log.h>
#define LOG_DOMAIN "converters"

#define ABSDIFF(x,y) ((x)>(y)?(x)-(y):(y)-(x))

struct bg_audio_converter_s
  {
  gavl_audio_converter_t * cnv;
  const gavl_audio_options_t * opt;
  gavl_audio_frame_t * frame;

  /* Input stuff */
  bg_read_audio_func_t read_func;
  void * read_priv;
  int read_stream;

  gavl_audio_format_t frame_format;
  int in_rate;
  int out_rate;
  int last_samples;
  };

bg_audio_converter_t * bg_audio_converter_create(const gavl_audio_options_t * opt)
  {
  bg_audio_converter_t * ret;
  ret = calloc(1, sizeof(*ret));
  ret->opt = opt;
  ret->cnv = gavl_audio_converter_create();
  return ret;
  }

int bg_audio_converter_init(bg_audio_converter_t * cnv,
                            const gavl_audio_format_t * in_format,
                            const gavl_audio_format_t * out_format)
  {
  int result;
  gavl_audio_options_t * cnv_opt;
  
  /* Free previous stuff */
  if(cnv->frame)
    {
    gavl_audio_frame_destroy(cnv->frame);
    cnv->frame = (gavl_audio_frame_t*)0;
    }
  /* Set options */
  cnv_opt = gavl_audio_converter_get_options(cnv->cnv);
  gavl_audio_options_copy(cnv_opt, cnv->opt);
  
  result = gavl_audio_converter_init(cnv->cnv, in_format, out_format);

  if(result)
    {
    gavl_audio_format_copy(&cnv->frame_format, in_format);
    cnv->in_rate = in_format->samplerate;
    cnv->out_rate = out_format->samplerate;
    cnv->last_samples = 0;
    }
  return result;
  }

void bg_audio_converter_connect_input(bg_audio_converter_t * cnv,
                                      bg_read_audio_func_t func, void * priv,
                                      int stream)
  {
  cnv->read_func = func;
  cnv->read_priv = priv;
  cnv->read_stream = stream;
  }

int bg_audio_converter_read(void * priv, gavl_audio_frame_t* frame, int stream,
                            int num_samples)
  {
  bg_audio_converter_t * cnv = (bg_audio_converter_t *)priv;

  if(cnv->last_samples != num_samples)
    {
    if(cnv->frame && (cnv->last_samples < num_samples))
      {
      gavl_audio_frame_destroy(cnv->frame);
      cnv->frame = (gavl_audio_frame_t*)0;
      }
    if(cnv->in_rate != cnv->out_rate)
      {
      cnv->frame_format.samples_per_frame =
        ((num_samples - 10) * cnv->in_rate) / cnv->out_rate;
      }
    else
      cnv->frame_format.samples_per_frame = num_samples;

    cnv->last_samples = num_samples;
    }
  if(!cnv->frame)
    cnv->frame = gavl_audio_frame_create(&cnv->frame_format);

  cnv->read_func(cnv->read_priv, cnv->frame, cnv->read_stream, cnv->frame_format.samples_per_frame);
  gavl_audio_convert(cnv->cnv, cnv->frame, frame);
  return frame->valid_samples;
  }

void bg_audio_converter_destroy(bg_audio_converter_t * cnv)
  {
  if(cnv->frame)
    gavl_audio_frame_destroy(cnv->frame);
  if(cnv->cnv)
    gavl_audio_converter_destroy(cnv->cnv);
  free(cnv);
  }

/* Video */

struct bg_video_converter_s
  {
  gavl_video_converter_t * cnv;
  const gavl_video_options_t * opt;
  gavl_video_frame_t * frame;
  gavl_video_frame_t * next_frame;
  
  /* Input stuff */
  bg_read_video_func_t read_func;
  void * read_priv;
  int read_stream;
  
  int out_pts;
  
  int convert_gavl;
  int convert_framerate;

  gavl_video_format_t in_format;
  gavl_video_format_t out_format;

  int eof;
  };

bg_video_converter_t * bg_video_converter_create(const gavl_video_options_t * opt)
  {
  bg_video_converter_t * ret;
  ret = calloc(1, sizeof(*ret));
  ret->opt = opt;
  ret->cnv = gavl_video_converter_create();
  return ret;

  }

int bg_video_converter_init(bg_video_converter_t * cnv,
                            const gavl_video_format_t * in_format,
                            const gavl_video_format_t * out_format)
  {
  gavl_video_options_t * cnv_opt;
  int ret;
  
  /* Free previous stuff */
  if(cnv->frame)
    {
    gavl_video_frame_destroy(cnv->frame);
    cnv->frame = (gavl_video_frame_t*)0;
    }
  if(cnv->next_frame)
    {
    gavl_video_frame_destroy(cnv->next_frame);
    cnv->next_frame = (gavl_video_frame_t*)0;
    }

  /* Copy format */
  gavl_video_format_copy(&cnv->in_format, in_format);
  gavl_video_format_copy(&cnv->out_format, out_format);
  
  /* Set options */
  cnv_opt = gavl_video_converter_get_options(cnv->cnv);
  gavl_video_options_copy(cnv_opt, cnv->opt);
  
  cnv->convert_gavl = gavl_video_converter_init(cnv->cnv, in_format, out_format);

  if(out_format->framerate_mode == GAVL_FRAMERATE_CONSTANT)
    {
    if( (in_format->framerate_mode != GAVL_FRAMERATE_CONSTANT) ||
        (in_format->timescale * out_format->frame_duration !=
         out_format->timescale * in_format->frame_duration) )
      {
      cnv->convert_framerate = 1;
      bg_log(BG_LOG_INFO, LOG_DOMAIN, "Doing framerate conversion %5.2f (%s) -> %5.2f (%s)",
             (float)(cnv->in_format.timescale) / (float)(cnv->in_format.frame_duration),
             (cnv->in_format.framerate_mode == GAVL_FRAMERATE_VARIABLE ? "nonconstant" : "constant"),
             (float)(cnv->out_format.timescale) / (float)(cnv->out_format.frame_duration),
             (cnv->out_format.framerate_mode == GAVL_FRAMERATE_VARIABLE ? "nonconstant" : "constant"));
      }
    }
  
  if(cnv->convert_gavl || cnv->convert_framerate)
    {
    cnv->frame = gavl_video_frame_create(in_format);
    gavl_video_frame_clear(cnv->frame, in_format);
    cnv->frame->time_scaled = GAVL_TIME_UNDEFINED;
    }
  if(cnv->convert_framerate)
    {
    cnv->next_frame = gavl_video_frame_create(in_format);
    gavl_video_frame_clear(cnv->next_frame, in_format);
    cnv->next_frame->time_scaled = GAVL_TIME_UNDEFINED;
    }
  cnv->out_pts = 0;
  cnv->eof = 0;

  ret = cnv->convert_framerate + cnv->convert_gavl;
  
  bg_log(BG_LOG_DEBUG, LOG_DOMAIN, "Initialized video converter, %d steps",
         ret);
  return ret;
  }

void bg_video_converter_connect_input(bg_video_converter_t * cnv,
                                      bg_read_video_func_t func, void * priv,
                                      int stream)
  {
  cnv->read_func = func;
  cnv->read_priv = priv;
  cnv->read_stream = stream;
  }

int bg_video_converter_read(void * priv, gavl_video_frame_t* frame, int stream)
  {
  int64_t in_pts;
  int result;
  bg_video_converter_t * cnv = (bg_video_converter_t *)priv;
  gavl_video_frame_t * tmp_frame;
  
  
  if(!cnv->convert_framerate)
    {
    result = cnv->read_func(cnv->read_priv, cnv->frame, cnv->read_stream);
    if(result)
      gavl_video_convert(cnv->cnv, cnv->frame, frame);
    return result;
    }
  else
    {
    /* Read first frames */
    if((cnv->frame->time_scaled == GAVL_TIME_UNDEFINED) &&
       !cnv->read_func(cnv->read_priv, cnv->frame, cnv->read_stream))
      return 0;

    if((cnv->next_frame->time_scaled == GAVL_TIME_UNDEFINED) &&
       !cnv->read_func(cnv->read_priv, cnv->next_frame, cnv->read_stream))
      return 0;
    
    in_pts = gavl_time_rescale(cnv->out_format.timescale,
                               cnv->out_format.timescale,
                               cnv->out_pts);
    /* Last frame was already returned */
    if(cnv->eof)
      return 0;
    else if(in_pts >= cnv->next_frame->time_scaled)
      {
      do{
        tmp_frame = cnv->frame;
        cnv->frame = cnv->next_frame;
        cnv->next_frame = tmp_frame;
        
        result = cnv->read_func(cnv->read_priv, cnv->next_frame, cnv->read_stream);
        if(!result)
          {
          cnv->eof = 1;
          break;
          }
        } while(cnv->next_frame->time_scaled < cnv->next_frame->time_scaled);

      if(cnv->eof)
        tmp_frame = cnv->next_frame;
      else if(ABSDIFF(cnv->next_frame->time_scaled, in_pts) <
              ABSDIFF(cnv->frame->time_scaled, in_pts))
        tmp_frame = cnv->next_frame;
      else
        tmp_frame = cnv->frame;
      
      if(cnv->convert_gavl)
        gavl_video_convert(cnv->cnv, tmp_frame, frame);
      else
        gavl_video_frame_copy(&cnv->out_format, tmp_frame, frame);
      
      frame->time_scaled = cnv->out_pts;
      cnv->out_pts += cnv->out_format.frame_duration;
      }
    }
  return 1;
  }

void bg_video_converter_destroy(bg_video_converter_t * cnv)
  {
  if(cnv->frame)
    gavl_video_frame_destroy(cnv->frame);
  if(cnv->next_frame)
    gavl_video_frame_destroy(cnv->next_frame);
  if(cnv->cnv)
    gavl_video_converter_destroy(cnv->cnv);
  free(cnv);
  }

void bg_video_converter_reset(bg_video_converter_t * cnv, int64_t out_pts)
  {
  if(cnv->frame)      cnv->frame->time_scaled      = GAVL_TIME_UNDEFINED;
  if(cnv->next_frame) cnv->next_frame->time_scaled = GAVL_TIME_UNDEFINED;

  cnv->eof = 0;
  cnv->out_pts = out_pts;
  }
