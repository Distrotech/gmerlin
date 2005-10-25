/*****************************************************************
 
  demux_ogg.c
 
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
#include <stdio.h>
#include <string.h>

#include <avdec_private.h>
#include <vorbis_comment.h>

#include <ogg/ogg.h>

/* Fourcc definitions.
   Each private stream data also has a fourcc, which lets us
   detect OGM formats independently of the actual fourcc used here */

#define FOURCC_VORBIS    BGAV_MK_FOURCC('V','B','I','S')
#define FOURCC_THEORA    BGAV_MK_FOURCC('T','H','R','A')
#define FOURCC_FLAC      BGAV_MK_FOURCC('F','L','A','C')
#define FOURCC_OGM_AUDIO BGAV_MK_FOURCC('O','G','M','A')
#define FOURCC_OGM_VIDEO BGAV_MK_FOURCC('O','G','M','V')

#define BYTES_TO_READ 8500 /* Same as in vorbisfile */

/* Currently, we support one single vorbis encoded audio stream */

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
  } track_priv_t;

typedef struct
  {
  uint32_t fourcc_priv;
  ogg_stream_state os;
  
  int header_packets_read;
  int header_packets_needed;

  int64_t last_granulepos;
  
  int keyframe_granule_shift;
  } stream_priv_t;

typedef struct
  {
  ogg_sync_state  oy;
  ogg_page        current_page;
  ogg_packet      op;

  int64_t position;
  int64_t data_start;
  int current_page_size;

  /* Serial  number of the last page of the entire file */
  int last_page_serialno;
  } ogg_priv;

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
  if((bgav_input_read_data(input, ret->type, 8) < 8) ||
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
  else
    {
    fprintf(stderr, "Unknown type %.8s\n", ret->type);
    return 0;
    }
  }

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

static void seek_byte(bgav_demuxer_context_t * ctx, int64_t pos)
  {
  ogg_priv * priv = (ogg_priv*)(ctx->priv);
  ogg_sync_reset(&(priv->oy));
  //  ogg_page_clear(&(priv->os));
  bgav_input_seek(ctx->input, pos, SEEK_SET);
  }

static int get_data(bgav_demuxer_context_t * ctx)
  {
  char * buf;
  int result;
  
  ogg_priv * priv = (ogg_priv*)(ctx->priv);
  
  buf = ogg_sync_buffer(&(priv->oy), BYTES_TO_READ);
  result = bgav_input_read_data(ctx->input, (uint8_t*)buf, BYTES_TO_READ);

  //  fprintf(stderr, "Get data\n");
  //  bgav_hexdump(buf, 512, 16);
  
  ogg_sync_wrote(&(priv->oy), result);
  return result;
  }

/* Append extradata to a stream */

static void append_extradata(bgav_stream_t * s, ogg_packet * op)
  {
  s->ext_data = realloc(s->ext_data, s->ext_size + sizeof(*op) + op->bytes);
  memcpy(s->ext_data + s->ext_size, op, sizeof(*op));
  memcpy(s->ext_data + s->ext_size + sizeof(*op), op->packet, op->bytes);
  s->ext_size += (sizeof(*op) + op->bytes);
  fprintf(stderr, "Appended extradata %ld bytes, fourcc: ", (sizeof(*op) + op->bytes));
  bgav_dump_fourcc(s->fourcc);
  fprintf(stderr, "\n");
  //  bgav_hexdump(op->packet, op->bytes, 16);
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

  fprintf(stderr, "Setting up track at %lld\n", start_position);

  /* If we can seek, seek to the start point. If we can't seek, we are already
     at the right position */
  if(ctx->input->input->seek_byte)
    seek_byte(ctx, start_position);
  
  /* Get the first page of each stream */
  while(1)
    {
    while(!ogg_sync_pageout(&(priv->oy), &(priv->current_page)))
      if(!get_data(ctx))
        {
        fprintf(stderr, "EOF while setting up track\n");
        return 0;
        }
    if(!ogg_page_bos(&(priv->current_page)))
      {
      //      fprintf(stderr, "Found first non bos page %d\n",
      //              ogg_page_pageno(&(priv->current_page)));
      break;
      }
    /* Setup stream */
    serialno = ogg_page_serialno(&(priv->current_page));
    fprintf(stderr, "Serialno: %d\n", serialno);
    
    ogg_stream = calloc(1, sizeof(*ogg_stream));
    ogg_stream_init(&ogg_stream->os, serialno);
    ogg_stream_pagein(&ogg_stream->os, &(priv->current_page));

    if(ogg_stream_packetout(&ogg_stream->os, &priv->op) != 1)
      {
      fprintf(stderr, "Cannot get first packet of stream\n");
      return 0;
      }

    fprintf(stderr, "First packet (%ld bytes):\n", priv->op.bytes);
    bgav_hexdump(priv->op.packet, priv->op.bytes, 16);

    /* Vorbis */
    if((priv->op.bytes > 7) &&
       (priv->op.packet[0] == 0x01) &&
       !strncmp((char*)(priv->op.packet+1), "vorbis", 6))
      {
      fprintf(stderr, "Detected Vorbis data\n");
      s = bgav_track_add_audio_stream(track);
      s->fourcc = FOURCC_VORBIS;
      ogg_stream->fourcc_priv = FOURCC_VORBIS;
      
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
      }

    /* Theora */
    else if((priv->op.bytes > 7) &&
            (priv->op.packet[0] == 0x80) &&
            !strncmp((char*)(priv->op.packet+1), "theora", 6))
      {
      fprintf(stderr, "Detected Theora data\n");
      s = bgav_track_add_video_stream(track);
      s->fourcc = FOURCC_THEORA;
      ogg_stream->fourcc_priv = FOURCC_THEORA;
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
      
      fprintf(stderr, "Got Granule shift: %d, fps: %d:%d\n",
              ogg_stream->keyframe_granule_shift,
              s->data.video.format.timescale,
              s->data.video.format.frame_duration);
      
      /* Read remaining header packets from this page */
      while(ogg_stream_packetout(&ogg_stream->os, &priv->op) == 1)
        {
        append_extradata(s, &priv->op);
        ogg_stream->header_packets_read++;
        if(ogg_stream->header_packets_read == ogg_stream->header_packets_needed)
          break;
        }
      }

    /* Flac (old format?) */
    else if((priv->op.bytes == 4) &&
            !strncmp((char*)(priv->op.packet), "fLaC", 4))
      {
      /* Probably some old format? */
      fprintf(stderr, "Detected FLAC data\n");
      s = bgav_track_add_audio_stream(track);
      s->fourcc = FOURCC_FLAC;
      ogg_stream->fourcc_priv = FOURCC_FLAC;
      s->priv   = ogg_stream;
      s->stream_id = serialno;

      ogg_stream->header_packets_needed = 1;
      ogg_stream->header_packets_read = 0;

      while(ogg_stream_packetout(&ogg_stream->os, &priv->op) == 1)
        {
        fprintf(stderr, "FLAC extradata %ld bytes\n", priv->op.bytes);
        bgav_hexdump(priv->op.packet, priv->op.bytes, 16);
        }
      }

    /* OGM Video stream */
    else if((priv->op.bytes >= 9) &&
            (priv->op.packet[0] == 0x01) &&
            !strncmp((char*)(priv->op.packet+1), "video", 5))
      {
      fprintf(stderr, "Detected OGM video data\n");
      s = bgav_track_add_video_stream(track);
      
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
      
      ogm_header_dump(&ogm_header);

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

      gavl_video_format_dump(&s->data.video.format);
      
      s->fourcc = ogm_header.subtype;
      ogg_stream->header_packets_needed = 2;
      ogg_stream->header_packets_read = 1;
      }
    }

  /* Now, read header pages until we are done */

  done = 0;
  while(!done)
    {
    fprintf(stderr, "Got header page (no: %ld, packets: %d, serialno: %d)\n",
            ogg_page_pageno(&(priv->current_page)),
            ogg_page_packets(&(priv->current_page)),
            ogg_page_serialno(&(priv->current_page)));

    s = bgav_track_find_stream_all(track, ogg_page_serialno(&(priv->current_page)));
    if(!s)
      continue;

    ogg_stream = (stream_priv_t*)(s->priv);
    ogg_stream_pagein(&(ogg_stream->os), &(priv->current_page));

    switch(ogg_stream->fourcc_priv)
      {
      case FOURCC_THEORA:
      case FOURCC_VORBIS:
        /* Read remaining header packets from this page */
        while(ogg_stream_packetout(&ogg_stream->os, &priv->op) == 1)
          {
          append_extradata(s, &priv->op);
          ogg_stream->header_packets_read++;
          if(ogg_stream->header_packets_read == ogg_stream->header_packets_needed)
            {
            break;
            }
          }
        break;
      case FOURCC_OGM_VIDEO:
        /* Comment packet (can be ignored) */
        ogg_stream->header_packets_read++;
      case FOURCC_FLAC:
        while(ogg_stream_packetout(&ogg_stream->os, &priv->op) == 1)
          {

          switch(priv->op.packet[0] & 0x7f)
            {
            case 0: /* STREAMINFO, this is the only info we'll tell to the flac demuxer */
              fprintf(stderr, "FLAC extradata %ld bytes\n", priv->op.bytes);
              bgav_hexdump(priv->op.packet, priv->op.bytes, 16);
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
                (priv->op.packet[14] << 12) |
                (priv->op.packet[15] << 4) |
                ((priv->op.packet[16] >> 4)&0xf);
              fprintf(stderr, "Flac samplerate: %d\n", 
                      s->data.audio.format.samplerate);
                      
              break;
            case 1:
              fprintf(stderr, "Found vorbis comment in flac header\n");
              break;
            }
          ogg_stream->header_packets_read++;
          if(!(priv->op.packet[0] & 0x80))
            ogg_stream->header_packets_needed++;
          }
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

    /* Read the next page if we aren't done yet */

    if(!done)
      {
      while(!ogg_sync_pageout(&(priv->oy), &(priv->current_page)))
        if(!get_data(ctx))
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
                int * serialno)
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

static int64_t find_last_page(bgav_demuxer_context_t * ctx, int64_t pos1, int64_t pos2,
                              int * serialno)
  {
  int64_t page_pos, last_page_pos = -1, start_pos;
  int this_serialno, last_serialno = 0;
  start_pos = pos2 - BYTES_TO_READ;
  
  while(1)
    {
    //    fprintf(stderr, "find_last_page %lld %lld...", start_pos, pos2);    
    page_pos = find_first_page(ctx, start_pos, pos2, &this_serialno);
    
    //    fprintf(stderr, "Pos: %lld\n", page_pos);
    if(page_pos == -1)
      {
      if(last_page_pos >= 0)     /* No more pages in range -> return last one */
        {
        if(serialno)
          *serialno = last_serialno;
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
      last_serialno = this_serialno;
      last_page_pos = page_pos;
      start_pos = page_pos+1;
      }
    }
  return -1;
  }


static int track_has_serialno(bgav_track_t * track, int serialno)
  {
  int i;

  for(i = 0; i < track->num_audio_streams; i++)
    {
    if(track->audio_streams[i].stream_id == serialno)
      return 1;
    }

  for(i = 0; i < track->num_video_streams; i++)
    {
    if(track->video_streams[i].stream_id == serialno)
      return 1;
    }
  return 0;
  }

/* Find the next track starting with the current position */
static int64_t find_next_track(bgav_demuxer_context_t * ctx, int64_t start_pos)
  {
  int64_t pos1, pos2, test_pos, page_pos_1, page_pos_2;
  bgav_track_t * last_track, *new_track;
  ogg_priv * priv;
  int serialno_1, serialno_2;

  priv = (ogg_priv *)(ctx->priv);

  
  /* Check, if there are tracks left to find */
  last_track = &(ctx->tt->tracks[ctx->tt->num_tracks-1]);

  if(track_has_serialno(last_track, priv->last_page_serialno))
    return -1;

  /* Do bisection search */
  pos1 = start_pos;
  pos2 = ctx->input->total_bytes;

  while(1)
    {
    test_pos = (pos1 + pos2) / 2;

    //    fprintf(stderr, "Find next track %lld %lld %lld\n",
    //            pos1, pos2, test_pos);
    
    page_pos_1 = find_last_page(ctx, start_pos, test_pos, &serialno_1);
    //    fprintf(stderr, "page_pos_1: %lld\n", page_pos_1);
    //    fprintf(stderr, "page_1: pos: %lld, serialno: %d\n", page_pos_1, serialno_1);
    
    if(page_pos_1 < 0)
      return -1;

    
    if(!track_has_serialno(last_track, serialno_1))
      {
      pos2 = page_pos_1;
      continue;
      }

    page_pos_2 = find_first_page(ctx, test_pos, ctx->input->total_bytes, &serialno_2);
    //    fprintf(stderr, "page_pos_2: %lld\n", page_pos_2);
    //    fprintf(stderr, "page_2: pos: %lld, serialno: %d\n", page_pos_2, serialno_2);

    if(page_pos_2 < 0)
      return -1;
    
    if(track_has_serialno(last_track, serialno_2))
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
    
    return page_pos_2;
    }
  return -1;
  }

static int open_ogg(bgav_demuxer_context_t * ctx,
                    bgav_redirector_context_t ** redir)
  {
  int64_t result;
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

    fprintf(stderr, "Getting last page...");    
    if(find_last_page(ctx, 0, ctx->input->total_bytes, &priv->last_page_serialno) < 0)
      {
      fprintf(stderr, "failed\n");
      return 0;
      }
    fprintf(stderr, "done\n");
    fprintf(stderr, "Last page serialno: %d\n", priv->last_page_serialno);
    result = priv->data_start;
    while(1)
      {
      result = find_next_track(ctx, result);
      if(result == -1)
        break;
      }
    
    }
  
  if(ctx->input->input->seek_byte)
    ctx->can_seek = 1;
  ctx->stream_description = bgav_strndup("Ogg bitstream", NULL);
  return 1;
  }

static int next_packet_ogg(bgav_demuxer_context_t * ctx)
  {
  int len_bytes;
  int64_t iframes;
  int64_t pframes;
  int serialno;
  bgav_packet_t * p;
  bgav_stream_t * s;
  stream_priv_t * ogg_stream;
  ogg_priv * priv = (ogg_priv*)(ctx->priv);
    
  while(!ogg_sync_pageout(&(priv->oy), &(priv->current_page)))
    {
    if(!get_data(ctx))
      {
      fprintf(stderr, "Ogg demuxer detected EOF\n");
      return 0;
      }
    }

  serialno = ogg_page_serialno(&(priv->current_page));
  s = bgav_track_find_stream(ctx->tt->current_track,
                             serialno);

  if(!s)
    return 1;

  ogg_stream = (stream_priv_t*)(s->priv);

  ogg_stream_pagein(&ogg_stream->os, &(priv->current_page));

  //  fprintf(stderr, "Page granulepos: %lld\n", ogg_page_granulepos(&(priv->current_page)));
    
  while(ogg_stream_packetout(&ogg_stream->os, &priv->op) == 1)
    {
    switch(ogg_stream->fourcc_priv)
      {
      case FOURCC_THEORA:
        /* Skip header packets */
        if(priv->op.packet[0] & 0x80)
          break;
        p = bgav_packet_buffer_get_packet_write(s->packet_buffer, s);
        bgav_packet_alloc(p, sizeof(priv->op) + priv->op.bytes);
        memcpy(p->data, &priv->op, sizeof(priv->op));
        memcpy(p->data + sizeof(priv->op), priv->op.packet, priv->op.bytes);
        p->data_size = sizeof(priv->op) + priv->op.bytes;

        fprintf(stderr, "Theora granulepos: %lld\n", priv->op.granulepos);

        if(priv->op.granulepos >= 0)
          {
          iframes = priv->op.granulepos >> ogg_stream->keyframe_granule_shift;
          pframes = priv->op.granulepos-(iframes<<ogg_stream->keyframe_granule_shift);
          fprintf(stderr, "Iframes: %lld, pframes: %lld, keyframe: %d\n", iframes, pframes,
                  !(priv->op.packet[0] & 0x40));
          }
        
        bgav_packet_done_write(p);
        break;
      case FOURCC_VORBIS:
        p = bgav_packet_buffer_get_packet_write(s->packet_buffer, s);
        bgav_packet_alloc(p, sizeof(priv->op) + priv->op.bytes);
        memcpy(p->data, &priv->op, sizeof(priv->op));
        memcpy(p->data + sizeof(priv->op), priv->op.packet, priv->op.bytes);
        p->data_size = sizeof(priv->op) + priv->op.bytes;
        bgav_packet_done_write(p);
        break;
      case FOURCC_OGM_VIDEO:
        if(priv->op.packet[0] & 0x01) /* Header is already read -> skip it */
          break;
        //        fprintf(stderr, "OGM video data (%d)\n", serialno);
        //        bgav_hexdump(priv->op.packet, 32, 16);
        //        return 0;
        /* Parse subheader (let's hope there aren't any packets with
           more than one video frame) */

        len_bytes =
          (priv->op.packet[0] >> 6) ||
          ((priv->op.packet[0] & 0x02) << 1);
        
        p = bgav_packet_buffer_get_packet_write(s->packet_buffer, s);
        
        bgav_packet_alloc(p, priv->op.bytes - 1 - len_bytes);
        memcpy(p->data, priv->op.packet + 1 + len_bytes, priv->op.bytes - 1 - len_bytes);
        p->data_size = priv->op.bytes - 1 - len_bytes;
        if(priv->op.packet[0] & 0x80)
          p->keyframe = 1;
        
        bgav_packet_done_write(p);
        break;
      case FOURCC_FLAC:
        /* Skip anything but audio frames */
        if((priv->op.packet[0] != 0xff) || ((priv->op.packet[1] & 0xfc) != 0xf8))
          break;

        p = bgav_packet_buffer_get_packet_write(s->packet_buffer, s);
        bgav_packet_alloc(p, priv->op.bytes);
        memcpy(p->data, priv->op.packet, priv->op.bytes);
        p->data_size = priv->op.bytes;
        bgav_packet_done_write(p);
        //        fprintf(stderr, "Read flac packet %ld bytes\n", priv->op.bytes);
        
      }
    }
  
  return 1;
  }

static void close_ogg(bgav_demuxer_context_t * ctx)
  {
  int i;
  ogg_priv * priv;

  for(i = 0; i < ctx->tt->num_tracks; i++)
    {
    if(ctx->tt->tracks[i].audio_streams)
      if(ctx->tt->tracks[i].audio_streams[0].ext_data)
        free(ctx->tt->tracks[i].audio_streams[0].ext_data);
    }
  
  priv = (ogg_priv *)ctx->priv;
  ogg_sync_clear(&priv->oy);
  //  bitstreams_free(&priv->streams);
  free(priv);
  }

static void select_track_ogg(bgav_demuxer_context_t * ctx,
                             int track)
  {
  //  ogg_priv * priv;
  track_priv_t * track_priv;
  
  track_priv = (track_priv_t*)(ctx->tt->current_track->priv);
  
  if(ctx->input->input->seek_byte && ctx->input->total_bytes)
    seek_byte(ctx, track_priv->start_pos);
  
  }

bgav_demuxer_t bgav_demuxer_ogg =
  {
    probe:        probe_ogg,
    open:         open_ogg,
    next_packet:  next_packet_ogg,
    //    seek:         seek_ogg,
    close:        close_ogg,
    select_track: select_track_ogg
  };

