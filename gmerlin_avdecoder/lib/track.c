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

bgav_stream_t * bgav_track_find_stream(bgav_track_t * t, int stream_id)
  {
  int i;
  for(i = 0; i < t->num_audio_streams; i++)
    {
    if(t->audio_streams[i].stream_id == stream_id)
      {
      if((t->audio_streams[i].action != BGAV_STREAM_MUTE) &&
         (t->audio_streams[i].action != BGAV_STREAM_UNSUPPORTED))
        return &(t->audio_streams[i]);
      return (bgav_stream_t *)0;
      }
    }
  for(i = 0; i < t->num_video_streams; i++)
    {
    if(t->video_streams[i].stream_id == stream_id)
      {
      if((t->video_streams[i].action != BGAV_STREAM_MUTE) &&
         (t->video_streams[i].action != BGAV_STREAM_UNSUPPORTED))
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
  gavl_time_t duration;
  
  //  fprintf(stderr, "Track %d:\n", (int)(bgav->current_track - bgav->tracks) +1);
  fprintf(stderr, "Name  %s:\n", t->name);

  description = bgav_get_description(b);
  
  fprintf(stderr, "Format: %s\n", (description ? description : 
                                   "Not specified"));
  //  fprintf(stderr, "Seekable: %s\n", (bgav->demuxer->can_seek ? "Yes" : "No"));

  bgav_metadata_dump(&(t->metadata));

  duration = t->duration;

  fprintf(stderr, "Duration: ");
  if(duration)
    {
    gavl_time_prettyprint(duration, duration_string);
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
  if(t->num_audio_streams)
    {
    for(i = 0; i < t->num_video_streams; i++)
      bgav_stream_free(&(t->video_streams[i]));
    free(t->video_streams);
    }
  if(t->name)
    free(t->name);
  }
