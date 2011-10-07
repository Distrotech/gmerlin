/*****************************************************************
 * gmerlin-avdecoder - a general purpose multimedia decoding library
 *
 * Copyright (c) 2001 - 2011 Members of the Gmerlin project
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


/* This demuxer supports 2 modes: Streaming and non-streaming.
   In non-streaming mode, we support all legal combinations
   of sequential and concurrent multiplexing, multiple tracks will
   be detected and can be selected induvidually.
   
   In streaming mode, we only support concurrent multiplexing. If new
   logical bitstreams start, we catch the new metadata and hope, that
   the new track has the same stream layout and format as before.
   
   As codecs we support:
   - Vorbis
   - Theora
   - Speex
   - Flac
   - OGM video (Typically some divx variant)
   - OGM Text subtitles
*/


#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <avdec_private.h>
#include <vorbis_comment.h>
#include <dirac_header.h>

#include <ogg/ogg.h>

/* Fourcc definitions.
   Each private stream data also has a fourcc, which lets us
   detect OGM streams independently of the actual fourcc used here */

#define FOURCC_VORBIS    BGAV_MK_FOURCC('V','B','I','S') /* MUST match BGAV_VORBIS
                                                            from audio_vorbis.c */
#define FOURCC_THEORA    BGAV_MK_FOURCC('T','H','R','A')
#define FOURCC_FLAC      BGAV_MK_FOURCC('F','L','A','C')
#define FOURCC_FLAC_NEW  BGAV_MK_FOURCC('F','L','C','N')
#define FOURCC_SPEEX     BGAV_MK_FOURCC('S','P','E','X')
#define FOURCC_OGM_AUDIO BGAV_MK_FOURCC('O','G','M','A')
#define FOURCC_OGM_VIDEO BGAV_MK_FOURCC('O','G','M','V')
#define FOURCC_DIRAC     BGAV_MK_FOURCC('B','B','C','D')

#define FOURCC_OGM_TEXT  BGAV_MK_FOURCC('T','E','X','T')

#define BYTES_TO_READ 8500 /* Same as in vorbisfile */

#define LOG_DOMAIN "ogg"

static int probe_ogg(bgav_input_context_t * input)
  {
  uint8_t probe_data[4];

  if(bgav_input_get_data(input, probe_data, 4) < 4)
    return 0;

  if((probe_data[0] == 'O') &&
     (probe_data[1] == 'g') &&
     (probe_data[2] == 'g') &&
     (probe_data[3] == 'S'))
    return 1;
  return 0;
  }


typedef struct
  {
  int64_t start_pos;
  int64_t end_pos;
  
  int * unsupported_streams;
  int num_unsupported_streams;
  } track_priv_t;

typedef struct
  {
  uint32_t fourcc_priv;
  ogg_stream_state os;
  
  int header_packets_read;
  int header_packets_needed;

  int64_t last_granulepos;
  int64_t prev_granulepos;      /* Granulepos of the previous page */

  int keyframe_granule_shift;

  bgav_metadata_t metadata;

  int64_t frame_counter;

  /* Set this during seeking */
  int do_sync;

  /* Position of the current and previous page */
  int64_t page_pos;
  int64_t prev_page_pos;

  /* Used for dirac */
  int interlaced_coding;
  } stream_priv_t;

typedef struct
  {
  ogg_sync_state  oy;
  ogg_page        current_page;
  ogg_packet      op;

  /* File position of the current page */
  int64_t     current_page_pos;
  
  int64_t end_pos;
  //  int current_page_size;

  /* Serial  number of the last page of the entire file */
  int last_page_serialno;
  
  /* current_page is valid */
  int page_valid;

  /* op is valid */
  int packet_valid;
  
  /* Remember to call metadata_change and name_change callbacks */
  int metadata_changed;

  /* Different timestamp handling if the stream is live */
  int is_live;
  
  /* Set to 1 to prevent new streamint tracks */
  int nonbos_seen;

  int is_ogm;

  } ogg_t;

/* Special header for OGM files */

typedef struct
  {
  char     type[8];
  uint32_t subtype;
  uint32_t size;
  uint64_t time_unit;
  uint64_t samples_per_unit;
  uint32_t default_len;
  uint32_t buffersize;
  uint16_t bits_per_sample;
  uint16_t padding;

  union
    {
    struct
      {
      uint32_t width;
      uint32_t height;
      } video;
    struct
      {
      uint16_t channels;
      uint16_t blockalign;
      uint32_t avgbytespersec;
      } audio;
    } data;
  } ogm_header_t;

static int ogm_header_read(bgav_input_context_t * input, ogm_header_t * ret)
  {
  if((bgav_input_read_data(input, (uint8_t*)ret->type, 8) < 8) ||
     !bgav_input_read_fourcc(input, &ret->subtype) ||
     !bgav_input_read_32_le(input, &ret->size) ||
     !bgav_input_read_64_le(input, &ret->time_unit) ||
     !bgav_input_read_64_le(input, &ret->samples_per_unit) ||
     !bgav_input_read_32_le(input, &ret->default_len) ||
     !bgav_input_read_32_le(input, &ret->buffersize) ||
     !bgav_input_read_16_le(input, &ret->bits_per_sample) ||
     !bgav_input_read_16_le(input, &ret->padding))
    return 0;

  if(!strncmp(ret->type, "video", 5))
    {
    if(!bgav_input_read_32_le(input, &ret->data.video.width) ||
       !bgav_input_read_32_le(input, &ret->data.video.height))
      return 0;
    return 1;
    }
  else if(!strncmp(ret->type, "audio", 5))
    {
    if(!bgav_input_read_16_le(input, &ret->data.audio.channels) ||
       !bgav_input_read_16_le(input, &ret->data.audio.blockalign) ||
       !bgav_input_read_32_le(input, &ret->data.audio.avgbytespersec))
      return 0;
    return 1;
    }
  else if(!strncmp(ret->type, "text", 5))
    {
    return 1;
    }
  else
    {
    bgav_log(input->opt, BGAV_LOG_WARNING, LOG_DOMAIN,
             "Unknown stream type \"%.8s\" in OGM header", ret->type);
    return 0;
    }
  }
#if 0
static void ogm_header_dump(ogm_header_t * h)
  {
  bgav_dprintf( "OGM Header\n");
  bgav_dprintf( "  Type              %.8s\n", h->type);
  bgav_dprintf( "  Subtype:          ");
  bgav_dump_fourcc(h->subtype);
  bgav_dprintf( "\n");

  bgav_dprintf( "  Size:             %d\n", h->size);
  bgav_dprintf( "  Time unit:        %" PRId64 "\n", h->time_unit);
  bgav_dprintf( "  Samples per unit: %" PRId64 "\n", h->samples_per_unit);
  bgav_dprintf( "  Default len:      %d\n", h->default_len);
  bgav_dprintf( "  Buffer size:      %d\n", h->buffersize);
  bgav_dprintf( "  Bits per sample:  %d\n", h->bits_per_sample);
  if(!strncmp(h->type, "video", 5))
    {
    bgav_dprintf( "  Width:            %d\n", h->data.video.width);
    bgav_dprintf( "  Height:           %d\n", h->data.video.height);
    }
  if(!strncmp(h->type, "audio", 5))
    {
    bgav_dprintf( "  Channels:         %d\n", h->data.audio.channels);
    bgav_dprintf( "  Block align:      %d\n", h->data.audio.blockalign);
    bgav_dprintf( "  Avg bytes per sec: %d\n", h->data.audio.avgbytespersec);
    }
  }
#endif
/* Dump entire structure of the file */
#if 0
static void dump_ogg(bgav_demuxer_context_t * ctx)
  {
  int i, j;
  bgav_track_t * track;
  track_priv_t * track_priv;
  stream_priv_t * stream_priv;
  bgav_stream_t * s;
  
  bgav_dprintf( "Ogg Structure\n");

  for(i = 0; i < ctx->tt->num_tracks; i++)
    {
    track = &ctx->tt->tracks[i];
    track_priv = (track_priv_t*)(track->priv);
    bgav_dprintf( "Track %d, start_pos: %" PRId64 ", end_pos: %" PRId64 "\n",
            i+1, track_priv->start_pos, track_priv->end_pos);
    
    for(j = 0; j < track->num_audio_streams; j++)
      {
      s = &track->audio_streams[j];
      stream_priv = (stream_priv_t*)(s->priv);
      bgav_dprintf( "Audio stream %d\n", j+1);
      bgav_dprintf( "  Serialno: %d\n", s->stream_id);
      bgav_dprintf( "  Language: %s\n", s->language);
      bgav_dprintf( "  Last granulepos: %" PRId64 "\n",
              stream_priv->last_granulepos);
      bgav_dprintf( "  Metadata:\n");
      bgav_metadata_dump(&stream_priv->metadata);
      }
    for(j = 0; j < track->num_video_streams; j++)
      {
      s = &track->video_streams[j];
      stream_priv = (stream_priv_t*)(s->priv);
      bgav_dprintf( "Video stream %d\n", j+1);
      bgav_dprintf( "  Serialno: %d\n", s->stream_id);
      bgav_dprintf( "  Last granulepos: %" PRId64 "\n",
              stream_priv->last_granulepos);
      bgav_dprintf( "  Metadata:\n");
      bgav_metadata_dump(&stream_priv->metadata);
      }
    for(j = 0; j < track->num_subtitle_streams; j++)
      {
      if(track->subtitle_streams[j].data.subtitle.subreader)
        continue;
      
      s = &track->subtitle_streams[j];
      stream_priv = (stream_priv_t*)(s->priv);
      bgav_dprintf( "Subtitle stream %d\n", j+1);
      bgav_dprintf( "  Serialno: %d\n", s->stream_id);
      bgav_dprintf( "  Language: %s\n", s->language);
      bgav_dprintf( "  Metadata:\n");
      bgav_metadata_dump(&stream_priv->metadata);
      }
    
    }
  
  }
#endif

/* Parse a Vorbis comment: This sets metadata and language */

static void parse_vorbis_comment(bgav_stream_t * s, uint8_t * data,
                                 int len)
  {
  const char * language;
  const char * field;
  stream_priv_t * stream_priv;
  bgav_vorbis_comment_t vc;
  bgav_input_context_t * input_mem;
  input_mem = bgav_input_open_memory(data, len, s->opt);

  memset(&vc, 0, sizeof(vc));

  stream_priv = (stream_priv_t*)(s->priv);
  
  if(!bgav_vorbis_comment_read(&vc, input_mem))
    return;

  bgav_metadata_free(&stream_priv->metadata);
  if(s->language[0] != '\0')
    s->language[0] = '\0';
  
  bgav_vorbis_comment_2_metadata(&vc, &stream_priv->metadata);

  field = bgav_vorbis_comment_get_field(&vc, "LANGUAGE");
  if(field)
    {
    language = bgav_lang_from_name(field);
    if(language)
      strcpy(s->language, language);
    }
  bgav_vorbis_comment_free(&vc);
  bgav_input_destroy(input_mem);
  }

/* Seek byte and reset the synchronizer */

static void seek_byte(bgav_demuxer_context_t * ctx, int64_t pos)
  {
  ogg_t * priv = ctx->priv;
  ogg_sync_reset(&priv->oy);
  //  ogg_page_clear(&priv->os);
  bgav_input_seek(ctx->input, pos, SEEK_SET);
  priv->page_valid = 0;
  }

/* Get new data */

static int get_data(bgav_demuxer_context_t * ctx)
  {
  int bytes_to_read;
  char * buf;
  int result;
  ogg_t * priv = ctx->priv;

  
  bytes_to_read = BYTES_TO_READ;
  if(priv->end_pos > 0)
    {
    if(ctx->input->position + bytes_to_read > priv->end_pos)
      bytes_to_read = priv->end_pos - ctx->input->position;
    if(bytes_to_read <= 0)
      return 0;
    }
  buf = ogg_sync_buffer(&priv->oy, bytes_to_read);
  result = bgav_input_read_data(ctx->input, (uint8_t*)buf, bytes_to_read);

  
  ogg_sync_wrote(&priv->oy, result);
  return result;
  }

/* Get new page. We pre-parse the data to ensure, that only complete pages
 * are processed. This enables us to get the file positions, at which the
 * pages start (needed for building an index).
 */

#define PAGE_HEADER_BYTES 27

static int get_page(bgav_demuxer_context_t * ctx)
  {
  uint8_t header[PAGE_HEADER_BYTES+255];
  int nsegs, i, result;
  int page_size;
  char * buf;
  int bytes_skipped = 0;
  ogg_t * priv = ctx->priv;

  if(priv->page_valid)
    return 1;

  /* Skip random garbage */
  while(1)
    {
    if(bgav_input_get_data(ctx->input, header, 4) < 4)
      return 0;
    if((header[0] == 'O') &&
       (header[1] == 'g') &&
       (header[2] == 'g') &&
       (header[3] == 'S'))
      break;
    bgav_input_skip(ctx->input, 1);
    bytes_skipped++;
    }
  if(bytes_skipped)
    bgav_log(ctx->opt, BGAV_LOG_WARNING, LOG_DOMAIN,
             "Skipped %d bytes of random garbage", bytes_skipped);


  if(bgav_input_get_data(ctx->input, header, PAGE_HEADER_BYTES) < PAGE_HEADER_BYTES)
    return 0;
 
  nsegs = header[PAGE_HEADER_BYTES-1];

  if(bgav_input_get_data(ctx->input, header, PAGE_HEADER_BYTES+nsegs) < PAGE_HEADER_BYTES+nsegs)
    return 0;
  
  page_size = PAGE_HEADER_BYTES + nsegs;
  for(i = 0; i < nsegs; i++)
    page_size += header[PAGE_HEADER_BYTES+i];

  buf = ogg_sync_buffer(&priv->oy, page_size);

  priv->current_page_pos = ctx->input->position;

  result = bgav_input_read_data(ctx->input, (uint8_t*)buf, page_size);
  
  ogg_sync_wrote(&priv->oy, result);

  if(ogg_sync_pageout(&priv->oy, &priv->current_page) != 1)
    {
    return 0;
    }
  priv->page_valid = 1;
  return 1;
  }

/* Append extradata to a stream */

static void append_extradata(bgav_stream_t * s, ogg_packet * op)
  {
  uint8_t * ptr;
  s->ext_data = realloc(s->ext_data, s->ext_size + op->bytes + 4);
  ptr = s->ext_data + s->ext_size;
  BGAV_32BE_2_PTR(op->bytes, ptr); ptr+=4;
  memcpy(ptr, op->packet, op->bytes);
  s->ext_size += op->bytes + 4;
  }

/* Get the fourcc from the identification packet */

static uint32_t detect_stream(ogg_packet * op)
  {
  if((op->bytes > 7) &&
     (op->packet[0] == 0x01) &&
     !strncmp((char*)(op->packet+1), "vorbis", 6))
    return FOURCC_VORBIS;

  else if((op->bytes > 7) &&
          (op->packet[0] == 0x80) &&
          !strncmp((char*)(op->packet+1), "theora", 6))
    return FOURCC_THEORA;

  else if((op->bytes == 4) &&
            !strncmp((char*)(op->packet), "fLaC", 4))
    return FOURCC_FLAC;

  else if((op->bytes > 5) &&
          (op->packet[0] == 0x7F) &&
          !strncmp((char*)(op->packet+1), "FLAC", 4))
    return FOURCC_FLAC_NEW;
  
  else if((op->bytes >= 80) &&
          !strncmp((char*)(op->packet), "Speex", 5))
    return FOURCC_SPEEX;

  else if((op->bytes >= 9) &&
          (op->packet[0] == 0x01) &&
          !strncmp((char*)(op->packet+1), "video", 5))
    return FOURCC_OGM_VIDEO;

  else if((op->bytes >= 9) &&
          (op->packet[0] == 0x01) &&
          !strncmp((char*)(op->packet+1), "text", 4))
    return FOURCC_OGM_TEXT;
  else if((op->bytes >= 4) &&
          !strncmp((char*)(op->packet), "BBCD", 4))
    return FOURCC_DIRAC;
  
  return 0;
  }

static void cleanup_stream_ogg(bgav_stream_t * s)
  {
  stream_priv_t * stream_priv;
  stream_priv = (stream_priv_t*)(s->priv);
  if(stream_priv)
    {
    ogg_stream_clear(&stream_priv->os);
    bgav_metadata_free(&stream_priv->metadata);
    free(stream_priv);
    }
  }

/* Set up a track, which starts at start position */

static int setup_track(bgav_demuxer_context_t * ctx, bgav_track_t * track,
                       int64_t start_position)
  {
  int i, done;
  track_priv_t * ogg_track;
  stream_priv_t * ogg_stream;
  bgav_stream_t * s;
  int serialno;
  ogg_t * priv;
  ogm_header_t ogm_header;
  bgav_input_context_t * input_mem;
  int header_bytes = 0;
  bgav_dirac_sequence_header_t dirac_header;
  
  priv = ctx->priv;

  ogg_track = calloc(1, sizeof(*ogg_track));
  ogg_track->start_pos = start_position;
  track->priv = ogg_track;

  /* If we can seek, seek to the start point. If we can't seek, we are already
     at the right position */
  if(ctx->input->input->seek_byte)
    seek_byte(ctx, start_position);
  
  /* Get the first page of each stream */
  while(1)
    {
    if(!get_page(ctx))
      {
      bgav_log(ctx->opt, BGAV_LOG_ERROR, LOG_DOMAIN, "EOF while setting up track");
      return 0;
      }
    if(!ogg_page_bos(&priv->current_page))
      {
      priv->page_valid = 1;
      break;
      }
    /* Setup stream */
    serialno = ogg_page_serialno(&priv->current_page);
    ogg_stream = calloc(1, sizeof(*ogg_stream));
    ogg_stream->last_granulepos = -1;

    ogg_stream_init(&ogg_stream->os, serialno);
    ogg_stream_pagein(&ogg_stream->os, &priv->current_page);
    priv->page_valid = 0;
    header_bytes +=
      priv->current_page.header_len + priv->current_page.body_len;
    if(ogg_stream_packetout(&ogg_stream->os, &priv->op) != 1)
      {
      return 0;
      }

    ogg_stream->fourcc_priv = detect_stream(&priv->op);
    
    switch(ogg_stream->fourcc_priv)
      {
      case FOURCC_VORBIS:
        s = bgav_track_add_audio_stream(track, ctx->opt);
        s->cleanup = cleanup_stream_ogg;
        s->fourcc = FOURCC_VORBIS;
        s->index_mode = INDEX_MODE_SIMPLE;
        s->priv   = ogg_stream;
        s->stream_id = serialno;
        s->flags |= STREAM_PARSE_FRAME;
        
        ogg_stream->header_packets_needed = 3;
        append_extradata(s, &priv->op);
        ogg_stream->header_packets_read = 1;

        /* Get channels */
        s->data.audio.format.num_channels =
          priv->op.packet[11];

        /* Get samplerate */
        s->data.audio.format.samplerate =
          BGAV_PTR_2_32LE(priv->op.packet + 12);
        s->timescale = s->data.audio.format.samplerate;
        bgav_vorbis_set_channel_setup(&s->data.audio.format);
        
        /* Read remaining header packets from this page */
        while(ogg_stream_packetout(&ogg_stream->os, &priv->op) == 1)
          {
          append_extradata(s, &priv->op);
          ogg_stream->header_packets_read++;
          if(ogg_stream->header_packets_read == ogg_stream->header_packets_needed)
            break;
          }
        break;
      case FOURCC_THEORA:
        s = bgav_track_add_video_stream(track, ctx->opt);
        s->cleanup = cleanup_stream_ogg;
        s->fourcc = FOURCC_THEORA;
        s->index_mode = INDEX_MODE_SIMPLE;
        s->priv   = ogg_stream;
        s->stream_id = serialno;
        s->data.video.frametime_mode = BGAV_FRAMETIME_PTS;
        ogg_stream->header_packets_needed = 3;
        append_extradata(s, &priv->op);
        ogg_stream->header_packets_read = 1;

        /* Get picture dimensions, fps and keyframe shift */

        s->data.video.format.frame_width =
          BGAV_PTR_2_16BE(priv->op.packet+10);
        s->data.video.format.frame_width *= 16;

        s->data.video.format.frame_height =
          BGAV_PTR_2_16BE(priv->op.packet+12);
        s->data.video.format.frame_height *= 16;
        
        s->data.video.format.image_width =
          BGAV_PTR_2_24BE(priv->op.packet+14);
        s->data.video.format.image_height =
          BGAV_PTR_2_24BE(priv->op.packet+17);
        
        s->data.video.format.timescale =
          BGAV_PTR_2_32BE(priv->op.packet+22);
        s->data.video.format.frame_duration =
          BGAV_PTR_2_32BE(priv->op.packet+26);

        s->data.video.format.pixel_width =
          BGAV_PTR_2_24BE(priv->op.packet+30);
        s->data.video.format.pixel_height =
          BGAV_PTR_2_24BE(priv->op.packet+33);

        // fprintf(stderr, "Got video format:\n");
        // gavl_video_format_dump(&s->data.video.format);
        
        ogg_stream->keyframe_granule_shift =
          (char) ((priv->op.packet[40] & 0x03) << 3);

        ogg_stream->keyframe_granule_shift |=
          (priv->op.packet[41] & 0xe0) >> 5;
        /* Read remaining header packets from this page */
        while(ogg_stream_packetout(&ogg_stream->os, &priv->op) == 1)
          {
          append_extradata(s, &priv->op);
          ogg_stream->header_packets_read++;
          if(ogg_stream->header_packets_read == ogg_stream->header_packets_needed)
            break;
          }
        break;
      case FOURCC_DIRAC:
        s = bgav_track_add_video_stream(track, ctx->opt);
        s->cleanup = cleanup_stream_ogg;
        s->fourcc = FOURCC_DIRAC;
        s->index_mode = INDEX_MODE_SIMPLE;
        s->priv   = ogg_stream;
        s->stream_id = serialno;
        s->data.video.frametime_mode = BGAV_FRAMETIME_PTS;
        ogg_stream->header_packets_needed = 1;
        ogg_stream->header_packets_read = 1;

        if(!bgav_dirac_sequence_header_parse(&dirac_header, 
                                             priv->op.packet, priv->op.bytes))
          return 0;
        bgav_dirac_sequence_header_dump(&dirac_header);
        if(!dirac_header.timescale || !dirac_header.frame_duration)
          {
          bgav_log(ctx->opt, BGAV_LOG_WARNING, LOG_DOMAIN,
                   "Dirac header contains no framerate, assuming 24 fps");
          dirac_header.timescale      = 24;
          dirac_header.frame_duration =  1;
          }
        s->data.video.format.timescale = dirac_header.timescale;
        s->data.video.format.frame_duration = dirac_header.frame_duration;

        if(dirac_header.picture_coding_mode == 1)
          ogg_stream->interlaced_coding = 1;
        
        while(ogg_stream_packetout(&ogg_stream->os, &priv->op) == 1)
          ;
        
        break;
      case FOURCC_FLAC_NEW:
        s = bgav_track_add_audio_stream(track, ctx->opt);
        s->cleanup = cleanup_stream_ogg;
        s->fourcc = FOURCC_FLAC;
        s->priv   = ogg_stream;
        s->index_mode = INDEX_MODE_SIMPLE;
        s->flags |= STREAM_NO_DURATIONS;
        ogg_stream->fourcc_priv = FOURCC_FLAC_NEW;
        s->ext_data = malloc(priv->op.bytes - 9);
        memcpy(s->ext_data, priv->op.packet + 9, priv->op.bytes - 9);
        s->ext_size = priv->op.bytes - 9;
        s->stream_id = serialno;
        
        /* We tell the decoder, that this is the last metadata packet */
        s->ext_data[4] |= 0x80;
        
        s->data.audio.format.samplerate = 
          (s->ext_data[18] << 12) |
          (s->ext_data[19] << 4) |
          ((s->ext_data[20] >> 4)&0xf);

        ogg_stream->header_packets_needed = BGAV_PTR_2_16BE(priv->op.packet+7)+1;
        ogg_stream->header_packets_read = 1;

        break;
      case FOURCC_FLAC:
        s = bgav_track_add_audio_stream(track, ctx->opt);
        s->cleanup = cleanup_stream_ogg;
        s->fourcc = FOURCC_FLAC;
        s->index_mode = INDEX_MODE_SIMPLE;
        s->flags |= STREAM_NO_DURATIONS;
        ogg_stream->fourcc_priv = FOURCC_FLAC;
        s->priv   = ogg_stream;
        s->stream_id = serialno;
        
        ogg_stream->header_packets_needed = 1;
        ogg_stream->header_packets_read = 0;
        
        while(ogg_stream_packetout(&ogg_stream->os, &priv->op) == 1)
          {
          }
        break;
      case FOURCC_SPEEX:
        s = bgav_track_add_audio_stream(track, ctx->opt);
        s->cleanup = cleanup_stream_ogg;
        s->fourcc = FOURCC_SPEEX;
        s->index_mode = INDEX_MODE_SIMPLE;
        s->flags |= STREAM_NO_DURATIONS;

        s->priv   = ogg_stream;
        s->stream_id = serialno;

        ogg_stream->header_packets_needed = 2;
        ogg_stream->header_packets_read = 1;

        /* First packet is also the extradata */
        s->ext_data = malloc(priv->op.bytes);
        memcpy(s->ext_data, priv->op.packet, priv->op.bytes);
        s->ext_size = priv->op.bytes;

        /* Samplerate */
        s->data.audio.format.samplerate = BGAV_PTR_2_32LE(priv->op.packet + 36);

        /* Extra packets */
        ogg_stream->header_packets_needed += BGAV_PTR_2_32LE(priv->op.packet + 68);
      
      
        while(ogg_stream_packetout(&ogg_stream->os, &priv->op) == 1)
          {
          }
        break;
      case FOURCC_OGM_VIDEO:
        
        s = bgav_track_add_video_stream(track, ctx->opt);
        s->cleanup = cleanup_stream_ogg;
      
        s->priv   = ogg_stream;
        s->stream_id = serialno;
        s->index_mode = INDEX_MODE_SIMPLE;
        
        s->data.video.frametime_mode = BGAV_FRAMETIME_PTS;
        
        input_mem = bgav_input_open_memory(priv->op.packet + 1, priv->op.bytes - 1, s->opt);

        if(!ogm_header_read(input_mem, &ogm_header))
          {
          bgav_log(ctx->opt, BGAV_LOG_ERROR, LOG_DOMAIN, "Reading OGM header failed");
          bgav_input_close(input_mem);
          bgav_input_destroy(input_mem);
          return 0;
          }
        bgav_input_close(input_mem);
        bgav_input_destroy(input_mem);
      
        //        ogm_header_dump(&ogm_header);

        /* Set up the stream from the OGM header */
        ogg_stream->fourcc_priv = FOURCC_OGM_VIDEO;

        s->data.video.format.image_width  = ogm_header.data.video.width;
        s->data.video.format.image_height = ogm_header.data.video.height;

        s->data.video.format.frame_width  = ogm_header.data.video.width;
        s->data.video.format.frame_height = ogm_header.data.video.height;
        s->data.video.format.pixel_width  = 1;
        s->data.video.format.pixel_height = 1;

        s->data.video.format.frame_duration = ogm_header.time_unit;
        s->data.video.format.timescale      = ogm_header.samples_per_unit * 10000000;

        //        gavl_video_format_dump(&s->data.video.format);
      
        s->fourcc = ogm_header.subtype;

        if(bgav_video_is_divx4(s->fourcc))
          {
          s->flags |= (STREAM_B_FRAMES |
                       STREAM_WRONG_B_TIMESTAMPS |
                       STREAM_PARSE_FRAME);
          }
        ogg_stream->header_packets_needed = 2;
        ogg_stream->header_packets_read = 1;
        priv->is_ogm = 1;

        break;
      case FOURCC_OGM_TEXT:
        s = bgav_track_add_subtitle_stream(track, ctx->opt, 1,
                                           NULL);
        s->cleanup = cleanup_stream_ogg;
        s->priv   = ogg_stream;
        s->stream_id = serialno;
        s->index_mode = INDEX_MODE_SIMPLE;

        input_mem =
          bgav_input_open_memory(priv->op.packet + 1,
                                 priv->op.bytes - 1, s->opt);
        
        if(!ogm_header_read(input_mem, &ogm_header))
          {
          bgav_log(ctx->opt, BGAV_LOG_ERROR, LOG_DOMAIN, "Reading OGM header failed");
          bgav_input_close(input_mem);
          bgav_input_destroy(input_mem);
          return 0;
          }
        bgav_input_close(input_mem);
        bgav_input_destroy(input_mem);
      
        //        ogm_header_dump(&ogm_header);

        /* Set up the stream from the OGM header */
        ogg_stream->fourcc_priv = FOURCC_OGM_TEXT;
        
        s->fourcc = FOURCC_OGM_TEXT;
        s->timescale = 10000000 / ogm_header.time_unit;
        s->description = bgav_sprintf("OGM subtitles");
        ogg_stream->header_packets_needed = 2;
        ogg_stream->header_packets_read = 1;
        priv->is_ogm = 1;
        break;
      default:
        bgav_log(ctx->opt, BGAV_LOG_ERROR, LOG_DOMAIN,
                 "Unsupported stream (serialno: %d)", serialno);
        
        ogg_track->unsupported_streams =
          realloc(ogg_track->unsupported_streams,
                  sizeof(*ogg_track->unsupported_streams) *
                  (ogg_track->num_unsupported_streams + 1));
        
        ogg_track->unsupported_streams[ogg_track->num_unsupported_streams] =
          serialno;
        ogg_track->num_unsupported_streams++;
        break;
      }
    }
  
  /*
   *  Now, read header pages until we are done, current_page still contains the
   *  first page which has no bos marker set
   */

  done = 0;
  while(!done)
    {
    s = bgav_track_find_stream_all(track, ogg_page_serialno(&priv->current_page));

    if(s)
      {
      ogg_stream = (stream_priv_t*)(s->priv);
      ogg_stream_pagein(&ogg_stream->os, &priv->current_page);
      priv->page_valid = 0;
      header_bytes +=
        priv->current_page.header_len + priv->current_page.body_len;
      
      switch(ogg_stream->fourcc_priv)
        {
        case FOURCC_THEORA:
        case FOURCC_VORBIS:
          /* Read remaining header packets from this page */
          while(ogg_stream_packetout(&ogg_stream->os, &priv->op) == 1)
            {
            append_extradata(s, &priv->op);
            ogg_stream->header_packets_read++;
            /* Second packet is vorbis comment starting after 7 bytes */
            if(ogg_stream->header_packets_read == 2)
              parse_vorbis_comment(s, priv->op.packet + 7, priv->op.bytes - 7);

            if(ogg_stream->header_packets_read == ogg_stream->header_packets_needed)
              break;
            }
          break;
        case FOURCC_OGM_TEXT:
          while(ogg_stream_packetout(&ogg_stream->os, &priv->op) == 1)
            {
            ogg_stream->header_packets_read++;
            /* Second packet is vorbis comment starting after 7 bytes */
            if(ogg_stream->header_packets_read == 2)
              parse_vorbis_comment(s, priv->op.packet + 7, priv->op.bytes - 7);
            if(ogg_stream->header_packets_read == ogg_stream->header_packets_needed)
              break;
            }
          break;
        case FOURCC_OGM_VIDEO:
          /* Comment packet (can be ignored) */
          ogg_stream->header_packets_read++;
          break;
        case FOURCC_SPEEX:
          /* Comment packet */
          ogg_stream_packetout(&ogg_stream->os, &priv->op);
          if(ogg_stream->header_packets_read == 1)
            parse_vorbis_comment(s, priv->op.packet, priv->op.bytes);
          ogg_stream->header_packets_read++;
          break;
        case FOURCC_FLAC_NEW:
          while(ogg_stream_packetout(&ogg_stream->os, &priv->op) == 1)
            {
            if((priv->op.packet[0] & 0x7f) == 0x04)
              {
              parse_vorbis_comment(s, priv->op.packet+4, priv->op.bytes-4);
              }
            ogg_stream->header_packets_read++;
            }
          break;
        case FOURCC_FLAC:
          while(ogg_stream_packetout(&ogg_stream->os, &priv->op) == 1)
            {

            switch(priv->op.packet[0] & 0x7f)
              {
              case 0: /* STREAMINFO, this is the only info we'll tell to the flac demuxer */
                if(s->ext_data)
                  {
                  return 0;
                  }
                s->ext_data = malloc(priv->op.bytes + 4);
                s->ext_data[0] = 'f';
                s->ext_data[1] = 'L';
                s->ext_data[2] = 'a';
                s->ext_data[3] = 'C';
                memcpy(s->ext_data + 4, priv->op.packet, priv->op.bytes);
                s->ext_size = priv->op.bytes + 4;

                /* We tell the decoder, that this is the last metadata packet */
                s->ext_data[4] |= 0x80;
              
                s->data.audio.format.samplerate = 
                  (s->ext_data[18] << 12) |
                  (s->ext_data[19] << 4) |
                  ((s->ext_data[20] >> 4)&0xf);
                break;
              case 1:
                parse_vorbis_comment(s, priv->op.packet+4, priv->op.bytes-4);
                break;
              }
            ogg_stream->header_packets_read++;
            if(!(priv->op.packet[0] & 0x80))
              ogg_stream->header_packets_needed++;
            }
        }
      }
    else /* no stream found */
      {
      priv->page_valid = 0;
      header_bytes +=
        priv->current_page.header_len + priv->current_page.body_len;
      }
    
    /* Check if we are done for all streams */
    done = 1;
    
    for(i = 0; i < track->num_audio_streams; i++)
      {
      ogg_stream = (stream_priv_t*)(track->audio_streams[i].priv);
      if(ogg_stream->header_packets_read < ogg_stream->header_packets_needed)
        done = 0;
      }

    if(done)
      {
      for(i = 0; i < track->num_video_streams; i++)
        {
        ogg_stream = (stream_priv_t*)(track->video_streams[i].priv);
        if(ogg_stream->header_packets_read < ogg_stream->header_packets_needed)
          done = 0;
        }
      }

    if(done)
      {
      for(i = 0; i < track->num_subtitle_streams; i++)
        {
        if(track->subtitle_streams[i].data.subtitle.subreader)
          continue;
        ogg_stream = (stream_priv_t*)(track->subtitle_streams[i].priv);
        if(ogg_stream->header_packets_read < ogg_stream->header_packets_needed)
          done = 0;
        }
      }

    /* Read the next page if we aren't done yet */

    if(!done)
      {
      if(!get_page(ctx))
        {
        bgav_log(ctx->opt, BGAV_LOG_ERROR, LOG_DOMAIN, "EOF while setting up track");
        return 0;
        }
      }
    }

  if(!(ctx->flags & BGAV_DEMUXER_HAS_DATA_START))
    {
    ctx->data_start += header_bytes;
    ctx->flags |= BGAV_DEMUXER_HAS_DATA_START;
    }
  
  return 1;
  }

/* Find the first first page between pos1 and pos2,
   return file position, -1 is returned on failure */

static int64_t
find_first_page(bgav_demuxer_context_t * ctx, int64_t pos1, int64_t pos2,
                int * serialno, int64_t * granulepos)
  {
  int64_t ret;
  int result;
  ogg_t * priv = ctx->priv;

  seek_byte(ctx, pos1);
  ret = pos1;

  while(1)
    {
    result = ogg_sync_pageseek(&priv->oy, &priv->current_page);
    
    if(!result) /* Need more data */
      {
      if(!get_data(ctx))
        return -1;
      }
    else if(result > 0) /* Page found, result is page size */
      {
      if(ret >= pos2)
        return -1;
      
      if(serialno)
        *serialno = ogg_page_serialno(&priv->current_page);
      if(granulepos)
        *granulepos = ogg_page_granulepos(&priv->current_page);
      return ret;
      }
    else if(result < 0) /* Skipped -result bytes */
      {
      ret -= result;
      }
    }
  return -1;
  }

/* Find the last page between pos1 and pos2,
   return file position, -1 is returned on failure */

static int64_t find_last_page(bgav_demuxer_context_t * ctx, int64_t pos1,
                              int64_t pos2,
                              int * serialno, int64_t * granulepos)
  {
  int64_t page_pos, last_page_pos = -1, start_pos;
  int this_serialno, last_serialno = 0;
  int64_t this_granulepos, last_granulepos = 0;
  start_pos = pos2 - BYTES_TO_READ;
  if(start_pos < 0)
    start_pos = 0;
  while(1)
    {
    page_pos = find_first_page(ctx, start_pos, pos2, &this_serialno,
                               &this_granulepos);
    
    if(page_pos == -1)
      {
      if(last_page_pos >= 0)     /* No more pages in range -> return last one */
        {
        if(serialno)
          *serialno = last_serialno;
        if(granulepos)
          *granulepos = last_granulepos;
        return last_page_pos;
        }
      else if(start_pos <= pos1) /* Beginning of seek range -> fail */
        return -1;
      else                       /* Go back a bit */
        {
        start_pos -= BYTES_TO_READ;
        if(start_pos < pos1)
          start_pos = pos1;
        }
      }
    else
      {
      last_serialno   = this_serialno;
      last_granulepos = this_granulepos;
      last_page_pos = page_pos;
      start_pos = page_pos+1;
      }
    }
  return -1;
  }

static int track_has_serialno(bgav_track_t * track,
                              int serialno, bgav_stream_t ** s)
  {
  track_priv_t * track_priv;
  int i;
  
  for(i = 0; i < track->num_audio_streams; i++)
    {
    if(track->audio_streams[i].stream_id == serialno)
      {
      if(s)
        *s = &track->audio_streams[i];
      return 1;
      }
    }

  for(i = 0; i < track->num_video_streams; i++)
    {
    if(track->video_streams[i].stream_id == serialno)
      {
      if(s)
        *s = &track->video_streams[i];
      return 1;
      }
    }

  if(s)
    *s = NULL;
  
  track_priv = (track_priv_t*)(track->priv);

  for(i = 0; i < track_priv->num_unsupported_streams; i++)
    {
    if(track_priv->unsupported_streams[i] == serialno)
      return 1;
    }
  return 0;
  }

/* Find the next track starting with the current position */
static int64_t find_next_track(bgav_demuxer_context_t * ctx,
                               int64_t start_pos, bgav_track_t * last_track)
  {
  int64_t pos1, pos2, test_pos, page_pos_1, page_pos_2, granulepos_1;
  bgav_track_t * new_track;
  ogg_t * priv;
  int serialno_1, serialno_2;
  bgav_stream_t * s;
  stream_priv_t * stream_priv;
  
  priv = ctx->priv;
  
  /* Do bisection search */
  pos1 = start_pos;
  pos2 = ctx->input->total_bytes;

  while(1)
    {
    test_pos = (pos1 + pos2) / 2;

    page_pos_1 = find_last_page(ctx, start_pos, test_pos, &serialno_1,
                                &granulepos_1);
    if(page_pos_1 < 0)
      return -1;
    
    if(!track_has_serialno(last_track, serialno_1, &s))
      {
      pos2 = page_pos_1;
      continue;
      }

    page_pos_2 = find_first_page(ctx, test_pos,
                                 ctx->input->total_bytes, &serialno_2,
                                 NULL);
    if(page_pos_2 < 0)
      return -1;
    
    if(track_has_serialno(last_track, serialno_2, NULL))
      {
      //      pos1 = test_pos;
      pos1 = page_pos_2;
      continue;
      }

    if(!ogg_page_bos(&priv->current_page))
      {
      bgav_log(ctx->opt, BGAV_LOG_ERROR, LOG_DOMAIN,
               "Page has no BOS marker");
      return 0;
      }

    new_track = bgav_track_table_append_track(ctx->tt);
    if(!setup_track(ctx, new_track, page_pos_2))
      {
      bgav_log(ctx->opt, BGAV_LOG_ERROR, LOG_DOMAIN,
               "Setting up track failed");
      return 0;
      }

    /* Now, we also know the last granulepos of last_track */

    if(s)
      {
      stream_priv = (stream_priv_t*)(s->priv);
      stream_priv->last_granulepos = granulepos_1;
      }
    return page_pos_2;
    }
  return -1;
  }

static gavl_time_t granulepos_2_time(bgav_stream_t * s,
                                     int64_t pos)
  {
  int64_t frames;

  stream_priv_t * stream_priv;
  stream_priv = (stream_priv_t *)(s->priv);
  switch(stream_priv->fourcc_priv)
    {
    case FOURCC_VORBIS:
    case FOURCC_FLAC:
    case FOURCC_FLAC_NEW:
    case FOURCC_SPEEX:
    case FOURCC_OGM_AUDIO:
      return gavl_samples_to_time(s->data.audio.format.samplerate,
                                  pos);
      break;
    case FOURCC_THEORA:
      stream_priv = (stream_priv_t*)(s->priv);

      frames = pos >> stream_priv->keyframe_granule_shift;
      frames +=
        pos-(frames<<stream_priv->keyframe_granule_shift);
      return gavl_frames_to_time(s->data.video.format.timescale,
                                 s->data.video.format.frame_duration,
                                 frames);
      break;
    case FOURCC_OGM_VIDEO:
      return gavl_frames_to_time(s->data.video.format.timescale,
                                 s->data.video.format.frame_duration,
                                 pos);
      break;
    case FOURCC_DIRAC:
      {
      int64_t high_word = pos >> 22;
      int32_t low_word  = pos & 0x3fffff;
      if(!stream_priv->interlaced_coding)
        return ((high_word + low_word) >> 10) * s->data.video.format.frame_duration;
      return ((high_word + low_word) >> 9) * s->data.video.format.frame_duration;
      }
    }
  return GAVL_TIME_UNDEFINED;
  }

static void get_last_granulepos(bgav_demuxer_context_t * ctx,
                                bgav_track_t * track)
  {
  stream_priv_t * stream_priv;
  track_priv_t * track_priv;
  int64_t pos, granulepos;
  int done = 0, i, serialno;
  bgav_stream_t * s;
  
  track_priv = (track_priv_t*)(track->priv);

  pos = track_priv->end_pos;
  
  while(!done)
    {
    /* Check if we are already done */
    done = 1;
    for(i = 0; i < track->num_audio_streams; i++)
      {
      stream_priv = (stream_priv_t*)(track->audio_streams[i].priv);
      if(stream_priv->last_granulepos < 0)
        {
        done = 0;
        break;
        }
      }
    if(done)
      {
      for(i = 0; i < track->num_video_streams; i++)
        {
        stream_priv = (stream_priv_t*)(track->video_streams[i].priv);
        if(stream_priv->last_granulepos < 0)
          {
          done = 0;
          break;
          }
        }
      }
    if(done)
      break;
    /* Get the next last page */
    
    pos = find_last_page(ctx, track_priv->start_pos,
                         pos, &serialno, &granulepos);

    if(pos < 0) /* Should never happen */
      break;

    s = NULL;
    if(track_has_serialno(track, serialno, &s) && s)
      {
      stream_priv = (stream_priv_t*)(s->priv);
      if(stream_priv->last_granulepos < 0)
        stream_priv->last_granulepos = granulepos;
      }
    }
  
  }

/* There is no central location for the metadata. Therefore
 * we merge the vorbis comments from all streams and hope,
 * that people encoded them not too idiotic.
 */
   
static void get_metadata(bgav_track_t * track)
  {
  stream_priv_t * stream_priv;
  int i;
  bgav_metadata_free(&track->metadata);

  for(i = 0; i < track->num_audio_streams; i++)
    {
    stream_priv = (stream_priv_t *)(track->audio_streams[i].priv);
    bgav_metadata_merge2(&track->metadata, &stream_priv->metadata);
    }
  for(i = 0; i < track->num_video_streams; i++)
    {
    stream_priv = (stream_priv_t *)(track->video_streams[i].priv);
    bgav_metadata_merge2(&track->metadata, &stream_priv->metadata);
    }
  }

static int open_ogg(bgav_demuxer_context_t * ctx)
  {
  gavl_time_t stream_duration;
  int i, j;
  track_priv_t * track_priv_1, * track_priv_2;
  bgav_track_t * last_track;
  bgav_stream_t * s;
  stream_priv_t * stream_priv;
  int64_t result, last_granulepos;
  bgav_input_context_t * input_save = NULL;
  
  ogg_t * priv;
  
  priv = calloc(1, sizeof(*priv));
  ctx->priv = priv;
  
  ogg_sync_init(&priv->oy);

  ctx->tt = bgav_track_table_create(1);
  
  ctx->data_start = ctx->input->position;

  if(!ctx->input->input->seek_byte)
    {
    input_save = ctx->input;
    ctx->input = bgav_input_open_as_buffer(ctx->input);
    }
  
  /* Set up the first track */
  if(!setup_track(ctx, ctx->tt->cur, ctx->data_start))
    return 0;

  if(input_save)
    {
    bgav_input_close(ctx->input);
    bgav_input_destroy(ctx->input);
    ctx->input = input_save;
    }
  else
    {
    bgav_input_seek(ctx->input, ctx->data_start, SEEK_SET);
    }
  
  if(ctx->input->input->seek_byte && ctx->input->total_bytes)
    {
    /* Get the last page of the stream and check for the serialno */

    if(find_last_page(ctx, 0, ctx->input->total_bytes,
                      &priv->last_page_serialno,
                      &last_granulepos) < 0)
      {
      return 0;
      }
    result = ctx->data_start;
    while(1)
      {
      last_track = &ctx->tt->tracks[ctx->tt->num_tracks-1];

      if(track_has_serialno(last_track, priv->last_page_serialno, &s))
        {
        if(s && last_granulepos >= 0)
          {
          stream_priv = (stream_priv_t*)(s->priv);
          stream_priv->last_granulepos = last_granulepos;
          }
        break;
        }
      
      /* Check, if there are tracks left to find */
      result = find_next_track(ctx, result, last_track);
      if(result == -1)
        break;
      }

    /* Get the end positions of all tracks */

    track_priv_1 = (track_priv_t*)(ctx->tt->tracks[0].priv);
    for(i = 0; i < ctx->tt->num_tracks; i++)
      {
      if(i == ctx->tt->num_tracks-1)
        track_priv_1->end_pos = ctx->input->total_bytes;
      else
        {
        track_priv_2 = (track_priv_t*)(ctx->tt->tracks[i+1].priv);
        track_priv_1->end_pos = track_priv_2->start_pos;
        track_priv_1 = track_priv_2;
        }
      }

    /* Get the last granulepositions for streams, which, don't have one
       yet */

    for(i = 0; i < ctx->tt->num_tracks; i++)
      get_last_granulepos(ctx, &ctx->tt->tracks[i]);

    /* Get the duration and reset all streams */

    for(i = 0; i < ctx->tt->num_tracks; i++)
      {
      for(j = 0; j < ctx->tt->tracks[i].num_audio_streams; j++)
        {
        stream_priv =
          (stream_priv_t*)(ctx->tt->tracks[i].audio_streams[j].priv);

        ogg_stream_reset(&stream_priv->os);
        
        stream_duration =
          granulepos_2_time(&ctx->tt->tracks[i].audio_streams[j],
                            stream_priv->last_granulepos);
        if((ctx->tt->tracks[i].duration == GAVL_TIME_UNDEFINED) ||
           (ctx->tt->tracks[i].duration < stream_duration))
          ctx->tt->tracks[i].duration = stream_duration;
        }

      for(j = 0; j < ctx->tt->tracks[i].num_video_streams; j++)
        {
        stream_priv =
          (stream_priv_t*)(ctx->tt->tracks[i].video_streams[j].priv);

        ogg_stream_reset(&stream_priv->os);
        
        stream_duration =
          granulepos_2_time(&ctx->tt->tracks[i].video_streams[j],
                            stream_priv->last_granulepos);
        if((ctx->tt->tracks[i].duration == GAVL_TIME_UNDEFINED) ||
           (ctx->tt->tracks[i].duration < stream_duration))
          ctx->tt->tracks[i].duration = stream_duration;
        }
      for(j = 0; j < ctx->tt->tracks[i].num_subtitle_streams; j++)
        {
        if(ctx->tt->tracks[i].subtitle_streams[j].data.subtitle.subreader)
          continue;
    
        stream_priv =
          (stream_priv_t*)(ctx->tt->tracks[i].subtitle_streams[j].priv);

        ogg_stream_reset(&stream_priv->os);
        }
      
      get_metadata(&ctx->tt->tracks[i]);

      /* If we have more than one track,. we'll want the track name from the
         metadata */
      if(ctx->tt->num_tracks > 1)
        {
        if(ctx->tt->tracks[i].metadata.artist && ctx->tt->tracks[i].metadata.title)
          ctx->tt->tracks[i].name = bgav_sprintf("%s - %s",
                                                 ctx->tt->tracks[i].metadata.artist,
                                                 ctx->tt->tracks[i].metadata.title);
        else if(ctx->tt->tracks[i].metadata.title)
          ctx->tt->tracks[i].name = bgav_sprintf("%s",
                                                 ctx->tt->tracks[i].metadata.title);
        
        else
          ctx->tt->tracks[i].name = bgav_sprintf("Track %d", i+1);
        }
      }
    }
  else /* Streaming case */
    {
    if(ctx->input->metadata.title)
      ctx->tt->tracks[0].name = bgav_strdup(ctx->input->metadata.title);
    get_metadata(&ctx->tt->tracks[0]);

    /* Set end position to -1 */
    track_priv_1 = (track_priv_t*)(ctx->tt->tracks[0].priv);
    track_priv_1->end_pos = -1;

    priv->is_live = 1;
    }
  
  //  dump_ogg(ctx);
  
  if(ctx->input->input->seek_byte)
    ctx->flags |= (BGAV_DEMUXER_CAN_SEEK|BGAV_DEMUXER_SEEK_ITERATIVE);
  ctx->stream_description = bgav_strdup("Ogg bitstream");
  ctx->index_mode = INDEX_MODE_MIXED;
  return 1;
  }

static int new_streaming_track(bgav_demuxer_context_t * ctx)
  {
  uint32_t fourcc;
  stream_priv_t * stream_priv;
  ogg_stream_state os;
  int serialno;
  int done, audio_done, video_done;
  ogg_t * priv = ctx->priv;
    
  /*
   *  Ok, we try to get the new stuff, update the serial numbers from
   *  the streams, and otherwise do as if nothing had happened...
   */

  /* Right now, we accept at most 1 audio and 1 video stream. */
  if((ctx->tt->cur->num_audio_streams > 1) ||
     (ctx->tt->cur->num_video_streams > 1))
    return 0;

  audio_done = ctx->tt->cur->num_audio_streams ? 0 : 1;
  video_done = ctx->tt->cur->num_video_streams ? 0 : 1;
    
  /* Get the identification headers of the streams */
  while(ogg_page_bos(&priv->current_page))
    {
    serialno = ogg_page_serialno(&priv->current_page);
    
    ogg_stream_init(&os, serialno);
    ogg_stream_pagein(&os, &priv->current_page);
    priv->page_valid = 0;


    if(ogg_stream_packetout(&os, &priv->op) != 1)
      return 0;
    
    fourcc = detect_stream(&priv->op);

    /* Check which stream this could be */

    done = 0;
    
    if(!audio_done)
      {
      stream_priv = (stream_priv_t*)(ctx->tt->cur->audio_streams->priv);
      if(fourcc == stream_priv->fourcc_priv)
        {
        ogg_stream_init(&stream_priv->os, serialno);
        ctx->tt->cur->audio_streams->stream_id = serialno;
        done = 1;
        audio_done = 1;
        }
      }
    if(!done && !video_done)
      {
      stream_priv = (stream_priv_t*)(ctx->tt->cur->video_streams->priv);
      if(fourcc == stream_priv->fourcc_priv)
        {
        ogg_stream_init(&stream_priv->os, serialno);
        ctx->tt->cur->video_streams->stream_id = serialno;
        done = 1;
        video_done = 1;
        }
      }
    if(!get_page(ctx))
      return 0;
    }
  return 1;
  }

static char * get_name(bgav_metadata_t * m)
  {
  if(m->artist && m->title)
    return bgav_sprintf("%s - %s", m->artist, m->title);
  else if(m->title)
    return bgav_sprintf("%s", m->title);
  return NULL;
  }

static void metadata_changed(bgav_demuxer_context_t * ctx)
  {
  char * name;
  ogg_t * priv = ctx->priv;

  if(ctx->opt->metadata_change_callback || ctx->opt->name_change_callback)
    {
    get_metadata(ctx->tt->cur);
    bgav_metadata_merge2(&ctx->tt->cur->metadata, &ctx->input->metadata);
    }
  
  if(ctx->opt->metadata_change_callback)
    {
    ctx->opt->metadata_change_callback(ctx->opt->metadata_change_callback_data,
                                       &ctx->tt->cur->metadata);
    }
  if(ctx->opt->name_change_callback)
    {
    name = get_name(&ctx->tt->cur->metadata);
    if(name)
      {
      ctx->opt->name_change_callback(ctx->opt->name_change_callback_data,
                                     name);
      free(name);
      }
    }
  priv->metadata_changed = 0;
  }

static void set_packet_pos(ogg_t * op, stream_priv_t * sp,
                           int * page_continued,
                           bgav_packet_t * p)
  {
  if(*page_continued)
    {
    p->position = sp->prev_page_pos;
    *page_continued = 0;
    }
  else
    {
    p->position = op->current_page_pos;
    }
  /* Next pages will start here or later */
  sp->prev_page_pos = op->current_page_pos;
  }

static int check_header_packet(ogg_t * op, bgav_stream_t * s, ogg_packet * p)
  {
  stream_priv_t * sp = (stream_priv_t *)s->priv;
  switch(sp->fourcc_priv)
    {
    case FOURCC_THEORA:
      /* Skip header packets */
      if(p->packet[0] & 0x80)
        {
        if(op->is_live && (p->packetno == 1))
          {
          parse_vorbis_comment(s, p->packet + 7, p->bytes - 7);
          op->metadata_changed = 1;
          }
        return 0;
        }
      return 1;
      break;
    case FOURCC_DIRAC:
      {
      /* Dirac header packet is a sequence
         header followed by a sequence end code */
      int len, next, code;
      uint8_t * ptr = p->packet;
      len = p->bytes;
      
      code = bgav_dirac_get_code(ptr, len, &next);
      if(code != DIRAC_CODE_SEQUENCE)
        return 1;

      ptr += next;
      len -= next;

      if(!len)
        return 1;
      
      code = bgav_dirac_get_code(ptr, len, &next);
      if(code != DIRAC_CODE_END)
        return 1;
      
      return 0;
      }
      break;
    case FOURCC_OGM_VIDEO:
      if(p->packet[0] & 0x01) /* Header is already read -> skip it */
        return 0;
      return 1;
      break;
    case FOURCC_VORBIS:
      if(p->packet[0] & 0x01)
        {
        /* Get metadata */
        if(op->is_live && (p->packetno == 1))
          {
          parse_vorbis_comment(s, p->packet + 7, p->bytes - 7);
          op->metadata_changed = 1;
          }
        return 0;
        }
      return 1;
      break;
    case FOURCC_FLAC:
    case FOURCC_FLAC_NEW:
      /* Skip anything but audio frames */
      if((p->packet[0] != 0xff) || ((p->packet[1] & 0xfc) != 0xf8))
        return 0;
      return 1;
      break;
    case FOURCC_SPEEX:
      if(p->packetno < sp->header_packets_needed)
        return 0;
      return 1;
      break;
    case FOURCC_OGM_TEXT:
      if(p->packet[0] & 0x01) /* Header is already read -> skip it */
        return 0;
      if(!(p->packet[0] & 0x08))
        return 0;
      return 1;
      break;
      
    }
  return 1;
  }

static int next_packet_ogg(bgav_demuxer_context_t * ctx)
  {
  int len_bytes, i;
  int64_t iframes;
  int64_t pframes;
  int serialno;
  bgav_packet_t * p = NULL;
  bgav_stream_t * s;
  int64_t granulepos;
  stream_priv_t * stream_priv = NULL;
  ogg_t * priv = ctx->priv;
  int subtitle_duration;
  int page_continued;

  int ret = 0;
  int done = 0;

  while(!done)
    {
    if(!get_page(ctx))
      return 0;

    ret = 1;
    
    serialno   = ogg_page_serialno(&priv->current_page);
    granulepos = ogg_page_granulepos(&priv->current_page);
    
    if(ogg_page_bos(&priv->current_page))
      {
      if(!ctx->input->input->seek_byte && priv->nonbos_seen)
        {
        if(!new_streaming_track(ctx))
          return 0;
        else
          serialno = ogg_page_serialno(&priv->current_page);
        }
      }
    else
      {
      serialno = ogg_page_serialno(&priv->current_page);
      priv->nonbos_seen = 1;
      }
  
    s = bgav_track_find_stream(ctx, serialno);

    if(!s)
      {
      if(!ctx->next_packet_pos ||
         (ctx->input->position >= ctx->next_packet_pos))
        done = 1;
      
      priv->page_valid = 0;
      continue;
      }

    stream_priv = s->priv;
    
    page_continued = ogg_page_continued(&priv->current_page);
  
    ogg_stream_pagein(&stream_priv->os, &priv->current_page);
    priv->page_valid = 0;
    
    // http://xiph.org/ogg/doc/framing.html
    // A special value of '-1' (in two's complement) indicates that no
    // packets finish on this page.

    if(granulepos == -1)
      {
      if(!ctx->next_packet_pos ||
         (ctx->input->position >= ctx->next_packet_pos))
        done = 1;
      continue;
      }
    while(ogg_stream_packetout(&stream_priv->os, &priv->op) == 1)
      {
      // fprintf(stderr, "got packet %d bytes\n", priv->op.bytes);
      // bgav_hexdump(priv->op.packet, 16, 16);

      switch(stream_priv->fourcc_priv)
        {
        case FOURCC_THEORA:
          if(stream_priv->do_sync)
            {
            //            fprintf(stderr, "Resync theora %ld\n",
            //                    stream_priv->prev_granulepos);
            if((stream_priv->prev_granulepos == -1) &&
               (stream_priv->frame_counter < 0))
              break;
            else
              {
              if((stream_priv->prev_granulepos >= 0) &&
                 (stream_priv->frame_counter < 0))
                {
                iframes =
                  stream_priv->prev_granulepos >> stream_priv->keyframe_granule_shift;
                pframes =
                  stream_priv->prev_granulepos-(iframes<<stream_priv->keyframe_granule_shift);

                stream_priv->frame_counter = pframes + iframes;

                STREAM_SET_SYNC(s, stream_priv->frame_counter *
                                s->data.video.format.frame_duration);
                }
            
              if(stream_priv->frame_counter >= 0)
                {
                if(!(priv->op.packet[0] & 0x40))
                  {
                  stream_priv->do_sync = 0;
                  
                  }
                else
                  {
                  stream_priv->frame_counter++;
                  break;
                  }
                }
              }
            }
        
          if(!check_header_packet(priv, s, &priv->op))
            break;
        
          p = bgav_stream_get_packet_write(s);
          bgav_packet_alloc(p, priv->op.bytes);
          memcpy(p->data, priv->op.packet, priv->op.bytes);
          p->data_size = priv->op.bytes;

          if(!(priv->op.packet[0] & 0x40))
            {
            PACKET_SET_KEYFRAME(p);
            PACKET_SET_CODING_TYPE(p, BGAV_CODING_TYPE_I);
            }
          else
            PACKET_SET_CODING_TYPE(p, BGAV_CODING_TYPE_P);

          // fprintf(stderr, "Granulepos: %lld\n", priv->op.granulepos);
          
          p->pts = stream_priv->frame_counter *
            s->data.video.format.frame_duration;
          p->duration = s->data.video.format.frame_duration;
          stream_priv->frame_counter++;

          if(priv->op.e_o_s)
            p->flags |= PACKET_FLAG_LAST;
          else
            p->flags &= ~PACKET_FLAG_LAST;
          
          set_packet_pos(priv, stream_priv, &page_continued, p);
          
          bgav_stream_done_packet_write(s, p);
          break;

        case FOURCC_DIRAC:
          if(stream_priv->do_sync)
            {
            if(stream_priv->prev_granulepos == -1)
              break;
            else
              {
              /* Set sync time */
              if(priv->op.granulepos >= 0)
                STREAM_SET_SYNC(s, granulepos_2_time(s, priv->op.granulepos));
              
              /* Check for keyframe */
              if(!((priv->op.granulepos >> 22) & 0xff) &&
                 !(priv->op.granulepos & 0xff))
                stream_priv->do_sync = 0;
              else
                break;
              }
            }
        
          if(!check_header_packet(priv, s, &priv->op))
            break;
        
          p = bgav_stream_get_packet_write(s);
          bgav_packet_alloc(p, priv->op.bytes);
          memcpy(p->data, priv->op.packet, priv->op.bytes);
          p->data_size = priv->op.bytes;

          if(!((priv->op.granulepos >> 22) & 0xff) &&
             !(priv->op.granulepos & 0xff))
            PACKET_SET_KEYFRAME(p);
          
          p->pts = granulepos_2_time(s, priv->op.granulepos);
          p->dts = priv->op.granulepos >> 31;
          
          if(!stream_priv->interlaced_coding)
            p->dts /= 2;
          
          p->duration = s->data.video.format.frame_duration;
          
          set_packet_pos(priv, stream_priv, &page_continued, p);
          
          bgav_stream_done_packet_write(s, p);
          break;
        case FOURCC_OGM_VIDEO:
          if(stream_priv->do_sync)
            {
            /* Didn't get page yet */
            if(stream_priv->prev_granulepos == -1)
              break;
            else
              {
              if(stream_priv->frame_counter == -1)
                {
                stream_priv->frame_counter = stream_priv->prev_granulepos;
                }
              /* Skip non keyframes after seeking */
              if(!(priv->op.packet[0] & 0x08))
                {
                stream_priv->frame_counter++;
                break;
                }
              else
                {
                stream_priv->do_sync = 0;

                STREAM_SET_SYNC(s, (int64_t)s->data.video.format.frame_duration *
                                stream_priv->frame_counter);
                
                }
              }
            }
          if(!check_header_packet(priv, s, &priv->op))
            {
            break;
            }
          //        return 0;
          /* Parse subheader (let's hope there aren't any packets with
             more than one video frame) */
          len_bytes =
            (priv->op.packet[0] >> 6) |
            ((priv->op.packet[0] & 0x02) << 1);
        
          p = bgav_stream_get_packet_write(s);
        
          bgav_packet_alloc(p, priv->op.bytes - 1 - len_bytes);
          memcpy(p->data, priv->op.packet + 1 + len_bytes,
                 priv->op.bytes - 1 - len_bytes);
          p->data_size = priv->op.bytes - 1 - len_bytes;
          if(priv->op.packet[0] & 0x08)
            PACKET_SET_KEYFRAME(p);
          p->pts =
            s->data.video.format.frame_duration * stream_priv->frame_counter;
          
          stream_priv->frame_counter++;

          if(s->action == BGAV_STREAM_PARSE)
            s->duration =
              s->data.video.format.frame_duration * stream_priv->frame_counter;
        
          set_packet_pos(priv, stream_priv, &page_continued, p);
          bgav_stream_done_packet_write(s, p);
          break;
        case FOURCC_VORBIS:
          /* Resync if necessary */
          if(stream_priv->do_sync)
            {
#if 0
            if(priv->is_ogm)
              {
              if(priv->op.granulepos < 0)
                break;
              stream_priv->do_sync = 0;
              STREAM_SET_SYNC(s, priv->op.granulepos);
              // fprintf(stderr, "set_sync_ogm: %ld\n", priv->op.granulepos);
              }
            else
#endif
            if(stream_priv->prev_granulepos == -1)
              break;
            else
              {
              stream_priv->do_sync = 0;
              STREAM_SET_SYNC(s, stream_priv->prev_granulepos);
              }
            }
          
          if(!check_header_packet(priv, s, &priv->op))
            break;

          //          fprintf(stderr, "ogm granulepos: %ld\n", priv->op.granulepos);
          
          p = bgav_stream_get_packet_write(s);
          set_packet_pos(priv, stream_priv, &page_continued, p);
          PACKET_SET_KEYFRAME(p);
          // s->packet->pts = stream_priv->prev_granulepos;
          
          bgav_packet_alloc(p, priv->op.bytes);
          memcpy(p->data, priv->op.packet, priv->op.bytes);
          p->data_size = priv->op.bytes;
          
          // fprintf(stderr, "priv->op.granulepos: %ld\n", priv->op.granulepos);

          /* Check whether to close this packet */

          /* Close this packet */
          if(priv->op.e_o_s)
            p->flags |= PACKET_FLAG_LAST;
          else
            p->flags &= ~PACKET_FLAG_LAST;
          
          bgav_stream_done_packet_write(s, p);
          
          break;
        case FOURCC_FLAC:
        case FOURCC_FLAC_NEW:
        case FOURCC_SPEEX:
          /* Resync if necessary */
          if(stream_priv->do_sync)
            {
            if(stream_priv->prev_granulepos == -1)
              {
              break;
              }
            else
              {
              stream_priv->do_sync = 0;
              STREAM_SET_SYNC(s, stream_priv->prev_granulepos);
              }
            }
        
          if(!check_header_packet(priv, s, &priv->op))
            break;
        
          p = bgav_stream_get_packet_write(s);
          bgav_packet_alloc(p, priv->op.bytes);
          memcpy(p->data, priv->op.packet, priv->op.bytes);
          p->data_size = priv->op.bytes;

          if(stream_priv->prev_granulepos >= 0)
            {
            p->pts = stream_priv->prev_granulepos;
            }
          
          if((s->action == BGAV_STREAM_PARSE) && (priv->op.granulepos > 0))
            s->duration = priv->op.granulepos;

          set_packet_pos(priv, stream_priv, &page_continued, p);
          bgav_stream_done_packet_write(s, p);
          break;
        case FOURCC_OGM_TEXT:
          if(priv->op.packet[0] & 0x01) /* Header is already read -> skip it */
            {
            break;
            }
          if(!(priv->op.packet[0] & 0x08))
            {
            break;
            }
        
          len_bytes =
            (priv->op.packet[0] >> 6) |
            ((priv->op.packet[0] & 0x02) << 1);
          subtitle_duration = 0;
          for(i = len_bytes; i > 0; i--)
            {
            subtitle_duration <<= 8;
            subtitle_duration |= priv->op.packet[i];
            }
          if((priv->op.packet[1 + len_bytes] == ' ') &&
             (priv->op.packet[2 + len_bytes] == '\0'))
            break;
        
          p = bgav_stream_get_packet_write(s);
          
          bgav_packet_set_text_subtitle(p, (char*)(priv->op.packet + 1 + len_bytes),
                                        -1, granulepos, subtitle_duration);
          set_packet_pos(priv, stream_priv, &page_continued, p);

          if(s->action == BGAV_STREAM_PARSE)
            s->duration = granulepos + subtitle_duration;
        
          bgav_stream_done_packet_write(s, p);
          break;
        }
      if(stream_priv)
        stream_priv->prev_granulepos = priv->op.granulepos;
      }

    if(p) /* True if we read non header data from the stream */
      {
      if(priv->metadata_changed)
        metadata_changed(ctx);
      }

    if(!ctx->next_packet_pos || (ctx->input->position >= ctx->next_packet_pos))
      done = 1;
    }

  
  return ret;
  }

static void reset_track(bgav_track_t * track, int bos)
  {
  stream_priv_t * stream_priv;
  int i;
  int position = bos ? 0 : -1;
  int do_sync = bos ? 0 : 1;
  
  for(i = 0; i < track->num_audio_streams; i++)
    {
    stream_priv =
      (stream_priv_t*)(track->audio_streams[i].priv);
    stream_priv->prev_granulepos = position;
    stream_priv->frame_counter = position;
    stream_priv->do_sync = do_sync;
    ogg_stream_reset(&stream_priv->os);

    if(bos)
      STREAM_SET_SYNC(&track->audio_streams[i], 0);
    }

  for(i = 0; i < track->num_video_streams; i++)
    {
    stream_priv =
      (stream_priv_t*)(track->video_streams[i].priv);

    stream_priv->prev_granulepos = position;
    stream_priv->frame_counter = position;
    stream_priv->do_sync = do_sync;
    ogg_stream_reset(&stream_priv->os);
    if(bos)
      STREAM_SET_SYNC(&track->video_streams[i], 0);

    }
  
  for(i = 0; i < track->num_subtitle_streams; i++)
    {
    if(track->subtitle_streams[i].data.subtitle.subreader)
      continue;
    stream_priv =
      (stream_priv_t*)(track->subtitle_streams[i].priv);
    ogg_stream_reset(&stream_priv->os);
    if(bos)
      STREAM_SET_SYNC(&track->subtitle_streams[i], 0);
    }
  }

static void resync_ogg(bgav_demuxer_context_t * ctx, bgav_stream_t * s)
  {
  stream_priv_t * stream_priv;

  stream_priv =
    (stream_priv_t*)(s->priv);

  switch(s->type)
    {
    case BGAV_STREAM_AUDIO:
      stream_priv->prev_granulepos = STREAM_GET_SYNC(s);
      ogg_stream_reset(&stream_priv->os);
      break;
    case BGAV_STREAM_VIDEO:
      stream_priv->frame_counter = STREAM_GET_SYNC(s) /
        s->data.video.format.frame_duration;
      ogg_stream_reset(&stream_priv->os);
      break;
    case BGAV_STREAM_SUBTITLE_TEXT:
      if(!s->data.subtitle.subreader)
        ogg_stream_reset(&stream_priv->os);
      break;
    case BGAV_STREAM_SUBTITLE_OVERLAY:
    case BGAV_STREAM_UNKNOWN:
      break;
    }
  }

/* Seeking in ogg: Gets quite complicated so we use iterative seeking */

static void seek_ogg(bgav_demuxer_context_t * ctx, int64_t time, int scale)
  {
  int i, done;
  track_priv_t * track_priv;
  stream_priv_t * stream_priv;
  int64_t filepos;

  //  fprintf(stderr, "seek_ogg %ld %d\n", time, scale);

  // Seeking to 0 is handled specially:
  // It happens if we want to seek into the first pages
  // (when prev_granulepos is not set yet)
  if(!time) 
    {
    seek_byte(ctx, 0);
    reset_track(ctx->tt->cur, 1);
    return;
    }
  
  /* Seek to the file position */
  track_priv = (track_priv_t*)(ctx->tt->cur->priv);
  filepos = track_priv->start_pos +
    (int64_t)((double)gavl_time_unscale(scale, time) /
              (double)(ctx->tt->cur->duration) *
              (track_priv->end_pos - track_priv->start_pos));
  if(filepos <= track_priv->start_pos)
    filepos = find_first_page(ctx, track_priv->start_pos, track_priv->end_pos,
                              NULL, NULL);
  else
    filepos = find_last_page(ctx, track_priv->start_pos, filepos,
                             NULL, NULL);
  seek_byte(ctx, filepos);

  /* Reset all the streams and set the stream times to -1 */
  reset_track(ctx->tt->cur, 0);
  
  /* Now, resync the streams (probably skipping lots of pages) */

  while(1)
    {
    /* Oops, reached EOF, go one page back */
    if(!next_packet_ogg(ctx)) 
      {
      filepos = find_last_page(ctx, track_priv->start_pos, filepos,
                               NULL, NULL);
      //      seek_byte(ctx, filepos);
      reset_track(ctx->tt->cur, 0);
      //      fprintf(stderr, "Reached EOF\n");
      }
    
    done = 1;

    for(i = 0; i < ctx->tt->cur->num_audio_streams; i++)
      {
      stream_priv =
        (stream_priv_t*)(ctx->tt->cur->audio_streams[i].priv);
      
      if(stream_priv->do_sync &&
         (ctx->tt->cur->audio_streams[i].action != BGAV_STREAM_MUTE))
        done = 0;
      }
    if(done)
      {
      for(i = 0; i < ctx->tt->cur->num_video_streams; i++)
        {
        stream_priv =
          (stream_priv_t*)(ctx->tt->cur->video_streams[i].priv);
        if(stream_priv->do_sync &&
           (ctx->tt->cur->video_streams[i].action != BGAV_STREAM_MUTE))
          done = 0;
        }
      
      }
    if(done)
      break;
    }
  }


static void close_ogg(bgav_demuxer_context_t * ctx)
  {
  int i;
  ogg_t * priv;
  track_priv_t * track_priv;
  
  for(i = 0; i < ctx->tt->num_tracks; i++)
    {
    track_priv = (track_priv_t*)(ctx->tt->tracks[i].priv);
    if(track_priv->unsupported_streams)
      free(track_priv->unsupported_streams);
    free(track_priv);
    }
  
  priv = ctx->priv;
  ogg_sync_clear(&priv->oy);

  /* IMPORTANT: The paket will be freed by the last ogg_stream */
  //  ogg_packet_clear(&priv->op);
  free(priv);
  }

static void init_track(bgav_track_t * track)
  {
  stream_priv_t * stream_priv;
  int i;
  
  for(i = 0; i < track->num_audio_streams; i++)
    {
    stream_priv =
      (stream_priv_t*)(track->audio_streams[i].priv);
    stream_priv->prev_granulepos = 0;
    stream_priv->frame_counter = 0;
    stream_priv->do_sync = 0;
    ogg_stream_reset(&stream_priv->os);
    }

  for(i = 0; i < track->num_video_streams; i++)
    {
    stream_priv =
      (stream_priv_t*)(track->video_streams[i].priv);
    stream_priv->prev_granulepos = 0;
    stream_priv->frame_counter = 0;
    stream_priv->do_sync = 0;
    ogg_stream_reset(&stream_priv->os);
    }
  
  for(i = 0; i < track->num_subtitle_streams; i++)
    {
    if(track->subtitle_streams[i].data.subtitle.subreader)
      continue;
    stream_priv =
      (stream_priv_t*)(track->subtitle_streams[i].priv);
    ogg_stream_reset(&stream_priv->os);
    }
  }

static int select_track_ogg(bgav_demuxer_context_t * ctx,
                             int track)
  {
  //  char * name;
  ogg_t * priv;
  track_priv_t * track_priv;
  
  priv = ctx->priv;
  
  track_priv = (track_priv_t*)(ctx->tt->cur->priv);
  
  if(ctx->input->input->seek_byte)
    {
    seek_byte(ctx, track_priv->start_pos);
    priv->end_pos = track_priv->end_pos;
    }
  init_track(ctx->tt->cur);
#if 0
    {
    if(ctx->opt->name_change_callback)
      {
      name = get_name(&ctx->tt->cur->metadata);
      if(name)
        {
        ctx->opt->name_change_callback(ctx->opt->name_change_callback_data,
                                       name);
        free(name);
        }
      }
    }
#endif
  return 1;
  }

const bgav_demuxer_t bgav_demuxer_ogg =
  {
    .probe =        probe_ogg,
    .open =         open_ogg,
    .next_packet =  next_packet_ogg,
    .seek =         seek_ogg,
    .resync =       resync_ogg,
    .close =        close_ogg,
    .select_track = select_track_ogg
  };

