/*****************************************************************
 * gmerlin-avdecoder - a general purpose multimedia decoding library
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

#include <avdec_private.h>
#include <parser.h>

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define LOG_DOMAIN "track"

bgav_stream_t * bgav_track_get_subtitle_stream(bgav_track_t * t, int index)
  {
  /* First come the overlay streams, then the text streams */

  if(index >= t->num_overlay_streams)
    return &t->text_streams[index - t->num_overlay_streams];
  else
    return &t->overlay_streams[index];
  }

bgav_stream_t *
bgav_track_add_audio_stream(bgav_track_t * t, const bgav_options_t * opt)
  {
  bgav_stream_t * ret;
  t->num_audio_streams++;
  t->audio_streams = realloc(t->audio_streams, t->num_audio_streams * 
                             sizeof(*(t->audio_streams)));

  ret = &t->audio_streams[t->num_audio_streams-1];
  
  bgav_stream_init(ret, opt);
  bgav_stream_create_packet_pool(ret);
  bgav_stream_create_packet_buffer(ret);

  // ret->data.audio.bits_per_sample = 16;
  ret->type = BGAV_STREAM_AUDIO;
  ret->track = t;
  
  return ret;
  }

bgav_stream_t *
bgav_track_add_video_stream(bgav_track_t * t, const bgav_options_t * opt)
  {
  bgav_stream_t * ret;
  t->num_video_streams++;
  t->video_streams = realloc(t->video_streams, t->num_video_streams * 
                             sizeof(*(t->video_streams)));
  
  ret = &t->video_streams[t->num_video_streams-1];
  bgav_stream_init(ret, opt);
  bgav_stream_create_packet_pool(ret);
  bgav_stream_create_packet_buffer(ret);
  ret->type = BGAV_STREAM_VIDEO;
  ret->opt = opt;
  ret->track = t;
  ret->data.video.format.interlace_mode = GAVL_INTERLACE_UNKNOWN;
  ret->data.video.format.framerate_mode = GAVL_FRAMERATE_UNKNOWN;
  return ret;
  }

static bgav_stream_t * add_text_stream(bgav_track_t * t,
                                       const bgav_options_t * opt,
                                       const char * charset,
                                       bgav_subtitle_reader_context_t * r)
  {
  bgav_stream_t * ret;
  
  t->num_text_streams++;
  t->text_streams = realloc(t->text_streams, t->num_text_streams * 
                                sizeof(*(t->text_streams)));
  
  ret = &t->text_streams[t->num_text_streams-1];
  bgav_stream_init(ret, opt);
  bgav_stream_create_packet_pool(ret);

  ret->flags |= STREAM_DISCONT;
  if(!r)
    bgav_stream_create_packet_buffer(ret);
  else
    {
    ret->data.subtitle.subreader = r;
    ret->flags |= STREAM_SUBREADER;
    }
  ret->type = BGAV_STREAM_SUBTITLE_TEXT;
  if(charset)
    ret->data.subtitle.charset =
      bgav_strdup(charset);
  else if(r && r->charset) // Reader knows about charset and will do the
    // conversion.

    ret->data.subtitle.charset = bgav_strdup(BGAV_UTF8);
  else
    ret->data.subtitle.charset =
      bgav_strdup(ret->opt->default_subtitle_encoding);
  
  ret->track = t;
  return ret;
  }

static bgav_stream_t * add_overlay_stream(bgav_track_t * t,
                                          const bgav_options_t * opt,
                                          bgav_subtitle_reader_context_t * r)
  {
  bgav_stream_t * ret;
  
  t->num_overlay_streams++;
  t->overlay_streams = realloc(t->overlay_streams, t->num_overlay_streams * 
                                sizeof(*(t->overlay_streams)));

  ret = &t->overlay_streams[t->num_overlay_streams-1];
  bgav_stream_init(ret, opt);
  bgav_stream_create_packet_pool(ret);

  ret->flags |= STREAM_DISCONT;
  if(!r)
    bgav_stream_create_packet_buffer(ret);
  else
    {
    ret->data.subtitle.subreader = r;
    ret->flags |= STREAM_SUBREADER;
    }
  ret->type = BGAV_STREAM_SUBTITLE_OVERLAY;
  ret->track = t;
  return ret;
  }

bgav_stream_t *
bgav_track_add_text_stream(bgav_track_t * t, const bgav_options_t * opt,
                           const char * charset)
  {
  return add_text_stream(t, opt, charset, NULL);
  }

bgav_stream_t *
bgav_track_add_overlay_stream(bgav_track_t * t, const bgav_options_t * opt)
  {
  return add_overlay_stream(t, opt, NULL);
  }

bgav_stream_t *
bgav_track_attach_subtitle_reader(bgav_track_t * t,
                                  const bgav_options_t * opt,
                                  bgav_subtitle_reader_context_t * r)
  {
  bgav_stream_t * ret;

  if(r->reader->type == BGAV_STREAM_SUBTITLE_TEXT)
    ret = add_text_stream(t, opt,
                          NULL, r);
  else
    ret = add_overlay_stream(t, opt, r);
  
  if(r->info)
    gavl_metadata_set(&ret->m, GAVL_META_LABEL, r->info);
  return ret;
  }

bgav_stream_t *
bgav_track_find_stream_all(bgav_track_t * t, int stream_id)
  {
  int i;
  for(i = 0; i < t->num_audio_streams; i++)
    {
    if(t->audio_streams[i].stream_id == stream_id)
      return &t->audio_streams[i];
    }
  for(i = 0; i < t->num_video_streams; i++)
    {
    if(t->video_streams[i].stream_id == stream_id)
      return &t->video_streams[i];
    }
  for(i = 0; i < t->num_text_streams; i++)
    {
    if((t->text_streams[i].stream_id == stream_id) &&
       (!t->text_streams[i].data.subtitle.subreader))
      return &t->text_streams[i];
    }
  for(i = 0; i < t->num_overlay_streams; i++)
    {
    if((t->overlay_streams[i].stream_id == stream_id) &&
       (!t->overlay_streams[i].data.subtitle.subreader))
      return &t->overlay_streams[i];
    }
  return NULL;
  }

bgav_stream_t * bgav_track_find_stream(bgav_demuxer_context_t * ctx,
                                       int stream_id)
  {
  int i;
  bgav_track_t * t;
  if(ctx->demux_mode == DEMUX_MODE_FI)
    {
    if(ctx->request_stream && (stream_id == ctx->request_stream->stream_id))
      return ctx->request_stream;
    else
      return NULL;
    }
  t = ctx->tt->cur;
  
  for(i = 0; i < t->num_audio_streams; i++)
    {
    if(t->audio_streams[i].stream_id == stream_id)
      {
      if(t->audio_streams[i].action != BGAV_STREAM_MUTE)
        return &t->audio_streams[i];
      else
        return NULL;
      }
    }
  for(i = 0; i < t->num_video_streams; i++)
    {
    if(t->video_streams[i].stream_id == stream_id)
      {
      if(t->video_streams[i].action != BGAV_STREAM_MUTE)
        return &t->video_streams[i];
      else
        return NULL;
      }
    }
  for(i = 0; i < t->num_text_streams; i++)
    {
    if((t->text_streams[i].stream_id == stream_id) &&
       (!t->text_streams[i].data.subtitle.subreader))
      {
      if(t->text_streams[i].action != BGAV_STREAM_MUTE)
        return &t->text_streams[i];
      else
        return NULL;
      }
    }
  for(i = 0; i < t->num_overlay_streams; i++)
    {
    if((t->overlay_streams[i].stream_id == stream_id) &&
       (!t->overlay_streams[i].data.subtitle.subreader))
      {
      if(t->overlay_streams[i].action != BGAV_STREAM_MUTE)
        return &t->overlay_streams[i];
      else
        return NULL;
      }
    }
  return NULL;
  }

#define FREE(ptr) if(ptr){free(ptr);ptr=NULL;}
  
void bgav_track_stop(bgav_track_t * t)
  {
  int i;
  
  for(i = 0; i < t->num_audio_streams; i++)
    bgav_stream_stop(&t->audio_streams[i]);
  for(i = 0; i < t->num_video_streams; i++)
    bgav_stream_stop(&t->video_streams[i]);
  for(i = 0; i < t->num_text_streams; i++)
    bgav_stream_stop(&t->text_streams[i]);
  for(i = 0; i < t->num_overlay_streams; i++)
    bgav_stream_stop(&t->overlay_streams[i]);
  }

typedef struct
  {
  int num_active_subtitle_streams;
  bgav_track_t * t;
  bgav_demuxer_context_t * demuxer;
  } start_subtitle_t;

static int start_subtitle(void * data, bgav_stream_t * s)
  {
  gavl_video_format_t * video_format;
  bgav_stream_t * video_stream;
  start_subtitle_t * ss = data;
  
  if(s->action == BGAV_STREAM_MUTE)
    return 1;
  ss->num_active_subtitle_streams++;
  if(!s->data.subtitle.video_stream &&
     ss->t->num_video_streams)
    {
    s->data.subtitle.video_stream =
      ss->t->video_streams;
    }
  if(!s->data.subtitle.video_stream)
    {
    bgav_log(s->opt, BGAV_LOG_ERROR, LOG_DOMAIN,
             "Cannot decode subtitles (no video)");
    return 0;
    }
  video_stream = s->data.subtitle.video_stream;
    
  /* Check, if we must get the video format from the decoder */
  video_format =  &video_stream->data.video.format;

  if((video_stream->action == BGAV_STREAM_MUTE) &&
     !video_stream->initialized)
    {
    /* Start the video decoder to get the format */
    video_stream->action = BGAV_STREAM_DECODE;
    video_stream->demuxer = ss->demuxer;
    bgav_stream_start(video_stream);
    bgav_stream_stop(video_stream);
    video_stream->action = BGAV_STREAM_MUTE;
    }
    
  if((!video_format->image_width || !video_format->image_height ||
      !video_format->timescale) &&
     (video_stream->action != BGAV_STREAM_PARSE))
    {
    bgav_log(s->opt, BGAV_LOG_ERROR, LOG_DOMAIN,
             "Starting subtitle decoder failed (cannot get video format)");
    return 0;
    }
    
  /*
   *  For text subtitles, copy the video format.
   *  TODO: This shouldn't be necessary!!
   */
    
  if(s->type == BGAV_STREAM_SUBTITLE_TEXT)
    {
    gavl_video_format_copy(&s->data.subtitle.video.format,
                           video_format);
    }
    
  if(!bgav_stream_start(s))
    {
    bgav_log(s->opt, BGAV_LOG_ERROR, LOG_DOMAIN,
             "Starting subtitle decoder failed");
    return 0;
    }
  return 1;
  }

int bgav_track_start(bgav_track_t * t, bgav_demuxer_context_t * demuxer)
  {
  start_subtitle_t ss;
  int i;
  int num_active_audio_streams = 0;
  int num_active_video_streams = 0;
  
  for(i = 0; i < t->num_audio_streams; i++)
    {
    if(t->audio_streams[i].action == BGAV_STREAM_MUTE)
      continue;
    num_active_audio_streams++;
    if(!bgav_stream_start(&t->audio_streams[i]))
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
    if(!bgav_stream_start(&t->video_streams[i]))
      {
      bgav_log(demuxer->opt, BGAV_LOG_ERROR, LOG_DOMAIN,
               "Starting video decoder for stream %d failed", i+1);
      return 0;
      }
    }

  ss.demuxer = demuxer;
  ss.t = t;
  ss.num_active_subtitle_streams = 0;
  
  if(!bgav_streams_foreach(t->text_streams, t->num_text_streams, start_subtitle, &ss) ||
     !bgav_streams_foreach(t->overlay_streams, t->num_overlay_streams, start_subtitle, &ss))
    return 0;
  
  if((!num_active_audio_streams && !num_active_video_streams &&
      ss.num_active_subtitle_streams) ||
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
  
  description = bgav_get_description(b);
  
  bgav_dprintf( "Format:   %s\n", (description ? description : 
                                   "Not specified"));
  bgav_dprintf( "Seekable: %s\n",
                ((b->demuxer->flags & BGAV_DEMUXER_CAN_SEEK) ? "Yes" : "No"));

  bgav_dprintf( "Duration: ");
  if(t->duration != GAVL_TIME_UNDEFINED)
    {
    gavl_time_prettyprint(t->duration, duration_string);
    bgav_dprintf( "%s\n", duration_string);
    }
  else
    bgav_dprintf( "Not specified (maybe live)\n");

  bgav_diprintf(2, "Metadata\n");
  gavl_metadata_dump(&t->metadata, 4);

  if(t->chapter_list)
    gavl_chapter_list_dump(t->chapter_list);
  
  for(i = 0; i < t->num_audio_streams; i++)
    {
    bgav_stream_dump(&t->audio_streams[i]);
    bgav_audio_dump(&t->audio_streams[i]);
    }
  for(i = 0; i < t->num_video_streams; i++)
    {
    bgav_stream_dump(&t->video_streams[i]);
    bgav_video_dump(&t->video_streams[i]);
    }
  for(i = 0; i < t->num_overlay_streams; i++)
    {
    bgav_stream_dump(&t->overlay_streams[i]);
    bgav_subtitle_dump(&t->overlay_streams[i]);
    }
  for(i = 0; i < t->num_text_streams; i++)
    {
    bgav_stream_dump(&t->text_streams[i]);
    bgav_subtitle_dump(&t->text_streams[i]);
    }
  
  }

void bgav_track_free(bgav_track_t * t)
  {
  int i;
  
  gavl_metadata_free(&t->metadata);
  if(t->chapter_list)
    gavl_chapter_list_destroy(t->chapter_list);
  
  if(t->audio_streams)
    {
    for(i = 0; i < t->num_audio_streams; i++)
      bgav_stream_free(&t->audio_streams[i]);
    free(t->audio_streams);
    }
  if(t->video_streams)
    {
    for(i = 0; i < t->num_video_streams; i++)
      bgav_stream_free(&t->video_streams[i]);
    free(t->video_streams);
    }
  if(t->text_streams)
    {
    for(i = 0; i < t->num_text_streams; i++)
      bgav_stream_free(&t->text_streams[i]);
    free(t->text_streams);
    }
  if(t->overlay_streams)
    {
    for(i = 0; i < t->num_overlay_streams; i++)
      bgav_stream_free(&t->overlay_streams[i]);
    free(t->overlay_streams);
    }
  }

static void remove_stream(bgav_stream_t * stream_array, int index, int num)
  {
  /* Streams are sometimes also removed for other reasons */
  bgav_stream_free(&stream_array[index]);
  if(index < num - 1)
    {
    memmove(&stream_array[index],
            &stream_array[index+1],
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
  /* Remove this stream from the subtitle streams as well */
  int i;
  for(i = 0; i < track->num_text_streams; i++)
    {
    if(track->text_streams[i].data.subtitle.video_stream ==
       &track->video_streams[stream])
      track->text_streams[i].data.subtitle.video_stream = NULL;
    }
  for(i = 0; i < track->num_overlay_streams; i++)
    {
    if(track->overlay_streams[i].data.subtitle.video_stream ==
       &track->video_streams[stream])
      track->overlay_streams[i].data.subtitle.video_stream = NULL;
    }
  
  remove_stream(track->video_streams, stream, track->num_video_streams);
  track->num_video_streams--;
  }

#if 0
void bgav_track_remove_subtitle_stream(bgav_track_t * track, int stream)
  {
  remove_stream(track->subtitle_streams, stream, track->num_subtitle_streams);
  track->num_subtitle_streams--;
  }
#endif

void bgav_track_remove_text_stream(bgav_track_t * track, int stream)
  {
  remove_stream(track->text_streams, stream, track->num_text_streams);
  track->num_text_streams--;
  }

void bgav_track_remove_overlay_stream(bgav_track_t * track, int stream)
  {
  remove_stream(track->overlay_streams, stream, track->num_overlay_streams);
  track->num_overlay_streams--;
  }

void bgav_track_remove_unsupported(bgav_track_t * track)
  {
  int i;
  bgav_stream_t * s;

  i = 0;
  while(i < track->num_audio_streams)
    {
    s = &track->audio_streams[i];
    if(!bgav_find_audio_decoder(s->fourcc))
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
    else if((s->flags & (STREAM_PARSE_FULL|STREAM_PARSE_FRAME)) &&
       !bgav_audio_parser_supported(s->fourcc))
      {
      if(!(s->fourcc & 0xffff0000))
        bgav_log(s->opt, BGAV_LOG_WARNING, LOG_DOMAIN,
                 "No audio parser found for WAVId 0x%04x",
                 s->fourcc);
      else
        bgav_log(s->opt, BGAV_LOG_WARNING, LOG_DOMAIN,
                 "No audio parser found for fourcc %c%c%c%c (0x%08x)",
                 (s->fourcc & 0xFF000000) >> 24,
                 (s->fourcc & 0x00FF0000) >> 16,
                 (s->fourcc & 0x0000FF00) >> 8,
                 (s->fourcc & 0x000000FF),
                 s->fourcc);


      bgav_track_remove_audio_stream(track, i);
      }
    else
      i++;
    }
  i = 0;
  while(i < track->num_video_streams)
    {
    s = &track->video_streams[i];
    if(!bgav_find_video_decoder(s->fourcc))
      {
      bgav_log(s->opt, BGAV_LOG_WARNING, LOG_DOMAIN,
               "No video decoder found for fourcc %c%c%c%c (0x%08x)",
               (s->fourcc & 0xFF000000) >> 24,
               (s->fourcc & 0x00FF0000) >> 16,
               (s->fourcc & 0x0000FF00) >> 8,
               (s->fourcc & 0x000000FF),
               s->fourcc);
      bgav_track_remove_video_stream(track, i);
      }
    else if((s->flags & (STREAM_PARSE_FULL|STREAM_PARSE_FRAME)) &&
       !bgav_video_parser_supported(s->fourcc))
      {
      bgav_log(s->opt, BGAV_LOG_WARNING, LOG_DOMAIN,
               "No video parser found for fourcc %c%c%c%c (0x%08x)",
               (s->fourcc & 0xFF000000) >> 24,
               (s->fourcc & 0x00FF0000) >> 16,
               (s->fourcc & 0x0000FF00) >> 8,
               (s->fourcc & 0x000000FF),
               s->fourcc);
      bgav_track_remove_video_stream(track, i);
      }
    else
      i++;
    }
  }

int bgav_track_foreach(bgav_track_t * t,
                     int (*action)(void * priv, bgav_stream_t * s), void * priv)
  {
  if(!bgav_streams_foreach(t->audio_streams,   t->num_audio_streams,   action, priv) ||
     !bgav_streams_foreach(t->video_streams,   t->num_video_streams,   action, priv) ||
     !bgav_streams_foreach(t->text_streams,    t->num_text_streams,    action, priv) ||
     !bgav_streams_foreach(t->overlay_streams, t->num_overlay_streams, action, priv))
    return 0;
  return 1;
  }

static int clear_stream(void * priv, bgav_stream_t * s)
  {
  bgav_stream_clear(s);
  return 1;
  }

void bgav_track_clear(bgav_track_t * track)
  {
  bgav_track_foreach(track, clear_stream, NULL);
  }

void bgav_track_resync(bgav_track_t * track)
  {
  int i;
  bgav_stream_t * s;
  
  for(i = 0; i < track->num_audio_streams; i++)
    {
    s = &track->audio_streams[i];

    if(s->action != BGAV_STREAM_DECODE)
      continue;
    bgav_audio_resync(s);
    }
  for(i = 0; i < track->num_video_streams; i++)
    {
    s = &track->video_streams[i];

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
    s = &track->video_streams[i];
    
    if(!bgav_stream_skipto(s, &t, scale))
      {
      return 0;
      }
    if(!i)
      *time = t;
    }
  for(i = 0; i < track->num_audio_streams; i++)
    {
    s = &track->audio_streams[i];

    if(!bgav_stream_skipto(s, time, scale))
      {
      return 0;
      }
    }
  return 1;
  }

typedef struct
  {
  bgav_track_t * t;
  } calc_duration_t;

static int calc_duration_audio(void * priv, bgav_stream_t * s)
  {
  gavl_time_t test_duration;
  calc_duration_t * cd = priv;
  
  test_duration =
      gavl_time_unscale(s->data.audio.format.samplerate,
                        s->duration);
  if(cd->t->duration < test_duration)
    cd->t->duration = test_duration;
  
  return 1;
  }

static int calc_duration_video(void * priv, bgav_stream_t * s)
  {
  gavl_time_t test_duration;
  calc_duration_t * cd = priv;
  
  test_duration =
      gavl_time_unscale(s->data.video.format.timescale,
                        s->duration);
  if(cd->t->duration < test_duration)
    cd->t->duration = test_duration;
  
  return 1;
  }

static int calc_duration_subtitle(void * priv, bgav_stream_t * s)
  {
  gavl_time_t test_duration;
  calc_duration_t * cd = priv;
  
  test_duration = gavl_time_unscale(s->timescale, s->duration);
  if(cd->t->duration < test_duration)
    cd->t->duration = test_duration;
  
  return 1;
  }
  
void bgav_track_calc_duration(bgav_track_t * t)
  {
  calc_duration_t cd;
  cd.t = t;

  bgav_streams_foreach(t->audio_streams, t->num_audio_streams,
                       calc_duration_audio, &cd);
  bgav_streams_foreach(t->video_streams, t->num_video_streams,
                       calc_duration_video, &cd);
  bgav_streams_foreach(t->text_streams, t->num_text_streams,
                       calc_duration_subtitle, &cd);
  bgav_streams_foreach(t->overlay_streams, t->num_overlay_streams,
                       calc_duration_subtitle, &cd);
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
    if(!STREAM_IS_STILL(s) && (s->action == BGAV_STREAM_DECODE) &&
       (!STREAM_HAS_SYNC(s)))
      return 0;
    }
  return 1;
  }

static int mute_stream(void * priv, bgav_stream_t * s)
  {
  s->action = BGAV_STREAM_MUTE;
  return 1;
  }

void bgav_track_mute(bgav_track_t * t)
  {
  bgav_track_foreach(t, mute_stream, NULL);
  }

static int check_sync_time(bgav_stream_t * s, int64_t * t, int scale)
  {
  int64_t tt;
  if((s->action == BGAV_STREAM_MUTE) ||
     STREAM_IS_STILL(s))
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
  int64_t ret = GAVL_TIME_UNDEFINED;
  bgav_stream_t * s;
  int i;
  
  for(i = 0; i < t->num_audio_streams; i++)
    {
    s = &t->audio_streams[i];

    if(!check_sync_time(s, &ret, scale))
      return GAVL_TIME_UNDEFINED;
    }
  for(i = 0; i < t->num_video_streams; i++)
    {
    s = &t->video_streams[i];
    if(!check_sync_time(s, &ret, scale))
      return GAVL_TIME_UNDEFINED;
    }
  return ret;
  }

static int check_out_time(bgav_stream_t * s, int64_t * t, int scale,
                          int stream_scale)
  {
  int64_t tt;
  if((s->action == BGAV_STREAM_MUTE) ||
     STREAM_IS_STILL(s))
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
  int64_t ret = GAVL_TIME_UNDEFINED;
  bgav_stream_t * s;
  int i;
  
  for(i = 0; i < t->num_audio_streams; i++)
    {
    s = &t->audio_streams[i];
    if(!check_out_time(s, &ret, scale, s->data.audio.format.samplerate))
      return GAVL_TIME_UNDEFINED;
    }
  for(i = 0; i < t->num_video_streams; i++)
    {
    s = &t->video_streams[i];
    if(!check_out_time(s, &ret, scale, s->data.video.format.timescale))
      return GAVL_TIME_UNDEFINED;
    }
  return ret;
  }

static int set_eof_d(void * priv, bgav_stream_t * s)
  {
  s->flags |= STREAM_EOF_D;
  return 1;
  }

static int clear_eof_d(void * priv, bgav_stream_t * s)
  {
  s->flags &= ~STREAM_EOF_D;
  return 1;
  }

static int has_eof_d(void * priv, bgav_stream_t * s)
  {
  if((s->action != BGAV_STREAM_MUTE) && !(s->flags & STREAM_EOF_D))
    return 0;
  return 1;
  }


void bgav_track_set_eof_d(bgav_track_t * t)
  {
  bgav_track_foreach(t, set_eof_d, NULL);
  }

void bgav_track_clear_eof_d(bgav_track_t * t)
  {
  bgav_track_foreach(t, clear_eof_d, NULL);
  }

int bgav_track_eof_d(bgav_track_t * t)
  {
  return bgav_track_foreach(t, has_eof_d, NULL);
  }

void bgav_track_get_compression(bgav_track_t * t)
  {
  int i;
  bgav_stream_t * s;

  if(t->flags & TRACK_HAS_COMPRESSION)
    return;
  
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
  for(i = 0; i < t->num_overlay_streams; i++)
    {
    s = &t->overlay_streams[i];
    s->action = BGAV_STREAM_READRAW;
    }

  /* Get a first packet. This will complete the formats */
  for(i = 0; i < t->num_audio_streams; i++)
    {
    s = &t->audio_streams[i];
    bgav_stream_start(s);
    bgav_stream_peek_packet_read(s, NULL, 1);
    }
  for(i = 0; i < t->num_video_streams; i++)
    {
    s = &t->video_streams[i];
    bgav_stream_start(s);
    bgav_stream_peek_packet_read(s, NULL, 1);
    }
  for(i = 0; i < t->num_overlay_streams; i++)
    {
    s = &t->overlay_streams[i];
    bgav_stream_start(s);
    bgav_stream_peek_packet_read(s, NULL, 1);
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
  for(i = 0; i < t->num_overlay_streams; i++)
    {
    s = &t->overlay_streams[i];
    s->action = BGAV_STREAM_MUTE;
    }
  t->flags |= TRACK_HAS_COMPRESSION;
  }
