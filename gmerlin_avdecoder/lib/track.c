/*****************************************************************
 
  track.c
 
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

#include <avdec_private.h>
#include <stdlib.h>
#include <string.h>

bgav_stream_t *
bgav_track_add_audio_stream(bgav_track_t * t)
  {
  t->num_audio_streams++;
  t->audio_streams = realloc(t->audio_streams, t->num_audio_streams * 
                             sizeof(*(t->audio_streams)));
  bgav_stream_alloc(&(t->audio_streams[t->num_audio_streams-1]));
  t->audio_streams[t->num_audio_streams-1].data.audio.bits_per_sample = 16;
  t->audio_streams[t->num_audio_streams-1].type = BGAV_STREAM_AUDIO;
  return &(t->audio_streams[t->num_audio_streams-1]);
  }

bgav_stream_t *
bgav_track_add_video_stream(bgav_track_t * t)
  {
  t->num_video_streams++;
  t->video_streams = realloc(t->video_streams, t->num_video_streams * 
                             sizeof(*(t->video_streams)));
  bgav_stream_alloc(&(t->video_streams[t->num_video_streams-1]));
  t->video_streams[t->num_video_streams-1].type = BGAV_STREAM_VIDEO;
  return &(t->video_streams[t->num_video_streams-1]);
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
  return (bgav_stream_t *)0;
  }

bgav_stream_t * bgav_track_find_stream(bgav_track_t * t, int stream_id)
  {
  int i;
  for(i = 0; i < t->num_audio_streams; i++)
    {
    if(t->audio_streams[i].stream_id == stream_id)
      {
      if(t->audio_streams[i].action != BGAV_STREAM_MUTE)
        return &(t->audio_streams[i]);
      return (bgav_stream_t *)0;
      }
    }
  for(i = 0; i < t->num_video_streams; i++)
    {
    if(t->video_streams[i].stream_id == stream_id)
      {
      if(t->video_streams[i].action != BGAV_STREAM_MUTE)
        return &(t->video_streams[i]);
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
  }

void bgav_track_start(bgav_track_t * t, bgav_demuxer_context_t * demuxer)
  {
  int i;
  for(i = 0; i < t->num_audio_streams; i++)
    {
    t->audio_streams[i].demuxer = demuxer;
    bgav_stream_start(&(t->audio_streams[i]));
    }
  for(i = 0; i < t->num_video_streams; i++)
    {
    t->video_streams[i].demuxer = demuxer;
    bgav_stream_start(&(t->video_streams[i]));
    }
  
  }


void bgav_track_dump(bgav_t * b, bgav_track_t * t)
  {
  int i;
  const char * description;
  
  char duration_string[GAVL_TIME_STRING_LEN];
  
  //  fprintf(stderr, "Track %d:\n", (int)(bgav->current_track - bgav->tracks) +1);
  fprintf(stderr, "Name:  %s\n", t->name);

  description = bgav_get_description(b);
  
  fprintf(stderr, "Format: %s\n", (description ? description : 
                                   "Not specified"));
  //  fprintf(stderr, "Seekable: %s\n", (bgav->demuxer->can_seek ? "Yes" : "No"));

  bgav_metadata_dump(&(t->metadata));
  
  fprintf(stderr, "Duration: ");
  if(t->duration != GAVL_TIME_UNDEFINED)
    {
    gavl_time_prettyprint(t->duration, duration_string);
    fprintf(stderr, "%s\n", duration_string);
    }
  else
    fprintf(stderr, "Not specified (maybe live)\n");
  
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
  
  }

void bgav_track_free(bgav_track_t * t)
  {
  int i;
  
  bgav_metadata_free(&(t->metadata));
  
  if(t->num_audio_streams)
    {
    for(i = 0; i < t->num_audio_streams; i++)
      bgav_stream_free(&(t->audio_streams[i]));
    free(t->audio_streams);
    }
  if(t->num_video_streams)
    {
    for(i = 0; i < t->num_video_streams; i++)
      bgav_stream_free(&(t->video_streams[i]));
    free(t->video_streams);
    }
  if(t->name)
    free(t->name);
  }

static void remove_stream(bgav_stream_t * stream_array, int index, int num)
  {
  fprintf(stderr, "Warning, unsupported stream:\n");
  bgav_stream_dump(&(stream_array[index]));
  bgav_stream_free(&(stream_array[index]));
  if(index < num - 1)
    {
    memmove(&(stream_array[index]),
            &(stream_array[index+1]),
            sizeof(*stream_array) * (num - 1 - index));
    }
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
      remove_stream(track->audio_streams, i, track->num_audio_streams);
      track->num_audio_streams--;
      }
    else
      i++;
    }
  i = 0;
  while(i < track->num_video_streams)
    {
    s = &(track->video_streams[i]);
    if(!bgav_find_video_decoder(s))
      {
      remove_stream(track->video_streams, i, track->num_video_streams);
      track->num_video_streams--;
      }
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
  }

gavl_time_t bgav_track_resync_decoders(bgav_track_t * track)
  {
  int i;
  gavl_time_t ret = 0;
  bgav_stream_t * s;

  for(i = 0; i < track->num_audio_streams; i++)
    {
    s = &(track->audio_streams[i]);
    bgav_stream_resync_decoder(s);

    if(s->time == GAVL_TIME_UNDEFINED)
      {
      fprintf(stderr, "Couldn't resync after seeking, maybe EOF\n");
      return GAVL_TIME_UNDEFINED;
      }
    s->position =
      gavl_time_to_samples(s->data.audio.format.samplerate,
                           s->time);
    if(s->time > ret)
      ret = track->audio_streams[i].time;
    }
  for(i = 0; i < track->num_video_streams; i++)
    {
    s = &(track->video_streams[i]);
    bgav_stream_resync_decoder(s);

    if(s->time == GAVL_TIME_UNDEFINED)
      {
      fprintf(stderr, "Couldn't resync after seeking, maybe EOF\n");
      return GAVL_TIME_UNDEFINED;
      }
    s->position =
      gavl_time_to_frames(s->data.video.format.timescale,
                          s->data.video.format.frame_duration,
                          s->time);
    if(s->time > ret)
      ret = s->time;
    }
  return ret;
  }

void bgav_track_skipto(bgav_track_t * track, gavl_time_t time)
  {
  int i;
  bgav_stream_t * s;

  for(i = 0; i < track->num_audio_streams; i++)
    {
    s = &(track->audio_streams[i]);
    bgav_stream_skipto(s, time);
    }
  for(i = 0; i < track->num_video_streams; i++)
    {
    s = &(track->video_streams[i]);
    bgav_stream_skipto(s, time);
    }
  }

int bgav_track_has_sync(bgav_track_t * t)
  {
  int i;

  for(i = 0; i < t->num_audio_streams; i++)
    {
    if(((t->audio_streams[i].action == BGAV_STREAM_DECODE) ||
        (t->audio_streams[i].action == BGAV_STREAM_SYNC)) &&
       (t->audio_streams[i].time == GAVL_TIME_UNDEFINED))
      return 0;
    }
  for(i = 0; i < t->num_video_streams; i++)
    {
    if(((t->video_streams[i].action == BGAV_STREAM_DECODE) ||
        (t->video_streams[i].action == BGAV_STREAM_SYNC)) &&
       (t->video_streams[i].time == GAVL_TIME_UNDEFINED))
      return 0;
    }
  return 1;
  }
