/*****************************************************************
 
  demux_mpegvideo.c
 
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
#include <stdio.h>

#define SEQUENCE_HEADER    0x000001b3
#define SEQUENCE_EXTENSION 0x000001b5

/* Synchronization routines */

#define IS_START_CODE(h)  ((h&0xffffff00)==0x00000100)

/* We scan at most one kilobyte */

#define HEADER_BYTES 1024

static uint32_t next_start_code(bgav_input_context_t * ctx)
  {
  int bytes_skipped = 0;
  uint32_t c;
  while(1)
    {
    if(!bgav_input_get_32_be(ctx, &c))
      return 0;
    if(IS_START_CODE(c))
      {
      return c;
      }
    bgav_input_skip(ctx, 1);
    bytes_skipped++;
    if(bytes_skipped > HEADER_BYTES)
      return 0;
    }
  return 0;
  }

/* We parse only the interesting parts of
   the sequence headers, libmpeg2 can to this better */

typedef struct
  {
  uint32_t bitrate; /* In 400 bits per second */
  } sequence_header_t;

static int sequence_header_read(bgav_input_context_t * ctx,sequence_header_t * ret)
  {
  uint8_t buffer[8];
  if(bgav_input_read_data(ctx, buffer, 8) < 8)
    return 0;

  if((buffer[6] & 0x20) != 0x20)
    {
    fprintf(stderr, "missing marker bit!\n");
    return 0;        /* missing marker_bit */
    }
  ret->bitrate = (buffer[4]<<10)|(buffer[5]<<2)|(buffer[6]>>6);
  return 1;
  }

typedef struct
  {
  uint32_t bitrate_ext;
  } sequence_extension_t;

static int sequence_extension_read(bgav_input_context_t * ctx,
                            sequence_extension_t * ret)
  {
  uint8_t buffer[4];
  if(bgav_input_read_data(ctx, buffer, 4) < 4)
    return 0;

  if((buffer[3] & 0x01) != 0x01)
    {
    fprintf(stderr, "missing marker bit!\n");
    return 0;        /* missing marker_bit */
    }
  ret->bitrate_ext = ((buffer[2]<<25) | (buffer[3]<<17)) & 0x3ffc0000;
  return 1;
  }

/* Trivial demuxer for MPEG-1/2 video streams */

typedef struct
  {
  uint32_t data_start;
  int64_t data_size;
  int byte_rate;

  int64_t next_packet_time;
  } mpegvideo_priv_t;

static int probe_mpegvideo(bgav_input_context_t * input)
  {
  uint32_t header;
  /* Test for sequence header only */
  
  if(!bgav_input_get_32_be(input, &header))
    return 0;
  
  return (header == SEQUENCE_HEADER) ? 1 : 0;
  }

static int open_mpegvideo(bgav_demuxer_context_t * ctx,
                    bgav_redirector_context_t ** redir)
  {
  int mpeg2 = 0;
  sequence_header_t sh;
  sequence_extension_t se;

  mpegvideo_priv_t * priv;
  bgav_stream_t * s;
  uint8_t buffer[HEADER_BYTES];
  bgav_input_context_t * input_mem = (bgav_input_context_t*)0;
  
  priv = calloc(1, sizeof(*priv));
  ctx->priv = priv;

  /* Since we must pass the whole stream to libmpeg2,
     make our private buffer here */

  if(bgav_input_get_data(ctx->input, buffer, HEADER_BYTES) < HEADER_BYTES)
    goto fail;

  input_mem = bgav_input_open_memory(buffer, HEADER_BYTES);
                                        
  /* Read sequence header and check for sequence extension */

  if(next_start_code(input_mem) != SEQUENCE_HEADER)
    goto fail;

  bgav_input_skip(input_mem, 4);
  
  if(!sequence_header_read(input_mem, &sh))
    goto fail;

  if(next_start_code(input_mem) == SEQUENCE_EXTENSION)
    {
    /* MPEG-2 */

    bgav_input_skip(input_mem, 4);
    
    if(!sequence_extension_read(input_mem, &se))
      goto fail;
    mpeg2 = 1;
    }

  /* Now, we have the byte rate */

  priv->byte_rate = sh.bitrate;
  if(mpeg2)
    {
    priv->byte_rate += se.bitrate_ext;
    }
  else if(priv->byte_rate == 0x3ffff)
    priv->byte_rate = 0;
  
  priv->byte_rate *= 50;
  
  /* Create track */

  ctx->tt = bgav_track_table_create(1);
  
  s = bgav_track_add_video_stream(ctx->tt->current_track, ctx->opt);

  s->container_bitrate = priv->byte_rate * 8;
  
  /* We just set the fourcc, everything else will be set by the decoder */

  s->fourcc = BGAV_MK_FOURCC('m', 'p', 'g', 'v');
  
  //  bgav_stream_dump(s);

  if(ctx->input->total_bytes)
    priv->data_size = ctx->input->total_bytes;
  
  if(ctx->input->input->seek_byte && priv->byte_rate)
    ctx->can_seek = 1;

  if(ctx->input->total_bytes && priv->byte_rate)
    {
    ctx->tt->current_track->duration
      = ((int64_t)priv->data_size * (int64_t)GAVL_TIME_SCALE) / 
      (priv->byte_rate);
    }
  else
    ctx->tt->current_track->duration = GAVL_TIME_UNDEFINED;
  
  ctx->stream_description = bgav_sprintf("Elementary MPEG-%d video stream",
                                         1 + mpeg2);

  bgav_input_destroy(input_mem);
  return 1;

  fail:
  if(input_mem)
    bgav_input_destroy(input_mem);
  return 0;
  }

#define BYTES_TO_READ 1024

static int next_packet_mpegvideo(bgav_demuxer_context_t * ctx)
  {
  bgav_packet_t * p;
  bgav_stream_t * s;
  mpegvideo_priv_t * priv;
    
  priv = (mpegvideo_priv_t *)(ctx->priv);
  
  s = ctx->tt->current_track->video_streams;
  
  p = bgav_packet_buffer_get_packet_write(s->packet_buffer, s);

  //  p->timestamp_scaled = FRAME_SAMPLES * priv->frame_count;

  //  p->keyframe = 1;

  bgav_packet_alloc(p, BYTES_TO_READ);

  p->data_size = bgav_input_read_data(ctx->input, p->data, BYTES_TO_READ);
  
  //  fprintf(stderr, "Read packet %d\n", p->data_size);
  
  if(!p->data_size)
    return 0;

  if(priv->next_packet_time >= 0)
    {
    p->timestamp_scaled = priv->next_packet_time;
    priv->next_packet_time = -1;
    }
  bgav_packet_done_write(p);
  //  fprintf(stderr, "done\n");
  
  return 1;
  }

static void seek_mpegvideo(bgav_demuxer_context_t * ctx, gavl_time_t time)
  {
  int64_t file_position;
  mpegvideo_priv_t * priv;

  priv = (mpegvideo_priv_t *)(ctx->priv);
  bgav_stream_t * s;
  
  s = ctx->tt->current_track->video_streams;
    
  file_position = (time * (priv->byte_rate)) / GAVL_TIME_SCALE;
  
  s->time_scaled = gavl_time_scale(s->data.video.format.timescale, time);
  priv->next_packet_time = s->time_scaled;
  bgav_input_seek(ctx->input, file_position, SEEK_SET);
  }

static void close_mpegvideo(bgav_demuxer_context_t * ctx)
  {
  mpegvideo_priv_t * priv;
  priv = (mpegvideo_priv_t *)(ctx->priv);
  free(priv);
  }

bgav_demuxer_t bgav_demuxer_mpegvideo =
  {
    probe:       probe_mpegvideo,
    open:        open_mpegvideo,
    next_packet: next_packet_mpegvideo,
    seek:        seek_mpegvideo,
    close:       close_mpegvideo
  };

