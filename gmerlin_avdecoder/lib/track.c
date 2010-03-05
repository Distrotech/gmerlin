/*****************************************************************
 * gmerlin-avdecoder - a general purpose multimedia decoding library
 *
 * Copyright (c) 2001 - 2010 Members of the Gmerlin project
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

#include <avdec_private.h>
#include <parser.h>

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define LOG_DOMAIN "track"

bgav_stream_t *
bgav_track_add_audio_stream(bgav_track_t * t, const bgav_options_t * opt)
  {
  t->num_audio_streams++;
  t->audio_streams = realloc(t->audio_streams, t->num_audio_streams * 
                             sizeof(*(t->audio_streams)));
  bgav_stream_init(&(t->audio_streams[t->num_audio_streams-1]), opt);
  bgav_stream_create_packet_buffer(&(t->audio_streams[t->num_audio_streams-1]));
  t->audio_streams[t->num_audio_streams-1].data.audio.bits_per_sample = 16;
  t->audio_streams[t->num_audio_streams-1].type = BGAV_STREAM_AUDIO;
  t->audio_streams[t->num_audio_streams-1].track = t;
  return &(t->audio_streams[t->num_audio_streams-1]);
  }

bgav_stream_t *
bgav_track_add_video_stream(bgav_track_t * t, const bgav_options_t * opt)
  {
  t->num_video_streams++;
  t->video_streams = realloc(t->video_streams, t->num_video_streams * 
                             sizeof(*(t->video_streams)));
  bgav_stream_init(&(t->video_streams[t->num_video_streams-1]), opt);
  bgav_stream_create_packet_buffer(&(t->video_streams[t->num_video_streams-1]));
  t->video_streams[t->num_video_streams-1].type = BGAV_STREAM_VIDEO;
  t->video_streams[t->num_video_streams-1].opt = opt;
  t->video_streams[t->num_video_streams-1].track = t;
  return &(t->video_streams[t->num_video_streams-1]);
  }

static bgav_stream_t * add_subtitle_stream(bgav_track_t * t,
                                           const bgav_options_t * opt,
                                           int text,
                                           const char * charset,
                                           bgav_subtitle_reader_context_t * r)
  {
  bgav_stream_t * ret;
  
  t->num_subtitle_streams++;
  t->subtitle_streams = realloc(t->subtitle_streams, t->num_subtitle_streams * 
                                sizeof(*(t->subtitle_streams)));

  ret = &t->subtitle_streams[t->num_subtitle_streams-1];
  bgav_stream_init(ret, opt);
  if(!r)
    bgav_stream_create_packet_buffer(ret);
  else
    ret->data.subtitle.subreader = r;
  
  if(text)
    {
    ret->type = BGAV_STREAM_SUBTITLE_TEXT;
    if(charset)
      t->subtitle_streams[t->num_subtitle_streams-1].data.subtitle.charset =
        bgav_strdup(charset);
    }
  else
    t->subtitle_streams[t->num_subtitle_streams-1].type =
      BGAV_STREAM_SUBTITLE_OVERLAY;

  t->subtitle_streams[t->num_subtitle_streams-1].track = t;
  
  return ret;
  }
                                           
                                           

bgav_stream_t *
bgav_track_add_subtitle_stream(bgav_track_t * t, const bgav_options_t * opt,
                               int text,
                               const char * encoding)
  {
  return add_subtitle_stream(t,
                             opt,
                             text,
                             encoding,
                             (bgav_subtitle_reader_context_t*)0);
  }

bgav_stream_t *
bgav_track_attach_subtitle_reader(bgav_track_t * t,
                                  const bgav_options_t * opt,
                                  bgav_subtitle_reader_context_t * r)
  {
  bgav_stream_t * ret;
  ret = add_subtitle_stream(t, opt,
                            r->reader->read_subtitle_text ? 1 : 0,
                            (char*)0, r);
  if(r->info)
    ret->info = bgav_strdup(r->info);
  return ret;
  }

bgav_stream_t *
bgav_track_find_stream_all(bgav_track_t * t, int stream_id)
  {
  int i;
  for(i = 0; i < t->num_audio_streams; i++)
    {
    if(t->audio_streams[i].stream_id == stream_id)
      return &(t->audio_streams[i]);
    }
  for(i = 0; i < t->num_video_streams; i++)
    {
    if(t->video_streams[i].stream_id == stream_id)
      return &(t->video_streams[i]);
    }
  for(i = 0; i < t->num_subtitle_streams; i++)
    {
    if((t->subtitle_streams[i].stream_id == stream_id) &&
       (!t->subtitle_streams[i].data.subtitle.subreader))
      return &(t->subtitle_streams[i]);
    }
  return (bgav_stream_t *)0;
  }

bgav_stream_t * bgav_track_find_stream(bgav_demuxer_context_t * ctx, int stream_id)
  {
  int i;
  bgav_track_t * t;
  if(ctx->demux_mode == DEMUX_MODE_FI)
    {
    if(ctx->request_stream && (stream_id == ctx->request_stream->stream_id))
      return ctx->request_stream;
    else
      return (bgav_stream_t*)0;
    }
  t = ctx->tt->cur;
  
  for(i = 0; i < t->num_audio_streams; i++)
    {
    if(t->audio_streams[i].stream_id == stream_id)
      {
      if(t->audio_streams[i].action != BGAV_STREAM_MUTE)
        return &(t->audio_streams[i]);
      else
        return (bgav_stream_t *)0;
      }
    }
  for(i = 0; i < t->num_video_streams; i++)
    {
    if(t->video_streams[i].stream_id == stream_id)
      {
      if(t->video_streams[i].action != BGAV_STREAM_MUTE)
        return &(t->video_streams[i]);
      else
        return (bgav_stream_t *)0;
      }
    }
  for(i = 0; i < t->num_subtitle_streams; i++)
    {
    if((t->subtitle_streams[i].stream_id == stream_id) &&
       (!t->subtitle_streams[i].data.subtitle.subreader))
      {
      if(t->subtitle_streams[i].action != BGAV_STREAM_MUTE)
        return &(t->subtitle_streams[i]);
      else
        return (bgav_stream_t *)0;
      }
    }
  return (bgav_stream_t *)0;
  }

#define FREE(ptr) if(ptr){free(ptr);ptr=NULL;}
  
void bgav_track_stop(bgav_track_t * t)
  {
  int i;
  
  for(i = 0; i < t->num_audio_streams; i++)
    {
    bgav_stream_stop(&(t->audio_streams[i]));
    }
  for(i = 0; i < t->num_video_streams; i++)
    {
    bgav_stream_stop(&(t->video_streams[i]));
    }
  for(i = 0; i < t->num_subtitle_streams; i++)
    {
    bgav_stream_stop(&(t->subtitle_streams[i]));
    }
  }

int bgav_track_start(bgav_track_t * t, bgav_demuxer_context_t * demuxer)
  {
  int i;
  gavl_video_format_t * video_format;
  bgav_stream_t * video_stream;
  int num_active_audio_streams = 0;
  int num_active_video_streams = 0;
  int num_active_subtitle_streams = 0;
  
  for(i = 0; i < t->num_audio_streams; i++)
    {
    if(t->audio_streams[i].action == BGAV_STREAM_MUTE)
      continue;
    num_active_audio_streams++;
    if(!bgav_stream_start(&(t->audio_streams[i])))
      {
      bgav_log(demuxer->opt, BGAV_LOG_ERROR, LOG_DOMAIN,
               "Starting audio decoder for stream %d failed", i+1);
      return 0;
      }
    }
  for(i = 0; i < t->num_video_streams; i++)
    {
    if(t->video_streams[i].action == BGAV_STREAM_MUTE)
      continue;
    num_active_video_streams++;
    if(!bgav_stream_start(&(t->video_streams[i])))
      {
      bgav_log(demuxer->opt, BGAV_LOG_ERROR, LOG_DOMAIN,
               "Starting video decoder for stream %d failed", i+1);
      return 0;
      }
    }
  for(i = 0; i < t->num_subtitle_streams; i++)
    {
    if(t->subtitle_streams[i].action == BGAV_STREAM_MUTE)
      continue;
    num_active_subtitle_streams++;
    if(!t->subtitle_streams[i].data.subtitle.video_stream)
      t->subtitle_streams[i].data.subtitle.video_stream =
        t->video_streams;

    if(!t->subtitle_streams[i].data.subtitle.video_stream)
      {
      bgav_log(demuxer->opt, BGAV_LOG_ERROR, LOG_DOMAIN,
               "Cannot decode subtitles from stream %d (no video)",
               i+1);
      return 0;
      }
    video_stream = t->subtitle_streams[i].data.subtitle.video_stream;
    
    /* Check, if we must get the video format from the decoder */
    video_format =  &video_stream->data.video.format;

    if((video_stream->action == BGAV_STREAM_MUTE) &&
       !video_stream->initialized)
      {
      /* Start the video decoder to get the format */
      video_stream->action = BGAV_STREAM_DECODE;
      video_stream->demuxer = demuxer;
      bgav_stream_start(video_stream);
      bgav_stream_stop(video_stream);
      video_stream->action = BGAV_STREAM_MUTE;
      }
    
    if((!video_format->image_width || !video_format->image_height ||
        !video_format->timescale || !video_format->frame_duration) &&
       (video_stream->action != BGAV_STREAM_PARSE))
      {
      bgav_log(demuxer->opt, BGAV_LOG_ERROR, LOG_DOMAIN,
               "Starting subtitle decoder for stream %d failed (cannot get video format)",
               i+1);
      return 0;
      }
    
    /* For text subtitles, copy the video format */
    
    if(t->subtitle_streams[i].type == BGAV_STREAM_SUBTITLE_TEXT)
      {
      gavl_video_format_copy(&(t->subtitle_streams[i].data.subtitle.format), video_format);
      }
    
    if(!bgav_stream_start(&(t->subtitle_streams[i])))
      {
      bgav_log(demuxer->opt, BGAV_LOG_ERROR, LOG_DOMAIN,
               "Starting subtitle decoder for stream %d failed", i+1);
      return 0;
      }
    }

  if((!num_active_audio_streams && !num_active_video_streams &&
     num_active_subtitle_streams) ||
     (demuxer->demux_mode == DEMUX_MODE_FI))
    demuxer->flags |= BGAV_DEMUXER_PEEK_FORCES_READ;
  else
    demuxer->flags &= ~BGAV_DEMUXER_PEEK_FORCES_READ;
  
  return 1;
  }


void bgav_track_dump(bgav_t * b, bgav_track_t * t)
  {
  int i;
  const char * description;
  
  char duration_string[GAVL_TIME_STRING_LEN];
  
  bgav_dprintf( "Name:     %s\n", t->name);

  description = bgav_get_description(b);
  
  bgav_dprintf( "Format:   %s\n", (description ? description : 
                                   "Not specified"));
  bgav_dprintf( "Seekable: %s\n", ((b->demuxer->flags & BGAV_DEMUXER_CAN_SEEK) ? "Yes" : "No"));

  bgav_dprintf( "Duration: ");
  if(t->duration != GAVL_TIME_UNDEFINED)
    {
    gavl_time_prettyprint(t->duration, duration_string);
    bgav_dprintf( "%s\n", duration_string);
    }
  else
    bgav_dprintf( "Not specified (maybe live)\n");

  
  bgav_metadata_dump(&(t->metadata));

  if(t->chapter_list)
    bgav_chapter_list_dump(t->chapter_list);
  
  for(i = 0; i < t->num_audio_streams; i++)
    {
    bgav_stream_dump(&(t->audio_streams[i]));
    bgav_audio_dump(&(t->audio_streams[i]));
    }
  for(i = 0; i < t->num_video_streams; i++)
    {
    bgav_stream_dump(&(t->video_streams[i]));
    bgav_video_dump(&(t->video_streams[i]));
    }
  for(i = 0; i < t->num_subtitle_streams; i++)
    {
    bgav_stream_dump(&(t->subtitle_streams[i]));
    bgav_subtitle_dump(&(t->subtitle_streams[i]));
    }
  
  }

void bgav_track_free(bgav_track_t * t)
  {
  int i;
  
  bgav_metadata_free(&(t->metadata));
  if(t->chapter_list)
    bgav_chapter_list_destroy(t->chapter_list);
  
  if(t->audio_streams)
    {
    for(i = 0; i < t->num_audio_streams; i++)
      bgav_stream_free(&(t->audio_streams[i]));
    free(t->audio_streams);
    }
  if(t->video_streams)
    {
    for(i = 0; i < t->num_video_streams; i++)
      bgav_stream_free(&(t->video_streams[i]));
    free(t->video_streams);
    }
  if(t->subtitle_streams)
    {
    for(i = 0; i < t->num_subtitle_streams; i++)
      bgav_stream_free(&(t->subtitle_streams[i]));
    free(t->subtitle_streams);
    }
  if(t->name)
    free(t->name);
  }

static void remove_stream(bgav_stream_t * stream_array, int index, int num)
  {
  /* Streams are sometimes also removed for other reasons */
  bgav_stream_free(&stream_array[index]);
  if(index < num - 1)
    {
    memmove(&(stream_array[index]),
            &(stream_array[index+1]),
            sizeof(*stream_array) * (num - 1 - index));
    }
  }

void bgav_track_remove_audio_stream(bgav_track_t * track, int stream)
  {
  remove_stream(track->audio_streams, stream, track->num_audio_streams);
  track->num_audio_streams--;
  }

void bgav_track_remove_video_stream(bgav_track_t * track, int stream)
  {
  remove_stream(track->video_streams, stream, track->num_video_streams);
  track->num_video_streams--;
  }

void bgav_track_remove_subtitle_stream(bgav_track_t * track, int stream)
  {
  remove_stream(track->subtitle_streams, stream, track->num_subtitle_streams);
  track->num_subtitle_streams--;
  }

void bgav_track_remove_unsupported(bgav_track_t * track)
  {
  int i;
  bgav_stream_t * s;

  i = 0;
  while(i < track->num_audio_streams)
    {
    s = &(track->audio_streams[i]);
    if(!bgav_find_audio_decoder(s))
      {
      bgav_track_remove_audio_stream(track, i);

      if(!(s->fourcc & 0xffff0000))
        bgav_log(s->opt, BGAV_LOG_WARNING, LOG_DOMAIN,
                 "No audio decoder found for WAVId 0x%04x",
                 s->fourcc);
      else
        bgav_log(s->opt, BGAV_LOG_WARNING, LOG_DOMAIN,
                 "No audio decoder found for fourcc %c%c%c%c (0x%08x)",
                 (s->fourcc & 0xFF000000) >> 24,
                 (s->fourcc & 0x00FF0000) >> 16,
                 (s->fourcc & 0x0000FF00) >> 8,
                 (s->fourcc & 0x000000FF),
                 s->fourcc);
      }
    else if((s->flags & STREAM_PARSE_FULL) &&
       !bgav_audio_parser_supported(s->fourcc))
      bgav_track_remove_audio_stream(track, i);
    else
      i++;
    }
  i = 0;
  while(i < track->num_video_streams)
    {
    s = &(track->video_streams[i]);
    if(!bgav_find_video_decoder(s))
      {
      bgav_track_remove_video_stream(track, i);
      
      bgav_log(s->opt, BGAV_LOG_WARNING, LOG_DOMAIN,
               "No video decoder found for fourcc %c%c%c%c (0x%08x)",
               (s->fourcc & 0xFF000000) >> 24,
               (s->fourcc & 0x00FF0000) >> 16,
               (s->fourcc & 0x0000FF00) >> 8,
               (s->fourcc & 0x000000FF),
               s->fourcc);
      }
    else if((s->flags & (STREAM_PARSE_FULL|STREAM_PARSE_FRAME)) &&
       !bgav_video_parser_supported(s->fourcc))
      bgav_track_remove_video_stream(track, i);
    else
      i++;
    }
  }

void bgav_track_clear(bgav_track_t * track)
  {
  int i;
  for(i = 0; i < track->num_audio_streams; i++)
    bgav_stream_clear(&(track->audio_streams[i]));
  for(i = 0; i < track->num_video_streams; i++)
    bgav_stream_clear(&(track->video_streams[i]));
  for(i = 0; i < track->num_subtitle_streams; i++)
    bgav_stream_clear(&(track->subtitle_streams[i]));
  }

void bgav_track_resync(bgav_track_t * track)
  {
  int i;
  bgav_stream_t * s;
  
  for(i = 0; i < track->num_audio_streams; i++)
    {
    s = &(track->audio_streams[i]);

    if(s->action != BGAV_STREAM_DECODE)
      continue;
    bgav_audio_resync(s);
    }
  for(i = 0; i < track->num_video_streams; i++)
    {
    s = &(track->video_streams[i]);

    if(s->action != BGAV_STREAM_DECODE)
      continue;
    
    bgav_video_resync(s);
    
    }
  }

int bgav_track_skipto(bgav_track_t * track, int64_t * time, int scale)
  {
  int i;
  bgav_stream_t * s;
  int64_t t;
  
  for(i = 0; i < track->num_video_streams; i++)
    {
    t = *time;
    s = &(track->video_streams[i]);
    
    if(!bgav_stream_skipto(s, &t, scale))
      {
      return 0;
      }
    if(!i)
      *time = t;
    }
  for(i = 0; i < track->num_audio_streams; i++)
    {
    s = &(track->audio_streams[i]);

    if(!bgav_stream_skipto(s, time, scale))
      {
      return 0;
      }
    }
  return 1;
  }

void bgav_track_calc_duration(bgav_track_t * t)
  {
  int i;
  gavl_time_t test_duration;
  /* Get duration */
  for(i = 0; i < t->num_audio_streams; i++)
    {
    test_duration =
      gavl_time_unscale(t->audio_streams[i].data.audio.format.samplerate,
                        t->audio_streams[i].duration);
    if(t->duration < test_duration)
      t->duration = test_duration;
    }
  for(i = 0; i < t->num_video_streams; i++)
    {
    if(t->video_streams[i].data.video.format.framerate_mode == GAVL_FRAMERATE_STILL)
      continue;
    test_duration =
      gavl_time_unscale(t->video_streams[i].data.video.format.timescale,
                        t->video_streams[i].duration);
    if(t->duration < test_duration)
      t->duration = test_duration;
    }
  for(i = 0; i < t->num_subtitle_streams; i++)
    {
    test_duration =
      gavl_time_unscale(t->subtitle_streams[i].timescale,
                        t->subtitle_streams[i].duration);
    if(t->duration < test_duration)
      t->duration = test_duration;
    }
  }


int bgav_track_has_sync(bgav_track_t * t)
  {
  int i;
  bgav_stream_t * s;
  
  for(i = 0; i < t->num_audio_streams; i++)
    {
    s = &t->audio_streams[i];
    if((s->action == BGAV_STREAM_DECODE) &&
       (!STREAM_HAS_SYNC(s)))
      return 0;
    }
  for(i = 0; i < t->num_video_streams; i++)
    {
    s = &t->video_streams[i];
    if(!(s->flags & STREAM_STILL_MODE) && (s->action == BGAV_STREAM_DECODE) &&
       (!STREAM_HAS_SYNC(s)))
      return 0;
    }
  return 1;
  }

void bgav_track_mute(bgav_track_t * t)
  {
  int i;
  for(i = 0; i < t->num_audio_streams; i++)
    t->audio_streams[i].action = BGAV_STREAM_MUTE;
  for(i = 0; i < t->num_video_streams; i++)
    t->video_streams[i].action = BGAV_STREAM_MUTE;
  for(i = 0; i < t->num_subtitle_streams; i++)
    t->subtitle_streams[i].action = BGAV_STREAM_MUTE;
  }

static int check_sync_time(bgav_stream_t * s, int64_t * t, int scale)
  {
  int64_t tt;
  if((s->action == BGAV_STREAM_MUTE) ||
     (s->flags & STREAM_STILL_MODE))
    return 1;
  
  if(!STREAM_HAS_SYNC(s))
    return 0;

  tt = gavl_time_rescale(s->timescale, scale, STREAM_GET_SYNC(s));
  if(tt > *t)
    *t = tt;
  return 1;
  }

int64_t bgav_track_sync_time(bgav_track_t * t, int scale)
  {
  int64_t ret = BGAV_TIMESTAMP_UNDEFINED;
  bgav_stream_t * s;
  int i;
  
  for(i = 0; i < t->num_audio_streams; i++)
    {
    s = &t->audio_streams[i];

    if(!check_sync_time(s, &ret, scale))
      return BGAV_TIMESTAMP_UNDEFINED;
    }
  for(i = 0; i < t->num_video_streams; i++)
    {
    s = &t->video_streams[i];
    if(!check_sync_time(s, &ret, scale))
      return BGAV_TIMESTAMP_UNDEFINED;
    }
  return ret;
  }

static int check_out_time(bgav_stream_t * s, int64_t * t, int scale, int stream_scale)
  {
  int64_t tt;
  if((s->action == BGAV_STREAM_MUTE) ||
     (s->flags & STREAM_STILL_MODE))
    return 1;
  
  if(!STREAM_HAS_SYNC(s))
    return 0;

  tt = gavl_time_rescale(s->timescale, scale, STREAM_GET_SYNC(s));
  if(tt > *t)
    *t = tt;
  return 1;
  }


int64_t bgav_track_out_time(bgav_track_t * t, int scale)
  {
  int64_t ret = BGAV_TIMESTAMP_UNDEFINED;
  bgav_stream_t * s;
  int i;
  
  for(i = 0; i < t->num_audio_streams; i++)
    {
    s = &t->audio_streams[i];
    if(!check_out_time(s, &ret, scale, s->data.audio.format.samplerate))
      return BGAV_TIMESTAMP_UNDEFINED;
    }
  for(i = 0; i < t->num_video_streams; i++)
    {
    s = &t->video_streams[i];
    if(!check_out_time(s, &ret, scale, s->data.video.format.timescale))
      return BGAV_TIMESTAMP_UNDEFINED;
    }
  return ret;
  }

void bgav_track_set_eof_d(bgav_track_t * t)
  {
  int i;
  bgav_stream_t * s;
  for(i = 0; i < t->num_audio_streams; i++)
    {
    s = &t->audio_streams[i];
    s->flags |= STREAM_EOF_D;
    }
  for(i = 0; i < t->num_video_streams; i++)
    {
    s = &t->video_streams[i];
    s->flags |= STREAM_EOF_D;
    }
  for(i = 0; i < t->num_subtitle_streams; i++)
    {
    s = &t->subtitle_streams[i];
    s->flags |= STREAM_EOF_D;
    }
  }

void bgav_track_clear_eof_d(bgav_track_t * t)
  {
  int i;
  bgav_stream_t * s;
  for(i = 0; i < t->num_audio_streams; i++)
    {
    s = &t->audio_streams[i];
    s->flags &= ~STREAM_EOF_D;
    }
  for(i = 0; i < t->num_video_streams; i++)
    {
    s = &t->video_streams[i];
    s->flags &= ~STREAM_EOF_D;
    }
  for(i = 0; i < t->num_subtitle_streams; i++)
    {
    s = &t->subtitle_streams[i];
    s->flags &= ~STREAM_EOF_D;
    }
  }

int bgav_track_eof_d(bgav_track_t * t)
  {
  int i;
  bgav_stream_t * s;
  for(i = 0; i < t->num_audio_streams; i++)
    {
    s = &t->audio_streams[i];
    if((s->action != BGAV_STREAM_MUTE) && !(s->flags & STREAM_EOF_D))
      return 0;
    }
  for(i = 0; i < t->num_video_streams; i++)
    {
    s = &t->video_streams[i];
    if((s->action != BGAV_STREAM_MUTE) && !(s->flags & STREAM_EOF_D))
      return 0;
    }
  for(i = 0; i < t->num_subtitle_streams; i++)
    {
    s = &t->subtitle_streams[i];
    if((s->action != BGAV_STREAM_MUTE) && !(s->flags & STREAM_EOF_D))
      return 0;
    }
  return 1;
  }

void bgav_track_get_compression(bgav_track_t * t)
  {
  int i;
  bgav_stream_t * s;

  /* Set all streams to read mode */
  for(i = 0; i < t->num_audio_streams; i++)
    {
    s = &t->audio_streams[i];
    s->action = BGAV_STREAM_READRAW;
    }
  for(i = 0; i < t->num_video_streams; i++)
    {
    s = &t->video_streams[i];
    s->action = BGAV_STREAM_READRAW;
    }

  /* Get a first packet. This will complete the formats */
  for(i = 0; i < t->num_audio_streams; i++)
    {
    s = &t->audio_streams[i];
    if(s->flags & (STREAM_PARSE_FULL|STREAM_PARSE_FRAME))
      {
      bgav_stream_start(s);
      bgav_demuxer_peek_packet_read(s->demuxer, s, 1);
      }
    }
  for(i = 0; i < t->num_video_streams; i++)
    {
    s = &t->video_streams[i];
    if(s->flags & (STREAM_PARSE_FULL|STREAM_PARSE_FRAME))
      {
      bgav_stream_start(s);
      bgav_demuxer_peek_packet_read(s->demuxer, s, 1);
      }
    }

  /* Set all streams back to mute mode */
  for(i = 0; i < t->num_audio_streams; i++)
    {
    s = &t->audio_streams[i];
    s->action = BGAV_STREAM_MUTE;
    }
  for(i = 0; i < t->num_video_streams; i++)
    {
    s = &t->video_streams[i];
    s->action = BGAV_STREAM_MUTE;
    }
  
  
  }
