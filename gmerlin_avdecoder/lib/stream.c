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

#include <avdec_private.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

int bgav_stream_start(bgav_stream_t * stream)
  {
  int result = 1;

  if(!(stream->demuxer->flags & BGAV_DEMUXER_BUILD_INDEX) ||
     ((stream->demuxer->index_mode != INDEX_MODE_SIMPLE) &&
      (stream->demuxer->index_mode != INDEX_MODE_PTS) &&
      (stream->index_mode != INDEX_MODE_SIMPLE) &&
      (stream->index_mode != INDEX_MODE_PTS)))
    {
    switch(stream->type)
      {
      case BGAV_STREAM_VIDEO:
        result = bgav_video_start(stream);
        break;
      case BGAV_STREAM_AUDIO:
        result = bgav_audio_start(stream);
        break;
      case BGAV_STREAM_SUBTITLE_OVERLAY:
      case BGAV_STREAM_SUBTITLE_TEXT:
        result = bgav_subtitle_start(stream);
        break;
      default:
        break;
      }
    }
  
  if(result)
    stream->initialized = 1;
  return result;
  }

void bgav_stream_stop(bgav_stream_t * s)
  {
  if((s->action == BGAV_STREAM_DECODE) ||
     (s->action == BGAV_STREAM_PARSE))
    {
    switch(s->type)
      {
      case BGAV_STREAM_VIDEO:
        bgav_video_stop(s);
        break;
      case BGAV_STREAM_AUDIO:
        bgav_audio_stop(s);
        break;
      case BGAV_STREAM_SUBTITLE_TEXT:
      case BGAV_STREAM_SUBTITLE_OVERLAY:
        bgav_subtitle_stop(s);
      default:
        break;
      }
    }
  if(s->packet_buffer)
    bgav_packet_buffer_clear(s->packet_buffer);

  /* Clear eventually stored packets */
  s->packet = (bgav_packet_t*)0;
  s->index_position = s->first_index_position;
  s->in_position = 0;
  s->out_time = 0;
  s->packet_seq = 0;

  STREAM_UNSET_SYNC(s);
  
  if(s->parsed_packet)
    {
    bgav_packet_destroy(s->parsed_packet);
    s->parsed_packet = (bgav_packet_t*)0;
    }
  }

void bgav_stream_create_packet_buffer(bgav_stream_t * stream)
  {
  stream->packet_buffer = bgav_packet_buffer_create();
  }

void bgav_stream_init(bgav_stream_t * stream, const bgav_options_t * opt)
  {
  memset(stream, 0, sizeof(*stream));
  STREAM_UNSET_SYNC(stream);
  stream->first_index_position = INT_MAX;

  /* need to set this to -1 so we know, if this stream has packets at all */
  stream->last_index_position = -1; 
  stream->index_position = -1;
  stream->opt = opt;
  }


void bgav_stream_free(bgav_stream_t * s)
  {
  /* Cleanup must be called as long as the other
     members are still functional */
  if(s->cleanup)
    s->cleanup(s);
  if(s->description)
    free(s->description);
  if(s->info)
    free(s->info);
  if(s->file_index)
    bgav_file_index_destroy(s->file_index);
  
  if(s->packet_buffer)
    bgav_packet_buffer_destroy(s->packet_buffer);

  if(((s->type == BGAV_STREAM_SUBTITLE_TEXT) ||
      (s->type == BGAV_STREAM_SUBTITLE_OVERLAY)) &&
     s->data.subtitle.subreader)
    bgav_subtitle_reader_destroy(s);

  if(s->timecode_table)
    bgav_timecode_table_destroy(s->timecode_table);
  }

void bgav_stream_dump(bgav_stream_t * s)
  {
  switch(s->type)
    {
    case BGAV_STREAM_AUDIO:
      bgav_dprintf("============ Audio stream ============\n");
      break;
    case BGAV_STREAM_VIDEO:
      bgav_dprintf("============ Video stream ============\n");
      break;

    case BGAV_STREAM_SUBTITLE_TEXT:
      bgav_dprintf("=========== Text subtitles ===========\n");
      break;
    case BGAV_STREAM_SUBTITLE_OVERLAY:
      bgav_dprintf("========= Overlay subtitles ===========\n");
      break;
      
    case BGAV_STREAM_UNKNOWN:
      return;
    }

  if(s->language[0] != '\0')
    bgav_dprintf("  Language:          %s\n", bgav_lang_name(s->language));
  if(s->info)
    bgav_dprintf("  Info:              %s\n", s->info);
  
  bgav_dprintf("  Type:              %s\n",
          (s->description ? s->description : "Not specified"));
  bgav_dprintf("  Fourcc:            ");
  bgav_dump_fourcc(s->fourcc);
  bgav_dprintf("\n");
  
  bgav_dprintf("  Stream ID:         %d (0x%x)\n",
          s->stream_id,
          s->stream_id);
  bgav_dprintf("  Codec bitrate:     ");
  if(s->codec_bitrate)
    bgav_dprintf("%d\n", s->codec_bitrate);
  else
    bgav_dprintf("Unspecified\n");

  bgav_dprintf("  Container bitrate: ");
  if(s->container_bitrate)
    bgav_dprintf("%d\n", s->container_bitrate);
  else
    bgav_dprintf("Unspecified\n");

  bgav_dprintf("  Timescale:         %d\n", s->timescale);
  bgav_dprintf("  Duration:          %"PRId64"\n", s->duration);
  bgav_dprintf("  Private data:      %p\n", s->priv);
  }

void bgav_stream_clear(bgav_stream_t * s)
  {
  if(s->packet_buffer)
    bgav_packet_buffer_clear(s->packet_buffer);
  s->packet = (bgav_packet_t*)0;
  s->in_position  = 0;
  s->out_time = BGAV_TIMESTAMP_UNDEFINED;
  STREAM_UNSET_SYNC(s);
  s->flags &= ~STREAM_EOF;
  }

int bgav_stream_skipto(bgav_stream_t * s, gavl_time_t * time, int scale)
  {
  if(s->action != BGAV_STREAM_DECODE)
    return 1;
  
  switch(s->type)
    {
    case BGAV_STREAM_AUDIO:
      return bgav_audio_skipto(s, time, scale);
      break;
    case BGAV_STREAM_VIDEO:
      return bgav_video_skipto(s, time, scale);
      break;
    case BGAV_STREAM_SUBTITLE_TEXT:
    case BGAV_STREAM_SUBTITLE_OVERLAY:
      return bgav_subtitle_skipto(s, time, scale);
      break;
    case BGAV_STREAM_UNKNOWN:
      break;
    }
  return 0;
  }

bgav_packet_t * bgav_stream_get_packet_write(bgav_stream_t * s)
  {
  return bgav_packet_buffer_get_packet_write(s->packet_buffer, s);
  }

int bgav_stream_get_index(bgav_stream_t * s)
  {
  switch(s->type)
    {
    case BGAV_STREAM_AUDIO:
      return (int)(s - s->track->audio_streams);
      break;
    case BGAV_STREAM_VIDEO:
      return (int)(s - s->track->video_streams);
      break;
    case BGAV_STREAM_SUBTITLE_TEXT:
    case BGAV_STREAM_SUBTITLE_OVERLAY:
      return (int)(s - s->track->subtitle_streams);
      break;
    case BGAV_STREAM_UNKNOWN:
      break;
    }
  return -1;
  }
