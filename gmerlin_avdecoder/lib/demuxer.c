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

//#define DUMP_SUPERINDEX    
#include <avdec_private.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern bgav_demuxer_t bgav_demuxer_asf;
extern bgav_demuxer_t bgav_demuxer_avi;
extern bgav_demuxer_t bgav_demuxer_rmff;
extern bgav_demuxer_t bgav_demuxer_quicktime;
extern bgav_demuxer_t bgav_demuxer_vivo;
extern bgav_demuxer_t bgav_demuxer_fli;

extern bgav_demuxer_t bgav_demuxer_wav;
extern bgav_demuxer_t bgav_demuxer_au;
extern bgav_demuxer_t bgav_demuxer_ircam;
extern bgav_demuxer_t bgav_demuxer_sphere;
extern bgav_demuxer_t bgav_demuxer_gsm;
extern bgav_demuxer_t bgav_demuxer_8svx;
extern bgav_demuxer_t bgav_demuxer_aiff;
extern bgav_demuxer_t bgav_demuxer_ra;
extern bgav_demuxer_t bgav_demuxer_mpegaudio;
extern bgav_demuxer_t bgav_demuxer_mpegvideo;
extern bgav_demuxer_t bgav_demuxer_mpegps;
extern bgav_demuxer_t bgav_demuxer_mpegts;
extern bgav_demuxer_t bgav_demuxer_flac;
extern bgav_demuxer_t bgav_demuxer_aac;
extern bgav_demuxer_t bgav_demuxer_nsv;

#ifdef HAVE_VORBIS
extern bgav_demuxer_t bgav_demuxer_ogg;
#endif

#ifdef HAVE_LIBDV
extern bgav_demuxer_t bgav_demuxer_dv;
#endif

#ifdef HAVE_LIBA52
extern bgav_demuxer_t bgav_demuxer_a52;
#endif

#ifdef HAVE_MUSEPACK
extern bgav_demuxer_t bgav_demuxer_mpc;
#endif

#ifdef HAVE_MJPEGTOOLS
extern bgav_demuxer_t bgav_demuxer_y4m;
#endif

typedef struct
  {
  bgav_demuxer_t * demuxer;
  char * format_name;
  } demuxer_t;

static demuxer_t demuxers[] =
  {
    { &bgav_demuxer_asf,       "Microsoft ASF/WMV/WMA" },
    { &bgav_demuxer_avi,       "Microsoft AVI" },
    { &bgav_demuxer_rmff,      "Real Media" },
    { &bgav_demuxer_ra,        "Real Audio" },
    { &bgav_demuxer_quicktime, "Quicktime/mp4/m4a" },
    { &bgav_demuxer_wav,       "Microsoft WAV" },
    { &bgav_demuxer_au,        "Sun AU" },
    { &bgav_demuxer_aiff,      "AIFF(C)" },
    { &bgav_demuxer_flac,      "FLAC" },
    { &bgav_demuxer_aac,       "AAC" },
    { &bgav_demuxer_vivo,      "Vivo" },
    { &bgav_demuxer_mpegvideo, "Mpeg Video" },
    { &bgav_demuxer_fli,       "FLI/FLC Animation" },
    { &bgav_demuxer_nsv,       "NullSoft Video" },
#ifdef HAVE_VORBIS
    { &bgav_demuxer_ogg, "Ogg Bitstream" },
#endif
#ifdef HAVE_LIBA52
    { &bgav_demuxer_a52, "A52 Bitstream" },
#endif
#ifdef HAVE_MUSEPACK
    { &bgav_demuxer_mpc, "Musepack" },
#endif
#ifdef HAVE_MJPEGTOOLS
    { &bgav_demuxer_y4m, "yuv4mpeg" },
#endif
#ifdef HAVE_LIBDV
    { &bgav_demuxer_dv, "DV" },
#endif
    { &bgav_demuxer_sphere, "nist Sphere"},
    { &bgav_demuxer_ircam, "IRCAM" },
    { &bgav_demuxer_8svx, "Amiga IFF" },
    { &bgav_demuxer_gsm, "raw gsm" },
  };

static demuxer_t sync_demuxers[] =
  {
    //    { &bgav_demuxer_mpegts,    "MPEG-2 transport stream" },
    { &bgav_demuxer_mpegaudio, "Mpeg Audio" },
    { &bgav_demuxer_mpegps, "Mpeg System" },
  };

static struct
  {
  bgav_demuxer_t * demuxer;
  char * mimetype;
  }
mimetypes[] =
  {
    { &bgav_demuxer_mpegaudio, "audio/mpeg" }
  };

static int num_demuxers = sizeof(demuxers)/sizeof(demuxers[0]);
static int num_sync_demuxers = sizeof(sync_demuxers)/sizeof(sync_demuxers[0]);
static int num_mimetypes = sizeof(mimetypes)/sizeof(mimetypes[0]);

static int demuxer_next_packet(bgav_demuxer_context_t * demuxer);


#define SYNC_BYTES 4096

bgav_demuxer_t * bgav_demuxer_probe(bgav_input_context_t * input)
  {
  int i;
  int bytes_skipped;
  uint8_t skip;
    
  //  uint8_t header[32];
  if(input->mimetype)
    {
    for(i = 0; i < num_mimetypes; i++)
      {
      if(!strcmp(mimetypes[i].mimetype, input->mimetype))
        {
        return mimetypes[i].demuxer;
        }
      }
    }
    
  for(i = 0; i < num_demuxers; i++)
    {
    if(demuxers[i].demuxer->probe(input))
      {
      fprintf(stderr, "Detected %s format\n", demuxers[i].format_name);
      return demuxers[i].demuxer;
      }
    }
  
  for(i = 0; i < num_sync_demuxers; i++)
    {
    if(sync_demuxers[i].demuxer->probe(input))
      {
      fprintf(stderr, "Detected %s format\n", sync_demuxers[i].format_name);
      return sync_demuxers[i].demuxer;
      }
    }

  /* Try again with skipping initial bytes */

  bytes_skipped = 0;

  while(bytes_skipped < SYNC_BYTES)
    {
    bytes_skipped++;
    if(!bgav_input_read_data(input, &skip, 1))
      return (bgav_demuxer_t *)0;

    for(i = 0; i < num_sync_demuxers; i++)
      {
      if(sync_demuxers[i].demuxer->probe(input))
        {
        fprintf(stderr, "Detected %s format after skipping %d bytes\n",
                sync_demuxers[i].format_name, bytes_skipped);
        return sync_demuxers[i].demuxer;
        }
      }
    
    }
  
  
  return (bgav_demuxer_t *)0;
  }

bgav_demuxer_context_t *
bgav_demuxer_create(const bgav_options_t * opt, bgav_demuxer_t * demuxer,
                    bgav_input_context_t * input)
  {
  bgav_demuxer_context_t * ret;
  
  ret = calloc(1, sizeof(*ret));
  ret->opt = opt;
  ret->demuxer = demuxer;
  ret->input = input;
    
  return ret;
  }

#define FREE(p) if(p){free(p);p=NULL;}

void bgav_demuxer_destroy(bgav_demuxer_context_t * ctx)
  {
  ctx->demuxer->close(ctx);
  if(ctx->tt)
    bgav_track_table_unref(ctx->tt);

  if(ctx->si)
    bgav_superindex_destroy(ctx->si);

  FREE(ctx->stream_description);
  free(ctx);
  }

/* Check and kick out streams, for which a header exists but no
   packets */

static void check_missing_streams(bgav_demuxer_context_t * ctx)
  {
  int i;

  i = 0;
  while(i < ctx->tt->current_track->num_audio_streams)
    {
    if((ctx->tt->current_track->audio_streams[i].last_index_position < 0) &&
       (!ctx->tt->current_track->audio_streams[i].no_packets))
      bgav_track_remove_audio_stream(ctx->tt->current_track, i);
    else
      i++;
    }

  i = 0;
  while(i < ctx->tt->current_track->num_video_streams)
    {
    if(ctx->tt->current_track->video_streams[i].last_index_position < 0)
      bgav_track_remove_video_stream(ctx->tt->current_track, i);
    else
      i++;
    }
  }

static void check_interleave(bgav_demuxer_context_t * ctx)
  {
  bgav_stream_t * s1;
  bgav_stream_t * s2 = (bgav_stream_t *)0;
  int i;
  int num;
  
  num = 0;
  for(i = 0; i < ctx->tt->current_track->num_audio_streams; i++)
    {
    if(!ctx->tt->current_track->audio_streams[i].no_packets)
      num++;
    }
  for(i = 0; i < ctx->tt->current_track->num_video_streams; i++)
    {
    if(!ctx->tt->current_track->video_streams[i].no_packets)
      num++;
    }
  if(num <= 1)
    return;
  
  if(ctx->tt->current_track->num_audio_streams &&
     ctx->tt->current_track->num_video_streams)
    {
    s1 = &(ctx->tt->current_track->audio_streams[0]);
    s2 = &(ctx->tt->current_track->video_streams[0]);
    }
  else if(ctx->tt->current_track->num_audio_streams)
    {
    s1 = &(ctx->tt->current_track->audio_streams[0]);
    s1 = &(ctx->tt->current_track->audio_streams[1]);
    }
  else 
    {
    s1 = &(ctx->tt->current_track->video_streams[0]);
    s1 = &(ctx->tt->current_track->video_streams[1]);
    }

  if((s1->last_index_position < s2->first_index_position) ||
     (s2->last_index_position < s1->first_index_position))
    {
    ctx->non_interleaved = 1;

    /* Adjust index positions for the streams */

    for(i = 0; i < ctx->tt->current_track->num_audio_streams; i++)
      {
      s1 = &(ctx->tt->current_track->audio_streams[i]);
      s1->index_position = s1->first_index_position;
      }
    for(i = 0; i < ctx->tt->current_track->num_video_streams; i++)
      {
      s1 = &(ctx->tt->current_track->video_streams[i]);
      s1->index_position = s1->first_index_position;
      }
    }
  }

int bgav_demuxer_start(bgav_demuxer_context_t * ctx,
                       bgav_redirector_context_t ** redir)
  {
  if(!ctx->demuxer->open(ctx, redir))
    return 0;
  
  if(ctx->si)
    {
    check_missing_streams(ctx);
    check_interleave(ctx);
#ifdef DUMP_SUPERINDEX    
    bgav_superindex_dump(ctx->si);
#endif
    if(ctx->non_interleaved)
      {
      //      fprintf(stderr, "Non interleaved\n");
      
      if(!ctx->input->input->seek_byte)
        {
        fprintf(stderr, "Non interleaved file from non seekable source\n");
        return 0;
        }
      }
    }
#if 0
  /* This is important since some demuxers set up incredibly important format informations
     during this call */
  if(!demuxer_next_packet(ctx))
    {
    fprintf(stderr, "demuxer_next_packet failed\n");
    return 0;
    }
#endif

  
  return 1;
  }

void bgav_demuxer_stop(bgav_demuxer_context_t * ctx)
  {
  ctx->demuxer->close(ctx);
  ctx->priv = NULL;
  FREE(ctx->stream_description);
  }

static int next_packet_interleaved(bgav_demuxer_context_t * ctx)
  {
  bgav_stream_t * stream;
  bgav_packet_t * p;
  
  //  fprintf(stderr, "next_packet_interleaved\n");
  
  if(ctx->si->current_position >= ctx->si->num_entries)
    {
    //    fprintf(stderr, "EOF\n");
    return 0;
    }

  if(ctx->input->position >=
     ctx->si->entries[ctx->si->num_entries - 1].offset +
     ctx->si->entries[ctx->si->num_entries - 1].size)
    {
    return 0;
    }
  stream =
    bgav_track_find_stream(ctx->tt->current_track,
                           ctx->si->entries[ctx->si->current_position].stream_id);
  
  if(!stream) /* Skip unused stream */
    {
    bgav_input_skip(ctx->input,
                    ctx->si->entries[ctx->si->current_position].size);
    ctx->si->current_position++;
    return 1;
    }
  
  if(ctx->seeking && (stream->index_position > ctx->si->current_position))
    {
    bgav_input_skip(ctx->input,
                    ctx->si->entries[ctx->si->current_position].size);
    ctx->si->current_position++;
    return 1;
    }

  /* Skip until this packet */

  if(ctx->si->entries[ctx->si->current_position].offset > ctx->input->position)
    {
    bgav_input_skip(ctx->input, ctx->si->entries[ctx->si->current_position].offset - ctx->input->position);
    }

  if(ctx->read_packet)
    {
    if(!ctx->read_packet(ctx, ctx->si->entries[ctx->si->current_position].size))
      return 0;
    }
  else
    {
    p = bgav_packet_buffer_get_packet_write(stream->packet_buffer, stream);
    bgav_packet_alloc(p, ctx->si->entries[ctx->si->current_position].size);
    p->data_size = ctx->si->entries[ctx->si->current_position].size;
    
    //  fprintf(stderr, "size: %d\n", p->data_size);
    
    p->timestamp_scaled = ctx->si->entries[ctx->si->current_position].time;
    p->samples = ctx->si->entries[ctx->si->current_position].samples;
    if(bgav_input_read_data(ctx->input, p->data, p->data_size) < p->data_size)
      return 0;
    
    bgav_packet_done_write(p);
    }
  
  //  fprintf(stderr, "Current chunk: %d\n", priv->current_packet);
  ctx->si->current_position++;
  return 1;
  }

static int next_packet_noninterleaved(bgav_demuxer_context_t * ctx)
  {
  int i;
  bgav_stream_t * stream = (bgav_stream_t*)0;
  bgav_stream_t * test_stream;
  bgav_packet_t * p;
  
  //  fprintf(stderr, "next_packet_noninterleaved\n");
  
  /* We read data for the first stream with an empty packet buffer */

  for(i = 0; i < ctx->tt->current_track->num_audio_streams; i++)
    {
    test_stream = &(ctx->tt->current_track->audio_streams[i]);
    if(test_stream->no_packets)
      continue;
#if 0
    fprintf(stderr, "A: %d %d %d\n",
            bgav_packet_buffer_is_empty(test_stream->packet_buffer),
            stream_priv->index_position, stream_priv->last_index_position);
#endif
    if((test_stream->action != BGAV_STREAM_MUTE) &&
       bgav_packet_buffer_is_empty(test_stream->packet_buffer) &&
       (test_stream->index_position <= test_stream->last_index_position))
      {
      stream = test_stream;
      break;
      }
    }
  if(!stream)
    {
    for(i = 0; i < ctx->tt->current_track->num_video_streams; i++)
      {
      test_stream = &(ctx->tt->current_track->video_streams[i]);
      if(test_stream->no_packets)
        continue;
#if 0
      fprintf(stderr, "V: %d %d %d\n",
            bgav_packet_buffer_is_empty(test_stream->packet_buffer),
            stream_priv->index_position, stream_priv->last_index_position);
#endif
      if((test_stream->action != BGAV_STREAM_MUTE) &&
         bgav_packet_buffer_is_empty(test_stream->packet_buffer) &&
         (test_stream->index_position <= test_stream->last_index_position))
        {
        stream = test_stream;
        break;
        }
      }
    }

  if(!stream)
    {
    //    fprintf(stderr, "EOF\n");
    return 0;
    }

  while(ctx->si->entries[stream->index_position].stream_id != stream->stream_id)
    {
    stream->index_position++;
    }
  
  bgav_input_seek(ctx->input,
                  ctx->si->entries[stream->index_position].offset,
                  SEEK_SET);

  p = bgav_packet_buffer_get_packet_write(stream->packet_buffer, stream);
  p->data_size = ctx->si->entries[stream->index_position].size;
  bgav_packet_alloc(p, p->data_size);
  
  p->timestamp_scaled = ctx->si->entries[stream->index_position].time;
  p->samples          = ctx->si->entries[stream->index_position].samples;
    
  if(bgav_input_read_data(ctx->input, p->data, p->data_size) < p->data_size)
    return 0;
  bgav_packet_done_write(p);
  
  //  fprintf(stderr, "Current chunk: %d\n", stream_priv->index_position);
  stream->index_position++;
  return 1;
  
  }

static int demuxer_next_packet(bgav_demuxer_context_t * demuxer)
  {
  int ret, i;
  if(demuxer->si) /* Superindex present */
    {
    if(demuxer->non_interleaved)
      return next_packet_noninterleaved(demuxer);
    else
      return next_packet_interleaved(demuxer);
    }
  else
    {
    ret = demuxer->demuxer->next_packet(demuxer);

    if(!ret)
      {
      /* Some demuxers have packets stored in the streams,
         we flush them here */
 
      for(i = 0; i < demuxer->tt->current_track->num_audio_streams; i++)
        {
        if(demuxer->tt->current_track->audio_streams[i].packet)
          {
          bgav_packet_done_write(demuxer->tt->current_track->audio_streams[i].packet);
          demuxer->tt->current_track->audio_streams[i].packet = (bgav_packet_t*)0;
          ret = 1;
          }
        }
      for(i = 0; i < demuxer->tt->current_track->num_video_streams; i++)
        {
        if(demuxer->tt->current_track->video_streams[i].packet)
          {
          bgav_packet_done_write(demuxer->tt->current_track->video_streams[i].packet);
          demuxer->tt->current_track->video_streams[i].packet = (bgav_packet_t*)0;
          ret = 1;
          }
        }
      }
    return ret;
    }
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
    if(!demuxer_next_packet(demuxer))
      return (bgav_packet_t*)0;
    }
  s->time_scaled = ret->timestamp_scaled;
  return ret;
  }

void 
bgav_demuxer_done_packet_read(bgav_demuxer_context_t * demuxer,
                              bgav_packet_t * p)
  {
  bgav_packet_t * next = (bgav_packet_t*)0;

  p->valid = 0;

  if(p->stream->type == BGAV_STREAM_VIDEO)
    {
    p->stream->data.video.last_frame_time = p->timestamp_scaled;

    while(!(next = bgav_packet_buffer_peek_packet_read(p->stream->packet_buffer)))
      {
      if(!demuxer_next_packet(demuxer))
        {
        p->stream->data.video.last_frame_duration = p->stream->data.video.format.frame_duration;
        return;
        }
      }
    p->stream->data.video.last_frame_duration =
      next->timestamp_scaled - p->stream->data.video.last_frame_time;
    }
  }

/* Seek functions with superindex */

static void seek_si(bgav_demuxer_context_t * ctx, gavl_time_t time)
  {
  uint32_t i, j;
  int32_t start_packet;
  int32_t end_packet;
  bgav_track_t * track;
  int no_packets = 0;
    
  //  fprintf(stderr, "Seek si %f %lld\n", gavl_time_to_seconds(time), time);
  track = ctx->tt->current_track;
  
  /* Set the packet indices of the streams to -1 */
  for(j = 0; j < track->num_audio_streams; j++)
    {
    track->audio_streams[j].index_position = -1;
    }
  for(j = 0; j < track->num_video_streams; j++)
    {
    track->video_streams[j].index_position = -1;
    }

  //  fprintf(stderr, "Blupp 1\n");
  
  /* Seek the start chunks indices of all streams */
  
  for(j = 0; j < track->num_audio_streams; j++)
    {
    if(track->audio_streams[j].no_packets)
      {
      no_packets = 1;
      continue;
      }
    bgav_superindex_seek(ctx->si, &(track->audio_streams[j]), time);
    //    fprintf(stderr, "Audio position %d\n", track->audio_streams[j].index_position);
    }
  for(j = 0; j < track->num_video_streams; j++)
    {
    if(track->video_streams[j].no_packets)
      {
      no_packets = 1;
      continue;
      }
    bgav_superindex_seek(ctx->si, &(track->video_streams[j]), time);
    //    fprintf(stderr, "Video position %d\n", track->video_streams[j].index_position);
    }

  if(no_packets)
    {
    for(j = 0; j < track->num_audio_streams; j++)
      {
      if(track->audio_streams[j].no_packets)
        track->audio_streams[j].time_scaled = track->audio_streams[j].sync_stream->time_scaled;
      }
    for(j = 0; j < track->num_video_streams; j++)
      {
      if(track->video_streams[j].no_packets)
        track->video_streams[j].time_scaled = track->video_streams[j].sync_stream->time_scaled;
      }
    } 
#if 0
  fprintf(stderr, "********* Blupp 2, time 1: %f, time 2: %f\n",
          (float)track->audio_streams[0].time_scaled / track->audio_streams[0].timescale,
          (float)track->video_streams[0].time_scaled / track->video_streams[0].timescale);
#endif
  /* Find the start and end packet */

  if(!ctx->non_interleaved)
    {
    start_packet = 0x7FFFFFFF;
    end_packet   = 0x0;
    
    for(j = 0; j < track->num_audio_streams; j++)
      {
      if(track->audio_streams[j].no_packets)
        continue;
      if(start_packet > track->audio_streams[j].index_position)
        start_packet = track->audio_streams[j].index_position;
      if(end_packet < track->audio_streams[j].index_position)
        end_packet = track->audio_streams[j].index_position;
      }
    for(j = 0; j < track->num_video_streams; j++)
      {
      if(track->video_streams[j].no_packets)
        continue;
      if(start_packet > track->video_streams[j].index_position)
        start_packet = track->video_streams[j].index_position;
      if(end_packet < track->video_streams[j].index_position)
        end_packet = track->video_streams[j].index_position;
      }

    //    fprintf(stderr, "Blupp 3 %d %d\n", start_packet, end_packet);

    /* Do the seek */
    ctx->si->current_position = start_packet;
    bgav_input_seek(ctx->input, ctx->si->entries[ctx->si->current_position].offset,
                    SEEK_SET);

    ctx->seeking = 1;
    for(i = start_packet; i <= end_packet; i++)
      next_packet_interleaved(ctx);

    //    fprintf(stderr, "Blupp 4\n");

    ctx->seeking = 0;
    }

  //  fprintf(stderr, "seek done\n");
  }

/* Maximum allowed seek tolerance, decrease if you want it more exact */

// #define SEEK_TOLERANCE (GAVL_TIME_SCALE/2)

#define SEEK_TOLERANCE 0
/* We skip at most this time */

#define MAX_SKIP_TIME  (GAVL_TIME_SCALE*5)

static void skip_to(bgav_t * b, bgav_track_t * track, gavl_time_t * time)
  {
  //  fprintf(stderr, "bgav_track_skipto: %f\n", gavl_time_to_seconds(*time));
  if(!bgav_track_skipto(track, time))
    b->eof = 1;
  }

void
bgav_seek(bgav_t * b, gavl_time_t * time)
  {
  gavl_time_t diff_time;
  gavl_time_t sync_time;
  gavl_time_t seek_time;

  gavl_time_t last_seek_time = GAVL_TIME_UNDEFINED;
  gavl_time_t last_seek_time_2nd = GAVL_TIME_UNDEFINED;

  gavl_time_t last_sync_time = GAVL_TIME_UNDEFINED;
  gavl_time_t last_sync_time_2nd = GAVL_TIME_UNDEFINED;
    
  bgav_track_t * track = b->tt->current_track;
  int num_iterations = 0;
  seek_time = *time;
      
  while(1)
    {
    num_iterations++;
    
    /* First step: Flush all fifos */
    bgav_track_clear(track);
    
    /* Second step: Let the demuxer seek */
    /* This will set the "time" members of all streams */

    if(b->demuxer->si)
      seek_si(b->demuxer, seek_time);
    else
      b->demuxer->demuxer->seek(b->demuxer, seek_time);
    sync_time = bgav_track_resync_decoders(track);
    if(sync_time == GAVL_TIME_UNDEFINED)
      {
      fprintf(stderr, "Warning: Undefined sync time after seeking\n");
      return;
      }
    /* If demuxer already seeked perfectly, break here */

    if(!b->demuxer->demuxer->seek_iterative)
      {

      if(*time > sync_time)
        skip_to(b, track, time);
      else
        {
        skip_to(b, track, &sync_time);
        *time = sync_time;
        }
      break;
      }
    /* Check if we should end this */
    
    if((last_sync_time != GAVL_TIME_UNDEFINED) &&
       (last_sync_time_2nd != GAVL_TIME_UNDEFINED))
      {
      if((sync_time == last_sync_time) || 
         (sync_time == last_sync_time_2nd))
        {
        /* Go to the smaller of both times */
        
        if(last_sync_time < *time) /* Go to last sync time */
          {
          if(last_sync_time != sync_time)
            {
            if(b->demuxer->si)
              seek_si(b->demuxer, seek_time);
            else
              b->demuxer->demuxer->seek(b->demuxer, seek_time);
            sync_time = bgav_track_resync_decoders(track);
            }
          }
        else /* Go to 2nd last sync time */
          {
          if(last_sync_time != sync_time)
            {
            b->demuxer->demuxer->seek(b->demuxer, seek_time);
            sync_time = bgav_track_resync_decoders(track);
            }
          }

        
        if(*time > sync_time)
          skip_to(b, track, time);
        else
          {
          skip_to(b, track, &sync_time);
          *time = sync_time;
          }
        break;
        // fprintf(stderr, "Exiting otherwise infinite loop\n");
        }
      
      }

    last_seek_time_2nd = last_seek_time;
    last_sync_time_2nd = last_sync_time;

    last_seek_time = seek_time;
    last_sync_time = sync_time;
    
    
    diff_time = *time - sync_time;
#if 0
    fprintf(stderr, "Seeked, time: %f seek_time: %f, last_seek_time: %f\n",
            gavl_time_to_seconds(*time),
            gavl_time_to_seconds(seek_time),
            gavl_time_to_seconds(last_seek_time));
#endif
    if(diff_time > MAX_SKIP_TIME)
      {
      seek_time += (diff_time * 2)/3;
      }
    else if(diff_time > 0)
      {
      skip_to(b, track, time);
      break;
      }
    else if(diff_time < -SEEK_TOLERANCE)
      {
      /* We get a bit more back than the error was */
      seek_time += (diff_time * 3)/2;
      if(seek_time < 0)
        seek_time = 0;
      }
    else
      {
      skip_to(b, track, &sync_time);
      *time = sync_time;
      break;
      }
    }
  //  fprintf(stderr, "Seek done, %d iterations\n", num_iterations);
  }

void bgav_formats_dump()
  {
  int i;
  fprintf(stderr, "<h2>Formats</h2>\n<ul>");
  for(i = 0; i < num_demuxers; i++)
    fprintf(stderr, "<li>%s\n", demuxers[i].format_name);
  for(i = 0; i < num_sync_demuxers; i++)
    fprintf(stderr, "<li>%s\n", sync_demuxers[i].format_name);
  fprintf(stderr, "</ul>\n");
  }
