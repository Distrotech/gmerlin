/*****************************************************************
 * gmerlin-avdecoder - a general purpose multimedia decoding library
 *
 * Copyright (c) 2001 - 2012 Members of the Gmerlin project
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

// #define DUMP_SUPERINDEX
#include <avdec_private.h>
#include <parser.h>

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

extern const bgav_demuxer_t bgav_demuxer_ape;
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
extern const bgav_demuxer_t bgav_demuxer_adts;
extern const bgav_demuxer_t bgav_demuxer_adif;
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
extern const bgav_demuxer_t bgav_demuxer_matroska;
extern const bgav_demuxer_t bgav_demuxer_gavf;
extern const bgav_demuxer_t bgav_demuxer_y4m;

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
    { &bgav_demuxer_asf,       "ASF/WMV/WMA" },
    { &bgav_demuxer_adif,      "ADIF" },
    { &bgav_demuxer_avi,       "AVI" },
    { &bgav_demuxer_rmff,      "Real Media" },
    { &bgav_demuxer_ra,        "Real Audio" },
    { &bgav_demuxer_quicktime, "Quicktime/mp4/m4a" },
    { &bgav_demuxer_ape,       "APE"               },
    { &bgav_demuxer_wav,       "WAV" },
    { &bgav_demuxer_au,        "Sun AU" },
    { &bgav_demuxer_aiff,      "AIFF(C)" },
    { &bgav_demuxer_flac,      "FLAC" },
    { &bgav_demuxer_vivo,      "Vivo" },
    { &bgav_demuxer_mpegvideo, "Elementary video" },
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
    { &bgav_demuxer_matroska,  "Matroska" },
#ifdef HAVE_VORBIS
    { &bgav_demuxer_ogg, "Ogg Bitstream" },
#endif
#ifdef HAVE_LIBA52
    { &bgav_demuxer_a52, "A52 Bitstream" },
#endif
#ifdef HAVE_MUSEPACK
    { &bgav_demuxer_mpc, "Musepack" },
#endif
    { &bgav_demuxer_y4m, "yuv4mpeg" },
    { &bgav_demuxer_dv, "DV" },
    { &bgav_demuxer_mxf, "MXF" },
    { &bgav_demuxer_sphere, "nist Sphere"},
    { &bgav_demuxer_ircam, "IRCAM" },
    { &bgav_demuxer_gsm, "raw gsm" },
    { &bgav_demuxer_daud, "D-Cinema audio" },
    { &bgav_demuxer_vmd,  "Sierra VMD" },
    { &bgav_demuxer_gavf,  "GAVF" },
  };

static const demuxer_t sync_demuxers[] =
  {
    { &bgav_demuxer_mpegts,    "MPEG-2 transport stream" },
    { &bgav_demuxer_mpegaudio, "MPEG Audio" },
    { &bgav_demuxer_adts,      "ADTS" },
    { &bgav_demuxer_mpegps,    "MPEG System" },
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

// int bgav_demuxer_next_packet(bgav_demuxer_context_t * demuxer);


#define SYNC_BYTES (32*1024)

const bgav_demuxer_t * bgav_demuxer_probe(bgav_input_context_t * input,
                                          bgav_yml_node_t * yml)
  {
  int i;
  int bytes_skipped;
  uint8_t skip;

  if(!yml)
    {
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
        bgav_log(input->opt, BGAV_LOG_DEBUG, LOG_DOMAIN,
                 "Detected %s format", demuxers[i].format_name);
        return demuxers[i].demuxer;
        }
      }
  
    for(i = 0; i < num_sync_demuxers; i++)
      {
      if(sync_demuxers[i].demuxer->probe(input))
        {
        bgav_log(input->opt, BGAV_LOG_DEBUG, LOG_DOMAIN,
                 "Detected %s format",
                 sync_demuxers[i].format_name);
        return sync_demuxers[i].demuxer;
        }
      }
    }
  else
    {
    for(i = 0; i < num_yml_demuxers; i++)
      {
      if(yml_demuxers[i].demuxer->probe_yml(yml))
        {
        bgav_log(input->opt, BGAV_LOG_DEBUG, LOG_DOMAIN,
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
      return NULL;

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
  
  return NULL;
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
      {
      bgav_log(ctx->opt, BGAV_LOG_WARNING, LOG_DOMAIN,
               "Removing audio stream %d (no packets found)", i+1);
      bgav_track_remove_audio_stream(ctx->tt->cur, i);
      }
    else
      {
      bgav_superindex_set_durations(ctx->si, &ctx->tt->cur->audio_streams[i]);
      i++;
      }
    }

  i = 0;
  while(i < ctx->tt->cur->num_video_streams)
    {
    if(ctx->tt->cur->video_streams[i].last_index_position < 0)
      {
      bgav_log(ctx->opt, BGAV_LOG_WARNING, LOG_DOMAIN,
               "Removing video stream %d (no packets found)", i+1);
      bgav_track_remove_video_stream(ctx->tt->cur, i);
      }
    else
      {
      bgav_superindex_set_durations(ctx->si, &ctx->tt->cur->video_streams[i]);
      
      if((ctx->tt->cur->video_streams[i].flags &
          (STREAM_B_FRAMES|STREAM_DTS_ONLY)) == STREAM_B_FRAMES)
        bgav_superindex_set_coding_types(ctx->si,
                                         &ctx->tt->cur->video_streams[i]);
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
      ctx->tt->cur->subtitle_streams[i].start_time =
        ctx->si->entries[ctx->tt->cur->subtitle_streams[i].first_index_position].pts;
      i++;
      }
    }
  if(ctx->tt->cur->duration == GAVL_TIME_UNDEFINED)
    bgav_track_calc_duration(ctx->tt->cur);
  }

static void check_interleave(bgav_demuxer_context_t * ctx)
  {
  int i;
  bgav_stream_t ** streams = NULL;
  int index, num_streams;

  num_streams =
    ctx->tt->cur->num_audio_streams +
    ctx->tt->cur->num_video_streams +
    ctx->tt->cur->num_subtitle_streams;

  streams = malloc(num_streams * sizeof(*streams));

  index = 0;
  
  for(i = 0; i < ctx->tt->cur->num_audio_streams; i++)
    streams[index++] = &ctx->tt->cur->audio_streams[i];

  for(i = 0; i < ctx->tt->cur->num_video_streams; i++)
    streams[index++] = &ctx->tt->cur->video_streams[i];
  
  for(i = 0; i < ctx->tt->cur->num_subtitle_streams; i++)
    streams[index++] = &ctx->tt->cur->subtitle_streams[i];
  
  /* If sample accurate decoding was requested, use non-interleaved mode */
  if((ctx->opt->sample_accurate == 1) || (ctx->flags & BGAV_DEMUXER_BUILD_INDEX))
    {
    ctx->demux_mode = DEMUX_MODE_SI_NI;
    }
  /* One stream always means non-interleaved */
  else if(num_streams <= 1)
    {
    ctx->demux_mode = DEMUX_MODE_SI_NI;
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
    //#ifdef DUMP_SUPERINDEX
    if(ctx->opt->dump_indices)
      bgav_superindex_dump(ctx->si);
    // #endif
    }
  return 1;
  }

void bgav_demuxer_stop(bgav_demuxer_context_t * ctx)
  {
  ctx->demuxer->close(ctx);
  ctx->priv = NULL;
    
  /* Reset global variables */
  ctx->flags &= ~(BGAV_DEMUXER_SI_SEEKING |
                  BGAV_DEMUXER_HAS_TIMESTAMP_OFFSET);
  
  ctx->timestamp_offset = 0;
  if(ctx->si)
    {
    bgav_superindex_destroy(ctx->si);
    ctx->si = NULL;
    }
  
  }

int bgav_demuxer_next_packet_interleaved(bgav_demuxer_context_t * ctx)
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
    //    bgav_input_skip_dump(ctx->input,
    //                         ctx->si->entries[ctx->si->current_position].size);
    
#if 0
    fprintf(stderr, "Skip unused %d\n",
            ctx->si->entries[ctx->si->current_position].stream_id);
#endif
    ctx->si->current_position++;
    return 1;
    }
#if 0
  if(stream->type == BGAV_STREAM_SUBTITLE_TEXT)
    {
    fprintf(stderr, "Got subtitle packet\n");
    }
#endif
  if((ctx->flags & BGAV_DEMUXER_SI_SEEKING) &&
     (stream->index_position > ctx->si->current_position))
    {
    ctx->si->current_position++;
    return 1;
    }

  /* Skip until this packet */

  if(ctx->si->entries[ctx->si->current_position].offset > ctx->input->position)
    {
    // fprintf(stderr, "Skipping %ld bytes\n",
    //            ctx->si->entries[ctx->si->current_position].offset - ctx->input->position);
    bgav_input_skip(ctx->input,
                    ctx->si->entries[ctx->si->current_position].offset - ctx->input->position);
    }
  
  p = bgav_stream_get_packet_write(stream);
  bgav_packet_alloc(p, ctx->si->entries[ctx->si->current_position].size);
  p->data_size = ctx->si->entries[ctx->si->current_position].size;

  p->flags = ctx->si->entries[ctx->si->current_position].flags;
  
  p->pts = ctx->si->entries[ctx->si->current_position].pts;
  p->duration = ctx->si->entries[ctx->si->current_position].duration;
  p->position = ctx->si->current_position;
  
  if(bgav_input_read_data(ctx->input, p->data, p->data_size) < p->data_size)
    return 0;
  
  if(stream->process_packet)
    stream->process_packet(stream, p);
  
  bgav_stream_done_packet_write(stream, p);
  
  ctx->si->current_position++;
  return 1;
  }

static int next_packet_noninterleaved(bgav_demuxer_context_t * ctx)
  {
  bgav_packet_t * p;
  bgav_stream_t * s = NULL;

  s = ctx->request_stream;
  
  if(s->index_position > s->last_index_position)
    return 0;
  
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
  
  p->pts = ctx->si->entries[s->index_position].pts;
  p->duration = ctx->si->entries[s->index_position].duration;

  p->flags = ctx->si->entries[s->index_position].flags;
  p->position = s->index_position;
  
  if(bgav_input_read_data(ctx->input, p->data, p->data_size) < p->data_size)
    return 0;

  if(s->process_packet)
    s->process_packet(s, p);
  
  bgav_stream_done_packet_write(s, p);
  
  s->index_position++;
  return 1;
  
  }

int bgav_demuxer_next_packet(bgav_demuxer_context_t * demuxer)
  {
  int ret = 0, i;
  //   fprintf(stderr, "bgav_demuxer_next_packet\n");
  switch(demuxer->demux_mode)
    {
    case DEMUX_MODE_SI_I:
      if(bgav_track_eof_d(demuxer->tt->cur))
        return 0;
      
      ret = bgav_demuxer_next_packet_interleaved(demuxer);
      if(!ret)
        bgav_track_set_eof_d(demuxer->tt->cur);
      
      break;
    case DEMUX_MODE_SI_NI:
      if(demuxer->request_stream->flags & STREAM_EOF_D)
        return 0;
      ret = next_packet_noninterleaved(demuxer);
      if(!ret)
        demuxer->request_stream->flags |= STREAM_EOF_D;
      break;
    case DEMUX_MODE_FI:
      ret = bgav_demuxer_next_packet_fileindex(demuxer);
      if(!ret)
        demuxer->request_stream->flags |= STREAM_EOF_D;
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
            bgav_stream_done_packet_write(&demuxer->tt->cur->audio_streams[i],
                                          demuxer->tt->cur->audio_streams[i].packet);
            demuxer->tt->cur->audio_streams[i].packet = NULL;
            ret = 1;
            }
          }
        for(i = 0; i < demuxer->tt->cur->num_video_streams; i++)
          {
          if(demuxer->tt->cur->video_streams[i].packet)
            {
            bgav_stream_done_packet_write(&demuxer->tt->cur->video_streams[i],
                                          demuxer->tt->cur->video_streams[i].packet);
            demuxer->tt->cur->video_streams[i].packet = NULL;
            ret = 1;
            }
          }
        bgav_track_set_eof_d(demuxer->tt->cur);
        }
      break;
    }
  
  return ret;
  }

gavl_source_status_t
bgav_demuxer_get_packet_read(void * stream1, bgav_packet_t ** ret)
  {
  bgav_stream_t * s = stream1;
  bgav_demuxer_context_t * demuxer = s->demuxer;
  
  demuxer->request_stream = s;
  
  while(!bgav_packet_buffer_peek_packet_read(s->packet_buffer))
    {
    if(s->flags & STREAM_EOF_D)
      return GAVL_SOURCE_EOF;
    
    if(!bgav_demuxer_next_packet(demuxer))
      {
      s->flags |= STREAM_EOF_D;
      return GAVL_SOURCE_EOF;
      }
    }
  
  demuxer->request_stream = NULL;
  *ret = bgav_packet_buffer_get_packet_read(s->packet_buffer);
  return GAVL_SOURCE_OK;
  }

gavl_source_status_t
bgav_demuxer_peek_packet_read(void * stream1, bgav_packet_t ** ret,
                              int force)
  {
  bgav_packet_t * p;
  
  bgav_stream_t * s = stream1;
  bgav_demuxer_context_t * demuxer = s->demuxer;
  
  if(demuxer->flags & BGAV_DEMUXER_PEEK_FORCES_READ)
    force = 1;

  p = bgav_packet_buffer_peek_packet_read(s->packet_buffer);

  if(p)
    {
    if(ret)
      *ret = p;
    return GAVL_SOURCE_OK;
    }
  
  if(!force)
    return GAVL_SOURCE_AGAIN;
  
  demuxer->request_stream = s;
  while(!bgav_packet_buffer_peek_packet_read(s->packet_buffer))
    {
    if(!bgav_demuxer_next_packet(demuxer))
      return GAVL_SOURCE_EOF;
    }
  demuxer->request_stream = NULL;

  if(ret)
    *ret = bgav_packet_buffer_peek_packet_read(s->packet_buffer);
  return GAVL_SOURCE_OK;
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
