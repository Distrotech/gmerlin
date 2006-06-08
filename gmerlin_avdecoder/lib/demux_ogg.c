/*****************************************************************
 
  demux_ogg.c
 
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

/* This demuxer supports 2 modes: Streaming and non-streaming.
   In non-streaming mode, we support all legal combinations
   of sequential and concurrent multiplexing, multiple tracks will
   be detected and can be selected induvidually.
   
   In streaming mode, we only support concurrent multiplexing. If new
   logical bitstreams start, we catch the new metadata and hope, that
   the new track has the same stream layout and format as before.
   
   As streams we support:
   - Vorbis
   - Theora
   - Speex
   - Flac
   - OGM video (Typically some divx variant)
*/


#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <avdec_private.h>
#include <vorbis_comment.h>

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

#define FOURCC_OGM_TEXT  BGAV_MK_FOURCC('T','E','X','T')

#define BYTES_TO_READ 8500 /* Same as in vorbisfile */

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
  } stream_priv_t;

typedef struct
  {
  ogg_sync_state  oy;
  ogg_page        current_page;
  ogg_packet      op;

  int64_t end_pos;
  int64_t data_start;
  //  int current_page_size;

  /* Serial  number of the last page of the entire file */
  int last_page_serialno;
  
  /* current_page is valid */
  int page_valid;
  
  /* Remember to call metadata_change and name_change callbacks */
  int metadata_changed;

  /* Different timestamp handling if the stream is live */
  int is_live;
  } ogg_priv;

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
    //    fprintf(stderr, "Found subtitles %lld %lld\n",
    //            input->position, input->total_bytes);
    //    bgav_input_skip_dump(input, input->total_bytes - input->position);
    return 1;
    }
  else
    {
    fprintf(stderr, "Unknown type %.8s\n", ret->type);
    return 0;
    }
  }
#if 0
static void ogm_header_dump(ogm_header_t * h)
  {
  fprintf(stderr, "OGM Header\n");
  fprintf(stderr, "  Type              %.8s\n", h->type);
  fprintf(stderr, "  Subtype:          ");
  bgav_dump_fourcc(h->subtype);
  fprintf(stderr, "\n");

  fprintf(stderr, "  Size:             %d\n", h->size);
  fprintf(stderr, "  Time unit:        %lld\n", h->time_unit);
  fprintf(stderr, "  Samples per unit: %lld\n", h->samples_per_unit);
  fprintf(stderr, "  Default len:      %d\n", h->default_len);
  fprintf(stderr, "  Buffer size:      %d\n", h->buffersize);
  fprintf(stderr, "  Bits per sample:  %d\n", h->bits_per_sample);
  if(!strncmp(h->type, "video", 5))
    {
    fprintf(stderr, "  Width:            %d\n", h->data.video.width);
    fprintf(stderr, "  Height:           %d\n", h->data.video.height);
    }
  if(!strncmp(h->type, "audio", 5))
    {
    fprintf(stderr, "  Channels:         %d\n", h->data.audio.channels);
    fprintf(stderr, "  Block align:      %d\n", h->data.audio.blockalign);
    fprintf(stderr, "  Avg bytes per sec: %d\n", h->data.audio.avgbytespersec);
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
  
  fprintf(stderr, "Ogg Structure\n");

  for(i = 0; i < ctx->tt->num_tracks; i++)
    {
    track = &(ctx->tt->tracks[i]);
    track_priv = (track_priv_t*)(track->priv);
    fprintf(stderr, "Track %d, start_pos: %lld, end_pos: %lld\n",
            i+1, track_priv->start_pos, track_priv->end_pos);
    
    for(j = 0; j < track->num_audio_streams; j++)
      {
      s = &track->audio_streams[j];
      stream_priv = (stream_priv_t*)(s->priv);
      fprintf(stderr, "Audio stream %d\n", j+1);
      fprintf(stderr, "  Serialno: %d\n", s->stream_id);
      fprintf(stderr, "  Language: %s\n", s->language);
      fprintf(stderr, "  Last granulepos: %lld\n",
              stream_priv->last_granulepos);
      fprintf(stderr, "  Metadata:\n");
      bgav_metadata_dump(&stream_priv->metadata);
      }
    for(j = 0; j < track->num_video_streams; j++)
      {
      s = &track->video_streams[j];
      stream_priv = (stream_priv_t*)(s->priv);
      fprintf(stderr, "Video stream %d\n", j+1);
      fprintf(stderr, "  Serialno: %d\n", s->stream_id);
      fprintf(stderr, "  Last granulepos: %lld\n",
              stream_priv->last_granulepos);
      fprintf(stderr, "  Metadata:\n");
      bgav_metadata_dump(&stream_priv->metadata);
      }
    for(j = 0; j < track->num_subtitle_streams; j++)
      {
      if(track->subtitle_streams[j].data.subtitle.subreader)
        continue;
      
      s = &track->subtitle_streams[j];
      stream_priv = (stream_priv_t*)(s->priv);
      fprintf(stderr, "Subtitle stream %d\n", j+1);
      fprintf(stderr, "  Serialno: %d\n", s->stream_id);
      fprintf(stderr, "  Language: %s\n", s->language);
      fprintf(stderr, "  Metadata:\n");
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
  input_mem = bgav_input_open_memory(data, len);

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
  //  fprintf(stderr, "Got vorbis comment\n");
  //  bgav_vorbis_comment_dump(&vc);
  
  bgav_vorbis_comment_free(&vc);
  bgav_input_destroy(input_mem);
  }

/* Seek byte and reset the synchronizer */

static void seek_byte(bgav_demuxer_context_t * ctx, int64_t pos)
  {
  ogg_priv * priv = (ogg_priv*)(ctx->priv);
  ogg_sync_reset(&(priv->oy));
  //  ogg_page_clear(&(priv->os));
  bgav_input_seek(ctx->input, pos, SEEK_SET);
  priv->page_valid = 0;
  }

/* Get new data */

static int get_data(bgav_demuxer_context_t * ctx)
  {
  int bytes_to_read;
  char * buf;
  int result;
  ogg_priv * priv = (ogg_priv*)(ctx->priv);

  
  bytes_to_read = BYTES_TO_READ;
  if(priv->end_pos > 0)
    {
    //    fprintf(stderr, "Get data %lld %d %lld\n",
    //            ctx->input->position, bytes_to_read, priv->end_pos);
    
    if(ctx->input->position + bytes_to_read > priv->end_pos)
      bytes_to_read = priv->end_pos - ctx->input->position;
    if(bytes_to_read <= 0)
      return 0;
    }
  buf = ogg_sync_buffer(&(priv->oy), bytes_to_read);
  result = bgav_input_read_data(ctx->input, (uint8_t*)buf, bytes_to_read);

  //  fprintf(stderr, "Get data\n");
  //  bgav_hexdump(buf, 512, 16);
  
  ogg_sync_wrote(&(priv->oy), result);
  return result;
  }

/* Get new page */

static int get_page(bgav_demuxer_context_t * ctx)
  {
  ogg_priv * priv = (ogg_priv*)(ctx->priv);

  if(priv->page_valid)
    return 1;

  while(ogg_sync_pageout(&(priv->oy), &(priv->current_page)) != 1)
    {
    if(!get_data(ctx))
      return 0;
    }
  //  fprintf(stderr, "Got page: %lld\n", ogg_page_granulepos(&(priv->current_page)));
  priv->page_valid = 1;
  return 1;
  }

/* Append extradata to a stream */

static void append_extradata(bgav_stream_t * s, ogg_packet * op)
  {
  s->ext_data = realloc(s->ext_data, s->ext_size + sizeof(*op) + op->bytes);
  memcpy(s->ext_data + s->ext_size, op, sizeof(*op));
  memcpy(s->ext_data + s->ext_size + sizeof(*op), op->packet, op->bytes);
  s->ext_size += (sizeof(*op) + op->bytes);
  //  fprintf(stderr, "Appended extradata %ld bytes, fourcc: ", (sizeof(*op) + op->bytes));
  //  bgav_dump_fourcc(s->fourcc);
  //  fprintf(stderr, "\n");
  //  bgav_hexdump(op->packet, op->bytes, 16);
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
  
  return 0;
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
  ogg_priv * priv;
  ogm_header_t ogm_header;
  bgav_input_context_t * input_mem;
    
  priv = (ogg_priv *)(ctx->priv);

  ogg_track = calloc(1, sizeof(*ogg_track));
  ogg_track->start_pos = start_position;
  track->priv = ogg_track;

  //  fprintf(stderr, "Setting up track at %lld\n", start_position);

  /* If we can seek, seek to the start point. If we can't seek, we are already
     at the right position */
  if(ctx->input->input->seek_byte)
    seek_byte(ctx, start_position);
  
  /* Get the first page of each stream */
  while(1)
    {
    if(!get_page(ctx))
      {
      fprintf(stderr, "EOF while setting up track\n");
      return 0;
      }
    if(!ogg_page_bos(&(priv->current_page)))
      {
      //      fprintf(stderr, "Found first non bos page %d\n",
      //              ogg_page_pageno(&(priv->current_page)));
      priv->page_valid = 1;
      break;
      }
    /* Setup stream */
    serialno = ogg_page_serialno(&(priv->current_page));
    //    fprintf(stderr, "Serialno: %d\n", serialno);
    
    ogg_stream = calloc(1, sizeof(*ogg_stream));
    ogg_stream->last_granulepos = -1;
    
    ogg_stream_init(&ogg_stream->os, serialno);
    ogg_stream_pagein(&ogg_stream->os, &(priv->current_page));
    priv->page_valid = 0;
    
    if(ogg_stream_packetout(&ogg_stream->os, &priv->op) != 1)
      {
      fprintf(stderr, "Cannot get first packet of stream\n");
      return 0;
      }

    ogg_stream->fourcc_priv = detect_stream(&priv->op);
    
    //    fprintf(stderr, "First packet (%ld bytes):\n", priv->op.bytes);
    //    bgav_hexdump(priv->op.packet, priv->op.bytes, 16);

    switch(ogg_stream->fourcc_priv)
      {
      case FOURCC_VORBIS:
        //        fprintf(stderr, "Detected Vorbis data\n");
        s = bgav_track_add_audio_stream(track, ctx->opt);
        s->fourcc = FOURCC_VORBIS;
        
        s->priv   = ogg_stream;
        s->stream_id = serialno;

        ogg_stream->header_packets_needed = 3;
        append_extradata(s, &priv->op);
        ogg_stream->header_packets_read = 1;

        /* Get samplerate */
        s->data.audio.format.samplerate =
          BGAV_PTR_2_32LE(priv->op.packet + 12);
      
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
        //        fprintf(stderr, "Detected Theora data\n");
        s = bgav_track_add_video_stream(track, ctx->opt);
        s->fourcc = FOURCC_THEORA;
        s->priv   = ogg_stream;
        s->stream_id = serialno;
        ogg_stream->header_packets_needed = 3;
        append_extradata(s, &priv->op);
        ogg_stream->header_packets_read = 1;

        /* Get fps and keyframe shift */
        s->data.video.format.timescale = BGAV_PTR_2_32BE(priv->op.packet+22);
        s->data.video.format.frame_duration =
          BGAV_PTR_2_32BE(priv->op.packet+26);

        ogg_stream->keyframe_granule_shift =
          (char) ((priv->op.packet[40] & 0x03) << 3);

        ogg_stream->keyframe_granule_shift |=
          (priv->op.packet[41] & 0xe0) >> 5;
#if 0
        fprintf(stderr, "Got Granule shift: %d, fps: %d:%d\n",
                ogg_stream->keyframe_granule_shift,
                s->data.video.format.timescale,
                s->data.video.format.frame_duration);
#endif
        /* Read remaining header packets from this page */
        while(ogg_stream_packetout(&ogg_stream->os, &priv->op) == 1)
          {
          append_extradata(s, &priv->op);
          ogg_stream->header_packets_read++;
          if(ogg_stream->header_packets_read == ogg_stream->header_packets_needed)
            break;
          }
        break;
      case FOURCC_FLAC_NEW:
        fprintf(stderr, "Detected FLAC data (new format), serialno: %d\n", serialno);
        s = bgav_track_add_audio_stream(track, ctx->opt);
        s->fourcc = FOURCC_FLAC;
        s->priv   = ogg_stream;

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

        //        fprintf(stderr, "Header packets: %d\n", ogg_stream->header_packets_needed);
        
        break;
      case FOURCC_FLAC:
        //        fprintf(stderr, "Detected FLAC data (old format)\n");
        s = bgav_track_add_audio_stream(track, ctx->opt);
        s->fourcc = FOURCC_FLAC;
        ogg_stream->fourcc_priv = FOURCC_FLAC;
        s->priv   = ogg_stream;
        s->stream_id = serialno;
        
        ogg_stream->header_packets_needed = 1;
        ogg_stream->header_packets_read = 0;
        
        while(ogg_stream_packetout(&ogg_stream->os, &priv->op) == 1)
          {
          //        fprintf(stderr, "FLAC extradata %ld bytes\n", priv->op.bytes);
          //        bgav_hexdump(priv->op.packet, priv->op.bytes, 16);
          }
        break;
      case FOURCC_SPEEX:
        //        fprintf(stderr, "Detected Speex data (header size: %ld)\n", priv->op.bytes);
        s = bgav_track_add_audio_stream(track, ctx->opt);
        s->fourcc = FOURCC_SPEEX;
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
          //          fprintf(stderr, "Speex extradata %ld bytes\n", priv->op.bytes);
          //          bgav_hexdump(priv->op.packet, priv->op.bytes, 16);
          }
        break;
      case FOURCC_OGM_VIDEO:
        //        fprintf(stderr, "Detected OGM video data\n");
        
        s = bgav_track_add_video_stream(track, ctx->opt);
      
        s->priv   = ogg_stream;
        s->stream_id = serialno;

        input_mem = bgav_input_open_memory(priv->op.packet + 1, priv->op.bytes - 1);

        if(!ogm_header_read(input_mem, &ogm_header))
          {
          fprintf(stderr, "Reading OGM header failed\n");
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
        ogg_stream->header_packets_needed = 2;
        ogg_stream->header_packets_read = 1;
        break;
      case FOURCC_OGM_TEXT:
        //        fprintf(stderr, "Detected OGM text data\n");
        s = bgav_track_add_subtitle_stream(track, ctx->opt, 1,
                                           (const char*)0);
        s->priv   = ogg_stream;
        s->stream_id = serialno;

        input_mem = bgav_input_open_memory(priv->op.packet + 1, priv->op.bytes - 1);

        if(!ogm_header_read(input_mem, &ogm_header))
          {
          fprintf(stderr, "Reading OGM header failed\n");
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
        ogg_stream->header_packets_needed = 2;
        ogg_stream->header_packets_read = 1;
        break;
      default:
#if 1
        fprintf(stderr,
                "Warning, unsupported stream (serialno: %d), first bytes:\n",
                serialno);
        bgav_hexdump(priv->op.packet,
                     priv->op.bytes < 16 ? priv->op.bytes : 16, 16);
#endif
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
#if 0
    fprintf(stderr, "Got header page (no: %ld, packets: %d, serialno: %d)\n",
            ogg_page_pageno(&(priv->current_page)),
            ogg_page_packets(&(priv->current_page)),
            ogg_page_serialno(&(priv->current_page)));
#endif
    s = bgav_track_find_stream_all(track, ogg_page_serialno(&(priv->current_page)));

    if(s)
      {
      ogg_stream = (stream_priv_t*)(s->priv);
      ogg_stream_pagein(&(ogg_stream->os), &(priv->current_page));
      priv->page_valid = 0;
  
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
              //              fprintf(stderr, "Found vorbis comment in new flac header\n");
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
                // fprintf(stderr, "FLAC extradata %ld bytes\n", priv->op.bytes);
                // bgav_hexdump(priv->op.packet, priv->op.bytes, 16);
                if(s->ext_data)
                  {
                  fprintf(stderr, "Error, flac extradata already set\n");
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
                //                fprintf(stderr, "Flac samplerate: %d\n", 
                //                        s->data.audio.format.samplerate);
                      
                break;
              case 1:
                parse_vorbis_comment(s, priv->op.packet+4, priv->op.bytes-4);
                // fprintf(stderr, "Found vorbis comment in old flac header\n");
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
      fprintf(stderr, "Unsupported header packet\n");
      bgav_hexdump(priv->current_page.body, priv->current_page.body_len, 16);
      priv->page_valid = 0;
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
        fprintf(stderr, "EOF while setting up track\n");
        return 0;
        }
      }
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
  ogg_priv * priv = (ogg_priv*)(ctx->priv);

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
        *serialno = ogg_page_serialno(&(priv->current_page));
      if(granulepos)
        *granulepos = ogg_page_granulepos(&(priv->current_page));
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
    //    fprintf(stderr, "find_last_page %lld %lld...", start_pos, pos2);    
    page_pos = find_first_page(ctx, start_pos, pos2, &this_serialno,
                               &this_granulepos);
    
    //    fprintf(stderr, "Pos: %lld\n", page_pos);
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
    *s = (bgav_stream_t*)0;
  
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
  ogg_priv * priv;
  int serialno_1, serialno_2;
  bgav_stream_t * s;
  stream_priv_t * stream_priv;
  
  priv = (ogg_priv *)(ctx->priv);
  
  /* Do bisection search */
  pos1 = start_pos;
  pos2 = ctx->input->total_bytes;

  while(1)
    {
    test_pos = (pos1 + pos2) / 2;

    //    fprintf(stderr, "Find next track %lld %lld %lld\n",
    //            pos1, pos2, test_pos);
    
    page_pos_1 = find_last_page(ctx, start_pos, test_pos, &serialno_1,
                                &granulepos_1);
    //    fprintf(stderr, "page_pos_1: %lld\n", page_pos_1);
    //    fprintf(stderr, "page_1: pos: %lld, serialno: %d\n", page_pos_1, serialno_1);
    
    if(page_pos_1 < 0)
      return -1;
    
    if(!track_has_serialno(last_track, serialno_1, &s))
      {
      pos2 = page_pos_1;
      continue;
      }

    page_pos_2 = find_first_page(ctx, test_pos,
                                 ctx->input->total_bytes, &serialno_2,
                                 (int64_t*)0);
    //    fprintf(stderr, "page_pos_2: %lld\n", page_pos_2);
    //    fprintf(stderr, "page_2: pos: %lld, serialno: %d\n", page_pos_2, serialno_2);

    if(page_pos_2 < 0)
      return -1;
    
    if(track_has_serialno(last_track, serialno_2, (bgav_stream_t**)0))
      {
      //      pos1 = test_pos;
      pos1 = page_pos_2;
      continue;
      }

    if(!ogg_page_bos(&priv->current_page))
      {
      fprintf(stderr, "Error: Page has no BOS marker\n");
      return 0;
      }

    //    fprintf(stderr, "Page number: %d\n", ogg_page_pageno(&priv->current_page));
    
    new_track = bgav_track_table_append_track(ctx->tt);
    if(!setup_track(ctx, new_track, page_pos_2))
      {
      fprintf(stderr, "Error: Setting up track failed\n");
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

      frames =
        pos >> stream_priv->keyframe_granule_shift;
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
    //    fprintf(stderr, "get_last_granulepos %lld\n",
    //            pos);
    
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

    s = (bgav_stream_t*)0;
    if(track_has_serialno(track, serialno, &s) && s)
      {
      stream_priv = (stream_priv_t*)(s->priv);
      if(stream_priv->last_granulepos < 0)
        stream_priv->last_granulepos = granulepos;
      //      fprintf(stderr, "Got granulepos: %lld %lld\n",
      //              stream_priv->last_granulepos, granulepos);
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


static int open_ogg(bgav_demuxer_context_t * ctx,
                    bgav_redirector_context_t ** redir)
  {
  gavl_time_t stream_duration;
  int i, j;
  track_priv_t * track_priv_1, * track_priv_2;
  bgav_track_t * last_track;
  bgav_stream_t * s;
  stream_priv_t * stream_priv;
  int64_t result, last_granulepos;
  ogg_priv * priv;
  
  priv = calloc(1, sizeof(*priv));
  ctx->priv = priv;
  
  ogg_sync_init(&(priv->oy));

  ctx->tt = bgav_track_table_create(1);
  priv->data_start = ctx->input->position;
  
  /* Set up the first track */
  if(!setup_track(ctx, ctx->tt->current_track, priv->data_start))
    return 0;
  
  if(ctx->input->input->seek_byte && ctx->input->total_bytes)
    {
    /* Get the last page of the stream and check for the serialno */

    //    fprintf(stderr, "Getting last page...");
    if(find_last_page(ctx, 0, ctx->input->total_bytes,
                      &priv->last_page_serialno,
                      &last_granulepos) < 0)
      {
      //      fprintf(stderr, "failed\n");
      return 0;
      }
    //    fprintf(stderr, "done\n");
    //    fprintf(stderr, "Last page: serialno: %d, granulepos: %lld\n",
    //            priv->last_page_serialno, last_granulepos);
    result = priv->data_start;
    while(1)
      {
      last_track = &(ctx->tt->tracks[ctx->tt->num_tracks-1]);

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
      get_last_granulepos(ctx, &(ctx->tt->tracks[i]));

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
    ctx->can_seek = 1;
  ctx->stream_description = bgav_strdup("Ogg bitstream");
  ctx->seek_iterative = 1;
  
  return 1;
  }

static int new_streaming_track(bgav_demuxer_context_t * ctx)
  {
  uint32_t fourcc;
  stream_priv_t * stream_priv;
  ogg_stream_state os;
  int serialno;
  int done, audio_done, video_done;
  ogg_priv * priv = (ogg_priv*)(ctx->priv);
    
  /*
   *  Ok, we try to get the new stuff, update the serial numbers from
   *  the streams, and otherwise do as if nothing had happened...
   */

  /* Right now, we accept at most 1 audio and 1 video stream. */
  if((ctx->tt->current_track->num_audio_streams > 1) ||
     (ctx->tt->current_track->num_video_streams > 1))
    return 0;

  audio_done = ctx->tt->current_track->num_audio_streams ? 0 : 1;
  video_done = ctx->tt->current_track->num_video_streams ? 0 : 1;
    
  /* Get the identification headers of the streams */
  while(ogg_page_bos(&priv->current_page))
    {
    serialno = ogg_page_serialno(&(priv->current_page));
    
    ogg_stream_init(&os, serialno);
    ogg_stream_pagein(&os, &(priv->current_page));
    priv->page_valid = 0;


    if(ogg_stream_packetout(&os, &priv->op) != 1)
      return 0;
    
    fourcc = detect_stream(&priv->op);

    /* Check which stream this could be */

    done = 0;
    
    if(!audio_done)
      {
      stream_priv = (stream_priv_t*)(ctx->tt->current_track->audio_streams->priv);
      if(fourcc == stream_priv->fourcc_priv)
        {
        ogg_stream_init(&stream_priv->os, serialno);
        ctx->tt->current_track->audio_streams->stream_id = serialno;
        done = 1;
        audio_done = 1;
        }
      }
    if(!done && !video_done)
      {
      stream_priv = (stream_priv_t*)(ctx->tt->current_track->video_streams->priv);
      if(fourcc == stream_priv->fourcc_priv)
        {
        ogg_stream_init(&stream_priv->os, serialno);
        ctx->tt->current_track->video_streams->stream_id = serialno;
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
  return (char*)0;
  }

static void metadata_changed(bgav_demuxer_context_t * ctx)
  {
  char * name;
  ogg_priv * priv = (ogg_priv*)(ctx->priv);

  if(ctx->opt->metadata_change_callback || ctx->opt->name_change_callback)
    {
    get_metadata(ctx->tt->current_track);
    bgav_metadata_merge2(&ctx->tt->current_track->metadata, &(ctx->input->metadata));
    }
  
  if(ctx->opt->metadata_change_callback)
    {
    ctx->opt->metadata_change_callback(ctx->opt->metadata_change_callback_data,
                                       &ctx->tt->current_track->metadata);
    }
  if(ctx->opt->name_change_callback)
    {
    name = get_name(&ctx->tt->current_track->metadata);
    if(name)
      {
      ctx->opt->name_change_callback(ctx->opt->name_change_callback_data,
                                     name);
      free(name);
      }
    }
  priv->metadata_changed = 0;
  }

static int next_packet_ogg(bgav_demuxer_context_t * ctx)
  {
  int len_bytes, i;
  int64_t iframes;
  int64_t pframes;
  int serialno;
  bgav_packet_t * p = (bgav_packet_t *)0;
  bgav_stream_t * s;
  int64_t granulepos;
  stream_priv_t * stream_priv = (stream_priv_t*)0;
  ogg_priv * priv = (ogg_priv*)(ctx->priv);
  int subtitle_duration;
  
  if(!get_page(ctx))
    return 0;
  
  serialno   = ogg_page_serialno(&(priv->current_page));
  granulepos = ogg_page_granulepos(&(priv->current_page));

  //  fprintf(stderr, "Serialno: %d\n", serialno);
  
  if(ogg_page_bos(&(priv->current_page)) && !ctx->input->input->seek_byte)
    {
    if(!new_streaming_track(ctx))
      return 0;
    else
      {
      serialno = ogg_page_serialno(&(priv->current_page));
      //      fprintf(stderr, "New stream detected, serialno %d\n",
      //              serialno);
      
      }
    }

  s = bgav_track_find_stream(ctx->tt->current_track,
                             serialno);

  if(!s)
    {
    //    fprintf(stderr, "No stream found for serialno %d\n", serialno);
    priv->page_valid = 0;
    return 1;
    }
  stream_priv = (stream_priv_t*)(s->priv);
#if 0
  fprintf(stderr, "Page granulepos: %lld, cont: %d\n",
          ogg_page_granulepos(&(priv->current_page)),
          ogg_page_continued(&(priv->current_page)));
#endif
  if(ogg_stream_pagein(&stream_priv->os, &(priv->current_page)))
    fprintf(stderr, "ogg_stream_pagein failed\n");
  priv->page_valid = 0;
  
  
  while(ogg_stream_packetout(&stream_priv->os, &priv->op))
    {
    /* If we are in the process of resyncing, we must skip a whole page since we need
       the granulepos for the next page */
    
    //    if(stream_priv->do_sync && (stream_priv->prev_granulepos == -1))
    //      continue;
    
    // fprintf(stderr, "packetno: %lld\n", priv->op.packetno);
    
    switch(stream_priv->fourcc_priv)
      {
      case FOURCC_THEORA:

        if(stream_priv->do_sync)
          {
          if(stream_priv->prev_granulepos == -1)
            break;
          else
            {
            if((priv->op.granulepos >= 0) && !(priv->op.packet[0] & 0x40))
              {
              iframes =
                priv->op.granulepos >> stream_priv->keyframe_granule_shift;
              pframes =
                priv->op.granulepos-(iframes<<stream_priv->keyframe_granule_shift);
              
              //          fprintf(stderr, "Iframes: %lld, pframes: %lld, keyframe: %d\n", iframes, pframes,
              //                  !(priv->op.packet[0] & 0x40));
              s->time_scaled = (pframes + iframes) * (s->data.video.format.frame_duration);
              stream_priv->do_sync = 0;
              }
            }
          }
        
        /* Skip header packets */
        if(priv->op.packet[0] & 0x80)
          {
          if(!ctx->input->input->seek_byte && (priv->op.packetno == 1))
            {
            parse_vorbis_comment(s, priv->op.packet + 7, priv->op.bytes - 7);
            priv->metadata_changed = 1;
            //            fprintf(stderr, "Got metadata\n");
            //            bgav_metadata_dump(&stream_priv->metadata);
            }
          //          else
          //            fprintf(stderr, "Skipping theora header\n");
          break;
          }
        
        //        fprintf(stderr, "Theora data\n");
        p = bgav_packet_buffer_get_packet_write(s->packet_buffer, s);
        bgav_packet_alloc(p, sizeof(priv->op) + priv->op.bytes);
        memcpy(p->data, &priv->op, sizeof(priv->op));
        memcpy(p->data + sizeof(priv->op), priv->op.packet, priv->op.bytes);
        p->data_size = sizeof(priv->op) + priv->op.bytes;
        
        //        fprintf(stderr, "Theora granulepos: %lld\n", granulepos);

        if(priv->is_live)
          {
          p->timestamp_scaled = (stream_priv->frame_counter) *
            s->data.video.format.frame_duration;
          stream_priv->frame_counter++;
          }
        else if(priv->op.granulepos >= 0)
          {
          iframes =
            priv->op.granulepos >> stream_priv->keyframe_granule_shift;
          pframes =
            priv->op.granulepos-(iframes<<stream_priv->keyframe_granule_shift);

          //          fprintf(stderr, "Iframes: %lld, pframes: %lld, keyframe: %d\n", iframes, pframes,
          //                  !(priv->op.packet[0] & 0x40));
          p->timestamp_scaled = (pframes + iframes) * s->data.video.format.frame_duration;
          
          }

        //        fprintf(stderr, "Theora timestamp 3: %lld\n", p->timestamp_scaled);
        
        bgav_packet_done_write(p);
        break;
      case FOURCC_VORBIS:
        /* Resync if necessary */
        if(stream_priv->do_sync)
          {
          if(stream_priv->prev_granulepos == -1)
            break;
          else
            {
            stream_priv->do_sync = 0;
            s->time_scaled = stream_priv->prev_granulepos;
#if 0
            fprintf(stderr, "Vorbis resync %f\n",
                    (float)(s->time_scaled) /
                    (float)(s->data.audio.format.samplerate));
#endif
            }
          }
        
        if(priv->op.packet[0] & 0x01)
          {
          /* Get metadata */
          if(!ctx->input->input->seek_byte && (priv->op.packetno == 1))
            {
            parse_vorbis_comment(s, priv->op.packet + 7, priv->op.bytes - 7);
            priv->metadata_changed = 1;
            //            fprintf(stderr, "Got metadata\n");
            //            bgav_metadata_dump(&stream_priv->metadata);
            }
          //          else
          //            fprintf(stderr, "Skipping vorbis header %lld\n", priv->op.packetno);
          
          break;
          }
        
        //        fprintf(stderr, "Vorbis data\n");
        p = bgav_packet_buffer_get_packet_write(s->packet_buffer, s);
        bgav_packet_alloc(p, sizeof(priv->op) + priv->op.bytes);
        memcpy(p->data, &priv->op, sizeof(priv->op));
        memcpy(p->data + sizeof(priv->op), priv->op.packet, priv->op.bytes);
        p->data_size = sizeof(priv->op) + priv->op.bytes;
        bgav_packet_done_write(p);
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
              //              fprintf(stderr, "Skipping non keyframe %lld %lld\n", stream_priv->frame_counter, stream_priv->prev_granulepos);
              stream_priv->frame_counter++;
              break;
              }
            else
              {
              stream_priv->do_sync = 0;
              s->time_scaled =
                (int64_t)s->data.video.format.frame_duration *
                stream_priv->frame_counter;
#if 0
              fprintf(stderr, "OGM Resync: %f (duration: %d, stream_priv->frame_counter: %lld)\n",
                      (float)(s->data.video.format.frame_duration *
                              stream_priv->frame_counter) /
                      (float)(s->data.video.format.timescale),
                      s->data.video.format.frame_duration,
                      stream_priv->frame_counter);
#endif         
              // bgav_hexdump(priv->op.packet, 16, 16);
              }
            }
          }
#if 0
        if(priv->op.packet[0] & 0x08)
          fprintf(stderr, "keyframe\n");
        else
          fprintf(stderr, "NO keyframe\n");
#endif
        if(priv->op.packet[0] & 0x01) /* Header is already read -> skip it */
          {
          //          fprintf(stderr, "Skipping OGM video header, granulepos: %lld\n",
          //                  granulepos);
          break;
          }
        //        fprintf(stderr, "OGM video data (%d)\n", serialno);
        //        bgav_hexdump(priv->op.packet, 32, 16);
        //        return 0;
        /* Parse subheader (let's hope there aren't any packets with
           more than one video frame) */
#if 0
        fprintf(stderr, "OGM Page granulepos: %lld\n",
                granulepos);
#endif
        len_bytes =
          (priv->op.packet[0] >> 6) |
          ((priv->op.packet[0] & 0x02) << 1);
        
        p = bgav_packet_buffer_get_packet_write(s->packet_buffer, s);
        
        bgav_packet_alloc(p, priv->op.bytes - 1 - len_bytes);
        memcpy(p->data, priv->op.packet + 1 + len_bytes,
               priv->op.bytes - 1 - len_bytes);
        p->data_size = priv->op.bytes - 1 - len_bytes;
        if(priv->op.packet[0] & 0x08)
          p->keyframe = 1;
        p->timestamp_scaled =
          s->data.video.format.frame_duration * stream_priv->frame_counter;
        stream_priv->frame_counter++;
        bgav_packet_done_write(p);
        break;
      case FOURCC_FLAC:
      case FOURCC_FLAC_NEW:
        /* Resync if necessary */
        if(stream_priv->do_sync)
          {
          if(stream_priv->prev_granulepos == -1)
            break;
          else
            {
            stream_priv->do_sync = 0;
            s->time_scaled = stream_priv->prev_granulepos;
            }
          }
        
        /* Skip anything but audio frames */
        if((priv->op.packet[0] != 0xff) || ((priv->op.packet[1] & 0xfc) != 0xf8))
          break;

                
        p = bgav_packet_buffer_get_packet_write(s->packet_buffer, s);
        bgav_packet_alloc(p, priv->op.bytes);
        memcpy(p->data, priv->op.packet, priv->op.bytes);
        p->data_size = priv->op.bytes;
        bgav_packet_done_write(p);
        //        fprintf(stderr, "Read flac packet %ld bytes\n", priv->op.bytes);
        break;
      case FOURCC_SPEEX:
        //        fprintf(stderr, "Speex granulepos: %lld\n", granulepos);
        /* Resync if necessary */
        if(stream_priv->do_sync)
          {
          if(stream_priv->prev_granulepos == -1)
            break;
          else
            {
            stream_priv->do_sync = 0;
            s->time_scaled = stream_priv->prev_granulepos;
            }
          }
        
        if(priv->op.packetno < stream_priv->header_packets_needed)
          {
          //          fprintf(stderr, "Skipping speex header\n");
          break;
          }
        /* Set raw data */
        
        p = bgav_packet_buffer_get_packet_write(s->packet_buffer, s);
        bgav_packet_alloc(p, priv->op.bytes);
        memcpy(p->data, priv->op.packet, priv->op.bytes);
        p->data_size = priv->op.bytes;
        bgav_packet_done_write(p);
        break;
      case FOURCC_OGM_TEXT:
        //        fprintf(stderr, "Subtitle packet:\n");
        //        bgav_hexdump(priv->op.packet, priv->op.bytes, 16);

        if(priv->op.packet[0] & 0x01) /* Header is already read -> skip it */
          {
          //          fprintf(stderr, "Skipping OGM subtitle header, granulepos: %lld\n",
          //                  granulepos);
          break;
          }
        if(!(priv->op.packet[0] & 0x08))
          {
          //          fprintf(stderr, "Got non keyframe in subtitle stream\n");
          break;
          }
        
        len_bytes =
          (priv->op.packet[0] >> 6) |
          ((priv->op.packet[0] & 0x02) << 1);
        //        fprintf(stderr, "Len: %d\n", len_bytes);
        subtitle_duration = 0;
        for(i = len_bytes; i > 0; i--)
          {
          subtitle_duration <<= 8;
          subtitle_duration |= priv->op.packet[i];
          }
#if 0
        fprintf(stderr, "Granulepos: %lld, duration: %d\n",
                granulepos, subtitle_duration);
        fprintf(stderr, "Subtitle: %s\n", priv->op.packet + 1 + len_bytes);
#endif
        if((priv->op.packet[1 + len_bytes] == ' ') &&
           (priv->op.packet[2 + len_bytes] == '\0'))
          break;
        
        p = bgav_packet_buffer_get_packet_write(s->packet_buffer, s);

        bgav_packet_set_text_subtitle(p, (char*)(priv->op.packet + 1 + len_bytes),
                                      -1, granulepos, subtitle_duration);
        bgav_packet_done_write(p);
        break;
      }
    }

  if(p) /* True if we read non header data from the stream */
    {
    if(priv->metadata_changed)
      metadata_changed(ctx);
    }
  if(stream_priv)
    {
    stream_priv->prev_granulepos = granulepos;
    }
  
  return 1;
  }

static void reset_track(bgav_track_t * track)
  {
  stream_priv_t * stream_priv;
  int i;
  
  for(i = 0; i < track->num_audio_streams; i++)
    {
    stream_priv =
      (stream_priv_t*)(track->audio_streams[i].priv);
    stream_priv->prev_granulepos = -1;
    stream_priv->frame_counter = -1;
    stream_priv->do_sync = 1;
    ogg_stream_reset(&stream_priv->os);
    
    track->audio_streams[i].time_scaled = -1;
    }

  for(i = 0; i < track->num_video_streams; i++)
    {
    stream_priv =
      (stream_priv_t*)(track->video_streams[i].priv);

    stream_priv->prev_granulepos = -1;
    stream_priv->frame_counter = -1;
    stream_priv->do_sync = 1;
    
    ogg_stream_reset(&stream_priv->os);
    
    track->video_streams[i].time_scaled = -1;
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

/* Seeking in ogg: Gets quite complicated so we use iterative seeking */

static void seek_ogg(bgav_demuxer_context_t * ctx, gavl_time_t time)
  {
  int i, done;
  track_priv_t * track_priv;
  stream_priv_t * stream_priv;
  int64_t filepos;
  
  /* Seek to the file position */
  track_priv = (track_priv_t*)(ctx->tt->current_track->priv);
  filepos = track_priv->start_pos +
    (int64_t)((double)(time) / (double)(ctx->tt->current_track->duration) *
              (track_priv->end_pos - track_priv->start_pos));

  //  fprintf(stderr, "filepos: %lld [%lld .. %lld]\n", filepos,
  //          track_priv->start_pos, track_priv->end_pos);
  
  if(filepos <= track_priv->start_pos)
    filepos = find_first_page(ctx, track_priv->start_pos, track_priv->end_pos,
                              (int*)0, (int64_t*)0);
  else
    filepos = find_last_page(ctx, track_priv->start_pos, filepos,
                             (int*)0, (int64_t*)0);
  seek_byte(ctx, filepos);

  //  fprintf(stderr, "Filepos: %lld\n", filepos);
  
  /* Reset all the streams and set the stream times to -1 */

  reset_track(ctx->tt->current_track);
  
  /* Now, resync the streams (probably skipping lots of pages) */

  while(1)
    {
    /* Oops, reached EOF, go one page back */
    if(!next_packet_ogg(ctx)) 
      {
      filepos = find_last_page(ctx, track_priv->start_pos, filepos,
                               (int*)0, (int64_t*)0);
      reset_track(ctx->tt->current_track);
      }
    
    done = 1;

    for(i = 0; i < ctx->tt->current_track->num_audio_streams; i++)
      {
      stream_priv =
        (stream_priv_t*)(ctx->tt->current_track->audio_streams[i].priv);
      
      if(stream_priv->do_sync &&
         (ctx->tt->current_track->audio_streams[i].action != BGAV_STREAM_MUTE))
        done = 0;
      }
    if(done)
      {
      for(i = 0; i < ctx->tt->current_track->num_video_streams; i++)
        {
        stream_priv =
          (stream_priv_t*)(ctx->tt->current_track->video_streams[i].priv);
        if(stream_priv->do_sync &&
           (ctx->tt->current_track->video_streams[i].action != BGAV_STREAM_MUTE))
          done = 0;
        }
      
      }
    if(done)
      break;
    }
  }

static void close_ogg(bgav_demuxer_context_t * ctx)
  {
  int i, j;
  ogg_priv * priv;
  track_priv_t * track_priv;
  stream_priv_t * stream_priv;
  
  for(i = 0; i < ctx->tt->num_tracks; i++)
    {
    track_priv = (track_priv_t*)(ctx->tt->tracks[i].priv);
    if(track_priv->unsupported_streams)
      free(track_priv->unsupported_streams);
    free(track_priv);
    
    for(j = 0; j < ctx->tt->tracks[i].num_audio_streams; j++)
      {
      stream_priv = (stream_priv_t*)(ctx->tt->tracks[i].audio_streams[j].priv);
      if(stream_priv)
        {
        ogg_stream_clear(&stream_priv->os);
        bgav_metadata_free(&stream_priv->metadata);
        free(stream_priv);
        }
      if(ctx->tt->tracks[i].audio_streams[j].ext_data)
        free(ctx->tt->tracks[i].audio_streams[j].ext_data);
      }

    for(j = 0; j < ctx->tt->tracks[i].num_video_streams; j++)
      {
      stream_priv = (stream_priv_t*)(ctx->tt->tracks[i].video_streams[j].priv);
      if(stream_priv)
        {
        ogg_stream_clear(&stream_priv->os);
        bgav_metadata_free(&stream_priv->metadata);
        free(stream_priv);
        }
      if(ctx->tt->tracks[i].video_streams[j].ext_data)
        free(ctx->tt->tracks[i].video_streams[j].ext_data);
      }
    }
  
  priv = (ogg_priv *)ctx->priv;
  ogg_sync_clear(&priv->oy);

  /* IMPORTANT: The paket will be freed by the last ogg_stream */
  //  ogg_packet_clear(&priv->op);
  free(priv);
  }

static void select_track_ogg(bgav_demuxer_context_t * ctx,
                             int track)
  {
  char * name;
  ogg_priv * priv;
  track_priv_t * track_priv;
  
  priv = (ogg_priv *)(ctx->priv);
  
  track_priv = (track_priv_t*)(ctx->tt->current_track->priv);
  
  if(ctx->input->input->seek_byte && ctx->input->total_bytes)
    {
    seek_byte(ctx, track_priv->start_pos);
    priv->end_pos = track_priv->end_pos;
    }
  else
    {
    if(ctx->opt->name_change_callback)
      {
      name = get_name(&ctx->tt->current_track->metadata);
      if(name)
        {
        ctx->opt->name_change_callback(ctx->opt->name_change_callback_data,
                                       name);
        free(name);
        }
      }
    }
  }

bgav_demuxer_t bgav_demuxer_ogg =
  {
    probe:        probe_ogg,
    open:         open_ogg,
    next_packet:  next_packet_ogg,
    seek:         seek_ogg,
    close:        close_ogg,
    select_track: select_track_ogg
  };

