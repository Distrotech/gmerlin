/*****************************************************************
 
  demuxer.c
 
  Copyright (c) 2003-2004 by Burkhard Plaum - plaum@ipf.uni-stuttgart.de
 
  http://gmerlin.sourceforge.net
 
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
 
  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111, USA.
 
*****************************************************************/

#include <stdlib.h>
#include <string.h>
#include <avdec_private.h>

extern bgav_demuxer_t bgav_demuxer_asf;
extern bgav_demuxer_t bgav_demuxer_avi;
extern bgav_demuxer_t bgav_demuxer_rmff;
extern bgav_demuxer_t bgav_demuxer_quicktime;

extern bgav_demuxer_t bgav_demuxer_wav;
extern bgav_demuxer_t bgav_demuxer_au;
extern bgav_demuxer_t bgav_demuxer_aiff;
extern bgav_demuxer_t bgav_demuxer_ra;

bgav_demuxer_context_t * bgav_demuxer_create(bgav_input_context_t * input)
  {
  bgav_demuxer_context_t * ret;
  int i;
  bgav_demuxer_t * demuxer = (bgav_demuxer_t*)0;
  
  if(bgav_demuxer_asf.probe(input))
    {
    demuxer = &bgav_demuxer_asf;
    fprintf(stderr, "Detected ASF format\n");
    }
  else if(bgav_demuxer_quicktime.probe(input))
    {
    demuxer = &bgav_demuxer_quicktime;
    fprintf(stderr, "Detected Quicktime format\n");
    }
  else if(bgav_demuxer_avi.probe(input))
    {
    demuxer = &bgav_demuxer_avi;
    fprintf(stderr, "Detected AVI format\n");
    }
  else if(bgav_demuxer_rmff.probe(input))
    {
    demuxer = &bgav_demuxer_rmff;
    fprintf(stderr, "Detected Real format\n");
    }
  else if(bgav_demuxer_wav.probe(input))
    {
    demuxer = &bgav_demuxer_wav;
    fprintf(stderr, "Detected WAV format\n");
    }
  else if(bgav_demuxer_au.probe(input))
    {
    demuxer = &bgav_demuxer_au;
    fprintf(stderr, "Detected Sun .au format\n");
    }
  else if(bgav_demuxer_aiff.probe(input))
    {
    demuxer = &bgav_demuxer_aiff;
    fprintf(stderr, "Detected Apple AIFF format\n");
    }
  else if(bgav_demuxer_ra.probe(input))
    {
    demuxer = &bgav_demuxer_ra;
    fprintf(stderr, "Detected Real audio format\n");
    }
  
  /* TODO: Other formats */
  else
    return (bgav_demuxer_context_t*)0;

  ret = calloc(1, sizeof(*ret));
  
  ret->demuxer = demuxer;
  ret->input = input;
  
  if(!ret->demuxer->open(ret))
    {
    free(ret);
    return (bgav_demuxer_context_t*)0;
    }

  /* Remove unsupported streams */

  if(ret->num_audio_streams)
    {
    ret->audio_stream_index = malloc(ret->num_audio_streams * sizeof(int));
    
    for(i = 0; i < ret->num_audio_streams; i++)
      {
      if(!bgav_find_audio_decoder(&(ret->audio_streams[i])))
        {
        fprintf(stderr, "Unsupported Audio codec: ");
        bgav_dump_fourcc(ret->audio_streams[i].fourcc);
        fprintf(stderr, "\n");
        ret->audio_streams[i].action = BGAV_STREAM_UNSUPPORTED;
        }
      else    
        {
        ret->audio_stream_index[ret->supported_audio_streams] = i;
        ret->supported_audio_streams++;
        }
      }
    }

  if(ret->num_video_streams)
    {
    ret->video_stream_index = malloc(ret->num_video_streams * sizeof(int));
    
    for(i = 0; i < ret->num_video_streams; i++)
      {
      if(!bgav_find_video_decoder(&(ret->video_streams[i])))
        {
        fprintf(stderr, "Unsupported Video codec: ");
        bgav_dump_fourcc(ret->video_streams[i].fourcc);
        fprintf(stderr, "\n");
        ret->video_streams[i].action = BGAV_STREAM_UNSUPPORTED;
        }
      else    
        {
        ret->video_stream_index[ret->supported_video_streams] = i;
        ret->supported_video_streams++;
        }
      }
    }
  
  return ret;
  }

#define FREE(p) if(p)free(p)

void bgav_demuxer_destroy(bgav_demuxer_context_t * ctx)
  {
  int i;
  ctx->demuxer->close(ctx);
  
  if(ctx->video_stream_index)
    free(ctx->video_stream_index);
  if(ctx->audio_stream_index)
    free(ctx->audio_stream_index);
  
  for(i = 0; i < ctx->num_audio_streams; i++)
    FREE(ctx->audio_streams[i].description);
  for(i = 0; i < ctx->num_video_streams; i++)
    FREE(ctx->video_streams[i].description);
  FREE(ctx->stream_description);
  free(ctx->audio_streams);
  free(ctx->video_streams);
  free(ctx);
  }

bgav_stream_t *
bgav_demuxer_add_audio_stream(bgav_demuxer_context_t * d)
  {
  d->num_audio_streams++;
  d->audio_streams = realloc(d->audio_streams, d->num_audio_streams * 
                             sizeof(*(d->audio_streams)));
  memset(&(d->audio_streams[d->num_audio_streams-1]),
         0, sizeof(d->audio_streams[0]));
  d->audio_streams[d->num_audio_streams-1].demuxer = d;
  d->audio_streams[d->num_audio_streams-1].data.audio.bits_per_sample = 16;
  d->audio_streams[d->num_audio_streams-1].type = BGAV_STREAM_AUDIO;
  return &(d->audio_streams[d->num_audio_streams-1]);
  }

bgav_stream_t *
bgav_demuxer_add_video_stream(bgav_demuxer_context_t * d)
  {
  d->num_video_streams++;
  d->video_streams = realloc(d->video_streams, d->num_video_streams * 
                             sizeof(*(d->video_streams)));
  memset(&(d->video_streams[d->num_video_streams-1]),
         0, sizeof(d->video_streams[0]));
  d->video_streams[d->num_video_streams-1].demuxer = d;
  d->video_streams[d->num_video_streams-1].type = BGAV_STREAM_VIDEO;
  return &(d->video_streams[d->num_video_streams-1]);
  }

bgav_stream_t * bgav_demuxer_find_stream(bgav_demuxer_context_t * ctx,
                                         int stream_id)
  {
  int i;
  for(i = 0; i < ctx->num_audio_streams; i++)
    {
    if(ctx->audio_streams[i].stream_id == stream_id)
      {
      if((ctx->audio_streams[i].action != BGAV_STREAM_MUTE) &&
         (ctx->audio_streams[i].action != BGAV_STREAM_UNSUPPORTED))
        return &(ctx->audio_streams[i]);
      return (bgav_stream_t *)0;
      }
    }
  for(i = 0; i < ctx->num_video_streams; i++)
    {
    if(ctx->video_streams[i].stream_id == stream_id)
      {
      if((ctx->video_streams[i].action != BGAV_STREAM_MUTE) &&
         (ctx->video_streams[i].action != BGAV_STREAM_UNSUPPORTED))
        return &(ctx->video_streams[i]);
      return (bgav_stream_t *)0;
      }
    }
  return (bgav_stream_t *)0;
  }

bgav_packet_t *
bgav_demuxer_get_packet_read(bgav_demuxer_context_t * demuxer,
                             bgav_stream_t * s)
  {
  bgav_packet_t * ret = (bgav_packet_t*)0;
  if(!s->packet_buffer)
    return (bgav_packet_t*)0;
  while(!(ret = bgav_packet_buffer_get_packet_read(s->packet_buffer)))
    {
    if(!demuxer->demuxer->next_packet(demuxer))
      return (bgav_packet_t*)0;
    }
  return ret;
  }

void 
bgav_demuxer_done_packet_read(bgav_demuxer_context_t * demuxer,
                              bgav_packet_t * p)
  {
  p->valid = 0;
  }

void
bgav_seek(bgav_t * b, gavl_time_t time)
  {
  int i;
  gavl_time_t packet_time;
  gavl_time_t diff_time;
  gavl_time_t new_time;
  bgav_stream_t * s;
  /* First step: Flush all fifos and decoders */
  bgav_demuxer_context_t * demuxer = b->demuxer;

  for(i = 0; i < demuxer->num_audio_streams; i++)
    {
    s = &(demuxer->audio_streams[i]);

    if(s->packet_buffer)
      bgav_packet_buffer_clear(s->packet_buffer);
    if(s->data.audio.decoder &&
       s->data.audio.decoder->decoder->clear)
      s->data.audio.decoder->decoder->clear(&(demuxer->audio_streams[i]));

    s->packet = NULL;
    }
  for(i = 0; i < demuxer->num_video_streams; i++)
    {
    s = &(demuxer->video_streams[i]);
    if(s->packet_buffer)
      bgav_packet_buffer_clear(s->packet_buffer);
    if(s->data.video.decoder &&
       s->data.video.decoder->decoder->clear)
      s->data.video.decoder->decoder->clear(s);
    s->packet = NULL;
    s->position = -1;
    s->time     = -1;
    }
  
  /* Second step: Let the demuxer seek */
  /* This will set the "time" members of all streams */
  
  demuxer->demuxer->seek(demuxer, time);
  
  /* Third step: Resync this mess */
  new_time = time;
  for(i = 0; i < demuxer->supported_video_streams; i++)
    {
    s = &(demuxer->video_streams[demuxer->video_stream_index[i]]);
    if(s->time == -1)
      {
      if(s->position == -1)
        fprintf(stderr, "Demuxer is buggy!\n");
      s->time =
        gavl_frames_to_time(s->data.video.format.framerate_num,
                            s->data.video.format.framerate_den,
                            s->position);
      }
    else if(s->position == -1)
      {
      if(s->time == -1)
        fprintf(stderr, "Demuxer is buggy!\n");
      s->position =
        gavl_time_to_frames(s->data.video.format.framerate_num,
                            s->data.video.format.framerate_den,
                            s->time);
      }

    /* Resync video stream */

    while(1)
      {
      while(!bgav_packet_buffer_get_timestamp(s->packet_buffer, &packet_time))
        demuxer->demuxer->next_packet(demuxer);
      if(packet_time < 0)
        packet_time = gavl_frames_to_time(s->data.video.format.framerate_num,
                                               s->data.video.format.framerate_den,
                                               s->position+1);
      if(packet_time < time)
        bgav_read_video(b, (gavl_video_frame_t*)0, i);
      else
        break;
      }
    fprintf(stderr,
            "Video stream %d, time: %lld diff: %lld\n",
            i+1, s->time,
            time - s->time);
    if(!i) /* We use the first video stream as the official new time */
      new_time = packet_time;
    }
  for(i = 0; i < demuxer->num_audio_streams; i++)
    {
    
    s = &(demuxer->audio_streams[demuxer->audio_stream_index[i]]);
    diff_time = new_time - s->time;

    if(s->time == -1)
      {
      if(s->position == -1)
        fprintf(stderr, "Demuxer is buggy!\n");
      s->time =
        gavl_samples_to_time(s->data.audio.format.samplerate,
                             s->position);
      }
    else if(s->position == -1)
      {
      if(s->time == -1)
        fprintf(stderr, "Demuxer is buggy!\n");
      s->position =
        gavl_time_to_samples(s->data.audio.format.samplerate,
                             s->time);
      }
    diff_time = new_time - s->time;
    /* Skip audio samples */
    
    fprintf(stderr,
            "Audio stream %d, time: %lld diff: %lld Samples: %lld\n",
            i+1, s->time,
            new_time - s->time,
            gavl_time_to_samples(s->data.audio.format.samplerate,
                                 diff_time));
    if(diff_time > 0)
      {
      bgav_audio_decode(s, (gavl_audio_frame_t*)0,
                        gavl_time_to_samples(s->data.audio.format.samplerate,
                                             diff_time));
      }
    }
  }

void bgav_demuxer_create_buffers(bgav_demuxer_context_t * demuxer)
  {
  int i;
  bgav_stream_t * stream;
  for(i = 0; i < demuxer->supported_audio_streams; i++)
    {
    stream = &(demuxer->audio_streams[demuxer->audio_stream_index[i]]);
    if((stream->action == BGAV_STREAM_DECODE) ||
       (stream->action == BGAV_STREAM_SYNC))
      {
      stream->packet_buffer = bgav_packet_buffer_create();
      }
    }
  for(i = 0; i < demuxer->supported_video_streams; i++)
    {
    stream = &(demuxer->video_streams[demuxer->video_stream_index[i]]);
    if((stream->action == BGAV_STREAM_DECODE) ||
       (stream->action == BGAV_STREAM_SYNC))
      {
      stream->packet_buffer = bgav_packet_buffer_create();
      }
    }
  }
  
