/*****************************************************************
 * gmerlin-avdecoder - a general purpose multimedia decoding library
 *
 * Copyright (c) 2001 - 2008 Members of the Gmerlin project
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

// #define DUMP_SUPERINDEX    
#include <avdec_private.h>
// #include <parser.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void skip_to(bgav_t * b, bgav_track_t * track, int64_t * time, int scale)
  {
  //  fprintf(stderr, "Skip to: %ld\n", *time);
  if(!bgav_track_skipto(track, time, scale))
    b->eof = 1;
  //  fprintf(stderr, "Skipped to: %ld\n", *time);
  }

/* Seek functions with superindex */

static void seek_si(bgav_t * b, bgav_demuxer_context_t * ctx,
                    int64_t time, int scale)
  {
  int64_t seek_time;
  int64_t orig_time;
  uint32_t i, j;
  int32_t start_packet;
  int32_t end_packet;
  bgav_track_t * track;
  
  track = ctx->tt->cur;
  bgav_track_clear(track);
  
  /* Set the packet indices of the streams to -1 */
  for(j = 0; j < track->num_audio_streams; j++)
    track->audio_streams[j].index_position = -1;
  for(j = 0; j < track->num_video_streams; j++)
    track->video_streams[j].index_position = -1;
  for(j = 0; j < track->num_subtitle_streams; j++)
    track->subtitle_streams[j].index_position = -1;
  
  /* Seek the start chunks indices of all streams */
  
  orig_time = time;
  
  for(j = 0; j < track->num_video_streams; j++)
    {
    seek_time = time;
    bgav_superindex_seek(ctx->si, &(track->video_streams[j]), &seek_time, scale);
    /* Synchronize time to the video stream */
    if(!j)
      {
      time = seek_time;
      }
    }
  for(j = 0; j < track->num_audio_streams; j++)
    {
    seek_time = time;
    bgav_superindex_seek(ctx->si, &(track->audio_streams[j]), &seek_time, scale);
    }
  for(j = 0; j < track->num_subtitle_streams; j++)
    {
    seek_time = time;
    if(!track->subtitle_streams[j].data.subtitle.subreader)
      bgav_superindex_seek(ctx->si, &(track->subtitle_streams[j]), &seek_time, scale);
    }
  
  /* Find the start and end packet */

  if(ctx->demux_mode == DEMUX_MODE_SI_I)
    {
    start_packet = 0x7FFFFFFF;
    end_packet   = 0x0;
    
    for(j = 0; j < track->num_audio_streams; j++)
      {
      if(start_packet > track->audio_streams[j].index_position)
        start_packet = track->audio_streams[j].index_position;
      if(end_packet < track->audio_streams[j].index_position)
        end_packet = track->audio_streams[j].index_position;
      }
    for(j = 0; j < track->num_video_streams; j++)
      {
      if(start_packet > track->video_streams[j].index_position)
        start_packet = track->video_streams[j].index_position;
      if(end_packet < track->video_streams[j].index_position)
        end_packet = track->video_streams[j].index_position;
      }

    /* Do the seek */
    ctx->si->current_position = start_packet;
    bgav_input_seek(ctx->input, ctx->si->entries[ctx->si->current_position].offset,
                    SEEK_SET);

    ctx->flags |= BGAV_DEMUXER_SI_SEEKING;
    for(i = start_packet; i <= end_packet; i++)
      bgav_demuxer_next_packet_interleaved(ctx);

    ctx->flags &= ~BGAV_DEMUXER_SI_SEEKING;
    }
  bgav_track_resync(track);
  skip_to(b, track, &orig_time, scale);
  }

/* Maximum allowed seek tolerance, decrease if you want it more exact */

// #define SEEK_TOLERANCE (GAVL_TIME_SCALE/2)

#define SEEK_TOLERANCE 0
/* We skip at most this time */

#define MAX_SKIP_TIME  (GAVL_TIME_SCALE*5)


static void seek_sa(bgav_t * b, int64_t * time, int scale)
  {
  int i;
  bgav_stream_t * s;
  for(i = 0; i < b->tt->cur->num_video_streams; i++)
    {
    if(b->tt->cur->video_streams[i].action != BGAV_STREAM_MUTE)
      {
      s = &b->tt->cur->video_streams[i];
      bgav_seek_video(b, i,
                      gavl_time_rescale(scale, s->data.video.format.timescale,
                                        *time) - s->start_time);
      if(s->eof)
        {
        b->eof = 1;
        return;
        }
      /*
       *  We align seeking at the first frame of the first video stream
       */

      if(!i)
        {
        *time =
          gavl_time_rescale(s->data.video.format.timescale,
                            scale,
                            s->out_time + s->start_time);
        }
      }
    }
    
  for(i = 0; i < b->tt->cur->num_audio_streams; i++)
    {
    if(b->tt->cur->audio_streams[i].action != BGAV_STREAM_MUTE)
      {
      s = &b->tt->cur->audio_streams[i];
      bgav_seek_audio(b, i,
                      gavl_time_rescale(scale, s->data.audio.format.samplerate,
                                        *time) - s->start_time);
      if(s->eof)
        {
        b->eof = 1;
        return;
        }
      }
    }
  for(i = 0; i < b->tt->cur->num_subtitle_streams; i++)
    {
    if(b->tt->cur->subtitle_streams[i].action != BGAV_STREAM_MUTE)
      {
      s = &b->tt->cur->subtitle_streams[i];
      bgav_seek_subtitle(b, i, gavl_time_rescale(scale, s->timescale, *time));
      }
    }
  return;
  
  }

static void seek_once(bgav_t * b, int64_t * time, int scale)
  {
  bgav_track_t * track = b->tt->cur;
  int64_t sync_time;

  bgav_track_clear(track);
  b->demuxer->demuxer->seek(b->demuxer, *time, scale);
  bgav_track_resync(track);
  sync_time = bgav_track_sync_time(track, scale);

  if(*time > sync_time)
    skip_to(b, track, time, scale);
  else
    {
    skip_to(b, track, &sync_time, scale);
    *time = sync_time;
    }
  
  //  return sync_time;
  }

static void seek_iterative(bgav_t * b, int64_t * time, int scale)
  {
  int num_seek = 0;
  int num_resync = 0;
  bgav_track_t * track = b->tt->cur;
  
  int64_t seek_time;
  int64_t diff_time;
  int64_t sync_time;
  int64_t out_time           = BGAV_TIMESTAMP_UNDEFINED;
  
  int64_t seek_time_upper    = BGAV_TIMESTAMP_UNDEFINED;
  int64_t seek_time_lower    = BGAV_TIMESTAMP_UNDEFINED;

  int64_t sync_time_upper    = BGAV_TIMESTAMP_UNDEFINED;
  int64_t sync_time_lower    = BGAV_TIMESTAMP_UNDEFINED;

  int64_t out_time_lower    = BGAV_TIMESTAMP_UNDEFINED;
  
  int64_t one_second = gavl_time_scale(scale, GAVL_TIME_SCALE);
  int final_seek = 0;
  
  seek_time = *time;

  //  fprintf(stderr, "****** Seek iterative ***********\n");
  
  while(1)
    {
    //    fprintf(stderr, "Seek time: %ld\n", seek_time);
    
    bgav_track_clear(track);
    b->demuxer->demuxer->seek(b->demuxer, seek_time, scale);
    num_seek++;
    
    sync_time = bgav_track_sync_time(track, scale);

    if(sync_time == BGAV_TIMESTAMP_UNDEFINED)
      {
      b->eof = 1;
      return;
      }
    
    //    fprintf(stderr, "Sync time: %ld\n", sync_time);
    
    diff_time = *time - sync_time;

    if(sync_time > *time) /* Sync time too late */
      {
      //      fprintf(stderr, "Sync time too late\n");

      if((sync_time_upper == BGAV_TIMESTAMP_UNDEFINED) ||
         (sync_time_upper > sync_time))
        {
        seek_time_upper = seek_time;
        sync_time_upper = sync_time;
        }
      /* If we were too early before, exit here */
      if(sync_time_lower != BGAV_TIMESTAMP_UNDEFINED)
        {
        seek_time = seek_time_lower;
        out_time = out_time_lower;
        final_seek = 1;
        break;
        }
      
      /* Go backward */
      seek_time -= ((3*(sync_time - *time))/2 + one_second);
      continue;
      }
    /* Sync time too early, but already been there: Exit */
    else if((sync_time_lower != BGAV_TIMESTAMP_UNDEFINED) &&
            (sync_time == sync_time_lower))
      {
      bgav_track_resync(track);
      num_resync++;
      break;
      }
    else /* Sync time too early, get out time */
      {
      bgav_track_resync(track);
      num_resync++;

      out_time = bgav_track_out_time(track, scale);

      //      fprintf(stderr, "Out time: %ld\n", out_time);

      if(out_time > *time) /* Out time too late */
        {
        //        fprintf(stderr, "Out time too late\n");
        seek_time -= ((3*(out_time - *time))/2 + one_second);
        continue;
        }
      else
        {
        //        fprintf(stderr, "Out time too early\n");
        /* If difference is less than half a second, exit here */
        if(*time - out_time < one_second / 2)
          break;
        
        /* Remember position and go a bit forward */
        if((out_time_lower == BGAV_TIMESTAMP_UNDEFINED) ||
           (out_time_lower < out_time))
          {
          seek_time_lower = seek_time;
          out_time_lower  = out_time;
          sync_time_lower = sync_time;
          
          seek_time += (*time - out_time)/2;
          continue;
          }
        /* Have been better before */
        else if(out_time <= out_time_lower)
          {
          seek_time = seek_time_lower;
          out_time = out_time_lower;
          final_seek = 1;
          break;
          }
        }
      }
    }

  if(final_seek)
    {
    bgav_track_clear(track);
    //    fprintf(stderr, "Final seek %ld\n", seek_time);
    b->demuxer->demuxer->seek(b->demuxer, seek_time, scale);
    num_seek++;
    bgav_track_resync(track);
    num_resync++;
    }

  if(*time > out_time)
    skip_to(b, track, time, scale);
  else
    {
    skip_to(b, track, &out_time, scale);
    *time = out_time;
    }

  //  fprintf(stderr, "Seeks: %d, resyncs: %d\n", num_seek, num_resync);
  
  }

void
bgav_seek_scaled(bgav_t * b, int64_t * time, int scale)
  {
  int i;
  bgav_track_t * track = b->tt->cur;
  
  //  fprintf(stderr, "bgav_seek_scaled: %ld\n", gavl_time_unscale(scale, *time));

  /* Clear EOF */
  b->demuxer->flags &= ~BGAV_DEMUXER_EOF;

  /* Seek with sample accuracy */
  if(b->tt->cur->sample_accurate)
    {
    seek_sa(b, time, scale);    
    }
  /* Seek with superindex */
  else if(b->demuxer->si && !(b->demuxer->flags & BGAV_DEMUXER_SI_PRIVATE_FUNCS))
    {
    seek_si(b, b->demuxer, *time, scale);
    }
  /* Seek once */
  else if(!(b->demuxer->flags & BGAV_DEMUXER_SEEK_ITERATIVE))
    {
    seek_once(b, time, scale);
    }
  /* Seek iterative */
  else
    {
    seek_iterative(b, time, scale);
    }
  
  /* Let the subtitle readers seek */
  for(i = 0; i < track->num_subtitle_streams; i++)
    {
    if(track->subtitle_streams[i].data.subtitle.subreader &&
       track->subtitle_streams[i].action != BGAV_STREAM_MUTE)
      {
      /* Clear EOF state */
      track->subtitle_streams[i].eof = 0;
      bgav_subtitle_reader_seek(&track->subtitle_streams[i],
                                *time, scale);
      }
    }

  }

void
bgav_seek(bgav_t * b, gavl_time_t * time)
  {
  bgav_seek_scaled(b, time, GAVL_TIME_SCALE);
  }
