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
extern bgav_demuxer_t bgav_demuxer_mpegaudio;
extern bgav_demuxer_t bgav_demuxer_mpegps;

static struct
  {
  bgav_demuxer_t * demuxer;
  char * format_name;
  }
demuxers[] =
  {
    { &bgav_demuxer_asf, "Microsoft ASF" },
    { &bgav_demuxer_avi, "Microsoft AVI" },
    { &bgav_demuxer_rmff, "Real Media" },
    { &bgav_demuxer_quicktime, "Apple Quicktime" },
    { &bgav_demuxer_wav, "Microsoft WAV" },
    { &bgav_demuxer_au, "AU" },
    { &bgav_demuxer_aiff, "AIFF" },
    { &bgav_demuxer_ra, "Real Audio" },
    { &bgav_demuxer_mpegaudio, "Mpeg Audio" },
    { &bgav_demuxer_mpegps, "Mpeg System" },
  };

static int num_demuxers = sizeof(demuxers)/sizeof(demuxers[0]);

bgav_demuxer_t * bgav_demuxer_probe(bgav_input_context_t * input)
  {
  int i;
  uint8_t header[32];
  for(i = 0; i < num_demuxers; i++)
    {
    if(demuxers[i].demuxer->probe(input))
      {
      fprintf(stderr, "Detected %s format\n", demuxers[i].format_name);
      return demuxers[i].demuxer;
      }
    }
  fprintf(stderr, "Cannot detect format, first 32 bytes follow\n");

  if(bgav_input_get_data(input, header, 32) == 32)
    bgav_hexdump(header, 32, 16);
  return (bgav_demuxer_t *)0;
  }

bgav_demuxer_context_t *
bgav_demuxer_create(bgav_demuxer_t * demuxer,
                    bgav_input_context_t * input)
  {
  bgav_demuxer_context_t * ret;
  
  ret = calloc(1, sizeof(*ret));
  
  ret->demuxer = demuxer;
  ret->input = input;
    
  return ret;
  }

#define FREE(p) if(p)free(p)

void bgav_demuxer_destroy(bgav_demuxer_context_t * ctx)
  {
  ctx->demuxer->close(ctx);
  if(ctx->tt)
    bgav_track_table_unref(ctx->tt);
  FREE(ctx->stream_description);
  free(ctx);
  }


int bgav_demuxer_start(bgav_demuxer_context_t * ctx,
                       bgav_redirector_context_t ** redir)
  {
  return ctx->demuxer->open(ctx, redir);
  }

void bgav_demuxer_stop(bgav_demuxer_context_t * ctx)
  {
  if(ctx->stream_description)
    free(ctx->stream_description);
  
  ctx->demuxer->close(ctx);
  ctx->priv = NULL;
  FREE(ctx->stream_description);
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
  bgav_track_t * track = b->tt->current_track;
  
  for(i = 0; i < track->num_audio_streams; i++)
    {
    s = &(track->audio_streams[i]);

    if(s->packet_buffer)
      bgav_packet_buffer_clear(s->packet_buffer);
    if(s->data.audio.decoder &&
       s->data.audio.decoder->decoder->clear)
      s->data.audio.decoder->decoder->clear(&(track->audio_streams[i]));

    s->packet = NULL;
    }
  for(i = 0; i < track->num_video_streams; i++)
    {
    s = &(track->video_streams[i]);
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
  for(i = 0; i < track->num_video_streams; i++)
    {
    s = &(track->video_streams[i]);
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
  for(i = 0; i < track->num_audio_streams; i++)
    {
    
    s = &(track->audio_streams[i]);
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

