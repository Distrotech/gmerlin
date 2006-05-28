/*****************************************************************
 
  track.c
 
  Copyright (c) 2003-2006 by Burkhard Plaum - plaum@ipf.uni-stuttgart.de
 
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
#include <stdio.h>

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
  ret->timescale = GAVL_TIME_SCALE;
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

bgav_stream_t * bgav_track_find_stream(bgav_track_t * t, int stream_id)
  {
  int i;
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
    if(t->audio_streams[i].action != BGAV_STREAM_MUTE)
      num_active_audio_streams++;
    t->audio_streams[i].demuxer = demuxer;
    if(!bgav_stream_start(&(t->audio_streams[i])))
      {
      demuxer->error_msg = 
        bgav_sprintf("Starting audio decoder for stream %d failed", i+1);
      return 0;
      }
    }
  for(i = 0; i < t->num_video_streams; i++)
    {
    if(t->video_streams[i].action != BGAV_STREAM_MUTE)
      num_active_video_streams++;
    t->video_streams[i].demuxer = demuxer;
    if(!bgav_stream_start(&(t->video_streams[i])))
      {
      demuxer->error_msg = 
        bgav_sprintf("Starting video decoder for stream %d failed", i+1);
      return 0;
      }
    }
  for(i = 0; i < t->num_subtitle_streams; i++)
    {
    if(t->subtitle_streams[i].action != BGAV_STREAM_MUTE)
      num_active_subtitle_streams++;
    t->subtitle_streams[i].demuxer = demuxer;
    
    if(!t->subtitle_streams[i].data.subtitle.video_stream)
      t->subtitle_streams[i].data.subtitle.video_stream =
        t->video_streams;

    if(!t->subtitle_streams[i].data.subtitle.video_stream)
      {
      demuxer->error_msg = 
        bgav_sprintf("Cannot decode subtitles from stream %d (no video)",
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
      bgav_stream_start(video_stream);
      bgav_stream_stop(video_stream);
      video_stream->action = BGAV_STREAM_MUTE;
      }
    
    if(!video_format->image_width || !video_format->image_height ||
       !video_format->timescale || !video_format->frame_duration)
      {
      demuxer->error_msg = 
        bgav_sprintf("Starting subtitle decoder for stream %d failed (cannot get video format)",
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
      demuxer->error_msg = 
        bgav_sprintf("Starting subtitle decoder for stream %d failed", i+1);
      return 0;
      }
    }

  if(!num_active_audio_streams && !num_active_video_streams &&
     num_active_subtitle_streams)
    demuxer->peek_forces_read = 1;
  else
    demuxer->peek_forces_read = 0;
  
  return 1;
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

  fprintf(stderr, "Duration: ");
  if(t->duration != GAVL_TIME_UNDEFINED)
    {
    gavl_time_prettyprint(t->duration, duration_string);
    fprintf(stderr, "%s\n", duration_string);
    }
  else
    fprintf(stderr, "Not specified (maybe live)\n");

  
  bgav_metadata_dump(&(t->metadata));
  
  
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
  if(t->num_subtitle_streams)
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

void bgav_track_remove_unsupported(bgav_track_t * track)
  {
  int i;
  bgav_stream_t * s;

  i = 0;
  while(i < track->num_audio_streams)
    {
    s = &(track->audio_streams[i]);
    if(!bgav_find_audio_decoder(s))
      bgav_track_remove_audio_stream(track, i);
    else
      i++;
    }
  i = 0;
  while(i < track->num_video_streams)
    {
    s = &(track->video_streams[i]);
    if(!bgav_find_video_decoder(s))
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

gavl_time_t bgav_track_resync_decoders(bgav_track_t * track)
  {
  int i;
  gavl_time_t ret = 0;
  gavl_time_t test_time;
  
  bgav_stream_t * s;

  for(i = 0; i < track->num_audio_streams; i++)
    {
    s = &(track->audio_streams[i]);

    if(s->action != BGAV_STREAM_DECODE)
      continue;
    
    bgav_stream_resync_decoder(s);

    if(s->time_scaled < 0)
      {
      fprintf(stderr, "Couldn't resync audio stream after seeking, maybe EOF\n");
      return GAVL_TIME_UNDEFINED;
      }
    test_time = gavl_samples_to_time(s->timescale, s->time_scaled);
    s->position =
      gavl_time_to_samples(s->data.audio.format.samplerate,
                           test_time);
    if(test_time > ret)
      ret = test_time;
    }
  for(i = 0; i < track->num_video_streams; i++)
    {
    s = &(track->video_streams[i]);

    if(s->action != BGAV_STREAM_DECODE)
      continue;
    
    bgav_stream_resync_decoder(s);
    
    if(s->time_scaled < 0)
      {
      fprintf(stderr, "Couldn't resync video stream after seeking, maybe EOF\n");
      return GAVL_TIME_UNDEFINED;
      }
    test_time = gavl_time_unscale(s->timescale, s->time_scaled);

    s->position =
      gavl_time_to_frames(s->data.video.format.timescale,
                          s->data.video.format.frame_duration,
                          test_time);
    if(test_time > ret)
      ret = test_time;
    }
  return ret;
  }

int bgav_track_skipto(bgav_track_t * track, gavl_time_t * time)
  {
  int i;
  bgav_stream_t * s;
  gavl_time_t t;
  
  for(i = 0; i < track->num_video_streams; i++)
    {
    t = *time;
    s = &(track->video_streams[i]);
    
    if(!bgav_stream_skipto(s, &t))
      {
      return 0;
      }
    if(!i)
      *time = t;
    }
  for(i = 0; i < track->num_audio_streams; i++)
    {
    s = &(track->audio_streams[i]);

    if(!bgav_stream_skipto(s, time))
      {
      return 0;
      }
    }
  return 1;
  }

int bgav_track_has_sync(bgav_track_t * t)
  {
  int i;

  for(i = 0; i < t->num_audio_streams; i++)
    {
    if((t->audio_streams[i].action == BGAV_STREAM_DECODE) &&
       (t->audio_streams[i].time_scaled < 0))
      return 0;
    }
  for(i = 0; i < t->num_video_streams; i++)
    {
    if((t->video_streams[i].action == BGAV_STREAM_DECODE) &&
       (t->video_streams[i].time_scaled < 0))
      return 0;
    }
  return 1;
  }
