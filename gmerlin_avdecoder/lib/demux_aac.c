/*****************************************************************
 
  demux_aac.c
 
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

/* Supported header types */

#define TYPE_ADIF 0
#define TYPE_ADTS 1

#define ADTS_SIZE 9
#define ADIF_SIZE 17

#define BYTES_TO_READ (768*GAVL_MAX_CHANNELS)

#define IS_ADIF(h) ((h[0] == 'A') && \
                    (h[1] == 'D') && \
                    (h[2] == 'I') && \
                    (h[3] == 'F'))
     
#define IS_ADTS(h) ((h[0] == 0xff) && \
                    ((h[1] & 0xf0) == 0xf0) && \
                    ((h[1] & 0x06) == 0x00))

static int adts_samplerates[] =
  {96000,88200,64000,48000,44100,
   32000,24000,22050,16000,12000,
   11025,8000,7350,0,0,0};

#if 0
static int samplerates[] =
  {
    96000,
    88200,
    64000,
    48000,
    44100,
    32000,
    24000,
    22050,
    16000,
    12000,
    11025,
    8000
  };
#endif

/* The following struct is not exactly the same as in the spec */

typedef struct
  {
  int mpeg_version;
  int profile;
  int samplerate;
  int channel_configuration;
  int frame_bytes;
  int num_blocks;
  int block_samples;
  } adts_header_t;
#if 0
static void adts_header_dump(adts_header_t * adts)
  {
  fprintf(stderr, "ADTS\n");
  fprintf(stderr, "  MPEG Version:          %d\n", adts->mpeg_version);
  fprintf(stderr, "  Profile:               ");
  
  if(adts->mpeg_version == 2)
    {
    switch(adts->profile)
      {
      case 0:
        fprintf(stderr, "MPEG-2 AAC Main profile\n");
        break;
      case 1:
        fprintf(stderr, "MPEG-2 AAC Low Complexity profile (LC)\n");
        break;
      case 2:
        fprintf(stderr, "MPEG-2 AAC Scalable Sample Rate profile (SSR)\n");
        break;
      case 3:
        fprintf(stderr, "MPEG-2 AAC (reserved)\n");
        break;
      }
    }
  else
    {
    switch(adts->profile)
      {
      case 0:
        fprintf(stderr, "MPEG-4 AAC MAIN\n");
        break;
      case 1:
        fprintf(stderr, "MPEG-4 AAC LC\n");
        break;
      case 2:
        fprintf(stderr, "MPEG-4 AAC SSR\n");
        break;
      case 3:
        fprintf(stderr, "MPEG-4 AAC LTP\n");
        break;
      }
    }
  fprintf(stderr, "  Samplerate:            %d\n", adts->samplerate);
  fprintf(stderr, "  Channel configuration: %d\n", adts->channel_configuration);
  fprintf(stderr, "  Frame bytes:           %d\n", adts->frame_bytes);
  fprintf(stderr, "  Num blocks:            %d\n", adts->num_blocks);
  fprintf(stderr, "  Samples per block:     %d\n", adts->block_samples);
  }
#endif
static int adts_header_read(uint8_t * data, adts_header_t * ret)
  {
  int protection_absent;

  if(!IS_ADTS(data))
    return 0;
  
  if(data[1] & 0x08)
    ret->mpeg_version = 2;
  else
    ret->mpeg_version = 4;

  protection_absent = data[1] & 0x01;

  ret->profile = (data[2] & 0xC0) >> 6;
  ret->samplerate = adts_samplerates[(data[2]&0x3C)>>2];
  ret->channel_configuration = ((data[2]&0x01)<<2)|((data[3]&0xC0)>>6);

  ret->frame_bytes = ((((unsigned int)data[3] & 0x3)) << 11)
    | (((unsigned int)data[4]) << 3) | (data[5] >> 5);
  
  ret->num_blocks = (data[6] & 0x03) + 1;

  if(ret->profile == 2) 
    ret->block_samples = 910;
  else
    ret->block_samples = 1024;
  return 1;
  }

/* AAC demuxer */

typedef struct
  {
  int type;

  int64_t data_start;
  int64_t data_size;
  
  uint32_t seek_table_size;

  int64_t sample_count;

  struct
    {
    int64_t position;
    int64_t time_scaled;
    } * seek_table;
  
  } aac_priv_t;

static int probe_aac(bgav_input_context_t * input)
  {
  uint8_t header[4];

  /* Support aac live streams */

  if(input->mimetype && !strcmp(input->mimetype, "audio/aacp"))
    return 1;

  if(bgav_input_get_data(input, header, 4) < 4)
    return 0;
  
  //  fprintf(stderr, "Probe AAC %08x\n", header);
  
  if(IS_ADIF(header) || IS_ADTS(header))
    return 1;
  return 0;
  }

#define TABLE_ALLOC 1024

static int open_adts(bgav_demuxer_context_t * ctx)
  {
  //  int i;
  uint8_t buf[ADTS_SIZE];

  adts_header_t adts;
  bgav_stream_t * s;
  aac_priv_t * priv;
  uint32_t seek_table_alloc;
  int64_t sample_count;
    
  priv = (aac_priv_t*)(ctx->priv);
  
  /* The first header will also be the streams extradata */
  s = ctx->tt->current_track->audio_streams;

  if(bgav_input_get_data(ctx->input, buf, ADTS_SIZE) < ADTS_SIZE)
    return 0;

  if(!adts_header_read(buf, &adts))
    return 0;
  
  if(adts.mpeg_version == 2)
    {
    switch(adts.profile)
      {
      case 0:
        ctx->stream_description =
          bgav_strndup("MPEG-2 AAC Main profile", NULL);
        break;
      case 1:
        ctx->stream_description =
          bgav_strndup("MPEG-2 AAC Low Complexity profile (LC)", NULL);
        break;
      case 2:
        ctx->stream_description =
          bgav_strndup("MPEG-2 AAC Scalable Sample Rate profile (SSR)", NULL);
        break;
      case 3:
        ctx->stream_description =
          bgav_strndup("MPEG-2 AAC (reserved)", NULL);
        break;
      }
    }
  else
    {
    switch(adts.profile)
      {
      case 0:
        ctx->stream_description =
          bgav_strndup("MPEG-4 AAC MAIN", NULL);
        break;
      case 1:
        ctx->stream_description =
          bgav_strndup("MPEG-4 AAC LC", NULL);
        break;
      case 2:
        ctx->stream_description =
          bgav_strndup("MPEG-4 AAC SSR", NULL);
        break;
      case 3:
        ctx->stream_description =
          bgav_strndup("MPEG-4 AAC LTP", NULL);
        break;
      }
    }
  
  //  adts_header_dump(&adts);
  
  /* Now, get the duration and create a seek table */

  if(ctx->input->input->seek_byte && ctx->input->total_bytes)
    {
    seek_table_alloc = TABLE_ALLOC;
    priv->seek_table = calloc(seek_table_alloc, sizeof(*(priv->seek_table)));

    sample_count = 0;

    priv->seek_table[0].position = priv->data_start;
    priv->seek_table[0].time_scaled     = 0;
    priv->seek_table_size = 1;
    
    while(1)
      {
      sample_count += adts.block_samples * adts.num_blocks;
      bgav_input_skip(ctx->input, adts.frame_bytes);

      /* Check is this was the last frame */
            
      if(ctx->input->position >= ctx->input->total_bytes)
        break;

      /* Create table entry */

      if(priv->seek_table_size >= seek_table_alloc)
        {
        seek_table_alloc += TABLE_ALLOC;
        priv->seek_table = realloc(priv->seek_table,
                                   seek_table_alloc*sizeof(*(priv->seek_table)));
        }

      priv->seek_table[priv->seek_table_size].position = ctx->input->position;
      priv->seek_table[priv->seek_table_size].time_scaled = sample_count;
      priv->seek_table_size++;

      priv->data_size = ctx->input->position - priv->data_start;

      if(bgav_input_get_data(ctx->input, buf, ADTS_SIZE) < ADTS_SIZE)
        break;
      
      if(!adts_header_read(buf, &adts))
        break;
      }
#if 0
    for(i = 0; i < priv->seek_table_size; i++)
      {
      fprintf(stderr, "Pos: %8lld, time: %f\n",
              priv->seek_table[i].position,
              gavl_time_to_seconds(priv->seek_table[i].time));
      }
#endif
    bgav_input_seek(ctx->input, priv->data_start, SEEK_SET);
    ctx->can_seek = 1;
    
    ctx->tt->current_track->duration =
      gavl_samples_to_time(adts.samplerate, sample_count);
    }
  
  return 1;
  }

static int open_adif(bgav_demuxer_context_t * ctx)
  {
  uint8_t buf[ADIF_SIZE];
  int skip_size;
  aac_priv_t * priv;
  priv = (aac_priv_t*)(ctx->priv);

  /* Try to get the bitrate */

  if(bgav_input_get_data(ctx->input, buf, ADIF_SIZE) < ADIF_SIZE)
    return 0;

  skip_size = (buf[4] & 0x80) ? 9 : 0;

  if(buf[4 + skip_size] & 0x10)
    {
    ctx->tt->current_track->audio_streams[0].container_bitrate = BGAV_BITRATE_VBR;
    }
  else
    {
    ctx->tt->current_track->audio_streams[0].container_bitrate =
      ((unsigned int)(buf[4 + skip_size] & 0x0F)<<19) |
      ((unsigned int)buf[5 + skip_size]<<11) |
      ((unsigned int)buf[6 + skip_size]<<3) |
      ((unsigned int)buf[7 + skip_size] & 0xE0);
    ctx->tt->current_track->duration = (GAVL_TIME_SCALE * (priv->data_size) * 8) /
      (ctx->tt->current_track->audio_streams[0].container_bitrate);
    }
  return 1;
  }

static int open_aac(bgav_demuxer_context_t * ctx,
                    bgav_redirector_context_t ** redir)
  {
  uint8_t header[4];
  aac_priv_t * priv;
  bgav_stream_t * s;
  bgav_id3v1_tag_t * id3v1 = (bgav_id3v1_tag_t*)0;
  bgav_metadata_t id3v1_metadata, id3v2_metadata;
  
  priv = calloc(1, sizeof(*priv));
  ctx->priv = priv;

  /* Recheck header */

  while(1)
    {
    if(bgav_input_get_data(ctx->input, header, 4) < 4)
      return 0;
    
    if(IS_ADTS(header))
      {
      //      fprintf(stderr, "Found ADTS header\n");
      priv->type = TYPE_ADTS;
      break;
      }
    else if(IS_ADIF(header))
      {
      //      fprintf(stderr, "Found ADIF header\n");
      priv->type = TYPE_ADIF;
      //    return 0;
      break;
      }
    bgav_input_skip(ctx->input, 1);
    }
  
  /* Create track */

  priv->data_start = ctx->input->position;

  ctx->tt = bgav_track_table_create(1);

  /* Check for id3v1 tag at the end */

  if(ctx->input->input->seek_byte)
    {
    bgav_input_seek(ctx->input, -128, SEEK_END);
    if(bgav_id3v1_probe(ctx->input))
      {
      id3v1 = bgav_id3v1_read(ctx->input);
      //      fprintf(stderr, "Found ID3V1 tag\n");
      }
    bgav_input_seek(ctx->input, priv->data_start, SEEK_SET);
    }

  //  if(ctx->input->id3v2)
  //    bgav_id3v2_dump(ctx->input->id3v2);
  
  if(ctx->input->id3v2 && id3v1)
    {
    memset(&id3v1_metadata, 0, sizeof(id3v1_metadata));
    memset(&id3v2_metadata, 0, sizeof(id3v2_metadata));
    bgav_id3v1_2_metadata(id3v1, &id3v1_metadata);
    bgav_id3v2_2_metadata(ctx->input->id3v2, &id3v2_metadata);
    bgav_metadata_dump(&id3v2_metadata);

    bgav_metadata_merge(&(ctx->tt->current_track->metadata),
                        &id3v2_metadata, &id3v1_metadata);
    bgav_metadata_free(&id3v1_metadata);
    bgav_metadata_free(&id3v2_metadata);
    }
  else if(ctx->input->id3v2)
    bgav_id3v2_2_metadata(ctx->input->id3v2,
                          &(ctx->tt->current_track->metadata));
  else if(id3v1)
    bgav_id3v1_2_metadata(id3v1,
                          &(ctx->tt->current_track->metadata));

  if(ctx->input->total_bytes)
    priv->data_size = ctx->input->total_bytes - priv->data_start;

  if(id3v1)
    {
    bgav_id3v1_destroy(id3v1);
    priv->data_size -= 128;
    }

  s = bgav_track_add_audio_stream(ctx->tt->current_track);

  /* This fourcc reminds the decoder to call a different init function */

  s->fourcc = BGAV_MK_FOURCC('a', 'a', 'c', ' ');

  /* Initialize rest */
  
  switch(priv->type)
    {
    case TYPE_ADTS:
      if(!open_adts(ctx))
        goto fail;
      break;
    case TYPE_ADIF:
      if(!open_adif(ctx))
        goto fail;
      break;
    }
  
  //  bgav_stream_dump(s);
  
  //  ctx->stream_description = bgav_sprintf("AAC");
  return 1;
  
  fail:
  return 0;
  }

static int next_packet_adts(bgav_demuxer_context_t * ctx)
  {
  bgav_packet_t * p;
  bgav_stream_t * s;
  adts_header_t adts;
  aac_priv_t * priv;
  uint8_t buf[ADTS_SIZE];
  
  priv = (aac_priv_t *)(ctx->priv);

  s = ctx->tt->current_track->audio_streams;

  if(bgav_input_get_data(ctx->input, buf, ADTS_SIZE) < ADTS_SIZE)
    return 0;

  if(!adts_header_read(buf, &adts))
    return 0;

  //  fprintf(stderr, "next_packet_adts: %d blocks %d bytes\n",
  //          adts.num_blocks, adts.frame_bytes);
  
  p = bgav_packet_buffer_get_packet_write(s->packet_buffer, s);
  
  p->timestamp_scaled = priv->sample_count;
    
  p->keyframe = 1;

  bgav_packet_alloc(p, adts.frame_bytes);

  p->data_size = bgav_input_read_data(ctx->input, p->data, adts.frame_bytes);

  if(p->data_size < adts.frame_bytes)
    return 0;
  
  bgav_packet_done_write(p);

  priv->sample_count += adts.block_samples * adts.num_blocks;

  return 1;
  }

static int next_packet_adif(bgav_demuxer_context_t * ctx)
  {
  bgav_stream_t * s;
  bgav_packet_t * p;
  aac_priv_t * priv;
  int bytes_read;
  priv = (aac_priv_t *)(ctx->priv);
  s = ctx->tt->current_track->audio_streams;
  
  /* Just copy the bytes, we have no idea about
     aac frame boundaries or timestamps here */

  p = bgav_packet_buffer_get_packet_write(s->packet_buffer, s);
  bgav_packet_alloc(p, BYTES_TO_READ);
  
  bytes_read = bgav_input_read_data(ctx->input, p->data, BYTES_TO_READ);
  if(!bytes_read)
    return 0;
  p->data_size = bytes_read;

  bgav_packet_done_write(p);
  return 1;
  }

static int next_packet_aac(bgav_demuxer_context_t * ctx)
  {
  aac_priv_t * priv;
    
  priv = (aac_priv_t *)(ctx->priv);

  switch(priv->type)
    {
    case TYPE_ADTS:
      return next_packet_adts(ctx);
      break;
    case TYPE_ADIF:
      return next_packet_adif(ctx);
      break;
    }
  return 0;
  }

static void seek_aac(bgav_demuxer_context_t * ctx, gavl_time_t time)
  {
  aac_priv_t * priv;
  uint32_t i;
  int64_t time_scaled;
  time_scaled =
    gavl_time_to_samples(ctx->tt->current_track->audio_streams[0].data.audio.format.samplerate,
                         time);
  
  priv = (aac_priv_t *)(ctx->priv);
  i = priv->seek_table_size - 1;
  
  while(priv->seek_table[i].time_scaled > time_scaled)
    i--;
  
  bgav_input_seek(ctx->input, priv->seek_table[i].position, SEEK_SET);
  ctx->tt->current_track->audio_streams->time_scaled = priv->seek_table[i].time_scaled;
  }

static void close_aac(bgav_demuxer_context_t * ctx)
  {
  aac_priv_t * priv;
  priv = (aac_priv_t *)(ctx->priv);

  if(priv->seek_table)
    free(priv->seek_table);

  free(priv);
  }

bgav_demuxer_t bgav_demuxer_aac =
  {
    probe:       probe_aac,
    open:        open_aac,
    next_packet: next_packet_aac,
    seek:        seek_aac,
    close:       close_aac
  };

