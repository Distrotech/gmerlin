/*****************************************************************
 
  demux_flv.c
 
  Copyright (c) 2006 by Burkhard Plaum - plaum@ipf.uni-stuttgart.de
 
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
#include <stdio.h>

#define AUDIO_ID 8
#define VIDEO_ID 9

typedef struct
  {
  int init;
  } flv_priv_t;

static int probe_flv(bgav_input_context_t * input)
  {
  uint8_t test_data[12];
  if(bgav_input_get_data(input, test_data, 12) < 12)
    return 0;
  if((test_data[0] ==  'F') &&
     (test_data[1] ==  'L') &&
     (test_data[2] ==  'V') &&
     (test_data[3] ==  0x01))
    return 1;
  return 0;
  }

/* Process one FLV chunk, initialize streams if init flag is set */

typedef struct
  {
  uint8_t type;
  uint32_t data_size;
  uint32_t timestamp;
  uint32_t reserved;
  } flv_tag;

static int flv_tag_read(bgav_input_context_t * ctx, flv_tag * ret)
  {
  if(!bgav_input_read_data(ctx, &ret->type, 1) ||
     !bgav_input_read_24_be(ctx, &ret->data_size) ||
     !bgav_input_read_24_be(ctx, &ret->timestamp) ||
     !bgav_input_read_32_be(ctx, &ret->reserved))
    return 0;
  return 1;
  }

#if 0
static void flv_tag_dump(flv_tag * t)
  {
  fprintf(stderr, "FLVTAG\n");
  fprintf(stderr, "  type:      %d\n", t->type);
  fprintf(stderr, "  data_size: %d\n", t->data_size);
  fprintf(stderr, "  timestamp: %d\n", t->timestamp);
  fprintf(stderr, "  reserved:  %d\n", t->reserved);
  }
#endif

static int next_packet_flv(bgav_demuxer_context_t * ctx)
  {
  uint8_t flags;
  uint8_t vp6_header[16];
  bgav_packet_t * p;
  bgav_stream_t * s;
  flv_priv_t * priv;
  flv_tag t;

  int packet_size = 0;
  int keyframe = 1;
  int adpcm_bits;
  uint8_t tmp_8;
  priv = (flv_priv_t *)(ctx->priv);
  
  /* Previous tag size (ignore this?) */
  bgav_input_skip(ctx->input, 4);
  
  if(!flv_tag_read(ctx->input, &t))
    return 0;

  //  flv_tag_dump(&t);

  if(priv->init)
    s = bgav_track_find_stream_all(ctx->tt->current_track, t.type);
  else
    s = bgav_track_find_stream(ctx->tt->current_track, t.type);

  if(!s)
    {
    //    fprintf(stderr, "Skipping %d unknown bytes\n", t.data_size);
    bgav_input_skip(ctx->input, t.data_size);
    return 1;
    }

  if(s->stream_id == AUDIO_ID)
    {
    if(!bgav_input_read_data(ctx->input, &flags, 1))
      return 0;
    
    if(!s->fourcc) /* Initialize */
      {
      s->data.audio.bits_per_sample = (flags & 2) ? 16 : 8;
      
      if((flags >> 4) == 5)
        {
        s->data.audio.format.samplerate = 8000;
        s->data.audio.format.num_channels = 1;
        }
      else
        {
        s->data.audio.format.samplerate = (44100<<((flags>>2)&3))>>3;
        s->data.audio.format.num_channels = (flags&1)+1;
        }

      switch(flags >> 4)
        {
        case 0: /* Uncompressed, Big endian */
          s->fourcc = BGAV_MK_FOURCC('t', 'w', 'o', 's');
          break;
        case 1: /* Flash ADPCM */
          s->fourcc = BGAV_MK_FOURCC('F', 'L', 'A', '1');

          /* Set block align for the case, adpcm frames are split over
             several packets */
          if(!bgav_input_get_data(ctx->input, &tmp_8, 1))
            return 0;
          adpcm_bits = 2 +
            s->data.audio.format.num_channels * (((tmp_8 >> 6) + 2) * 4096 + 16 + 6);
          
          s->data.audio.block_align = (adpcm_bits + 7) / 8;
          //          fprintf(stderr, "adpcm_bits: %d, block_align: %d\n",
          //                  adpcm_bits, s->data.audio.block_align);
          break;
        case 2: /* MP3 */
          s->fourcc = BGAV_MK_FOURCC('.', 'm', 'p', '3');
          break;
        case 3: /* Uncompressed, Little endian */
          s->fourcc = BGAV_MK_FOURCC('s', 'o', 'w', 't');
          break;
        case 5: /* NellyMoser (unsupported) */
          s->fourcc = BGAV_MK_FOURCC('F', 'L', 'A', '2');
          break;
        default: /* Set some nonsense so we can finish initializing */
          s->fourcc = BGAV_MK_FOURCC('?', '?', '?', '?');
          fprintf(stderr, "Unknown audio codec tag: %d\n", flags >> 4);
          break;
        }
      }
    packet_size = t.data_size - 1;
    }
  else if(s->stream_id == VIDEO_ID)
    {
    if(!bgav_input_read_data(ctx->input, &flags, 1))
      return 0;
    
    if(!s->fourcc) /* Initialize */
      {
      switch(flags & 0xF)
        {
        case 2: /* H263 based */
          s->fourcc = BGAV_MK_FOURCC('F', 'L', 'V', '1');
          break;
        case 3: /* Screen video (unsupported) */
          s->fourcc = BGAV_MK_FOURCC('F', 'L', 'V', 'S');
          break;
        case 4: /* VP6 (Flash 8?) */
          s->fourcc = BGAV_MK_FOURCC('V', 'P', '6', '2');
#if 1
          if(bgav_input_get_data(ctx->input, vp6_header, 5) < 5)
            return 0;
          /* Get the video size. The image is flipped vertically,
             so we set a negative height */
          s->data.video.format.image_height = -vp6_header[3] * 16;
          s->data.video.format.image_width  = vp6_header[4] * 16;

          s->data.video.format.frame_height = -s->data.video.format.image_height;
          s->data.video.format.frame_width  = s->data.video.format.image_width;
          
          //          fprintf(stderr, "vp6 header: ");
          //          bgav_hexdump(vp6_header, 16, 16);
#endif
          break;
        default: /* Set some nonsense so we can finish initializing */
          s->fourcc = BGAV_MK_FOURCC('?', '?', '?', '?');
          fprintf(stderr, "Unknown video codec tag: %d\n", flags & 0xF);
          break;
        }
      /* Set the framerate */
      s->data.video.format.framerate_mode = GAVL_FRAMERATE_VARIABLE;
      /* Set some nonsense values */
      s->data.video.format.timescale = 1000;
      s->data.video.format.frame_duration = 40;

      /* Hopefully, FLV supports square pixels only */
      s->data.video.format.pixel_width = 1;
      s->data.video.format.pixel_height = 1;
      
      }

    if(s->fourcc == BGAV_MK_FOURCC('V', 'P', '6', '2'))
      {
      bgav_input_skip(ctx->input, 1);
      packet_size = t.data_size - 2;
      }
    else
      packet_size = t.data_size - 1;
    if((flags >> 4) != 1)
      keyframe = 0;
    }

  if(!packet_size)
    {
    fprintf(stderr, "Got zero packet size (somethings wrong?)\n");
    return 0;
    }

  p = bgav_packet_buffer_get_packet_write(s->packet_buffer, s);
  bgav_packet_alloc(p, packet_size);
  p->data_size = bgav_input_read_data(ctx->input, p->data, packet_size);

  if(p->data_size < packet_size) /* Got EOF in the middle of a packet */
    return 0; 
  
  p->timestamp_scaled = t.timestamp;
  p->keyframe = keyframe;
  bgav_packet_done_write(p);
  
  return 1;
  }


static int open_flv(bgav_demuxer_context_t * ctx,
                    bgav_redirector_context_t ** redir)
  {
  int64_t pos;
  uint8_t flags;
  uint32_t data_offset, tmp;
  
  bgav_stream_t * as = (bgav_stream_t*)0;
  bgav_stream_t * vs = (bgav_stream_t*)0;
  
  flv_priv_t * priv;
  flv_tag t;
  
  priv = calloc(1, sizeof(*priv));
  ctx->priv = priv;

  /* Create track */

  ctx->tt = bgav_track_table_create(1);

  /* Skip signature */
  bgav_input_skip(ctx->input, 4);

  /* Check what streams we have */
  if(!bgav_input_read_data(ctx->input, &flags, 1))
    goto fail;

  if(flags & 0x04)
    {
    as = bgav_track_add_audio_stream(ctx->tt->current_track, ctx->opt);
    as->stream_id = AUDIO_ID;
    as->timescale = 1000;
    }
  if(flags & 0x01)
    {
    vs = bgav_track_add_video_stream(ctx->tt->current_track, ctx->opt);
    vs->stream_id = VIDEO_ID;
    vs->timescale = 1000;
    }
  if(!bgav_input_read_32_be(ctx->input, &data_offset))
    goto fail;

  fprintf(stderr, "Flags 0x%02x, data_offset: %d\n", flags, data_offset);
  
  if(data_offset > ctx->input->position)
    {
    fprintf(stderr, "Skipping %lld header bytes\n",
            data_offset - ctx->input->position);
    bgav_input_skip(ctx->input, data_offset - ctx->input->position);
    }

  /* Get packets until we saw each stream at least once */
  priv->init = 1;
  while(1)
    {
    if(!next_packet_flv(ctx))
      goto fail;
    if((!as || as->fourcc) &&
       (!vs || vs->fourcc))
      break;
    }
  priv->init = 0;

  /* Get the duration from the timestamp of the last packet if
     the stream is seekable */
  if(ctx->input->input->seek_byte)
    {
    pos = ctx->input->position;
    bgav_input_seek(ctx->input, -4, SEEK_END);
    //    fprintf(stderr, "Pos: %lld, total_bytes: %lld\n",
    //            ctx->input->position, ctx->input->total_bytes);
    if(bgav_input_read_32_be(ctx->input, &tmp))
      {
      //      fprintf(stderr, "Last packet size: %d\n", tmp); 
      bgav_input_seek(ctx->input, -((int64_t)tmp+4), SEEK_END);
      //      fprintf(stderr, "Pos: %lld, total_bytes: %lld\n",
      //              ctx->input->position, ctx->input->total_bytes);
      
      if(flv_tag_read(ctx->input, &t))
        {
        ctx->tt->current_track->duration =
          gavl_time_unscale(1000, t.timestamp);
        //        fprintf(stderr, "Got packet:\n");
        //        flv_tag_dump(&t);
        }
      else
        fprintf(stderr, "Reading FLV tag failed\n");
      }
    bgav_input_seek(ctx->input, pos, SEEK_SET);
    }
  
  ctx->stream_description = bgav_sprintf("Flash video (FLV)");
  
  return 1;
  
  fail:
  return 0;
  }

static void close_flv(bgav_demuxer_context_t * ctx)
  {
  flv_priv_t * priv;
  priv = (flv_priv_t *)(ctx->priv);
  free(priv);
  }

bgav_demuxer_t bgav_demuxer_flv =
  {
    probe:       probe_flv,
    open:        open_flv,
    next_packet: next_packet_flv,
    close:       close_flv
  };


