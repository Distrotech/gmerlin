/*****************************************************************
 
  demux_musepack.c
 
  Copyright (c) 2005 by Burkhard Plaum - plaum@ipf.uni-stuttgart.de
 
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

#include <musepack/musepack.h>

/*
 *  Musepack "demuxer": It's somewhat weird because libmusepack doesn't
 *  allow split the format parsing / decoding process.
 *
 *  For this reason, we we do all the decoding here and fake a PCM stream
 *  format such that the real "decoder" will just buffer/copy the samples
 *  to the output
 * 
 *  With this in approach, we'll have complete support for all libmusepack
 *  features (incl. seeking) without too much code duplication.
 */

typedef struct
  {
  mpc_reader     reader;
  mpc_streaminfo si;
  mpc_decoder    dec;
  } mpc_priv_t;

static int probe_mpc(bgav_input_context_t * input)
  {
  uint8_t test_data[3];
  if(bgav_input_get_data(input, test_data, 3) < 3)
    return 0;

  if((test_data[0] == 'M') &&
     (test_data[1] == 'P') &&
     (test_data[2] == '+'))
    return 1;
  return 0;
  }

static mpc_int32_t mpc_read(void *t, void *ptr, mpc_int32_t size)
  {
  bgav_input_context_t * ctx;
  ctx = (bgav_input_context_t *)t;

  return bgav_input_read_data(ctx, ptr, size);
  }

static BOOL mpc_seek(void *t, mpc_int32_t offset)
  {
  bgav_input_context_t * ctx;
  ctx = (bgav_input_context_t *)t;
  bgav_input_seek(ctx, offset, SEEK_SET);
  return TRUE;
  }

static mpc_int32_t mpc_tell(void *t)
  {
  bgav_input_context_t * ctx;
  ctx = (bgav_input_context_t *)t;
  return ctx->position;
  }

static mpc_int32_t mpc_get_size(void *t)
  {
  bgav_input_context_t * ctx;
  ctx = (bgav_input_context_t *)t;
  return ctx->total_bytes;
  }

static BOOL mpc_canseek(void *t)
  {
  bgav_input_context_t * ctx;
  ctx = (bgav_input_context_t *)t;
  return ctx->input->seek_byte ? TRUE : FALSE;
  }

static int open_mpc(bgav_demuxer_context_t * ctx,
                    bgav_redirector_context_t ** redir)
  {
  bgav_stream_t * s;
  mpc_priv_t * priv;

  priv = calloc(1, sizeof(*priv));
  ctx->priv = priv;
  
  /* Setup reader */

  priv->reader.seek     = mpc_seek;
  priv->reader.read     = mpc_read;
  priv->reader.tell     = mpc_tell;
  priv->reader.get_size = mpc_get_size;
  priv->reader.canseek  = mpc_canseek;

  priv->reader.data     = ctx->input;

  /* Get stream info */

  mpc_streaminfo_init(&(priv->si));

  if(mpc_streaminfo_read(&(priv->si), &(priv->reader)) != ERROR_CODE_OK)
    return 0;

  /* Fire up decoder */

  mpc_decoder_setup(&(priv->dec), &(priv->reader));
  if(!mpc_decoder_initialize(&(priv->dec), &(priv->si)))
    return 0;
  
  /* Set up track table */

  ctx->tt = bgav_track_table_create(1);
  s = bgav_track_add_audio_stream(ctx->tt->current_track);

  s->data.audio.format.samplerate   = priv->si.sample_freq;
  s->data.audio.format.num_channels = priv->si.channels;

  s->fourcc = BGAV_MK_FOURCC('m', 'p', 'c', ' '); /* Native float format */

  s->timescale = priv->si.sample_freq;
  
  ctx->tt->current_track->duration =
    gavl_samples_to_time(s->data.audio.format.samplerate,
                         mpc_streaminfo_get_length_samples(&(priv->si)));

  ctx->stream_description = bgav_sprintf("Musepack Format");

  if(ctx->input->input->seek_byte)
    ctx->can_seek = 1;

  return 1;
  }

static int next_packet_mpc(bgav_demuxer_context_t * ctx)
  {
  unsigned int result;
  bgav_stream_t * s;
  bgav_packet_t * p;
  mpc_priv_t * priv;
  priv = (mpc_priv_t *)(ctx->priv);

  s = &(ctx->tt->current_track->audio_streams[0]);

  p = bgav_packet_buffer_get_packet_write(s->packet_buffer, s);

  bgav_packet_alloc(p, MPC_DECODER_BUFFER_LENGTH * sizeof(float));
  
  result = mpc_decoder_decode(&(priv->dec), (float*)(p->data), 0, 0);

  if(!result || (result == (unsigned int)(-1)))
    return 0;

  p->data_size = result * s->data.audio.format.num_channels * sizeof(float);

  bgav_packet_done_write(p);
  
  return 1;
  }

static void seek_mpc(bgav_demuxer_context_t * ctx, gavl_time_t time)
  {
  int64_t sample;
  bgav_stream_t * s;
  
  mpc_priv_t * priv;
  priv = (mpc_priv_t *)(ctx->priv);

  s = &(ctx->tt->current_track->audio_streams[0]);
  
  sample = gavl_time_to_samples(s->data.audio.format.samplerate,
                                time);

  mpc_decoder_seek_sample(&(priv->dec), sample);
  s->time_scaled = sample;
  }

static void close_mpc(bgav_demuxer_context_t * ctx)
  {
  mpc_priv_t * priv;
  priv = (mpc_priv_t *)(ctx->priv);
  free(priv);
  }

bgav_demuxer_t bgav_demuxer_mpc =
  {
    probe:       probe_mpc,
    open:        open_mpc,
    next_packet: next_packet_mpc,
    seek:        seek_mpc,
    close:       close_mpc
  };

