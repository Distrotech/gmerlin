/*****************************************************************
 
  demux_aiff.c
 
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
#include <avdec_private.h>

typedef struct
  {
  uint32_t fourcc;
  uint32_t size;
  } chunk_header_t;

typedef struct
  {
  uint32_t data_start;
  uint32_t data_size;
  } aiff_priv_t;

#define PADD(n) ((n&1)?(n+1):n)

static gavl_time_t pos_2_time(bgav_demuxer_context_t * ctx, int64_t pos)
  {
  aiff_priv_t * priv;
  bgav_stream_t * s;
  s = &(ctx->tt->current_track->audio_streams[0]);
  priv = (aiff_priv_t*)(ctx->priv);
  
  return ((pos - priv->data_start) * GAVL_TIME_SCALE) /
    (s->data.audio.format.samplerate * 
     s->data.audio.block_align);
  
  }

static int64_t time_2_pos(bgav_demuxer_context_t * ctx, gavl_time_t time)
  {
  aiff_priv_t * priv;
  priv = (aiff_priv_t*)(ctx->priv);

  bgav_stream_t * s;
  s = &(ctx->tt->current_track->audio_streams[0]);
  
  return priv->data_start +
    (time *
     s->data.audio.format.samplerate *
     s->data.audio.block_align)/ (GAVL_TIME_SCALE);
  }

static int read_chunk_header(bgav_input_context_t * input,
                             chunk_header_t * ch)
  {
  return bgav_input_read_fourcc(input, &(ch->fourcc)) &&
    bgav_input_read_32_be(input, &(ch->size));
  }

static int probe_aiff(bgav_input_context_t * input)
  {
  uint8_t test_data[12];
  if(bgav_input_get_data(input, test_data, 12) < 12)
    return 0;
  if((test_data[0] ==  'F') &&
     (test_data[1] ==  'O') &&
     (test_data[2] ==  'R') &&
     (test_data[3] ==  'M') &&
     (test_data[8] ==  'A') &&
     (test_data[9] ==  'I') &&
     (test_data[10] == 'F') &&
     (test_data[11] == 'F'))
    return 1;
  return 0;
  }

static int read_float_80(bgav_input_context_t * input, int32_t * ret)
  {
  uint8_t buffer[10];
  uint8_t * pos;
  
  uint32_t mantissa;
  uint32_t last = 0;
  uint8_t exp;

  if(bgav_input_read_data(input, buffer, 10) < 10)
    return 0;

  pos = buffer + 2;

  mantissa = BGAV_PTR_2_32BE(pos);

  exp = 30 - buffer[1];
  while (exp--)
    {
    last = mantissa;
    mantissa >>= 1;
    }
  if (last & 0x00000001) mantissa++;
  *ret = mantissa;
  return 1;
  }

static int open_aiff(bgav_demuxer_context_t * ctx,
                     bgav_redirector_context_t ** redir)
  {
  chunk_header_t ch;
  uint32_t fourcc;
  bgav_stream_t * s;
  aiff_priv_t * priv;
  int keep_going = 1;
  uint32_t num_sample_frames;
  uint16_t num_bits;
  uint16_t num_channels;
  uint32_t samplerate;

  /* Create track */
  ctx->tt = bgav_track_table_create(1);
  
  /* Check file magic */
  
  if(!read_chunk_header(ctx->input, &ch) ||
     (ch.fourcc != BGAV_MK_FOURCC('F','O','R','M')) ||
     !bgav_input_read_fourcc(ctx->input, &fourcc) ||
     (fourcc != BGAV_MK_FOURCC('A','I','F','F')))
    return 0;

  /* Allocate private struct */

  priv = calloc(1, sizeof(*priv));
  ctx->priv = priv;

  /* Read chunks until we are done */

  while(keep_going)
    {
    if(!read_chunk_header(ctx->input, &ch))
      return 0;
    switch(ch.fourcc)
      {
      case BGAV_MK_FOURCC('C','O','M','M'):
        s = bgav_track_add_audio_stream(ctx->tt->current_track);
        s->fourcc = BGAV_MK_FOURCC('a','i','f','f');
        
        if(!bgav_input_read_16_be(ctx->input, &(num_channels)) ||
           !bgav_input_read_32_be(ctx->input, &(num_sample_frames)) ||
           !bgav_input_read_16_be(ctx->input, &(num_bits)) ||
           !read_float_80(ctx->input, &(samplerate)))
          return 0;
        
        if(ch.size > 18)
          bgav_input_skip(ctx->input, ch.size - 18);

        s->data.audio.format.samplerate = samplerate;
        s->data.audio.format.num_channels = num_channels;
        s->data.audio.bits_per_sample = num_bits;
        if(s->data.audio.bits_per_sample <= 8)
          {
          s->data.audio.block_align = num_channels;
          }
        else if(s->data.audio.bits_per_sample <= 16)
          {
          s->data.audio.block_align = 2 * num_channels;
          }
        else if(s->data.audio.bits_per_sample <= 24)
          {
          s->data.audio.block_align = 3 * num_channels;
          }
        else if(s->data.audio.bits_per_sample <= 32)
          {
          s->data.audio.block_align = 4 * num_channels;
          }
        else
          {
          fprintf(stderr, "%d bit aiff not supported\n", 
                  s->data.audio.bits_per_sample);
          return 0;
          }
        s->data.audio.format.samples_per_frame = 1024;
        break;
      case BGAV_MK_FOURCC('S','S','N','D'):
        bgav_input_skip(ctx->input, 4); /* Offset */
        bgav_input_skip(ctx->input, 4); /* Blocksize */
        priv->data_start = ctx->input->position;
        priv->data_size = ch.size - 8;
        ctx->duration = pos_2_time(ctx, priv->data_size + priv->data_start);
        keep_going = 0;
        break;
      default:
        fprintf(stderr, "Skipping chunk ");
        bgav_dump_fourcc(ch.fourcc);
        fprintf(stderr, "\n");
        
        bgav_input_skip(ctx->input, PADD(ch.size));
        break;
      }
    }
  if(ctx->input->input->seek_byte)
    ctx->can_seek = 1;
  return 1;
  }

static int next_packet_aiff(bgav_demuxer_context_t * ctx)
  {
  bgav_packet_t * p;
  aiff_priv_t * priv;
  int bytes_to_read;
  int bytes_read;
  bgav_stream_t * s;
  priv = (aiff_priv_t *)ctx->priv;

  s = &(ctx->tt->current_track->audio_streams[0]);
  p = bgav_packet_buffer_get_packet_write(s->packet_buffer);
  
  bytes_to_read = s->data.audio.format.samples_per_frame * s->data.audio.block_align;
  
  bgav_packet_alloc(p, bytes_to_read);
  
  p->timestamp = pos_2_time(ctx, ctx->input->position);
  
  bytes_read = bgav_input_read_data(ctx->input, p->data, bytes_to_read);
  p->data_size = bytes_read;
  p->keyframe = 1;
  bgav_packet_done_write(p);

  if(!bytes_read)
    return 0;
  
  return 1;
  }

static void seek_aiff(bgav_demuxer_context_t * ctx, gavl_time_t time)
  {
  aiff_priv_t * priv;
  int64_t pos;
  priv = (aiff_priv_t *)ctx->priv;

  pos = time_2_pos(ctx, time);
  bgav_input_seek(ctx->input, pos, SEEK_SET);
  ctx->tt->current_track->audio_streams[0].time = time;
  }

static void close_aiff(bgav_demuxer_context_t * ctx)
  {
  aiff_priv_t * priv;
  priv = (aiff_priv_t *)ctx->priv;
  }

bgav_demuxer_t bgav_demuxer_aiff =
  {
    probe:       probe_aiff,
    open:        open_aiff,
    next_packet: next_packet_aiff,
    seek:        seek_aiff,
    close:       close_aiff
  };
