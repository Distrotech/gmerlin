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

#include <config.h>

#include <stdlib.h>
#include <string.h>

#include <avdec_private.h>

static int file_index_seek(bgav_file_index_t * idx, int64_t time)
  {
  int pos1, pos2, tmp;

  pos1 = 0;
  
  pos2 = idx->num_entries - 1;
  
  /* Binary search */
  while(1)
    {
    tmp = (pos1 + pos2)/2;
    
    if(idx->entries[tmp].time < time)
      pos1 = tmp;
    else
      pos2 = tmp;
    
    if(pos2 - pos1 <= 4)
      break;
    }
  
  while((idx->entries[pos2].time > time) && pos2)
    pos2--;
  
  return pos2;
  }

int bgav_can_seek_sample(bgav_t * bgav)
  {
  return bgav->tt->cur->sample_accurate;
  }

int64_t bgav_audio_duration(bgav_t * bgav, int stream)
  {
  return bgav->tt->cur->audio_streams[stream].duration;
  }

int64_t bgav_video_duration(bgav_t * bgav, int stream)
  {
  return bgav->tt->cur->video_streams[stream].duration;
  }

int64_t bgav_subtitle_duration(bgav_t * bgav, int stream)
  {
  return bgav->tt->cur->subtitle_streams[stream].duration;
  }

void bgav_seek_audio(bgav_t * bgav, int stream, int64_t sample)
  {
  bgav_stream_t * s;
  int64_t frame_time;
  s = &bgav->tt->cur->audio_streams[stream];
  bgav_stream_clear(s);

  if(bgav->demuxer->index_mode == INDEX_MODE_PCM)
    {
    bgav->demuxer->demuxer->seek(bgav->demuxer, sample,
                                 s->data.audio.format.samplerate);
    return;
    }
  else if(bgav->demuxer->index_mode == INDEX_MODE_SI_SA)
    {
    bgav_superindex_seek(bgav->demuxer->si, s,
                         gavl_time_rescale(s->data.audio.format.samplerate,
                                           s->timescale, sample),
                         s->timescale);
    }
  else /* Fileindex */
    {
    s->index_position = file_index_seek(s->file_index, sample);
    frame_time = s->file_index->entries[s->index_position].time;
    
    /* Handle preroll */
    while(s->index_position &&
          (frame_time - s->file_index->entries[s->index_position].time) < s->data.audio.preroll)
      s->index_position--;
    
    /* Decrease until a real packet starts */
    while(s->index_position &&
          (s->file_index->entries[s->index_position-1].position ==
           s->file_index->entries[s->index_position].position))
      s->index_position--;
    
    s->in_time = s->file_index->entries[s->index_position].time;
    s->out_time = frame_time;
    
    
    if(bgav->demuxer->demuxer->resync)
      bgav->demuxer->demuxer->resync(bgav->demuxer);
    }
  
  bgav_stream_resync_decoder(s);
  //  fprintf(stderr, "bgav_audio_skipto...");
  bgav_audio_skipto(s, &sample, s->data.audio.format.samplerate);
  //  fprintf(stderr, "done\n");
    
  s->in_time += s->first_timestamp;
  s->out_time += s->first_timestamp;
  }

void bgav_seek_video(bgav_t * bgav, int stream, int64_t time)
  {
  bgav_stream_t * s;
  int64_t frame_time;
  s = &bgav->tt->cur->video_streams[stream];
  bgav_stream_clear(s);

  if(bgav->demuxer->index_mode == INDEX_MODE_SI_SA)
    {
    bgav_superindex_seek(bgav->demuxer->si, s, time, s->timescale);
    }
  else /* Fileindex */
    {
    s->index_position = file_index_seek(s->file_index, time);
    //  fprintf(stderr, "Index pos: %d\n", s->index_position);
    /* Decrease until we have the keyframe before this frame */
    while(!s->file_index->entries[s->index_position].keyframe && s->index_position)
      s->index_position--;
    
    frame_time = s->file_index->entries[s->index_position].time;
    
    /* Decrease until a real packet starts */
    while(s->index_position &&
          (s->file_index->entries[s->index_position-1].position ==
           s->file_index->entries[s->index_position].position))
      s->index_position--;
    
    s->in_time = s->file_index->entries[s->index_position].time;
    s->out_time = frame_time;
    
    if(bgav->demuxer->demuxer->resync)
      bgav->demuxer->demuxer->resync(bgav->demuxer);
    }
  
  bgav_stream_resync_decoder(s);

  //  fprintf(stderr, "bgav_video_skipto %d...", s->index_position);
  bgav_video_skipto(s, &time, s->timescale);
  //  fprintf(stderr, "done\n");
  
  }

int64_t bgav_video_keyframe_before(bgav_t * bgav, int stream, int64_t time)
  {
  int pos;
  bgav_stream_t * s;
  s = &bgav->tt->cur->video_streams[stream];

  if(bgav->demuxer->index_mode == INDEX_MODE_SI_SA)
    {
    pos = s->last_index_position;
    while(pos >= s->first_index_position)
      {
      if((bgav->demuxer->si->entries[pos].stream_id == s->stream_id) &&
         (bgav->demuxer->si->entries[pos].keyframe) &&
         (bgav->demuxer->si->entries[pos].time < time))
        {
        break;
        }
      pos--;
      }
    if(pos < s->first_index_position)
      return BGAV_TIMESTAMP_UNDEFINED;
    }
  else /* Fileindex */
    {
    pos = file_index_seek(s->file_index, time);

    while(pos &&
          ((s->file_index->entries[pos].time >= time) ||
           !s->file_index->entries[pos].keyframe))
      pos--;
    
    if((s->file_index->entries[pos].time >= time) ||
       !s->file_index->entries[pos].keyframe)
      return BGAV_TIMESTAMP_UNDEFINED;
    return s->file_index->entries[pos].time;
    }
  /* Stupid gcc :( */
  return BGAV_TIMESTAMP_UNDEFINED;
  }

int64_t bgav_video_keyframe_after(bgav_t * bgav, int stream, int64_t time)
  {
  int pos;
  bgav_stream_t * s;
  s = &bgav->tt->cur->audio_streams[stream];

  if(bgav->demuxer->index_mode == INDEX_MODE_SI_SA)
    {
    pos = s->first_index_position;
    while(pos <= s->last_index_position)
      {
      if((bgav->demuxer->si->entries[pos].stream_id == s->stream_id) &&
         (bgav->demuxer->si->entries[pos].keyframe) &&
         (bgav->demuxer->si->entries[pos].time > time))
        {
        break;
        }
      pos++;
      }
    if(pos > s->last_index_position)
      return BGAV_TIMESTAMP_UNDEFINED;
    }
  else /* Fileindex */
    {
    pos = file_index_seek(s->file_index, time);

    while((pos < s->file_index->num_entries - 1) &&
          ((s->file_index->entries[pos].time <= time) ||
           !s->file_index->entries[pos].keyframe))
      pos++;

    if((s->file_index->entries[pos].time <= time) ||
       !s->file_index->entries[pos].keyframe)
      return BGAV_TIMESTAMP_UNDEFINED;
  
    return s->file_index->entries[pos].time;
    }
  /* Stupid gcc :( */
  return BGAV_TIMESTAMP_UNDEFINED;
  }

void bgav_seek_subtitle(bgav_t * bgav, int stream, int64_t time)
  {
  bgav_stream_t * s;
  s = &bgav->tt->cur->audio_streams[stream];
  bgav_stream_clear(s);
  /* TODO */
  }
