/*****************************************************************
 
  demux_wav.c
 
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

#include <avdec_private.h>
#include <stdlib.h>
#include <stdio.h>

/* Speaker copnfigurations */

#define SPEAKER_FRONT_LEFT 	        0x1
#define SPEAKER_FRONT_RIGHT 	        0x2
#define SPEAKER_FRONT_CENTER 	        0x4
#define SPEAKER_LOW_FREQUENCY 	        0x8
#define SPEAKER_BACK_LEFT 	        0x10
#define SPEAKER_BACK_RIGHT 	        0x20
#define SPEAKER_FRONT_LEFT_OF_CENTER 	0x40
#define SPEAKER_FRONT_RIGHT_OF_CENTER 	0x80
#define SPEAKER_BACK_CENTER 	        0x100
#define SPEAKER_SIDE_LEFT 	        0x200
#define SPEAKER_SIDE_RIGHT 	        0x400
#define SPEAKER_TOP_CENTER 	        0x800
#define SPEAKER_TOP_FRONT_LEFT 	        0x1000
#define SPEAKER_TOP_FRONT_CENTER 	0x2000
#define SPEAKER_TOP_FRONT_RIGHT 	0x4000
#define SPEAKER_TOP_BACK_LEFT 	        0x8000
#define SPEAKER_TOP_BACK_CENTER 	0x10000
#define SPEAKER_TOP_BACK_RIGHT 	        0x20000

/* WAV demuxer */

typedef struct
  {
  int32_t data_size;
  uint32_t data_start;
  int packet_size;
  } wav_priv_t;

static int probe_wav(bgav_input_context_t * input)
  {
  uint8_t test_data[12];
  if(bgav_input_get_data(input, test_data, 12) < 12)
    return 0;
  if((test_data[0] ==  'R') &&
     (test_data[1] ==  'I') &&
     (test_data[2] ==  'F') &&
     (test_data[3] ==  'F') &&
     (test_data[8] ==  'W') &&
     (test_data[9] ==  'A') &&
     (test_data[10] == 'V') &&
     (test_data[11] == 'E'))
    return 1;
  return 0;
  }

static int find_tag(bgav_demuxer_context_t * ctx, uint32_t tag)
  {
  uint32_t fourcc;
  uint32_t size;
  
  while(1)
    {
    if(!bgav_input_read_fourcc(ctx->input, &fourcc) ||
       !bgav_input_read_32_le(ctx->input, &size))
      return -1;
    if(fourcc == tag)
      return size;
    bgav_input_skip(ctx->input, size);
    }
  return -1;
  }

static int open_wav(bgav_demuxer_context_t * ctx,
                    bgav_redirector_context_t ** redir)
  {
  uint32_t fourcc;
  //  int size;
  uint32_t file_size;
  int format_size;
  bgav_stream_t * s;

  uint16_t tmp_16;
  uint32_t tmp_32;
  
  wav_priv_t * priv;
  
  priv = calloc(1, sizeof(*priv));
  ctx->priv = priv;

  /* Create track */

  ctx->tt = bgav_track_table_create(1);
  
  /* Recheck the header */
  
  if(!bgav_input_read_fourcc(ctx->input, &fourcc) ||
     (fourcc != BGAV_MK_FOURCC('R', 'I', 'F', 'F')))
    goto fail;
  
  if(!bgav_input_read_32_le(ctx->input, &file_size))
    goto fail;
    
  if(!bgav_input_read_fourcc(ctx->input, &fourcc) ||
     (fourcc != BGAV_MK_FOURCC('W', 'A', 'V', 'E')))
    goto fail;

  /* Search for the format tag */

  format_size = find_tag(ctx, BGAV_MK_FOURCC('f', 'm', 't', ' '));

  if(format_size < 0)
    goto fail;

  s = bgav_track_add_audio_stream(ctx->tt->current_track);

  if(!bgav_input_read_16_le(ctx->input, &tmp_16))
    goto fail;

  s->fourcc = BGAV_WAVID_2_FOURCC(tmp_16);

  if(!bgav_input_read_16_le(ctx->input, &tmp_16))
    goto fail;

  s->data.audio.format.num_channels = tmp_16;

  if(!bgav_input_read_32_le(ctx->input, &tmp_32))
    goto fail;
  s->data.audio.format.samplerate = tmp_32;

  if(!bgav_input_read_32_le(ctx->input, &tmp_32))
    goto fail;
  s->codec_bitrate = tmp_32 * 8;

  if(!bgav_input_read_16_le(ctx->input, &tmp_16))
    goto fail;
  s->data.audio.block_align = tmp_16;

  if(format_size == 14)
    {
    s->data.audio.bits_per_sample = 8;
    }
  else
    {
    if(!bgav_input_read_16_le(ctx->input, &tmp_16))
      goto fail;
    s->data.audio.bits_per_sample = tmp_16;

    if(format_size > 16)
      {
      if(!bgav_input_read_16_le(ctx->input, &tmp_16))
        goto fail;
      if(tmp_16)
        {
        if((s->fourcc == BGAV_WAVID_2_FOURCC(0xfffe)) && (tmp_16 == 22))
          {
          /* WAVEFORMATEXTENSIBLE */
          //          fprintf(stderr, "Detected WAVEFORMATEXTENSIBLE\n");
          if(!bgav_input_read_16_le(ctx->input, &tmp_16))
            goto fail;
          s->data.audio.bits_per_sample = tmp_16; /* Valid bits / sample */
          if(!bgav_input_read_32_le(ctx->input, &tmp_32))
            goto fail;
          //          fprintf(stderr, "Channel mask: %08x\n", tmp_32);
          if(!bgav_input_read_16_le(ctx->input, &tmp_16))
            goto fail;
          s->fourcc = BGAV_WAVID_2_FOURCC(tmp_16);
          
          bgav_input_skip(ctx->input, 14); /* Skip rest of GUID */ 
          }
        else
          {
          fprintf(stderr, "Reading extradata %d bytes\n", tmp_16);
          s->ext_size = tmp_16;
          s->ext_data = malloc(s->ext_size);
          if(bgav_input_read_data(ctx->input, s->ext_data, s->ext_size) <
             s->ext_size)
            goto fail;

          /* It is possible for the chunk to contain garbage at the end */
          if(format_size - s->ext_size - 18 > 0)
            bgav_input_skip(ctx->input, format_size - s->ext_size - 18);
          
          }
        }
      
      }
    }

  //  bgav_stream_dump(s);
  
  priv->data_size = find_tag(ctx, BGAV_MK_FOURCC('d', 'a', 't', 'a'));

  if(priv->data_size < 0)
    goto fail;
  priv->data_start = ctx->input->position;

  /* Packet size will be at least 1024 bytes */

  priv->packet_size = ((1024 + s->data.audio.block_align - 1) / 
                       s->data.audio.block_align) * s->data.audio.block_align;
  //  fprintf(stderr, "Packet size: %d Block align: %d\n", priv->packet_size,
  //          s->data.audio.block_align);

  if(ctx->input->input->seek_byte)
    ctx->can_seek = 1;

  ctx->tt->current_track->duration
    = ((int64_t)priv->data_size * (int64_t)GAVL_TIME_SCALE) / 
    (ctx->tt->current_track->audio_streams[0].codec_bitrate / 8);

  ctx->stream_description = bgav_sprintf("WAV Format");
  
  return 1;
  
  fail:
  return 0;
  }

static int next_packet_wav(bgav_demuxer_context_t * ctx)
  {
  bgav_packet_t * p;
  bgav_stream_t * s;
  wav_priv_t * priv;
  priv = (wav_priv_t *)(ctx->priv);
  
  s = bgav_track_find_stream(ctx->tt->current_track, 0);
  
  if(!s)
    return 1;

  p = bgav_packet_buffer_get_packet_write(s->packet_buffer, s);
  
  p->timestamp_scaled =
    ((ctx->input->position - priv->data_start) * s->data.audio.format.samplerate) /
    (s->codec_bitrate / 8);
  
  bgav_packet_alloc(p, priv->packet_size);
    
  p->data_size = bgav_input_read_data(ctx->input, p->data, priv->packet_size);

  //  fprintf(stderr, "Read packet %d\n", priv->data_size);
  
  p->keyframe = 1;
  
  if(!p->data_size)
    return 0;
  
  bgav_packet_done_write(p);
  //  fprintf(stderr, "done\n");
  
  return 1;
  }

static void seek_wav(bgav_demuxer_context_t * ctx, gavl_time_t time)
  {
  int64_t file_position;
  wav_priv_t * priv;
  priv = (wav_priv_t *)(ctx->priv);
  bgav_stream_t * s;

  s = ctx->tt->current_track->audio_streams;
    
  file_position = (time * (s->codec_bitrate / 8)) /
    GAVL_TIME_SCALE;
  file_position /= s->data.audio.block_align;
  file_position *= s->data.audio.block_align;

  /* Calculate the time before we add the start offset */
  s->time_scaled = ((int64_t)file_position * s->data.audio.format.samplerate) /
    (s->codec_bitrate / 8);
  
  file_position += priv->data_start;
  bgav_input_seek(ctx->input, file_position, SEEK_SET);
  }

static void close_wav(bgav_demuxer_context_t * ctx)
  {
  wav_priv_t * priv;
  priv = (wav_priv_t *)(ctx->priv);
  if(ctx->tt->current_track->audio_streams[0].ext_data)
    free(ctx->tt->current_track->audio_streams[0].ext_data);
  free(priv);
  }

bgav_demuxer_t bgav_demuxer_wav =
  {
    probe:       probe_wav,
    open:        open_wav,
    next_packet: next_packet_wav,
    seek:        seek_wav,
    close:       close_wav
  };

