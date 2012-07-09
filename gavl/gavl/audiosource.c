/*****************************************************************
 * gavl - a general purpose audio/video processing library
 *
 * Copyright (c) 2001 - 2012 Members of the Gmerlin project
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

#include <stdlib.h>
#include <stdio.h>


#include <gavl/gavl.h>

struct gavl_audio_source_s
  {
  gavl_audio_format_t src_format;
  gavl_audio_format_t dst_format;
  int src_flags;
  int dst_flags;

  int64_t next_pts;

  /* Samples in the frame from which we buffer */
  int frame_samples;

  /* For conversion */
  gavl_audio_frame_t * in_frame;
  gavl_audio_frame_t * out_frame;
  gavl_audio_frame_t * dst_frame;

  gavl_audio_frame_t * frame;
  
  /* Callback set by the client */
  
  gavl_audio_source_func_t func;
  void * priv;
  gavl_audio_converter_t * cnv;

  int passthrough;
  int do_convert;
  int skip_samples;
  };

gavl_audio_source_t *
gavl_audio_source_create(gavl_audio_source_func_t func,
                         void * priv,
                         int src_flags,
                         const gavl_audio_format_t * src_format)
  {
  gavl_audio_source_t * ret = calloc(1, sizeof(*ret));

  ret->func = func;
  ret->priv = priv;
  ret->src_flags = src_flags;
  gavl_audio_format_copy(&ret->src_format, src_format);
  ret->cnv = gavl_audio_converter_create();
  return ret;
  }

const gavl_audio_format_t *
gavl_audio_source_get_src_format(gavl_audio_source_t * s)
  {
  return &s->src_format;
  }

const gavl_audio_format_t *
gavl_audio_source_get_dst_format(gavl_audio_source_t * s)
  {
  return &s->dst_format;
  }

gavl_audio_options_t * gavl_audio_source_get_options(gavl_audio_source_t * s)
  {
  return gavl_audio_converter_get_options(s->cnv);
  }

void gavl_audio_source_reset(gavl_audio_source_t * s)
  {
  s->next_pts = GAVL_TIME_UNDEFINED;
  if(s->frame)
    s->frame->valid_samples = 0;
  s->skip_samples = 0;
#if 0
  if(s->src_fp)
    gavl_audio_frame_pool_reset(s->src_fp);
  if(s->dst_fp)
    gavl_audio_frame_pool_reset(s->dst_fp);
#endif
  }

void gavl_audio_source_destroy(gavl_audio_source_t * s)
  {
#if 0
  if(s->src_fp)
    gavl_audio_frame_pool_destroy(s->src_fp);
  if(s->dst_fp)
    gavl_audio_frame_pool_destroy(s->dst_fp);
#endif

  if(s->out_frame)
    gavl_audio_frame_destroy(s->out_frame);
  if(s->in_frame)
    gavl_audio_frame_destroy(s->in_frame);
  if(s->dst_frame)
    gavl_audio_frame_destroy(s->dst_frame);
  
  
  gavl_audio_converter_destroy(s->cnv);
  free(s);
  }

void
gavl_audio_source_set_dst(gavl_audio_source_t * s, int dst_flags,
                          const gavl_audio_format_t * dst_format)
  {
  s->next_pts = GAVL_TIME_UNDEFINED;
  s->dst_flags = dst_flags;

  if(dst_format)
    gavl_audio_format_copy(&s->dst_format, dst_format);
  else
    gavl_audio_format_copy(&s->dst_format, &s->src_format);

  s->do_convert =
    gavl_audio_converter_init(s->cnv,
                              &s->src_format, &s->dst_format);
  s->passthrough = 0;
  
  if(!s->do_convert &&
     (s->src_format.samples_per_frame == s->src_format.samples_per_frame))
    s->passthrough = 1;

  if(!s->passthrough)
    {
    
    }
  }

static void check_out_frame(gavl_audio_source_t * s)
  {
  if(!s->out_frame)
    {
    gavl_audio_format_t frame_format;
    gavl_audio_format_copy(&frame_format, &s->dst_format);
    frame_format.samples_per_frame =
      gavl_time_rescale(s->src_format.samplerate,
                        s->dst_format.samplerate,
                        s->src_format.samples_per_frame) + 10;
    s->out_frame = gavl_audio_frame_create(&frame_format);
    }
  }

static int process_input(gavl_audio_source_t * s, gavl_audio_frame_t * f)
  {
  if(s->skip_samples)
    {
    int skipped = gavl_audio_frame_skip(&s->src_format,
                                        f, s->skip_samples);;
    s->skip_samples -= skipped;
    s->next_pts += skipped;

    if(!f->valid_samples)
      return 0;
    else
      s->next_pts = gavl_time_rescale(s->src_format.samplerate,
                                      s->dst_format.samplerate,
                                      f->timestamp);
    }

  if(s->next_pts == GAVL_TIME_UNDEFINED)
    s->next_pts = gavl_time_rescale(s->src_format.samplerate,
                                    s->dst_format.samplerate,
                                    f->timestamp);

  //  fprintf(stderr, "Process input %ld\n", f->valid_samples);
  
  return 1;
  }

static void process_output(gavl_audio_source_t * s,
                           gavl_audio_frame_t * f)
  {
  //  fprintf(stderr, "Process output %ld %ld\n", s->next_pts, f->valid_samples);
  f->timestamp = s->next_pts;
  s->next_pts += f->valid_samples;
  }

static gavl_source_status_t
read_frame_internal(void * sp, gavl_audio_frame_t ** frame, int num_samples)
  {
  gavl_audio_source_t * s = sp;
  int samples_read = 0;
  int samples_copied;
  gavl_source_status_t ret = GAVL_SOURCE_OK;
  
  while(samples_read < num_samples)
    {
    /* Read new frame if neccesary */

    /* Check for passthrough */
    if(!s->frame || !s->frame->valid_samples)
      {
      if(s->passthrough && !s->skip_samples)
        {
        /* dst -> src */
        if((*frame && !(s->src_flags & GAVL_SOURCE_SRC_ALLOC)) ||
           (!(*frame) && (s->src_flags & GAVL_SOURCE_SRC_ALLOC)))
          {
          ret = s->func(s->priv, frame);
          if(ret == GAVL_SOURCE_OK)
            {
            process_input(s, *frame);
            process_output(s, *frame);
            }
          return ret;
          }
        }
      
      /* Read a new frame */
      if(s->do_convert)
        {
        gavl_audio_frame_t * in_frame;
      
        if(s->src_flags & GAVL_SOURCE_SRC_ALLOC)
          {
          if(!s->in_frame)
            s->in_frame = gavl_audio_frame_create(&s->src_format);
          in_frame = s->in_frame;
          }
        else
          in_frame = NULL;

        ret = s->func(s->priv, &in_frame);
        
        if(ret != GAVL_SOURCE_OK)
          break;
        
        if(!process_input(s, in_frame))
          continue;
        
        /* Get out frame */
        check_out_frame(s);
        gavl_audio_convert(s->cnv, in_frame, s->out_frame);
        s->frame = s->out_frame;
        }
      else
        {
        if(s->src_flags & GAVL_SOURCE_SRC_ALLOC)
          {
          s->frame = NULL;
          ret = s->func(s->priv, &s->frame);
          if(ret != GAVL_SOURCE_OK)
            break;
          
          if(!process_input(s, s->frame))
            continue;
          }
        else
          {
          check_out_frame(s);
          ret = s->func(s->priv, &s->out_frame);
          if(ret != GAVL_SOURCE_OK)
            break;
          s->frame = s->out_frame;
          if(!process_input(s, s->frame))
            continue;
          }
        }
      s->frame_samples = s->frame->valid_samples;
      }

    /* Copy samples */

    if(!(*frame))
      {
      if(!s->dst_frame)
        s->dst_frame = gavl_audio_frame_create(&s->dst_format);
      *frame = s->dst_frame;
      }
    
    samples_copied =
      gavl_audio_frame_copy(&s->dst_format,
                            *frame,                                     // dst
                            s->frame,                                   // src
                            samples_read,                               // dst_pos
                            s->frame_samples - s->frame->valid_samples, // src_pos
                            num_samples - samples_read,                 // dst_size
                            s->frame->valid_samples);                   // src_size
    s->frame->valid_samples -= samples_copied;
    samples_read += samples_copied;
    }
  
  if(samples_read)
    {
    ret = GAVL_SOURCE_OK;
    (*frame)->valid_samples = samples_read;
    process_output(s, *frame);
    }
  else
    (*frame)->valid_samples = 0;
  
  return ret;
  }

gavl_source_status_t
gavl_audio_source_read_frame(void * sp, gavl_audio_frame_t ** frame)
  {
  gavl_audio_source_t * s = sp;
  return read_frame_internal(s, frame, s->dst_format.samples_per_frame);
  }


/* For cases where it's not immediately known, how many samples will be
   processed */

int gavl_audio_source_read_samples(void * sp, gavl_audio_frame_t * frame,
                                   int num_samples)
  {
  gavl_audio_source_t * s = sp;
  if(read_frame_internal(s, &frame, num_samples) != GAVL_SOURCE_OK)
    return 0;
  return frame->valid_samples;
  }

void 
gavl_audio_source_skip(gavl_audio_source_t * s, int num_samples)
  {
  s->skip_samples += num_samples;
  }
