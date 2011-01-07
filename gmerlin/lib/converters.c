/*****************************************************************
 * gmerlin - a general purpose multimedia framework and applications
 *
 * Copyright (c) 2001 - 2011 Members of the Gmerlin project
 * gmerlin-general@lists.sourceforge.net
 * http://gmerlin.sourceforge.net
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 * *****************************************************************/

#include <gmerlin/converters.h>

#include <config.h>
#include <gmerlin/translation.h>
#include <gmerlin/utils.h>


#include <gmerlin/log.h>
#define LOG_DOMAIN "converters"

#define ABSDIFF(x,y) ((x)>(y)?(x)-(y):(y)-(x))

#define TIME_UNDEFINED 0x8000000000000000LL

struct bg_audio_converter_s
  {
  gavl_audio_converter_t * cnv;
  const gavl_audio_options_t * opt;
  gavl_audio_frame_t * in_frame;
  gavl_audio_frame_t * out_frame;

  /* Input stuff */
  bg_read_audio_func_t read_func;
  void * read_priv;
  int read_stream;

  bg_read_audio_func_t read_func_1;
  void * read_priv_1;
  int read_stream_1;
  
  /* Output stuff */
  int (*read_func_out)(bg_audio_converter_t*, gavl_audio_frame_t * frame, int num_samples);
  
  gavl_audio_format_t in_format;
  gavl_audio_format_t out_format;
  int in_rate;
  int out_rate;
  int last_samples;
  int64_t out_pts;
  };

bg_audio_converter_t *
bg_audio_converter_create(const gavl_audio_options_t * opt)
  {
  bg_audio_converter_t * ret;
  ret = calloc(1, sizeof(*ret));
  ret->opt = opt;
  ret->cnv = gavl_audio_converter_create();
  return ret;
  }

void bg_audio_converter_reset(bg_audio_converter_t * cnv)
  {
  if(cnv->out_frame)
    cnv->out_frame->valid_samples = 0;
  cnv->out_pts = TIME_UNDEFINED;
  }

static int
read_audio_priv(void * priv, gavl_audio_frame_t * frame, int stream,
                int num_samples)
  {
  int ret;
  bg_audio_converter_t * cnv = (bg_audio_converter_t *)priv;
  ret = cnv->read_func_1(cnv->read_priv_1, frame,
                         cnv->read_stream_1, num_samples);
  if(cnv->out_pts == TIME_UNDEFINED)
    cnv->out_pts = gavl_time_rescale(cnv->in_format.samplerate,
                                     cnv->out_format.samplerate,
                                     frame->timestamp);
  return ret;
  }

void bg_audio_converter_connect_input(bg_audio_converter_t * cnv,
                                      bg_read_audio_func_t func,
                                      void * priv,
                                      int stream)
  {
  cnv->read_func = read_audio_priv;
  cnv->read_priv = cnv;
  cnv->read_stream = 0;
  
  cnv->read_func_1 = func;
  cnv->read_priv_1 = priv;
  cnv->read_stream_1 = stream;
  }

static int audio_converter_read_noresample(bg_audio_converter_t * cnv,
                                           gavl_audio_frame_t* frame, int num_samples)
  {
  if(cnv->in_format.samples_per_frame < num_samples)
    {
    if(cnv->in_frame)
      {
      gavl_audio_frame_destroy(cnv->in_frame);
      cnv->in_frame = (gavl_audio_frame_t*)0;
      }
    cnv->in_format.samples_per_frame = num_samples + 1024;
    }
  
  if(!cnv->in_frame)
    cnv->in_frame = gavl_audio_frame_create(&cnv->in_format);

  if(!cnv->read_func(cnv->read_priv,
                     cnv->in_frame,
                     cnv->read_stream,
                     num_samples))
    return 0;
  
  gavl_audio_convert(cnv->cnv, cnv->in_frame, frame);
  return frame->valid_samples;
  }

static int audio_converter_read_resample(bg_audio_converter_t * cnv,
                                         gavl_audio_frame_t* frame, int num_samples)
  {
  int samples_copied;

  frame->valid_samples = 0;
  while(frame->valid_samples < num_samples)
    {
    /* Read samples */
    if(!cnv->out_frame->valid_samples)
      {
      if(!cnv->read_func(cnv->read_priv, cnv->in_frame, cnv->read_stream,
                         cnv->in_format.samples_per_frame))
        return frame->valid_samples;
      gavl_audio_convert(cnv->cnv, cnv->in_frame, cnv->out_frame);
      cnv->last_samples = cnv->out_frame->valid_samples;
      }
    /* Copy samples */

    samples_copied = gavl_audio_frame_copy(&cnv->out_format,
                                           frame,
                                           cnv->out_frame,
                                           frame->valid_samples,
                                           cnv->last_samples - cnv->out_frame->valid_samples,
                                           num_samples - frame->valid_samples,
                                           cnv->out_frame->valid_samples);

    cnv->out_frame->valid_samples -= samples_copied;
    frame->valid_samples += samples_copied;
    }
  return frame->valid_samples;
  }

int bg_audio_converter_init(bg_audio_converter_t * cnv,
                            const gavl_audio_format_t * in_format,
                            const gavl_audio_format_t * out_format)
  {
  int result;
  gavl_audio_options_t * cnv_opt;
  
  /* Free previous stuff */
  if(cnv->in_frame)
    {
    gavl_audio_frame_destroy(cnv->in_frame);
    cnv->in_frame = (gavl_audio_frame_t*)0;
    }
  if(cnv->out_frame)
    {
    gavl_audio_frame_destroy(cnv->out_frame);
    cnv->out_frame = (gavl_audio_frame_t*)0;
    }
  /* Set options */
  cnv_opt = gavl_audio_converter_get_options(cnv->cnv);
  gavl_audio_options_copy(cnv_opt, cnv->opt);
  
  result = gavl_audio_converter_init(cnv->cnv, in_format, out_format);

  if(result)
    {
    gavl_audio_format_copy(&cnv->in_format, in_format);
    gavl_audio_format_copy(&cnv->out_format, out_format);
    
    if(cnv->out_format.samplerate != cnv->in_format.samplerate)
      {
      cnv->out_format.samples_per_frame =
        (cnv->in_format.samples_per_frame * cnv->out_format.samplerate) /
        cnv->in_format.samplerate + 10;
      
      cnv->in_frame = gavl_audio_frame_create(&cnv->in_format);
      cnv->out_frame = gavl_audio_frame_create(&cnv->out_format);

      cnv->read_func_out = audio_converter_read_resample;
      }
    else
      cnv->read_func_out = audio_converter_read_noresample;
    
    cnv->last_samples = 0;
    }
  return result;
  }


int bg_audio_converter_read(void * priv,
                            gavl_audio_frame_t* frame, int stream,
                            int num_samples)
  {
  bg_audio_converter_t * cnv = (bg_audio_converter_t*)priv;
  int result;

  result = cnv->read_func_out(cnv, frame, num_samples);

  if(result)
    {
    frame->timestamp = cnv->out_pts;
    cnv->out_pts += frame->valid_samples;
    }
  return result;
  }

void bg_audio_converter_destroy(bg_audio_converter_t * cnv)
  {
  if(cnv->in_frame)
    gavl_audio_frame_destroy(cnv->in_frame);
  if(cnv->out_frame)
    gavl_audio_frame_destroy(cnv->out_frame);
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

  bg_read_video_func_t read_func_1;
  void * read_priv_1;
  int read_stream_1;
    
  int64_t out_pts;
  
  int convert_gavl;
  int convert_framerate;
  int rescale_timestamps;
  
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

  cnv->convert_framerate  = 0;
  cnv->convert_gavl       = 0;
  cnv->rescale_timestamps = 0;
  
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
      char * str1, *str2;
      
      cnv->convert_framerate = 1;

      if(cnv->in_format.framerate_mode == GAVL_FRAMERATE_VARIABLE)
        str1 = bg_strdup(NULL, TR("variable"));
      else
        str1 = bg_sprintf("%5.2f",
                          (float)(cnv->in_format.timescale) /
                          (float)(cnv->in_format.frame_duration));

      if(cnv->out_format.framerate_mode == GAVL_FRAMERATE_VARIABLE)
        str2 = bg_strdup(NULL, TR("variable"));
      else
        str2 = bg_sprintf("%5.2f",
                          (float)(cnv->out_format.timescale) /
                          (float)(cnv->out_format.frame_duration));
      
      bg_log(BG_LOG_INFO, LOG_DOMAIN, "Doing framerate conversion %s -> %s",
             str1, str2);
      free(str1);
      free(str2);
      }
    }
  if(!cnv->convert_framerate)
    {
    if(in_format->timescale != out_format->timescale)
      cnv->rescale_timestamps = 1;
    }
  
  if(cnv->convert_gavl || cnv->convert_framerate)
    {
    cnv->frame = gavl_video_frame_create(in_format);
    gavl_video_frame_clear(cnv->frame, in_format);
    cnv->frame->timestamp = GAVL_TIME_UNDEFINED;
    }
  if(cnv->convert_framerate)
    {
    cnv->next_frame = gavl_video_frame_create(in_format);
    gavl_video_frame_clear(cnv->next_frame, in_format);
    cnv->next_frame->timestamp = GAVL_TIME_UNDEFINED;
    }
  cnv->out_pts = 0;
  cnv->eof = 0;

  ret = cnv->convert_framerate + cnv->convert_gavl + cnv->rescale_timestamps;
  
  bg_log(BG_LOG_DEBUG, LOG_DOMAIN, "Initialized video converter, %d steps", ret);
  return ret;
  }

static int
read_video_priv(void * priv, gavl_video_frame_t * frame, int stream)
  {
  int ret;
  bg_video_converter_t * cnv = (bg_video_converter_t *)priv;
  ret = cnv->read_func_1(cnv->read_priv_1, frame,
                         cnv->read_stream_1);
  if(cnv->out_pts == TIME_UNDEFINED)
    cnv->out_pts = gavl_time_rescale(cnv->in_format.timescale,
                                     cnv->out_format.timescale,
                                     frame->timestamp);
  return ret;
  }

void bg_video_converter_connect_input(bg_video_converter_t * cnv,
                                      bg_read_video_func_t func,
                                      void * priv,
                                      int stream)
  {
  cnv->read_func = read_video_priv;
  cnv->read_priv = cnv;
  cnv->read_stream = 0;
  
  cnv->read_func_1 = func;
  cnv->read_priv_1 = priv;
  cnv->read_stream_1 = stream;
  }



int bg_video_converter_read(void * priv, gavl_video_frame_t* frame, int stream)
  {
  int64_t in_pts;
  int result;
  bg_video_converter_t * cnv = (bg_video_converter_t *)priv;
  gavl_video_frame_t * tmp_frame;
    
  if(!cnv->convert_framerate)
    {
    if(cnv->convert_gavl)
      {
      result = cnv->read_func(cnv->read_priv, cnv->frame, cnv->read_stream);
      if(result)
        gavl_video_convert(cnv->cnv, cnv->frame, frame);
      }
    else
      result = cnv->read_func(cnv->read_priv, frame, cnv->read_stream);
    if(cnv->rescale_timestamps)
      {
      frame->timestamp = gavl_time_rescale(cnv->in_format.timescale,
                                             cnv->out_format.timescale,
                                             frame->timestamp);
      frame->duration  = gavl_time_rescale(cnv->in_format.timescale,
                                           cnv->out_format.timescale,
                                           frame->duration);
      }
    return result;
    }
  else
    {
    /* Read first frames */
    if((cnv->frame->timestamp == GAVL_TIME_UNDEFINED) &&
       !cnv->read_func(cnv->read_priv, cnv->frame, cnv->read_stream))
      return 0;

    if((cnv->next_frame->timestamp == GAVL_TIME_UNDEFINED) &&
       !cnv->read_func(cnv->read_priv, cnv->next_frame, cnv->read_stream))
      return 0;
    
    in_pts = gavl_time_rescale(cnv->out_format.timescale,
                               cnv->in_format.timescale,
                               cnv->out_pts);
    /* Last frame was already returned */
    if(cnv->eof)
      return 0;

    while(in_pts >= cnv->next_frame->timestamp)
      {
      tmp_frame = cnv->frame;
      cnv->frame = cnv->next_frame;
      cnv->next_frame = tmp_frame;
      
      result = cnv->read_func(cnv->read_priv, cnv->next_frame, cnv->read_stream);
      if(!result)
        {
        cnv->eof = 1;
        break;
        }
      }
    
    if(cnv->eof)
      tmp_frame = cnv->next_frame;
    else if(ABSDIFF(cnv->next_frame->timestamp, in_pts) <
            ABSDIFF(cnv->frame->timestamp, in_pts))
      tmp_frame = cnv->next_frame;
    else
      tmp_frame = cnv->frame;
    
    if(cnv->convert_gavl)
      gavl_video_convert(cnv->cnv, tmp_frame, frame);
    else
      gavl_video_frame_copy(&cnv->out_format, frame, tmp_frame);
    
    frame->timestamp = cnv->out_pts;
    cnv->out_pts += cnv->out_format.frame_duration;

    /* Clear timecode */
    frame->timecode = GAVL_TIMECODE_UNDEFINED;
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

void bg_video_converter_reset(bg_video_converter_t * cnv)
  {
  if(cnv->frame)      cnv->frame->timestamp      = GAVL_TIME_UNDEFINED;
  if(cnv->next_frame) cnv->next_frame->timestamp = GAVL_TIME_UNDEFINED;
  
  cnv->eof = 0;
  cnv->out_pts = TIME_UNDEFINED;
  }
