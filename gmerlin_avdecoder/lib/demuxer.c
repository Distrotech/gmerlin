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

#define DUMP_SUPERINDEX    
#include <avdec_private.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define LOG_DOMAIN "demuxer"

extern const bgav_demuxer_t bgav_demuxer_asf;
extern const bgav_demuxer_t bgav_demuxer_avi;
extern const bgav_demuxer_t bgav_demuxer_rmff;
extern const bgav_demuxer_t bgav_demuxer_quicktime;
extern const bgav_demuxer_t bgav_demuxer_vivo;
extern const bgav_demuxer_t bgav_demuxer_fli;
extern const bgav_demuxer_t bgav_demuxer_flv;

extern const bgav_demuxer_t bgav_demuxer_wavpack;
extern const bgav_demuxer_t bgav_demuxer_tta;
extern const bgav_demuxer_t bgav_demuxer_voc;
extern const bgav_demuxer_t bgav_demuxer_wav;
extern const bgav_demuxer_t bgav_demuxer_au;
extern const bgav_demuxer_t bgav_demuxer_ircam;
extern const bgav_demuxer_t bgav_demuxer_sphere;
extern const bgav_demuxer_t bgav_demuxer_gsm;
extern const bgav_demuxer_t bgav_demuxer_8svx;
extern const bgav_demuxer_t bgav_demuxer_aiff;
extern const bgav_demuxer_t bgav_demuxer_ra;
extern const bgav_demuxer_t bgav_demuxer_mpegaudio;
extern const bgav_demuxer_t bgav_demuxer_mpegvideo;
extern const bgav_demuxer_t bgav_demuxer_mpegps;
extern const bgav_demuxer_t bgav_demuxer_mpegts;
extern const bgav_demuxer_t bgav_demuxer_mxf;
extern const bgav_demuxer_t bgav_demuxer_flac;
extern const bgav_demuxer_t bgav_demuxer_aac;
extern const bgav_demuxer_t bgav_demuxer_nsv;
extern const bgav_demuxer_t bgav_demuxer_4xm;
extern const bgav_demuxer_t bgav_demuxer_dsicin;
extern const bgav_demuxer_t bgav_demuxer_smaf;
extern const bgav_demuxer_t bgav_demuxer_psxstr;
extern const bgav_demuxer_t bgav_demuxer_tiertex;
extern const bgav_demuxer_t bgav_demuxer_smacker;
extern const bgav_demuxer_t bgav_demuxer_roq;
extern const bgav_demuxer_t bgav_demuxer_shorten;
extern const bgav_demuxer_t bgav_demuxer_daud;
extern const bgav_demuxer_t bgav_demuxer_nuv;
extern const bgav_demuxer_t bgav_demuxer_sol;
extern const bgav_demuxer_t bgav_demuxer_gif;
extern const bgav_demuxer_t bgav_demuxer_smjpeg;
extern const bgav_demuxer_t bgav_demuxer_vqa;
extern const bgav_demuxer_t bgav_demuxer_vmd;
extern const bgav_demuxer_t bgav_demuxer_avs;
extern const bgav_demuxer_t bgav_demuxer_wve;
extern const bgav_demuxer_t bgav_demuxer_mtv;
extern const bgav_demuxer_t bgav_demuxer_gxf;
extern const bgav_demuxer_t bgav_demuxer_dxa;
extern const bgav_demuxer_t bgav_demuxer_thp;
extern const bgav_demuxer_t bgav_demuxer_r3d;

#ifdef HAVE_VORBIS
extern const bgav_demuxer_t bgav_demuxer_ogg;
#endif

extern const bgav_demuxer_t bgav_demuxer_dv;

#ifdef HAVE_LIBA52
extern const bgav_demuxer_t bgav_demuxer_a52;
#endif

#ifdef HAVE_MUSEPACK
extern const bgav_demuxer_t bgav_demuxer_mpc;
#endif

#ifdef HAVE_MJPEGTOOLS
extern const bgav_demuxer_t bgav_demuxer_y4m;
#endif

#ifdef HAVE_LIBAVFORMAT
extern const bgav_demuxer_t bgav_demuxer_ffmpeg;
#endif

extern const bgav_demuxer_t bgav_demuxer_p2xml;

typedef struct
  {
  const bgav_demuxer_t * demuxer;
  char * format_name;
  } demuxer_t;

static const demuxer_t demuxers[] =
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
    { &bgav_demuxer_r3d,       "R3D" },
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
    { &bgav_demuxer_mxf, "MXF" },
    { &bgav_demuxer_sphere, "nist Sphere"},
    { &bgav_demuxer_ircam, "IRCAM" },
    { &bgav_demuxer_gsm, "raw gsm" },
    { &bgav_demuxer_daud, "D-Cinema audio" },
    { &bgav_demuxer_vmd,  "Sierra VMD" },
  };

static const demuxer_t sync_demuxers[] =
  {
    { &bgav_demuxer_mpegts,    "MPEG-2 transport stream" },
    { &bgav_demuxer_mpegaudio, "Mpeg Audio" },
    { &bgav_demuxer_mpegps, "Mpeg System" },
  };

static const demuxer_t yml_demuxers[] =
  {
    { &bgav_demuxer_p2xml,    "P2 xml" },
  };

static struct
  {
  const bgav_demuxer_t * demuxer;
  char * mimetype;
  }
mimetypes[] =
  {
    { &bgav_demuxer_mpegaudio, "audio/mpeg" }
  };

static const int num_demuxers = sizeof(demuxers)/sizeof(demuxers[0]);
static const int num_sync_demuxers = sizeof(sync_demuxers)/sizeof(sync_demuxers[0]);
static const int num_yml_demuxers = sizeof(yml_demuxers)/sizeof(yml_demuxers[0]);

static const int num_mimetypes = sizeof(mimetypes)/sizeof(mimetypes[0]);

int bgav_demuxer_next_packet(bgav_demuxer_context_t * demuxer);


#define SYNC_BYTES (32*1024)

const bgav_demuxer_t * bgav_demuxer_probe(bgav_input_context_t * input,
                                          bgav_yml_node_t ** yml)
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

  /* Check if this is an xml like file */
  if(bgav_yml_probe(input))
    {
    *yml = bgav_yml_parse(input);
    if(!(*yml))
      return (bgav_demuxer_t *)0;

    for(i = 0; i < num_yml_demuxers; i++)
      {
      if(yml_demuxers[i].demuxer->probe_yml(*yml))
        {
        bgav_log(input->opt, BGAV_LOG_INFO, LOG_DOMAIN,
                 "Detected %s format",
                 yml_demuxers[i].format_name);
        return yml_demuxers[i].demuxer;
        }
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
bgav_demuxer_create(const bgav_options_t * opt, const bgav_demuxer_t * demuxer,
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
  if(ctx->demuxer->close)
    ctx->demuxer->close(ctx);
  if(ctx->tt)
    bgav_track_table_unref(ctx->tt);

  if(ctx->si)
    bgav_superindex_destroy(ctx->si);
  if(ctx->edl)
    bgav_edl_destroy(ctx->edl);

  if(ctx->redirector)
    bgav_redirector_destroy(ctx->redirector);
  
  
  FREE(ctx->stream_description);
  free(ctx);
  }

/* Check and kick out streams, for which a header exists but no
   packets */

static void init_superindex(bgav_demuxer_context_t * ctx)
  {
  int i;

  i = 0;
  while(i < ctx->tt->cur->num_audio_streams)
    {
    if(ctx->tt->cur->audio_streams[i].last_index_position < 0)
      bgav_track_remove_audio_stream(ctx->tt->cur, i);
    else
      {
      bgav_superindex_set_durations(ctx->si, &ctx->tt->cur->audio_streams[i]);
      ctx->tt->cur->audio_streams[i].first_timestamp = 0;
      i++;
      }
    }

  i = 0;
  while(i < ctx->tt->cur->num_video_streams)
    {
    if(ctx->tt->cur->video_streams[i].last_index_position < 0)
      bgav_track_remove_video_stream(ctx->tt->cur, i);
    else
      {
      bgav_superindex_set_durations(ctx->si, &ctx->tt->cur->video_streams[i]);
      ctx->tt->cur->video_streams[i].first_timestamp = 0;
      i++;
      }
    }

  i = 0;
  while(i < ctx->tt->cur->num_subtitle_streams)
    {
    if(ctx->tt->cur->subtitle_streams[i].last_index_position < 0)
      bgav_track_remove_subtitle_stream(ctx->tt->cur, i);
    else
      {
      bgav_superindex_set_durations(ctx->si, &ctx->tt->cur->subtitle_streams[i]);
      ctx->tt->cur->subtitle_streams[i].first_timestamp =
        ctx->si->entries[ctx->tt->cur->subtitle_streams[i].first_index_position].time;
      i++;
      }
    }
  if(ctx->tt->cur->duration == GAVL_TIME_UNDEFINED)
    bgav_track_calc_duration(ctx->tt->cur);
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

  streams = malloc(num_streams * sizeof(*streams));

  index = 0;
  
  for(i = 0; i < ctx->tt->cur->num_audio_streams; i++)
    streams[index++] = &(ctx->tt->cur->audio_streams[i]);

  for(i = 0; i < ctx->tt->cur->num_video_streams; i++)
    streams[index++] = &(ctx->tt->cur->video_streams[i]);
  
  for(i = 0; i < ctx->tt->cur->num_subtitle_streams; i++)
    streams[index++] = &(ctx->tt->cur->subtitle_streams[i]);
  
  /* If sample accurate decoding was requested, use non-interleaved mode */
  if(ctx->opt->sample_accurate || (ctx->flags & BGAV_DEMUXER_BUILD_INDEX))
    {
    ctx->demux_mode = DEMUX_MODE_SI_NI;
    }
  /* One stream always means non-interleaved */
  else if(num_streams <= 1)
    {
    ctx->demux_mode = DEMUX_MODE_SI_I;
    }
  else
    {
    ctx->demux_mode = DEMUX_MODE_SI_I;
    
    if((streams[0]->last_index_position < streams[1]->first_index_position) ||
       (streams[1]->last_index_position < streams[0]->first_index_position))
      {
      ctx->demux_mode = DEMUX_MODE_SI_NI;
      }
    }
  
  /* Adjust index positions for the streams */
  for(i = 0; i < num_streams; i++)
    streams[i]->index_position = streams[i]->first_index_position;
  free(streams);
  }

int bgav_demuxer_start(bgav_demuxer_context_t * ctx,
                       bgav_yml_node_t * yml)
  {
  /* eof flag might be present from last track */
  ctx->flags &= ~BGAV_DEMUXER_EOF;

  if(yml)
    {
    if(!ctx->demuxer->open_yml(ctx, yml))
      return 0;
    }
  else if(!ctx->demuxer->open(ctx))
    return 0;
  
  if(ctx->si)
    {
    if(!(ctx->flags & BGAV_DEMUXER_SI_PRIVATE_FUNCS))
      {
      init_superindex(ctx);
      check_interleave(ctx);

      if((ctx->demux_mode == DEMUX_MODE_SI_NI) &&
         !ctx->input->input->seek_byte)
        {
        bgav_log(ctx->opt, BGAV_LOG_ERROR, LOG_DOMAIN,
                 "Non interleaved file from non seekable source");
        return 0;
        }
      }
#ifdef DUMP_SUPERINDEX    
    bgav_superindex_dump(ctx->si);
#endif
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
    bgav_track_find_stream(ctx,
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
  p->position = ctx->si->current_position;
  
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
  
  if(ctx->request_stream->index_position > ctx->request_stream->last_index_position)
    {
    if(ctx->tt->cur->sample_accurate)
      return 0;
    
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
  p->position = s->index_position;
  
  if(bgav_input_read_data(ctx->input, p->data, p->data_size) < p->data_size)
    return 0;

  if(s->process_packet)
    s->process_packet(s, p);
  
  bgav_packet_done_write(p);
  
  s->index_position++;
  return 1;
  
  }

int bgav_demuxer_next_packet(bgav_demuxer_context_t * demuxer)
  {
  int ret = 0, i;

  if(demuxer->flags & BGAV_DEMUXER_EOF)
    return 0;

  switch(demuxer->demux_mode)
    {
    case DEMUX_MODE_SI_I:
      ret = next_packet_interleaved(demuxer);
      if(!ret)
        demuxer->flags |= BGAV_DEMUXER_EOF;
      break;
    case DEMUX_MODE_SI_NI:
      ret = next_packet_noninterleaved(demuxer);
      if(!ret)
        demuxer->flags |= BGAV_DEMUXER_EOF;
      break;
    case DEMUX_MODE_FI:
      ret = bgav_demuxer_next_packet_fileindex(demuxer);
      break;
    case DEMUX_MODE_STREAM:
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
        demuxer->flags |= BGAV_DEMUXER_EOF;
        }
      break;
    }
  
  return ret;
  }

bgav_packet_t *
bgav_demuxer_get_packet_read(bgav_demuxer_context_t * demuxer,
                             bgav_stream_t * s)
  {
  int get_duration = 0;
  bgav_packet_t * ret = (bgav_packet_t*)0;
  
  if(!s->packet_buffer)
    return (bgav_packet_t*)0;
  if((s->type == BGAV_STREAM_VIDEO) &&
     ((s->data.video.frametime_mode == BGAV_FRAMETIME_PTS) ||
      (s->data.video.frametime_mode == BGAV_FRAMETIME_CODEC_PTS)))
    get_duration = 1;

  
  demuxer->request_stream = s; 
  while(!(ret = bgav_packet_buffer_get_packet_read(s->packet_buffer, get_duration)))
    {
    if(!bgav_demuxer_next_packet(demuxer))
      {
      if(get_duration)
        {
        ret = bgav_packet_buffer_get_packet_read(s->packet_buffer, 0);
        if(!ret)
          return (bgav_packet_t*)0;

        if(s->duration)
          ret->duration = s->duration - ret->pts;
        else
          ret->duration = 0;
        break;
        }
      else
        return (bgav_packet_t*)0;
      }
    }
  
  s->in_time = ret->pts;

  if(s->first_timestamp == BGAV_TIMESTAMP_UNDEFINED)
    {
    if(ret->pts != BGAV_TIMESTAMP_UNDEFINED)
      s->first_timestamp = ret->pts;
    }
  
  demuxer->request_stream = (bgav_stream_t*)0;
#if 0
  if(s->type == BGAV_STREAM_VIDEO)
    fprintf(stderr, "Got packet %ld %d\n",
            ret->position, ret->data_size);
#endif
  return ret;
  }

bgav_packet_t *
bgav_demuxer_peek_packet_read(bgav_demuxer_context_t * demuxer,
                              bgav_stream_t * s, int force)
  {
  bgav_packet_t * ret;
  int get_duration = 0;
  if(!s->packet_buffer)
    return 0;
  if((s->type == BGAV_STREAM_VIDEO) &&
     ((s->data.video.frametime_mode == BGAV_FRAMETIME_PTS) ||
      (s->data.video.frametime_mode == BGAV_FRAMETIME_CODEC_PTS)))
    get_duration = 1;
  
  if((demuxer->flags & BGAV_DEMUXER_PEEK_FORCES_READ) || force)
    {
    demuxer->request_stream = s; 
    while(!(ret = bgav_packet_buffer_peek_packet_read(s->packet_buffer,
                                                      get_duration)))
      {
      if(!bgav_demuxer_next_packet(demuxer))
        {
        if(get_duration)
          {
          ret = bgav_packet_buffer_peek_packet_read(s->packet_buffer, 0);
          if(!ret)
            return (bgav_packet_t*)0;

          if(s->duration)
            ret->duration = s->duration - ret->pts;
          else
            ret->duration = 0;
          break;
          }
        else
          return (bgav_packet_t*)0;
        }
      }
    demuxer->request_stream = (bgav_stream_t*)0;
    return ret;
    }
  else
    return bgav_packet_buffer_peek_packet_read(s->packet_buffer, get_duration);
  }


void 
bgav_demuxer_done_packet_read(bgav_demuxer_context_t * demuxer,
                              bgav_packet_t * p)
  {
  p->valid = 0;
#if 0
  if((p->stream->type == BGAV_STREAM_VIDEO) &&
     !(demuxer->flags & BGAV_DEMUXER_BUILD_INDEX))
    {
    p->stream->data.video.last_frame_time =
      gavl_time_rescale(p->stream->timescale,
                        p->stream->data.video.format.timescale,
                        p->pts);

    demuxer->request_stream = p->stream;
    
    while(!p->stream->packet_buffer->read_packet->valid)
      {
      if(!bgav_demuxer_next_packet(demuxer))
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
#endif
  }

/* Seek functions with superindex */

static void seek_si(bgav_demuxer_context_t * ctx, int64_t time, int scale)
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
    bgav_superindex_seek(ctx->si, &(track->audio_streams[j]), time, scale);
    }
  for(j = 0; j < track->num_video_streams; j++)
    {
    bgav_superindex_seek(ctx->si, &(track->video_streams[j]), time, scale);
    }
  for(j = 0; j < track->num_subtitle_streams; j++)
    {
    if(!track->subtitle_streams[j].data.subtitle.subreader)
      bgav_superindex_seek(ctx->si, &(track->subtitle_streams[j]), time, scale);
    }
  
  /* Find the start and end packet */

  if(ctx->demux_mode == DEMUX_MODE_SI_I)
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

static void skip_to(bgav_t * b, bgav_track_t * track, int64_t * time, int scale)
  {
  if(!bgav_track_skipto(track, time, scale))
    b->eof = 1;
  }

void
bgav_seek_scaled(bgav_t * b, int64_t * time, int scale)
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

  if(b->tt->cur->sample_accurate)
    {
    bgav_stream_t * s;
    for(i = 0; i < b->tt->cur->num_video_streams; i++)
      {
      if(b->tt->cur->video_streams[i].action != BGAV_STREAM_MUTE)
        {
        s = &b->tt->cur->video_streams[i];
        bgav_seek_video(b, i,
                        gavl_time_rescale(scale, s->data.video.format.timescale,
                                          *time) - s->first_timestamp);
        /*
         *  We align seeking at the first frame of the last video stream
         *  Note, that in 99.9 % of all cases, the last stream will be the
         *  first one as well
         */
        
        *time =
          gavl_time_rescale(s->data.video.format.timescale,
                            scale,
                            s->out_time + s->first_timestamp);
        }
      }
    
    for(i = 0; i < b->tt->cur->num_audio_streams; i++)
      {
      if(b->tt->cur->audio_streams[i].action != BGAV_STREAM_MUTE)
        {
        s = &b->tt->cur->audio_streams[i];
        bgav_seek_audio(b, i,
                        gavl_time_rescale(scale, s->data.audio.format.samplerate,
                                          *time) - s->first_timestamp);
        }
      }
    for(i = 0; i < b->tt->cur->num_subtitle_streams; i++)
      {
      if(b->tt->cur->subtitle_streams[i].action != BGAV_STREAM_MUTE)
        {
        s = &b->tt->cur->subtitle_streams[i];
        bgav_seek_subtitle(b, i, gavl_time_rescale(scale, s->timescale, *time));
        }
      }
    return;
    }
  
  while(1)
    {
    num_iterations++;
    
    /* First step: Flush all fifos */
    bgav_track_clear(track);
    
    /* Second step: Let the demuxer seek */
    /* This will set the "time" members of all streams */
    
    if(b->demuxer->si && !(b->demuxer->flags & BGAV_DEMUXER_SI_PRIVATE_FUNCS))
      seek_si(b->demuxer, seek_time, scale);
    else
      b->demuxer->demuxer->seek(b->demuxer, seek_time, scale);
    sync_time = bgav_track_resync_decoders(track, scale);
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
        skip_to(b, track, time, scale);
      else
        {
        skip_to(b, track, &sync_time, scale);
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
              seek_si(b->demuxer, seek_time, scale);
            else
              b->demuxer->demuxer->seek(b->demuxer, seek_time, scale);
            sync_time = bgav_track_resync_decoders(track, scale);
            }
          }
        else /* Go to 2nd last sync time */
          {
          if(last_sync_time != sync_time)
            {
            b->demuxer->demuxer->seek(b->demuxer, seek_time, scale);
            sync_time = bgav_track_resync_decoders(track, scale);
            }
          }

        
        if(*time > sync_time)
          skip_to(b, track, time, scale);
        else
          {
          skip_to(b, track, &sync_time, scale);
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
      skip_to(b, track, time, scale);
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
      skip_to(b, track, &sync_time, scale);
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
      track->subtitle_streams[i].eof = 0;
      bgav_subtitle_reader_seek(&track->subtitle_streams[i],
                                *time, scale);
      }
    }

  }

void
bgav_seek(bgav_t * b, gavl_time_t * time)
  {
  bgav_seek_scaled(b, time, GAVL_TIME_SCALE);
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
