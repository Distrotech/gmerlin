/*****************************************************************
 * gmerlin-avdecoder - a general purpose multimedia decoding library
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

#include <config.h>

#include <stdlib.h>
#include <string.h>

#include <avdec_private.h>
#include <parser.h>

static int file_index_seek(bgav_file_index_t * idx, int64_t time)
  {
  int pos1, pos2, tmp;

  pos1 = 0;
  
  pos2 = idx->num_entries - 1;
  
  /* Binary search */
  while(1)
    {
    tmp = (pos1 + pos2)/2;
    
    if(idx->entries[tmp].pts < time)
      pos1 = tmp;
    else
      pos2 = tmp;
    
    if(pos2 - pos1 <= 4)
      break;
    }
  
  while((idx->entries[pos2].pts > time) && pos2)
    pos2--;

  while(pos2 && (idx->entries[pos2-1].pts == idx->entries[pos2].pts))
    pos2--;
  
  return pos2;
  }

int bgav_can_seek_sample(bgav_t * bgav)
  {
  return !!(bgav->tt->cur->flags & TRACK_SAMPLE_ACCURATE);
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

int64_t bgav_audio_start_time(bgav_t * bgav, int stream)
  {
  return bgav->tt->cur->audio_streams[stream].start_time;
  }

int64_t bgav_video_start_time(bgav_t * bgav, int stream)
  {
  return bgav->tt->cur->video_streams[stream].start_time;
  }


void bgav_seek_audio(bgav_t * bgav, int stream, int64_t sample)
  {
  int64_t frame_time;
  bgav_stream_t * s;
  s = &bgav->tt->cur->audio_streams[stream];

  //  fprintf(stderr, "Seek audio: %ld\n", sample);
  
  if(sample >= s->duration) /* EOF */
    {
    s->flags |= STREAM_EOF_C;
    return;
    }

  s->flags &= ~(STREAM_EOF_C|STREAM_EOF_D);
  
  bgav_stream_clear(s);

  if(bgav->demuxer->index_mode == INDEX_MODE_PCM)
    {
    bgav->demuxer->demuxer->seek(bgav->demuxer, sample,
                                 s->data.audio.format.samplerate);
    //    return;
    }
  else if(bgav->demuxer->index_mode == INDEX_MODE_SI_SA)
    {
    frame_time = gavl_time_rescale(s->data.audio.format.samplerate,
                                   s->timescale, sample);
    bgav_superindex_seek(bgav->demuxer->si, s,
                         &frame_time,
                         s->timescale);
    
    s->out_time = gavl_time_rescale(s->timescale, s->data.audio.format.samplerate,
                                    STREAM_GET_SYNC(s));
    }
  else /* Fileindex */
    {
    s->index_position = file_index_seek(s->file_index, sample);
    frame_time = s->file_index->entries[s->index_position].pts;
    /* Handle preroll */
    while(s->index_position &&
          (frame_time - s->file_index->entries[s->index_position].pts) < s->data.audio.preroll)
      s->index_position--;
    
    /* Decrease until a real packet starts */
    while(s->index_position &&
          (s->file_index->entries[s->index_position-1].position ==
           s->file_index->entries[s->index_position].position))
      s->index_position--;
    
    s->out_time = s->file_index->entries[s->index_position].pts + s->start_time;
    sample += s->start_time;
    
    if(bgav->demuxer->demuxer->resync)
      bgav->demuxer->demuxer->resync(bgav->demuxer, s);
    }
  
  bgav_audio_resync(s);
 
  bgav_audio_skipto(s, &sample, s->data.audio.format.samplerate);
    
  }

void bgav_seek_video(bgav_t * bgav, int stream, int64_t time)
  {
  bgav_stream_t * s;
  int64_t frame_time;
  s = &bgav->tt->cur->video_streams[stream];
  
  //  fprintf(stderr, "Seek video: %ld\n", time);
  
  if(time >= s->duration) /* EOF */
    {
    s->flags |= STREAM_EOF_C;
    return;
    }
  
  s->flags &= ~(STREAM_EOF_C|STREAM_EOF_D);
  
  if(time == s->out_time)
    {
    return;
    }
  if((time > s->out_time) &&
     (bgav_video_keyframe_after(bgav, stream, s->out_time) > time))
    {
    //    fprintf(stderr, "Skip to: %ld\n", time);
    bgav_video_skipto(s, &time, s->data.video.format.timescale, 1);
    //    fprintf(stderr, "Skipped to: %ld %ld\n", time, s->out_time);
    return;
    }

  bgav_stream_clear(s);

  if(bgav->demuxer->index_mode == INDEX_MODE_SI_SA)
    {
    frame_time = time;
    bgav_superindex_seek(bgav->demuxer->si, s, &frame_time, s->timescale);
    s->out_time = bgav->demuxer->si->entries[s->index_position].pts;
    }
  else /* Fileindex */
    {
    if(!s->data.video.kft)
      s->data.video.kft = bgav_keyframe_table_create_fi(s->file_index);

    s->index_position =
      bgav_keyframe_table_seek(s->data.video.kft, time, &frame_time);
    //    fprintf(stderr, "Index position: %d, keyframe timestamp: %ld\n", s->index_position,
    //            frame_time);

    /* Decrease until a real packet starts */
    while(s->index_position &&
          (s->file_index->entries[s->index_position-1].position ==
           s->file_index->entries[s->index_position].position))
      s->index_position--;
    
    STREAM_SET_SYNC(s, s->file_index->entries[s->index_position].pts + s->start_time);
    //    s->out_time = frame_time + s->start_time;
    s->out_time = s->file_index->entries[s->index_position].pts + s->start_time;
    
    // if(s->data.video.parser)
    //   bgav_video_parser_reset(s->data.video.parser, BGAV_TIMESTAMP_UNDEFINED, frame_time);
    
    if(bgav->demuxer->demuxer->resync)
      bgav->demuxer->demuxer->resync(bgav->demuxer, s);
    }

  bgav_video_resync(s);


  time += s->start_time;
  
  //  fprintf(stderr, "Skip to: %ld\n", time);
  bgav_video_skipto(s, &time, s->data.video.format.timescale, 1);
  //  fprintf(stderr, "Skipped to: %ld %ld\n", time, s->out_time);
  }

int64_t bgav_video_stream_keyframe_before(bgav_stream_t * s, int64_t time)
  {
  int pos;
  if(s->demuxer->index_mode == INDEX_MODE_SI_SA)
    {
    pos = s->last_index_position;
    while(pos >= s->first_index_position)
      {
      if((s->demuxer->si->entries[pos].stream_id == s->stream_id) &&
         (s->demuxer->si->entries[pos].flags & PACKET_FLAG_KEY) &&
         (s->demuxer->si->entries[pos].pts < time))
        {
        break;
        }
      pos--;
      }
    if(pos < s->first_index_position)
      return BGAV_TIMESTAMP_UNDEFINED;
    else
      return s->demuxer->si->entries[pos].pts;
    }
  else /* Fileindex */
    {
    if(time >= s->duration)
      pos = s->file_index->num_entries-1;
    else
      pos = file_index_seek(s->file_index, time);

    while(pos &&
          ((s->file_index->entries[pos].pts >= time) ||
           !(s->file_index->entries[pos].flags & PACKET_FLAG_KEY)))
      pos--;
    
    if((s->file_index->entries[pos].pts >= time) ||
       !(s->file_index->entries[pos].flags & PACKET_FLAG_KEY))
      return BGAV_TIMESTAMP_UNDEFINED;
    return s->file_index->entries[pos].pts;
    }
  /* Stupid gcc :( */
  return BGAV_TIMESTAMP_UNDEFINED;
  }

int64_t bgav_video_keyframe_before(bgav_t * bgav, int stream, int64_t time)
  {
  return bgav_video_stream_keyframe_before(&bgav->tt->cur->video_streams[stream], time);
  }

int64_t bgav_video_stream_keyframe_after(bgav_stream_t * s, int64_t time)
  {
  int pos;
  if(s->demuxer->index_mode == INDEX_MODE_SI_SA)
    {
    pos = s->first_index_position;
    while(pos <= s->last_index_position)
      {
      if((s->demuxer->si->entries[pos].stream_id == s->stream_id) &&
         (s->demuxer->si->entries[pos].flags & PACKET_FLAG_KEY) &&
         (s->demuxer->si->entries[pos].pts > time))
        {
        break;
        }
      pos++;
      }
    if(pos > s->last_index_position)
      return BGAV_TIMESTAMP_UNDEFINED;
    else
      return s->demuxer->si->entries[pos].pts;
    }
  else if(s->file_index) /* Fileindex */
    {
    if(time >= s->duration)
      {
      return BGAV_TIMESTAMP_UNDEFINED;
      }
    pos = file_index_seek(s->file_index, time);

    while((pos < s->file_index->num_entries - 1) &&
          ((s->file_index->entries[pos].pts <= time) ||
           !(s->file_index->entries[pos].flags & PACKET_FLAG_KEY)))
      pos++;

    if((s->file_index->entries[pos].pts <= time) ||
       !(s->file_index->entries[pos].flags & PACKET_FLAG_KEY))
      {
      return BGAV_TIMESTAMP_UNDEFINED;
      }
    return s->file_index->entries[pos].pts;
    }
  return BGAV_TIMESTAMP_UNDEFINED;
  
  }

int64_t bgav_video_keyframe_after(bgav_t * bgav, int stream, int64_t time)
  {
  return bgav_video_stream_keyframe_after(&bgav->tt->cur->video_streams[stream], time);
  }

void bgav_seek_subtitle(bgav_t * bgav, int stream, int64_t time)
  {
  bgav_stream_t * s;
  s = &bgav->tt->cur->subtitle_streams[stream];
  bgav_stream_clear(s);

  s->flags &= ~(STREAM_EOF_C|STREAM_EOF_D);

  if(s->data.subtitle.subreader)
    {
    bgav_subtitle_reader_seek(s, time, s->timescale);
    return;
    }

  
  
  bgav_stream_clear(s);

  if(bgav->demuxer->index_mode == INDEX_MODE_SI_SA)
    {
    bgav_superindex_seek(bgav->demuxer->si, s, &time, s->timescale);
    s->out_time = bgav->demuxer->si->entries[s->index_position].pts;
    }
  else /* Fileindex */
    {
    s->index_position = file_index_seek(s->file_index, time);

    /* Decrease until a real packet starts */
    while(s->index_position &&
          (s->file_index->entries[s->index_position-1].position ==
           s->file_index->entries[s->index_position].position))
      s->index_position--;
    
    STREAM_SET_SYNC(s, s->file_index->entries[s->index_position].pts);
    s->out_time = STREAM_GET_SYNC(s);
    
    if(bgav->demuxer->demuxer->resync)
      bgav->demuxer->demuxer->resync(bgav->demuxer, s);
    }
  
  }

int bgav_set_sample_accurate(bgav_t * b)
  {
  int i;
  gavl_time_t t;

  if(!b->demuxer)
    return 0;
  
  switch(b->demuxer->index_mode)
    {
    case INDEX_MODE_NONE:
      return 0;
      break;
    case INDEX_MODE_PCM:
    case INDEX_MODE_SI_SA:
      if(!b->input->input->seek_byte)
        return 0;
      /* Format is already sample accurate */
      for(i = 0; i < b->tt->num_tracks; i++)
        b->tt->tracks[i].flags |= TRACK_SAMPLE_ACCURATE;
      return 1;
      break;
    case INDEX_MODE_MPEG:
    case INDEX_MODE_SIMPLE:
    case INDEX_MODE_MIXED:
      if(!b->input->index_file || !b->input->input->seek_byte)
        return 0;
      /* Try to read a file index */
      if(!bgav_read_file_index(b))
        {
        if(bgav_build_file_index(b, &t))
          {
          if(!b->opt.cache_time || ((t*1000)/GAVL_TIME_SCALE > b->opt.cache_time))
            bgav_write_file_index(b);
          }
        else
          return 0;
        }
      return 1;
      break;
    case INDEX_MODE_SI_PARSE:
      if(!b->input->index_file || !b->input->input->seek_byte)
        return 0;
      /* Try to read a file index */
      if(!bgav_read_file_index(b))
        {
        if(bgav_build_file_index(b, &t))
          {
          if(!b->opt.cache_time || ((t*1000)/GAVL_TIME_SCALE > b->opt.cache_time))
            bgav_write_file_index(b);
          }
        else
          return 0;
        }
      /* Merge file index into superindex. Warning: This breaks when
         multitrack formats with superindex appear */
      for(i = 0; i < b->tt->tracks[0].num_audio_streams; i++)
        {
        if(b->tt->tracks[0].audio_streams[i].file_index)
          {
          bgav_superindex_merge_fileindex(b->demuxer->si,
                                          &b->tt->tracks[0].audio_streams[i]);
          }
        }
      for(i = 0; i < b->tt->tracks[0].num_video_streams; i++)
        {
        if(b->tt->tracks[0].video_streams[i].file_index)
          {
          bgav_superindex_merge_fileindex(b->demuxer->si,
                                          &b->tt->tracks[0].video_streams[i]);
          }
        }
      /* After patching the superindex, we are sample accurate :) */
      b->demuxer->index_mode = INDEX_MODE_SI_SA;
      return 1;
      break;
    }
  return 0;
  }
