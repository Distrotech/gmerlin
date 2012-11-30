/*****************************************************************
 * gmerlin-encoders - encoder plugins for gmerlin
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


#include <config.h>

#include <string.h>

#include <gmerlin/translation.h>
#include <gmerlin/log.h>
#define LOG_DOMAIN "lame"

#include <bglame.h>

/* MPEG header detection: lame outputs incomplete frames,
   so we need to assemble them to packets */

/* The following code was ported from gmerlin_avdecoder (http://gmerlin.sourceforge.net) */

/* MPEG Audio header parsing code */

static int mpeg_bitrates[5][16] = {
  /* MPEG-1 */
  { 0,  32000,  64000,  96000, 128000, 160000, 192000, 224000,    // I
       256000, 288000, 320000, 352000, 384000, 416000, 448000, 0},
  { 0,  32000,  48000,  56000,  64000,  80000,  96000, 112000,    // II
       128000, 160000, 192000, 224000, 256000, 320000, 384000, 0 },
  { 0,  32000,  40000,  48000,  56000,  64000,  80000,  96000,    // III
       112000, 128000, 160000, 192000, 224000, 256000, 320000, 0 },

  /* MPEG-2 LSF */
  { 0,  32000,  48000,  56000,  64000,  80000,  96000, 112000,    // I
       128000, 144000, 160000, 176000, 192000, 224000, 256000, 0 },
  { 0,   8000,  16000,  24000,  32000,  40000,  48000,  56000,
        64000,  80000,  96000, 112000, 128000, 144000, 160000, 0 } // II & III
};

static int mpeg_samplerates[3][3] = {
  { 44100, 48000, 32000 }, // MPEG1
  { 22050, 24000, 16000 }, // MPEG2
  { 11025, 12000, 8000 }   // MPEG2.5
  };

#define MPEG_ID_MASK        0x00180000
#define MPEG_MPEG1          0x00180000
#define MPEG_MPEG2          0x00100000
#define MPEG_MPEG2_5        0x00000000

#define MPEG_LAYER_MASK     0x00060000
#define MPEG_LAYER_III      0x00020000
#define MPEG_LAYER_II       0x00040000
#define MPEG_LAYER_I        0x00060000
#define MPEG_PROTECTION     0x00010000
#define MPEG_BITRATE_MASK   0x0000F000
#define MPEG_FREQUENCY_MASK 0x00000C00
#define MPEG_PAD_MASK       0x00000200
#define MPEG_PRIVATE_MASK   0x00000100
#define MPEG_MODE_MASK      0x000000C0
#define MPEG_MODE_EXT_MASK  0x00000030
#define MPEG_COPYRIGHT_MASK 0x00000008
#define MPEG_HOME_MASK      0x00000004
#define MPEG_EMPHASIS_MASK  0x00000003
#define LAYER_I_SAMPLES       384
#define LAYER_II_III_SAMPLES 1152

/* Header detection stolen from the mpg123 plugin of xmms */

static int header_check(uint32_t head)
  {
  if ((head & 0xffe00000) != 0xffe00000)
    return 0;
  if (!((head >> 17) & 3))
    return 0;
  if (((head >> 12) & 0xf) == 0xf)
    return 0;
  if (!((head >> 12) & 0xf))
    return 0;
  if (((head >> 10) & 0x3) == 0x3)
    return 0;
  if (((head >> 19) & 1) == 1 &&
      ((head >> 17) & 3) == 3 &&
      ((head >> 16) & 1) == 1)
    return 0;
  if ((head & 0xffff0000) == 0xfffe0000)
    return 0;
  return 1;
  }

typedef enum
  {
    MPEG_VERSION_NONE = 0,
    MPEG_VERSION_1 = 1,
    MPEG_VERSION_2 = 2,
    MPEG_VERSION_2_5
  } mpeg_version_t;

#define CHANNEL_STEREO   0
#define CHANNEL_JSTEREO  1
#define CHANNEL_DUAL     2
#define CHANNEL_MONO     3

typedef struct
  {
  mpeg_version_t version;
  int layer;

  int bitrate_mode;
  int bitrate;
  
  int min_bitrate; /* ABR only */
  int max_bitrate; /* ABR only  */
  int samplerate;
  int frame_bytes;
  int channel_mode;
  int mode;
  int samples_per_frame;
  } mpeg_header;

static int decode_header(mpeg_header * h, uint8_t * ptr)
  {
  uint32_t header;
  int index;
  /* For calculation of the byte length of a frame */
  int pad;
  int slots_per_frame;
  h->frame_bytes = 0;
  header =
    ptr[3] | (ptr[2] << 8) | (ptr[1] << 16) | (ptr[0] << 24);
  if(!header_check(header))
    return 0;
  index = (header & MPEG_MODE_MASK) >> 6;
  switch(index)
    {
    case 0:
      h->channel_mode = CHANNEL_STEREO;
      break;
    case 1:
      h->channel_mode = CHANNEL_JSTEREO;
      break;
    case 2:
      h->channel_mode = CHANNEL_DUAL;
      break;
    case 3:
      h->channel_mode = CHANNEL_MONO;
      break;
    }
  /* Get Version */
  switch(header & MPEG_ID_MASK)
    {
    case MPEG_MPEG1:
      h->version = MPEG_VERSION_1;
        break;
    case MPEG_MPEG2:
      h->version = MPEG_VERSION_2;
      break;
    case MPEG_MPEG2_5:
      h->version = MPEG_VERSION_2_5;
      break;
    default:
      return 0;
    }
  /* Get Layer */
  switch(header & MPEG_LAYER_MASK)
    {
    case MPEG_LAYER_I:
      h->layer = 1;
      break;
    case MPEG_LAYER_II:
      h->layer = 2;
      break;
    case MPEG_LAYER_III:
      h->layer = 3;
      break;
    }
  index = (header & MPEG_BITRATE_MASK) >> 12;
  switch(h->version)
    {
    case MPEG_VERSION_1:
      switch(h->layer)
        {
        case 1:
          h->bitrate = mpeg_bitrates[0][index];
          break;
        case 2:
          h->bitrate = mpeg_bitrates[1][index];
          break;
        case 3:
          h->bitrate = mpeg_bitrates[2][index];
          break;
        }
      break;
    case MPEG_VERSION_2:
    case MPEG_VERSION_2_5:
      switch(h->layer)
        {
        case 1:
          h->bitrate = mpeg_bitrates[3][index];
          break;
        case 2:
        case 3:
          h->bitrate = mpeg_bitrates[4][index];
          break;
        }
      break;
    default: // This won't happen, but keeps gcc quiet
      return 0;
    }
  index = (header & MPEG_FREQUENCY_MASK) >> 10;
  switch(h->version)
    {
    case MPEG_VERSION_1:
      h->samplerate = mpeg_samplerates[0][index];
      break;
    case MPEG_VERSION_2:
      h->samplerate = mpeg_samplerates[1][index];
      break;
    case MPEG_VERSION_2_5:
      h->samplerate = mpeg_samplerates[2][index];
      break;
    default: // This won't happen, but keeps gcc quiet
      return 0;
    }
  pad = (header & MPEG_PAD_MASK) ? 1 : 0;
  if(h->layer == 1)
    {
    h->frame_bytes = ((12 * h->bitrate / h->samplerate) + pad) * 4;
    }
  else
    {
    slots_per_frame = ((h->layer == 3) &&
      ((h->version == MPEG_VERSION_2) ||
       (h->version == MPEG_VERSION_2_5))) ? 72 : 144;
    h->frame_bytes = (slots_per_frame * h->bitrate) / h->samplerate + pad;
    }
  // h->mode = (ptr[3] >> 6) & 3;
  h->samples_per_frame =
    (h->layer == 1) ? LAYER_I_SAMPLES : LAYER_II_III_SAMPLES;
  if(h->version != MPEG_VERSION_1)
    h->samples_per_frame /= 2;
  //  dump_header(h);
  return 1;
  }


/* Actual codec starts here */

struct bg_lame_s
  {
  gavl_packet_t gp;
  
  uint8_t * buffer;
  int buffer_alloc;
  int buffer_size;

  enum vbr_mode_e vbr_mode;

  /* Config stuff */
    
  int abr_min_bitrate;
  int abr_max_bitrate;
  int abr_bitrate;
  int cbr_bitrate;
  int vbr_quality;

  lame_t lame;
  gavl_audio_format_t format;
  
  gavl_audio_sink_t * sink;
  gavl_packet_sink_t * psink;

  int64_t in_pts;
  int64_t out_pts;
  int64_t delay;
  
  };

/* Supported samplerates for MPEG-1/2/2.5 */

static const int samplerates[] =
  {
    /* MPEG-2.5 */
    8000,
    11025,
    12000,
    /* MPEG-2 */
    16000,
    22050,
    24000,
    /* MPEG-1 */
    32000,
    44100,
    48000,
    /* End */
    0
  };

/* Find the correct bitrate */

static const int mpeg1_bitrates[] =
  {
    32,
    40,
    48,
    56,
    64,
    80,
    96,
    112,
    128,
    160,
    192,
    224,
    256,
    320
  };

static const int mpeg2_bitrates[] =
  {
    8,
    16,
    24,
    32,
    40,
    48,
    56,
    64,
    80,
    96,
    112,
    128,
    144,
    160
  };

static int get_bitrate(int bitrate, int samplerate)
  {
  int i;
  int const * bitrates;
  int diff;
  int min_diff = 1000000;
  int min_i = -1;

  if(samplerate >= 32000)
    bitrates = mpeg1_bitrates;
  else
    bitrates = mpeg2_bitrates;

  for(i = 0; i < sizeof(mpeg1_bitrates) / sizeof(mpeg1_bitrates[0]); i++)
    {
    if(bitrate == bitrates[i])
      return bitrate;
    diff = abs(bitrate - bitrates[i]);
    if(diff < min_diff)
      {
      min_diff = diff;
      min_i = i;
      }
    }
  if(min_i >= 0)
    return bitrates[min_i];
  return 128;
  }

bg_lame_t * bg_lame_create()
  {
  bg_lame_t * ret;
  ret = calloc(1, sizeof(*ret));
  ret->vbr_mode = vbr_off;
  ret->lame = lame_init();
  return ret;
  }
 


void bg_lame_set_packet_sink(bg_lame_t * lame,
                             gavl_packet_sink_t * sink)
  {
  lame->psink = sink;
  }


void bg_lame_set_parameter(bg_lame_t * lame,
                           const char * name,
                           const bg_parameter_value_t * v)
  {
  int i;
  
  if(!name)
    return;
  
  if(!strcmp(name, "bitrate_mode"))
    {
    if(!strcmp(v->val_str, "ABR"))
      {
      lame->vbr_mode = vbr_abr;
      }
    else if(!strcmp(v->val_str, "VBR"))
      {
      lame->vbr_mode = vbr_default;
      }
    else
      {
      lame->vbr_mode = vbr_off;
      }
    if(lame_set_VBR(lame->lame, lame->vbr_mode))
      bg_log(BG_LOG_ERROR, LOG_DOMAIN,  "lame_set_VBR failed");
    }
  else if(!strcmp(name, "stereo_mode"))
    {
    if(lame->format.num_channels == 1)
      return;
    i = NOT_SET;
    if(!strcmp(v->val_str, "Stereo"))
      {
      i = STEREO;
      }
    else if(!strcmp(v->val_str, "Joint stereo"))
      {
      i = JOINT_STEREO;
      }
    if(i != NOT_SET)
      {
      if(lame_set_mode(lame->lame, JOINT_STEREO))
        bg_log(BG_LOG_ERROR, LOG_DOMAIN,  "lame_set_mode failed");
      }
    }
  else if(!strcmp(name, "quality"))
    {
    if(lame_set_quality(lame->lame, v->val_i))
      bg_log(BG_LOG_ERROR, LOG_DOMAIN,  "lame_set_quality failed");
    }
  
  else if(!strcmp(name, "cbr_bitrate"))
    {
    lame->cbr_bitrate = v->val_i;
    }
  else if(!strcmp(name, "vbr_quality"))
    {
    lame->vbr_quality = v->val_i;
    }
  else if(!strcmp(name, "abr_bitrate"))
    {
    lame->abr_bitrate = v->val_i;
    }
  else if(!strcmp(name, "abr_min_bitrate"))
    {
    lame->abr_min_bitrate = v->val_i;
    }
  else if(!strcmp(name, "abr_max_bitrate"))
    {
    lame->abr_max_bitrate = v->val_i;
    }
  }

static int flush_packets(bg_lame_t * lame, int flush_all)
  {
  mpeg_header h;
  int ret = 0;
  memset(&h, 0, sizeof(h));
  
  while(1)
    {
    if(lame->buffer_size < 4)
      break;

    /* Got no header -> Things are screwed up */
    if(!decode_header(&h, lame->buffer))
      return -1;

    /* Output the last (possibly incomplete) packet */
    if((lame->buffer_size < h.frame_bytes) &&
       flush_all)
      {
      h.frame_bytes = lame->buffer_size;
      }
    
    if(lame->buffer_size >= h.frame_bytes)
      {
      /* Output packet */
      gavl_packet_alloc(&lame->gp, h.frame_bytes);
      memcpy(lame->gp.data, lame->buffer, h.frame_bytes);
      lame->gp.data_len = h.frame_bytes;

      /* PTS */

      lame->gp.pts = lame->out_pts;
      lame->gp.duration = lame->format.samples_per_frame;

      if(lame->gp.pts + lame->gp.duration > lame->in_pts)
        lame->gp.duration = lame->in_pts - lame->gp.pts;
      
      lame->out_pts += lame->gp.duration;

      /* Output packet */
      
      if(gavl_packet_sink_put_packet(lame->psink, &lame->gp) != GAVL_SINK_OK)
        return -1;

      /* Remove packet from buffer */
      
      lame->buffer_size -= h.frame_bytes;
      
      if(lame->buffer_size > 0)
        memmove(lame->buffer,
                lame->buffer + h.frame_bytes,
                lame->buffer_size);
      ret++;
      }
    else
      break;
    }
  return ret;
  }
  
static gavl_sink_status_t
write_audio_func(void * data, gavl_audio_frame_t * frame)
  {
  int bytes_encoded;
  bg_lame_t * lame = data;

  if(lame->in_pts == GAVL_TIME_UNDEFINED)
    {
    lame->in_pts = frame->timestamp;
    lame->out_pts = lame->in_pts - lame->delay;
    }
  
  bytes_encoded = lame_encode_buffer_float(lame->lame,
                                           frame->channels.f[0],
                                           (lame->format.num_channels > 1) ?
                                           frame->channels.f[1] :
                                           frame->channels.f[0],
                                           frame->valid_samples,
                                           lame->buffer + lame->buffer_size,
                                           lame->buffer_alloc - lame->buffer_size);

  lame->buffer_size += bytes_encoded;

  lame->in_pts += frame->valid_samples;
  
  if((bytes_encoded > 0) && (flush_packets(lame, 0) < 0))
    return GAVL_SINK_ERROR;
  else
    return GAVL_SINK_OK;
  }

gavl_audio_sink_t * bg_lame_open(bg_lame_t * lame,
                                 gavl_compression_info_t * ci,
                                 gavl_audio_format_t * fmt,
                                 gavl_metadata_t * m)
  {
  /* Copy and adjust format */
  
  fmt->sample_format = GAVL_SAMPLE_FLOAT;
  fmt->interleave_mode = GAVL_INTERLEAVE_NONE;
  fmt->samplerate = gavl_nearest_samplerate(fmt->samplerate,
                                            samplerates);
  
  if(fmt->num_channels > 2)
    {
    fmt->num_channels = 2;
    fmt->channel_locations[0] = GAVL_CHID_NONE;
    gavl_set_channel_setup(fmt);
    }

  if(lame_set_in_samplerate(lame->lame, fmt->samplerate))
    bg_log(BG_LOG_ERROR, LOG_DOMAIN,  "lame_set_in_samplerate failed");
  if(lame_set_num_channels(lame->lame,  fmt->num_channels))
    bg_log(BG_LOG_ERROR, LOG_DOMAIN,  "lame_set_num_channels failed");

  if(lame_set_scale(lame->lame, 32767.0))
    bg_log(BG_LOG_ERROR, LOG_DOMAIN,  "lame_set_scale failed");
  
  /* Finalize configuration and do some sanity checks */

  switch(lame->vbr_mode)
    {
    case vbr_abr:
      /* Average bitrate */
      if(lame_set_VBR_q(lame->lame, lame->vbr_quality))
        bg_log(BG_LOG_ERROR, LOG_DOMAIN,  "lame_set_VBR_q failed");

      if(lame_set_VBR_mean_bitrate_kbps(lame->lame, lame->abr_bitrate))
        bg_log(BG_LOG_ERROR, LOG_DOMAIN,
               "lame_set_VBR_mean_bitrate_kbps failed");
        
      if(lame->abr_min_bitrate)
        {
        lame->abr_min_bitrate =
          get_bitrate(lame->abr_min_bitrate, fmt->samplerate);
        if(lame->abr_min_bitrate > lame->abr_bitrate)
          {
          lame->abr_min_bitrate = get_bitrate(8, fmt->samplerate);
          }
        if(lame_set_VBR_min_bitrate_kbps(lame->lame, lame->abr_min_bitrate))
          bg_log(BG_LOG_ERROR, LOG_DOMAIN,
                 "lame_set_VBR_min_bitrate_kbps failed");
        }
      if(lame->abr_max_bitrate)
        {
        lame->abr_max_bitrate =
          get_bitrate(lame->abr_max_bitrate, fmt->samplerate);
        if(lame->abr_max_bitrate < lame->abr_bitrate)
          {
          lame->abr_max_bitrate = get_bitrate(320, fmt->samplerate);
          }
        if(lame_set_VBR_max_bitrate_kbps(lame->lame, lame->abr_max_bitrate))
          bg_log(BG_LOG_ERROR, LOG_DOMAIN,
                 "lame_set_VBR_max_bitrate_kbps failed");
        }
      break;
    case vbr_default:
      if(lame_set_VBR_q(lame->lame, lame->vbr_quality))
        bg_log(BG_LOG_ERROR, LOG_DOMAIN,  "lame_set_VBR_q failed");
      break;
    case vbr_off:
      lame->cbr_bitrate =
        get_bitrate(lame->cbr_bitrate, fmt->samplerate);
      if(lame_set_brate(lame->lame, lame->cbr_bitrate))
        bg_log(BG_LOG_ERROR, LOG_DOMAIN,  "lame_set_brate failed");
      break;
    default:
      break;
    }
  
  /* Write no xing header */
  lame_set_bWriteVbrTag(lame->lame, 0);
  
  if(lame_init_params(lame->lame) < 0)
    bg_log(BG_LOG_ERROR, LOG_DOMAIN,  "lame_init_params failed");
  
  fmt->samples_per_frame = lame_get_framesize(lame->lame);
  
  gavl_audio_format_copy(&lame->format, fmt);
  lame->sink = gavl_audio_sink_create(NULL, write_audio_func, lame, &lame->format);

  /* Allocate output buffer */
  
  lame->buffer_alloc = (5 * fmt->samples_per_frame) / 4 + 7200 + 4096;
  lame->buffer = malloc(lame->buffer_alloc);

  lame->in_pts = GAVL_TIME_UNDEFINED;
  lame->out_pts = GAVL_TIME_UNDEFINED;

  if(ci)
    {
    ci->id = GAVL_CODEC_ID_MP3;
    if(lame->vbr_mode == vbr_off)
      ci->bitrate = lame->cbr_bitrate * 1000;
    else
      ci->bitrate = GAVL_BITRATE_VBR;
    }
  if(m)
    {
    /* TODO: Set software */
    }

  
  /* Delay taken from ffmpeg */
  lame->delay = lame_get_encoder_delay(lame->lame) + 528 + 1; 
  
  return lame->sink;
  
  
  }

void bg_lame_destroy(bg_lame_t * lame)
  {
  int bytes_encoded;
  
  /* Flush */
  
  bytes_encoded = lame_encode_flush(lame->lame,
                                    lame->buffer + lame->buffer_size, 
                                    lame->buffer_alloc - lame->buffer_size);

  lame->buffer_size += bytes_encoded;

  if(lame->buffer_size)
    flush_packets(lame, 1);
  
  /* Destroy */
  if(lame->lame)
    {
    lame_close(lame->lame);
    lame->lame = NULL;
    }

  if(lame->buffer)
    {
    free(lame->buffer);
    lame->buffer = NULL;
    }

  if(lame->sink)
    {
    gavl_audio_sink_destroy(lame->sink);
    lame->sink = NULL;
    }
  
  gavl_packet_free(&lame->gp);

  free(lame);
  }
