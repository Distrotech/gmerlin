/*****************************************************************
 
  demuxer.c
 
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

// #define DUMP_SUPERINDEX    
#include <avdec_private.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define LOG_DOMAIN "demuxer"

extern bgav_demuxer_t bgav_demuxer_asf;
extern bgav_demuxer_t bgav_demuxer_avi;
extern bgav_demuxer_t bgav_demuxer_rmff;
extern bgav_demuxer_t bgav_demuxer_quicktime;
extern bgav_demuxer_t bgav_demuxer_vivo;
extern bgav_demuxer_t bgav_demuxer_fli;
extern bgav_demuxer_t bgav_demuxer_flv;

extern bgav_demuxer_t bgav_demuxer_wavpack;
extern bgav_demuxer_t bgav_demuxer_tta;
extern bgav_demuxer_t bgav_demuxer_voc;
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
extern bgav_demuxer_t bgav_demuxer_4xm;
extern bgav_demuxer_t bgav_demuxer_dsicin;
extern bgav_demuxer_t bgav_demuxer_smaf;
extern bgav_demuxer_t bgav_demuxer_psxstr;
extern bgav_demuxer_t bgav_demuxer_tiertex;
extern bgav_demuxer_t bgav_demuxer_smacker;
extern bgav_demuxer_t bgav_demuxer_roq;
extern bgav_demuxer_t bgav_demuxer_shorten;
extern bgav_demuxer_t bgav_demuxer_daud;
extern bgav_demuxer_t bgav_demuxer_nuv;
extern bgav_demuxer_t bgav_demuxer_sol;
extern bgav_demuxer_t bgav_demuxer_gif;
extern bgav_demuxer_t bgav_demuxer_smjpeg;
extern bgav_demuxer_t bgav_demuxer_vqa;
extern bgav_demuxer_t bgav_demuxer_vmd;
extern bgav_demuxer_t bgav_demuxer_avs;
extern bgav_demuxer_t bgav_demuxer_wve;
extern bgav_demuxer_t bgav_demuxer_mtv;
extern bgav_demuxer_t bgav_demuxer_gxf;
extern bgav_demuxer_t bgav_demuxer_dxa;
extern bgav_demuxer_t bgav_demuxer_thp;

#ifdef HAVE_VORBIS
extern bgav_demuxer_t bgav_demuxer_ogg;
#endif

extern bgav_demuxer_t bgav_demuxer_dv;

#ifdef HAVE_LIBA52
extern bgav_demuxer_t bgav_demuxer_a52;
#endif

#ifdef HAVE_MUSEPACK
extern bgav_demuxer_t bgav_demuxer_mpc;
#endif

#ifdef HAVE_MJPEGTOOLS
extern bgav_demuxer_t bgav_demuxer_y4m;
#endif

#ifdef HAVE_LIBAVFORMAT
extern bgav_demuxer_t bgav_demuxer_ffmpeg;
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
    { &bgav_demuxer_flv,       "Flash video (FLV)" },
    { &bgav_demuxer_nsv,       "NullSoft Video" },
    { &bgav_demuxer_wavpack,   "Wavpack" },
    { &bgav_demuxer_tta,       "True Audio" },
    { &bgav_demuxer_voc,       "Creative voice" },
    { &bgav_demuxer_4xm,       "4xm" },
    { &bgav_demuxer_dsicin,    "Delphine Software CIN" },
    { &bgav_demuxer_8svx,      "Amiga IFF" },
    { &bgav_demuxer_smaf,      "SMAF Ringtone" },
    { &bgav_demuxer_psxstr,    "Sony Playstation (PSX) STR" },
    { &bgav_demuxer_tiertex,   "Tiertex SEQ" },
    { &bgav_demuxer_smacker,   "Smacker" },
    { &bgav_demuxer_roq,       "ID Roq" },
    { &bgav_demuxer_shorten,   "Shorten" },
    { &bgav_demuxer_nuv,       "NuppelVideo/MythTV" },
    { &bgav_demuxer_sol,       "Sierra SOL" },
    { &bgav_demuxer_gif,       "GIF" },
    { &bgav_demuxer_smjpeg,    "SMJPEG" },
    { &bgav_demuxer_vqa,       "Westwood VQA" },
    { &bgav_demuxer_avs,       "AVS" },
    { &bgav_demuxer_wve,       "Electronicarts WVE" },
    { &bgav_demuxer_mtv,       "MTV" },
    { &bgav_demuxer_gxf,       "GXF" },
    { &bgav_demuxer_dxa,       "DXA" },
    { &bgav_demuxer_thp,       "THP" },
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
    { &bgav_demuxer_dv, "DV" },
    { &bgav_demuxer_sphere, "nist Sphere"},
    { &bgav_demuxer_ircam, "IRCAM" },
    { &bgav_demuxer_gsm, "raw gsm" },
    { &bgav_demuxer_daud, "D-Cinema audio" },
    { &bgav_demuxer_vmd,  "Sierra VMD" },
  };

static demuxer_t sync_demuxers[] =
  {
    { &bgav_demuxer_mpegts,    "MPEG-2 transport stream" },
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


#define SYNC_BYTES (32*1024)

bgav_demuxer_t * bgav_demuxer_probe(bgav_input_context_t * input)
  {
  int i;
  int bytes_skipped;
  uint8_t skip;
  
#ifdef HAVE_LIBAVFORMAT
  if(input->opt->prefer_ffmpeg_demuxers)
    {
    if(bgav_demuxer_ffmpeg.probe(input))
      return &bgav_demuxer_ffmpeg;
    }
#endif
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
      bgav_log(input->opt, BGAV_LOG_INFO, LOG_DOMAIN,
               "Detected %s format", demuxers[i].format_name);
      return demuxers[i].demuxer;
      }
    }
  
  for(i = 0; i < num_sync_demuxers; i++)
    {
    if(sync_demuxers[i].demuxer->probe(input))
      {
      bgav_log(input->opt, BGAV_LOG_INFO, LOG_DOMAIN,
               "Detected %s format",
               sync_demuxers[i].format_name);
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
        bgav_log(input->opt, BGAV_LOG_INFO, LOG_DOMAIN,
                 "Detected %s format after skipping %d bytes",
                 sync_demuxers[i].format_name, bytes_skipped);
        return sync_demuxers[i].demuxer;
        }
      }
    
    }
  
#ifdef HAVE_LIBAVFORMAT
  if(!input->opt->prefer_ffmpeg_demuxers && input->input->seek_byte)
    {
    bgav_input_seek(input, 0, SEEK_SET);
    if(bgav_demuxer_ffmpeg.probe(input))
      return &bgav_demuxer_ffmpeg;
    }
#endif
  
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
  while(i < ctx->tt->cur->num_audio_streams)
    {
    if(ctx->tt->cur->audio_streams[i].last_index_position < 0)
      bgav_track_remove_audio_stream(ctx->tt->cur, i);
    else
      i++;
    }

  i = 0;
  while(i < ctx->tt->cur->num_video_streams)
    {
    if(ctx->tt->cur->video_streams[i].last_index_position < 0)
      bgav_track_remove_video_stream(ctx->tt->cur, i);
    else
      i++;
    }
  }

static void check_interleave(bgav_demuxer_context_t * ctx)
  {
  int i;
  bgav_stream_t ** streams;
  int index, num_streams;

  num_streams =
    ctx->tt->cur->num_audio_streams +
    ctx->tt->cur->num_video_streams +
    ctx->tt->cur->num_subtitle_streams;
  
  if(num_streams <= 1)
    return;
  
  streams = malloc(num_streams * sizeof(*streams));
  index = 0;
  
  for(i = 0; i < ctx->tt->cur->num_audio_streams; i++)
    streams[index++] = &(ctx->tt->cur->audio_streams[i]);

  for(i = 0; i < ctx->tt->cur->num_video_streams; i++)
    streams[index++] = &(ctx->tt->cur->video_streams[i]);
  
  for(i = 0; i < ctx->tt->cur->num_subtitle_streams; i++)
    streams[index++] = &(ctx->tt->cur->subtitle_streams[i]);
  
  if((streams[0]->last_index_position < streams[1]->first_index_position) ||
     (streams[1]->last_index_position < streams[0]->first_index_position))
    {
    ctx->flags |= BGAV_DEMUXER_SI_NON_INTERLEAVED;

    /* Adjust index positions for the streams */
    
    for(i = 0; i < num_streams; i++)
      streams[i]->index_position = streams[i]->first_index_position;
    }
  free(streams);
  }

int bgav_demuxer_start(bgav_demuxer_context_t * ctx,
                       bgav_redirector_context_t ** redir)
  {
  /* eof flag might be present from last track */
  ctx->flags &= ~BGAV_DEMUXER_EOF;
  
  if(!ctx->demuxer->open(ctx, redir))
    return 0;
  
  if(ctx->si)
    {
#ifdef DUMP_SUPERINDEX    
    bgav_superindex_dump(ctx->si);
#endif

    if(!(ctx->flags & BGAV_DEMUXER_SI_PRIVATE_FUNCS))
      {
      check_missing_streams(ctx);
      check_interleave(ctx);

      if(ctx->flags & BGAV_DEMUXER_SI_NON_INTERLEAVED)
        {
        if(!ctx->input->input->seek_byte)
          {
          bgav_log(ctx->opt, BGAV_LOG_ERROR, LOG_DOMAIN,
                   "Non interleaved file from non seekable source");
          return 0;
          }
        }
      }
    }
 
  return 1;
  }

void bgav_demuxer_stop(bgav_demuxer_context_t * ctx)
  {
  ctx->demuxer->close(ctx);
  ctx->priv = (void*)0;
  FREE(ctx->stream_description);
  
  /* Reset global variables */
  ctx->flags &= ~(BGAV_DEMUXER_SI_SEEKING |
                  BGAV_DEMUXER_HAS_TIMESTAMP_OFFSET |
                  BGAV_DEMUXER_EOF);
  
  ctx->timestamp_offset = 0;
  if(ctx->si)
    {
    bgav_superindex_destroy(ctx->si);
    ctx->si = (bgav_superindex_t*)0;
    }
  
  }

static int next_packet_interleaved(bgav_demuxer_context_t * ctx)
  {
  bgav_stream_t * stream;
  bgav_packet_t * p;

  
  if(ctx->si->current_position >= ctx->si->num_entries)
    {
    return 0;
    }
  
  if(ctx->input->position >=
     ctx->si->entries[ctx->si->num_entries - 1].offset + 
     ctx->si->entries[ctx->si->num_entries - 1].size)
    {
    return 0;
    }
  stream =
    bgav_track_find_stream(ctx->tt->cur,
                           ctx->si->entries[ctx->si->current_position].stream_id);
  
  if(!stream) /* Skip unused stream */
    {
    bgav_input_skip(ctx->input,
                    ctx->si->entries[ctx->si->current_position].size);

    //    bgav_input_skip_dump(ctx->input,
    //                         ctx->si->entries[ctx->si->current_position].size);
    
    ctx->si->current_position++;
    return 1;
    }
  
  if((ctx->flags & BGAV_DEMUXER_SI_SEEKING) && (stream->index_position > ctx->si->current_position))
    {
    bgav_input_skip(ctx->input,
                    ctx->si->entries[ctx->si->current_position].size);
    ctx->si->current_position++;
    return 1;
    }

  /* Skip until this packet */

  if(ctx->si->entries[ctx->si->current_position].offset > ctx->input->position)
    {
    bgav_input_skip(ctx->input,
                    ctx->si->entries[ctx->si->current_position].offset - ctx->input->position);
    }

  
  p = bgav_stream_get_packet_write(stream);
  bgav_packet_alloc(p, ctx->si->entries[ctx->si->current_position].size);
  p->data_size = ctx->si->entries[ctx->si->current_position].size;
  p->keyframe = ctx->si->entries[ctx->si->current_position].keyframe;
  
  p->pts = ctx->si->entries[ctx->si->current_position].time;
  p->duration = ctx->si->entries[ctx->si->current_position].duration;
  if(bgav_input_read_data(ctx->input, p->data, p->data_size) < p->data_size)
    return 0;
  
  if(stream->process_packet)
    stream->process_packet(stream, p);
  
  bgav_packet_done_write(p);
  
  ctx->si->current_position++;
  return 1;
  }

static int next_packet_noninterleaved(bgav_demuxer_context_t * ctx)
  {
  int i;
  bgav_packet_t * p;
  bgav_stream_t * s = (bgav_stream_t*)0;
  bgav_stream_t * ts;
  
  if(ctx->request_stream->index_position >= ctx->request_stream->last_index_position)
    {
    /* Requested stream is finished, read data from another stream
       before returning 0 */

    for(i = 0; i < ctx->tt->cur->num_audio_streams; i++)
      {
      ts = &ctx->tt->cur->audio_streams[i];
      if((ts->action != BGAV_STREAM_MUTE) &&
         (ts->index_position < ts->last_index_position))
        {
        s = ts;
        break;
        }
      }

    if(!s)
      {
      for(i = 0; i < ctx->tt->cur->num_video_streams; i++)
        {
        ts = &ctx->tt->cur->video_streams[i];
        if((ts->action != BGAV_STREAM_MUTE) &&
           (ts->index_position < ts->last_index_position))
          {
          s = ts;
          break;
          }
        }
      }

    if(!s)
      {
      for(i = 0; i < ctx->tt->cur->num_subtitle_streams; i++)
        {
        ts = &ctx->tt->cur->subtitle_streams[i];
        if((ts->action != BGAV_STREAM_MUTE) &&
           (ts->index_position < ts->last_index_position))
          {
          s = ts;
          break;
          }
        }
      }

    if(!s)
      return 0;
    }
  else
    s = ctx->request_stream;
  
  /* If the file is truely noninterleaved, this isn't neccessary, but who knows? */
  while(ctx->si->entries[s->index_position].stream_id !=
        s->stream_id)
    {
    s->index_position++;
    }
  
  bgav_input_seek(ctx->input,
                  ctx->si->entries[s->index_position].offset,
                  SEEK_SET);

  p = bgav_stream_get_packet_write(s);
  p->data_size = ctx->si->entries[s->index_position].size;
  bgav_packet_alloc(p, p->data_size);
  
  p->pts = ctx->si->entries[s->index_position].time;
  p->duration = ctx->si->entries[s->index_position].duration;
  p->keyframe = ctx->si->entries[s->index_position].keyframe;
    
  if(bgav_input_read_data(ctx->input, p->data, p->data_size) < p->data_size)
    return 0;

  if(s->process_packet)
    s->process_packet(s, p);
  
  bgav_packet_done_write(p);
  
  s->index_position++;
  return 1;
  
  }

static int demuxer_next_packet(bgav_demuxer_context_t * demuxer)
  {
  int ret, i;

  if(demuxer->flags & BGAV_DEMUXER_EOF)
    return 0;
  
  if(demuxer->si && !(demuxer->flags & BGAV_DEMUXER_SI_PRIVATE_FUNCS)) /* Superindex present */
    {
    if(demuxer->flags & BGAV_DEMUXER_SI_NON_INTERLEAVED)
      ret = next_packet_noninterleaved(demuxer);
    else
      ret = next_packet_interleaved(demuxer);
    }
  else
    {
    ret = demuxer->demuxer->next_packet(demuxer);

    if(!ret)
      {
      /* Some demuxers have packets stored in the streams,
         we flush them here */
 
      for(i = 0; i < demuxer->tt->cur->num_audio_streams; i++)
        {
        if(demuxer->tt->cur->audio_streams[i].packet)
          {
          bgav_packet_done_write(demuxer->tt->cur->audio_streams[i].packet);
          demuxer->tt->cur->audio_streams[i].packet = (bgav_packet_t*)0;
          ret = 1;
          }
        }
      for(i = 0; i < demuxer->tt->cur->num_video_streams; i++)
        {
        if(demuxer->tt->cur->video_streams[i].packet)
          {
          bgav_packet_done_write(demuxer->tt->cur->video_streams[i].packet);
          demuxer->tt->cur->video_streams[i].packet = (bgav_packet_t*)0;
          ret = 1;
          }
        }
      }
    }
  if(!ret)
    demuxer->flags |= BGAV_DEMUXER_EOF;
  return ret;
  }

bgav_packet_t *
bgav_demuxer_get_packet_read(bgav_demuxer_context_t * demuxer,
                             bgav_stream_t * s)
  {
  bgav_packet_t * ret = (bgav_packet_t*)0;
  if(!s->packet_buffer)
    return (bgav_packet_t*)0;
  demuxer->request_stream = s; 
  while(!(ret = bgav_packet_buffer_get_packet_read(s->packet_buffer, 0)))
    {
    if(!demuxer_next_packet(demuxer))
      return (bgav_packet_t*)0;
    }
  s->time_scaled = ret->pts;

  if(!s->has_first_timestamp)
    {
    if(ret->pts != BGAV_TIMESTAMP_UNDEFINED)
      s->first_timestamp = ret->pts;
    s->has_first_timestamp = 1;
    }
  
  demuxer->request_stream = (bgav_stream_t*)0;
  return ret;
  }

bgav_packet_t *
bgav_demuxer_peek_packet_read(bgav_demuxer_context_t * demuxer,
                              bgav_stream_t * s, int force)
  {
  bgav_packet_t * ret;
  if(!s->packet_buffer)
    return 0;
  
  if((demuxer->flags & BGAV_DEMUXER_PEEK_FORCES_READ) || force)
    {
    demuxer->request_stream = s; 
    while(!(ret = bgav_packet_buffer_peek_packet_read(s->packet_buffer, s->vfr_timestamps)))
      {
      if(!demuxer_next_packet(demuxer))
        return 0;
      }
    demuxer->request_stream = (bgav_stream_t*)0;
    return ret;
    }
  else
    return bgav_packet_buffer_peek_packet_read(s->packet_buffer, s->vfr_timestamps);
  }


void 
bgav_demuxer_done_packet_read(bgav_demuxer_context_t * demuxer,
                              bgav_packet_t * p)
  {

  p->valid = 0;

  if(p->stream->type == BGAV_STREAM_VIDEO)
    {
    p->stream->data.video.last_frame_time =
      gavl_time_rescale(p->stream->timescale,
                        p->stream->data.video.format.timescale,
                        p->pts);

    demuxer->request_stream = p->stream;
    
    while(!p->stream->packet_buffer->read_packet->valid)
      {
      if(!demuxer_next_packet(demuxer))
        {
        p->stream->data.video.last_frame_duration =
          p->stream->data.video.format.frame_duration;
        return;
        }
      }
    demuxer->request_stream = (bgav_stream_t*)0;

    p->stream->data.video.last_frame_duration =
      gavl_time_rescale(p->stream->timescale,
                        p->stream->data.video.format.timescale,
                        p->pts) -
      p->stream->data.video.last_frame_time;
    }
  }

/* Seek functions with superindex */

static void seek_si(bgav_demuxer_context_t * ctx, gavl_time_t time)
  {
  uint32_t i, j;
  int32_t start_packet;
  int32_t end_packet;
  bgav_track_t * track;
    
  track = ctx->tt->cur;
  
  /* Set the packet indices of the streams to -1 */
  for(j = 0; j < track->num_audio_streams; j++)
    {
    track->audio_streams[j].index_position = -1;
    }
  for(j = 0; j < track->num_video_streams; j++)
    {
    track->video_streams[j].index_position = -1;
    }

  /* Seek the start chunks indices of all streams */
  
  for(j = 0; j < track->num_audio_streams; j++)
    {
    bgav_superindex_seek(ctx->si, &(track->audio_streams[j]), time);
    }
  for(j = 0; j < track->num_video_streams; j++)
    {
    bgav_superindex_seek(ctx->si, &(track->video_streams[j]), time);
    }
  for(j = 0; j < track->num_subtitle_streams; j++)
    {
    if(!track->subtitle_streams[j].data.subtitle.subreader)
      bgav_superindex_seek(ctx->si, &(track->subtitle_streams[j]), time);
    }
  
  /* Find the start and end packet */

  if(!(ctx->flags & BGAV_DEMUXER_SI_NON_INTERLEAVED))
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
      next_packet_interleaved(ctx);

    ctx->flags &= ~BGAV_DEMUXER_SI_SEEKING;
    }

  }

/* Maximum allowed seek tolerance, decrease if you want it more exact */

// #define SEEK_TOLERANCE (GAVL_TIME_SCALE/2)

#define SEEK_TOLERANCE 0
/* We skip at most this time */

#define MAX_SKIP_TIME  (GAVL_TIME_SCALE*5)

static void skip_to(bgav_t * b, bgav_track_t * track, gavl_time_t * time)
  {
  if(!bgav_track_skipto(track, time))
    b->eof = 1;
  }

void
bgav_seek(bgav_t * b, gavl_time_t * time)
  {
  int i;
  gavl_time_t diff_time;
  gavl_time_t sync_time;
  gavl_time_t seek_time;

  gavl_time_t last_seek_time = GAVL_TIME_UNDEFINED;
  gavl_time_t last_seek_time_2nd = GAVL_TIME_UNDEFINED;

  gavl_time_t last_sync_time = GAVL_TIME_UNDEFINED;
  gavl_time_t last_sync_time_2nd = GAVL_TIME_UNDEFINED;
    
  bgav_track_t * track = b->tt->cur;
  int num_iterations = 0;
  seek_time = *time;

  b->demuxer->flags &= ~BGAV_DEMUXER_EOF;
  
  while(1)
    {
    num_iterations++;
    
    /* First step: Flush all fifos */
    bgav_track_clear(track);
    
    /* Second step: Let the demuxer seek */
    /* This will set the "time" members of all streams */
    
    if(b->demuxer->si && !(b->demuxer->flags & BGAV_DEMUXER_SI_PRIVATE_FUNCS))
      seek_si(b->demuxer, seek_time);
    else
      b->demuxer->demuxer->seek(b->demuxer, seek_time);
    sync_time = bgav_track_resync_decoders(track);
    if(sync_time == GAVL_TIME_UNDEFINED)
      {
      bgav_log(&b->opt, BGAV_LOG_WARNING, LOG_DOMAIN,
               "Undefined sync time after seeking");
      return;
      }
    
    /* If demuxer already seeked perfectly, break here */

    if(!(b->demuxer->flags & BGAV_DEMUXER_SEEK_ITERATIVE))
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
        }
      
      }

    last_seek_time_2nd = last_seek_time;
    last_sync_time_2nd = last_sync_time;

    last_seek_time = seek_time;
    last_sync_time = sync_time;
    
    
    diff_time = *time - sync_time;

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

  /* Let the subtitle readers seek */
  for(i = 0; i < track->num_subtitle_streams; i++)
    {
    if(track->subtitle_streams[i].data.subtitle.subreader &&
       track->subtitle_streams[i].action != BGAV_STREAM_MUTE)
      {
      /* Clear EOF state */
      track->subtitle_streams[i].data.subtitle.eof = 0;
      bgav_subtitle_reader_seek(&track->subtitle_streams[i],
                                *time);
      }
    }

  }

void bgav_formats_dump()
  {
  int i;
  bgav_dprintf("<h2>Formats</h2>\n<ul>");
  for(i = 0; i < num_demuxers; i++)
    bgav_dprintf("<li>%s\n", demuxers[i].format_name);
  for(i = 0; i < num_sync_demuxers; i++)
    bgav_dprintf("<li>%s\n", sync_demuxers[i].format_name);
  bgav_dprintf("</ul>\n");
  }
