/*****************************************************************
 * gmerlin - a general purpose multimedia framework and applications
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

#include "gavf-decode.h"

#define LOG_DOMAIN "gavf-decode.albumstream"


int stream_replug(stream_t * s, bg_mediaconnector_stream_t * in_stream)
  {
  s->in_stream = in_stream;
 
  switch(s->in_stream->type)
    {
    case GAVF_STREAM_AUDIO:
      gavl_audio_source_set_dst(s->in_stream->asrc, 0, &s->afmt);

      if(gavl_audio_source_read_frame(s->in_stream->asrc,
                                      &s->next_aframe) != GAVL_SOURCE_OK)
        {
        bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Couldn't get first audio frame");
        return 0;
        }
      s->mute_time = gavl_time_scale(s->in_scale, s->a->end_time) - s->pts;

      if(s->mute_time < 0)
        s->mute_time = 0;
      
      s->mute_time += s->next_aframe->timestamp;
      
      bg_log(BG_LOG_DEBUG, LOG_DOMAIN, "Audio mute time: %"PRId64" (%"PRId64")",
              s->mute_time, gavl_time_unscale(s->in_scale, s->mute_time));
      
      break;
    case GAVF_STREAM_VIDEO:
      gavl_video_source_set_dst(in_stream->vsrc, 0, &s->vfmt);

      if(gavl_video_source_read_frame(s->in_stream->vsrc,
                                      &s->next_vframe) != GAVL_SOURCE_OK)
        {
        bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Couldn't get first video frame");
        return 0;
        }
      s->mute_time = gavl_time_scale(s->in_scale, s->a->end_time) - s->pts;

      if(s->mute_time < 0)
        s->mute_time = 0;
      
      s->mute_time += s->next_vframe->timestamp;

      bg_log(BG_LOG_DEBUG, LOG_DOMAIN, "Video mute time: %"PRId64" (%"PRId64")",
             s->mute_time, gavl_time_unscale(s->in_scale, s->mute_time));
 
      break;
    case GAVF_STREAM_TEXT:
      break;
    case GAVF_STREAM_OVERLAY:
      gavl_video_source_set_dst(in_stream->vsrc, 0, &s->vfmt);
      break;
    }
  return 1;
  }

  
static gavl_source_status_t read_audio(void * priv, gavl_audio_frame_t ** ret)
  {
  stream_t * s = priv;

  if(s->a->eof)
    return GAVL_SOURCE_EOF;
    
  if(s->mute_time)
    {
    if(!s->mute_aframe)
      s->mute_aframe = gavl_audio_frame_create(&s->afmt);
    gavl_audio_frame_mute(s->mute_aframe, &s->afmt);
    s->mute_aframe->valid_samples = s->mute_time;
    if(s->mute_aframe->valid_samples > s->afmt.samples_per_frame)
      s->mute_aframe->valid_samples = s->afmt.samples_per_frame;
    *ret = s->mute_aframe;
    s->mute_time -= s->mute_aframe->valid_samples;
    s->pts += s->mute_aframe->valid_samples;
    return GAVL_SOURCE_OK;
    }
  else if(s->next_aframe)
    {
    s->next_aframe->timestamp = s->pts;
    s->pts += s->next_aframe->valid_samples;
    
    *ret = s->next_aframe;
    
    s->next_aframe = NULL;

    return GAVL_SOURCE_OK;
    }
  else
    {
    gavl_audio_frame_t * f;

    while(1)
      {
      f = NULL;
      
      s->st = gavl_audio_source_read_frame(s->in_stream->asrc, &f);

      switch(s->st)
        {
        case GAVL_SOURCE_OK:
          
          f->timestamp = s->pts;
          s->pts += f->valid_samples;

          *ret = f;
          return s->st;
          break;
        case GAVL_SOURCE_EOF:
          if(!album_set_eof(s->a))
            {
            return s->a->eof ? GAVL_SOURCE_EOF : GAVL_SOURCE_AGAIN;
            }
          break;
        case GAVL_SOURCE_AGAIN:
          break;
        }
      }
    }
  
  return GAVL_SOURCE_EOF;
  }

static gavl_source_status_t read_video(void * priv, gavl_video_frame_t ** ret)
  {
  stream_t * s = priv;

  if(s->a->eof)
    return GAVL_SOURCE_EOF;

  if(s->mute_time)
    {
    if(!s->mute_vframe)
      {
      s->mute_vframe = gavl_video_frame_create(&s->vfmt);
      gavl_video_frame_clear(s->mute_vframe, &s->vfmt);
      }
    s->mute_vframe->timestamp = s->pts;
    s->mute_vframe->duration = s->mute_time;
    s->pts += s->mute_vframe->duration;
    *ret = s->mute_vframe;
    s->mute_time = 0;
    return GAVL_SOURCE_OK;
    }
  else if(s->next_vframe)
    {
    *ret = s->next_vframe;
    (*ret)->timestamp = s->pts;
    s->pts += (*ret)->duration;
    s->next_vframe = NULL;
    return GAVL_SOURCE_OK;
    }
  else
    {
    gavl_video_frame_t * f = NULL;

    while(1)
      {
      s->st = gavl_video_source_read_frame(s->in_stream->vsrc, &f);

      switch(s->st)
        {
        case GAVL_SOURCE_OK:
          *ret = f;
          (*ret)->timestamp = s->pts;
          s->pts += (*ret)->duration;
          return s->st;
          break;
        case GAVL_SOURCE_EOF:
          if(!album_set_eof(s->a))
            {
            return s->a->eof ? GAVL_SOURCE_EOF : GAVL_SOURCE_AGAIN;
            }
          break;
        case GAVL_SOURCE_AGAIN:
          /* Not handled */
          break;
        }

      }
    
    }
  
  return GAVL_SOURCE_EOF;
  }

static gavl_source_status_t read_overlay(void * priv, gavl_video_frame_t ** ret)
  {
  stream_t * s = priv;
  if(s->a->eof)
    return GAVL_SOURCE_EOF;
  return GAVL_SOURCE_EOF;
  }

static gavl_source_status_t read_text(void * priv, gavl_packet_t ** ret)
  {
  stream_t * s = priv;
  if(s->a->eof)
    return GAVL_SOURCE_EOF;
  return GAVL_SOURCE_EOF;
  }

static void stream_destroy(bg_mediaconnector_stream_t * ms)
  {
  stream_t * s = ms->priv;
  if(s->asrc)
    gavl_audio_source_destroy(s->asrc);
  if(s->vsrc)
    gavl_video_source_destroy(s->vsrc);
  if(s->psrc)
    gavl_packet_source_destroy(s->psrc);
  if(s->mute_aframe)
    gavl_audio_frame_destroy(s->mute_aframe);
  if(s->mute_vframe)
    gavl_video_frame_destroy(s->mute_vframe);
  free(s);
  }

int stream_create(bg_mediaconnector_stream_t * in_stream,
                  album_t * a)
  {
  bg_mediaconnector_stream_t * out_stream = NULL;
  stream_t * ret = calloc(1, sizeof(*ret));
  ret->in_stream = in_stream;
  ret->a = a;

  switch(ret->in_stream->type)
    {
    case GAVF_STREAM_AUDIO:
      gavl_audio_format_copy(&ret->afmt, gavl_audio_source_get_src_format(ret->in_stream->asrc));
      ret->in_scale = ret->afmt.samplerate;
      ret->asrc = 
        gavl_audio_source_create(read_audio, ret, 
                                 GAVL_SOURCE_SRC_ALLOC | GAVL_SOURCE_SRC_FRAMESIZE_MAX,
                                 &ret->afmt);
      out_stream = bg_mediaconnector_add_audio_stream(&a->out_conn,
                                                      NULL, ret->asrc, NULL, NULL);

      if(gavl_audio_source_read_frame(ret->in_stream->asrc,
                                      &ret->next_aframe) != GAVL_SOURCE_OK)
        {
        bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Couldn't get first audio frame");
        return 0;
        }
      ret->mute_time = ret->next_aframe->timestamp;
      break;
    case GAVF_STREAM_VIDEO:
      gavl_video_format_copy(&ret->vfmt, gavl_video_source_get_src_format(ret->in_stream->vsrc));

      /* Adjust format: We set the timescale to 1000 and the framerate to variable */
      ret->vfmt.timescale = 1000;
      ret->vfmt.frame_duration = 0;
      ret->vfmt.framerate_mode = GAVL_FRAMERATE_VARIABLE;

      ret->in_scale = ret->vfmt.timescale;
      
      gavl_video_source_set_dst(ret->in_stream->vsrc, 0, &ret->vfmt);
      
      ret->vsrc =
        gavl_video_source_create(read_video, ret, GAVL_SOURCE_SRC_ALLOC, &ret->vfmt);
      out_stream = bg_mediaconnector_add_video_stream(&a->out_conn,
                                                      NULL, ret->vsrc, NULL, NULL);


      if(gavl_video_source_read_frame(ret->in_stream->vsrc,
                                      &ret->next_vframe) != GAVL_SOURCE_OK)
        {
        bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Couldn't get first video frame");
        return 0;
        }
      ret->mute_time = ret->next_vframe->timestamp;
      
      break;
    case GAVF_STREAM_TEXT:
      ret->psrc =
        gavl_packet_source_create_text(read_text, ret, GAVL_SOURCE_SRC_ALLOC, ret->in_stream->timescale);
      out_stream = bg_mediaconnector_add_text_stream(&a->out_conn,
                                                     NULL, ret->psrc, ret->in_stream->timescale);
      break;
    case GAVF_STREAM_OVERLAY:
      gavl_video_format_copy(&ret->vfmt, gavl_video_source_get_src_format(ret->in_stream->vsrc));
      ret->in_scale = ret->vfmt.timescale;
      
      ret->vsrc =
        gavl_video_source_create(read_overlay, ret, GAVL_SOURCE_SRC_ALLOC,
                                 gavl_video_source_get_src_format(ret->in_stream->vsrc));
      out_stream = bg_mediaconnector_add_overlay_stream(&a->out_conn,
                                                         NULL, ret->vsrc, NULL, NULL);
      break;
    }

  if(ret)
    ret->out_scale = ret->in_scale;
  
  if(out_stream)
    {
    out_stream->priv = ret;
    out_stream->free_priv = stream_destroy;
    }
  return 1;
  }
