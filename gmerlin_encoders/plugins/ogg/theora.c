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


#include <string.h>
#include <stdlib.h>

#include <config.h>

#include <gavl/metatags.h>
#include <gavl/numptr.h>

#include <gmerlin/translation.h>
#include <gmerlin/plugin.h>
#include <gmerlin/pluginfuncs.h>
#include <gmerlin/utils.h>
#include <gmerlin/log.h>
#define LOG_DOMAIN "oggtheora"

#include <theora/theoraenc.h>

#include "ogg_common.h"

/*
 *  2-pass encoding was introduced in 1.1.0
 *  However use the THEORA_1_1 macro for other things
 *  introduced in that version
 */

#ifdef TH_ENCCTL_2PASS_IN
#define THEORA_1_1
#endif

typedef struct
  {
  /* Ogg theora stuff */
    
  th_info        ti;
  th_comment     tc;
  th_enc_ctx   * ts;
  
  gavl_video_format_t * format;
  int cbr;
  int max_keyframe_interval;
  
  th_ycbcr_buffer buf;

  float speed;
  
  int64_t frame_counter;
  
#ifdef THEORA_1_1
  int pass;
  FILE * stats_file;
  
  char * stats_buf;
  char * stats_ptr;
  int stats_size;
  int rate_flags;
#endif
  
  bg_encoder_framerate_t fr;
  
  int frames_since_keyframe;
  int64_t last_keyframe;

  gavl_packet_sink_t * psink;
  int64_t pts;
  
  } theora_t;

static void set_packet_sink(void * data, gavl_packet_sink_t * psink)
  {
  theora_t * theora = data;
  theora->psink = psink;
  }


static void * create_theora()
  {
  theora_t * ret;
  ret = calloc(1, sizeof(*ret));
  th_info_init(&ret->ti);
  
  return ret;
  }

static const bg_parameter_info_t parameters[] =
  {
    {
      .name =        "cbr",
      .long_name =   TRS("Use constant bitrate"),
      .type =        BG_PARAMETER_CHECKBUTTON,
      .help_string = TRS("For constant bitrate, choose a target bitrate. For variable bitrate, choose a nominal quality."),
    },
    {
      .name =        "target_bitrate",
      .long_name =   TRS("Target bitrate (kbps)"),
      .type =        BG_PARAMETER_INT,
      .val_min =     { .val_i = 45 },
      .val_max =     { .val_i = 2000 },
      .val_default = { .val_i = 250 },
    },
    {
      .name =      "quality",
      .long_name = TRS("Nominal quality"),
      .type =      BG_PARAMETER_SLIDER_INT,
      .val_min =     { .val_i = 0 },
      .val_max =     { .val_i = 63 },
      .val_default = { .val_i = 10 },
      .num_digits =  1,
      .help_string = TRS("Quality for VBR mode\n\
63: best (largest output file)\n\
0:  worst (smallest output file)"),
    },
    {
      .name =      "max_keyframe_interval",
      .long_name = TRS("Maximum keyframe interval"),
      .type =      BG_PARAMETER_INT,
      .val_min =     { .val_i = 1    },
      .val_max =     { .val_i = 1000 },
      .val_default = { .val_i = 64   },
    },
#ifdef THEORA_1_1
    {
      .name = "drop_frames",
      .long_name = TRS("Enable frame dropping"),
      .type = BG_PARAMETER_CHECKBUTTON,
      .val_default = { .val_i = 1 },
      .help_string = TRS("Drop frames to keep within bitrate buffer constraints. This can have a severe impact on quality, but is the only way to ensure that bitrate targets are met at low rates during sudden bursts of activity."),
    },
    {
      .name = "cap_overflow",
      .long_name = TRS("Don't bank excess bits for later use"),
      .type = BG_PARAMETER_CHECKBUTTON,
      .val_default = { .val_i = 1 },
      .help_string = TRS("Ignore bitrate buffer overflows. If the encoder uses so few bits that the reservoir of available bits overflows, ignore the excess. The encoder will not try to use these extra bits in future frames. At high rates this may cause the result to be undersized, but allows a client to play the stream using a finite buffer; it should normally be enabled."),
    },
    {
      .name = "cap_underflow",
      .long_name = TRS("Don't try to make up shortfalls later"),
      .type = BG_PARAMETER_CHECKBUTTON,
      .val_default = { .val_i = 0 },
      .help_string = TRS("Ignore bitrate buffer underflows. If the encoder uses so many bits that the reservoir of available bits underflows, ignore the deficit. The encoder will not try to make up these extra bits in future frames. At low rates this may cause the result to be oversized; it should normally be disabled."),
    },
#endif
    {
      .name =      "speed",
      .long_name = TRS("Encoding speed"),
      .type =      BG_PARAMETER_SLIDER_FLOAT,
      .val_min =     { .val_f = 0.0 },
      .val_max =     { .val_f = 1.0 },
      .val_default = { .val_f = 0.0 },
      .num_digits  = 2,
      .help_string = TRS("Higher speed levels favor quicker encoding over better quality per bit. Depending on the encoding mode, and the internal algorithms used, quality may actually improve, but in this case bitrate will also likely increase. In any case, overall rate/distortion performance will probably decrease."),
    },
    BG_ENCODER_FRAMERATE_PARAMS,
    { /* End of parameters */ }
  };

static const bg_parameter_info_t * get_parameters_theora()
  {
  return parameters;
  }

static void set_parameter_theora(void * data, const char * name,
                                 const bg_parameter_value_t * v)
  {
  theora_t * theora = data;
  
  if(!name)
    return;
  else if(bg_encoder_set_framerate_parameter(&theora->fr, name, v))
    return;
  else if(!strcmp(name, "target_bitrate"))
    theora->ti.target_bitrate = v->val_i * 1000;
  else if(!strcmp(name, "quality"))
    theora->ti.quality = v->val_i;
  else if(!strcmp(name, "cbr"))
    theora->cbr = v->val_i;
  else if(!strcmp(name, "max_keyframe_interval"))
    theora->max_keyframe_interval = v->val_i;
  else if(!strcmp(name, "speed"))
    theora->speed = v->val_f;
#ifdef THEORA_1_1
  else if(!strcmp(name, "drop_frames"))
    {
    if(v->val_i)
      theora->rate_flags |= TH_RATECTL_DROP_FRAMES;
    else
      theora->rate_flags &= ~TH_RATECTL_DROP_FRAMES;
    }
  else if(!strcmp(name, "cap_overflow"))
    {
    if(v->val_i)
      theora->rate_flags |= TH_RATECTL_CAP_OVERFLOW;
    else
      theora->rate_flags &= ~TH_RATECTL_CAP_OVERFLOW;
    }
  else if(!strcmp(name, "cap_underflow"))
    {
    if(v->val_i)
      theora->rate_flags |= TH_RATECTL_CAP_UNDERFLOW;
    else
      theora->rate_flags &= ~TH_RATECTL_CAP_UNDERFLOW;
    }
#endif
  }



static const gavl_pixelformat_t supported_pixelformats[] =
  {
    GAVL_YUV_420_P,
#ifdef THEORA_1_1
    GAVL_YUV_422_P,
    GAVL_YUV_444_P,
#endif
    GAVL_PIXELFORMAT_NONE,
  };

#define PTR_2_32BE(p) \
  ((*(p) << 24) |     \
   (*(p+1) << 16) |   \
   (*(p+2) << 8) |    \
   *(p+3))

#define PTR_2_32LE(p) \
  ((*(p+3) << 24) |     \
   (*(p+2) << 16) |   \
   (*(p+1) << 8) |    \
   *(p))

#define INT32LE_2_PTR(num, p) \
(p)[0] = (num) & 0xff; \
(p)[1] = ((num)>>8) & 0xff; \
(p)[2] = ((num)>>16) & 0xff; \
(p)[3] = ((num)>>24) & 0xff

#define INT32BE_2_PTR(num, p) \
(p)[3] = (num) & 0xff; \
(p)[2] = ((num)>>8) & 0xff; \
(p)[1] = ((num)>>16) & 0xff; \
(p)[0] = ((num)>>24) & 0xff


#define WRITE_STRING(s, p) string_len = strlen(s); \
  INT32LE_2_PTR(string_len, p); p += 4; \
  memcpy(p, s, string_len); \
  p+=string_len;

static const uint8_t comment_header[7] = { 0x81, 't', 'h', 'e', 'o', 'r', 'a' };

static int init_compressed_theora(bg_ogg_stream_t * s)
  {
  ogg_packet packet;
  
  uint8_t * ptr;
  uint32_t len;
  
  theora_t * theora = s->codec_priv;
  
  memset(&packet, 0, sizeof(packet));

  /* Write ID packet */
  ptr = s->ci.global_header;
  
  len = PTR_2_32BE(ptr); ptr += 4;

  packet.packet = ptr;
  packet.bytes  = len;

  theora->ti.keyframe_granule_shift = 
    (char) ((ptr[40] & 0x03) << 3);

  theora->ti.keyframe_granule_shift |=
    (ptr[41] & 0xe0) >> 5;

  if(!bg_ogg_stream_write_header_packet(s, &packet))
    return 0;
  
  ptr += len;

  /* Skip comment from codec header */

  len = GAVL_PTR_2_32BE(ptr); ptr += 4;
  ptr += len;

  /* Build comment packet */

  bg_ogg_create_comment_packet(comment_header, 7,
                               &s->m_stream, s->m_global, 1, &packet);

  if(!bg_ogg_stream_write_header_packet(s, &packet))
    return 0;

  bg_ogg_free_comment_packet(&packet);
  
  /* Codepages */
  len = PTR_2_32BE(ptr); ptr += 4;
  
  packet.packet = ptr;
  packet.bytes  = len;

  if(!bg_ogg_stream_write_header_packet(s, &packet))
    return 0;
  
  theora->frames_since_keyframe = -1;

  return 1;
  
  }

static gavl_sink_status_t
write_video_frame_theora(void * data, gavl_video_frame_t * frame)
  {
  theora_t * theora;
  int result;
  int i;
  ogg_packet op;
  gavl_packet_t gp;
  //  int64_t frame_index;
  
  theora = data;
  
  for(i = 0; i < 3; i++)
    {
    theora->buf[i].stride = frame->strides[i];
    theora->buf[i].data   = frame->planes[i];
    }

#ifdef THEORA_1_1
  if(theora->pass == 2)
    {
    /* Input pass data */
    int ret;
    
    while(theora->stats_ptr - theora->stats_buf < theora->stats_size)
      {
      
      ret = th_encode_ctl(theora->ts,
                          TH_ENCCTL_2PASS_IN,
                          theora->stats_ptr,
                          theora->stats_size -
                          (theora->stats_ptr - theora->stats_buf));

      if(ret < 0)
        {
        bg_log(BG_LOG_ERROR, LOG_DOMAIN, "passing 2 pass data failed");
        return GAVL_SINK_ERROR;
        }
      else if(!ret)
        break;
      else
        {
        theora->stats_ptr += ret;
        }
      }
    }
#endif
  
  result = th_encode_ycbcr_in(theora->ts, theora->buf);

#ifdef THEORA_1_1
  /* Output pass data */
  if(theora->pass == 1)
    {
    int ret;
    char * buf;
    ret = th_encode_ctl(theora->ts,
                        TH_ENCCTL_2PASS_OUT,
                        &buf, sizeof(buf));
    if(ret < 0)
      {
      bg_log(BG_LOG_ERROR, LOG_DOMAIN, "getting 2 pass data failed");
      return GAVL_SINK_ERROR;
      }
    fwrite(buf, 1, ret, theora->stats_file);
    }
#endif

  /* Output packet */
  
  if(!th_encode_packetout(theora->ts, 0, &op))
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN,
           "Theora encoder produced no packet");
    return GAVL_SINK_ERROR;
    }
  
  gavl_packet_init(&gp);
  bg_ogg_packet_to_gavl(&op, &gp, NULL);
  
  gp.pts      = theora->pts;
  gp.duration = theora->format->frame_duration;

  theora->pts += theora->format->frame_duration;
  
  if(!(op.packet[0] & 0x40)) // Keyframe
    gp.flags |= GAVL_PACKET_TYPE_I | GAVL_PACKET_KEYFRAME;
  else
    gp.flags |= GAVL_PACKET_TYPE_P;
  
#if 0
  fprintf(stderr, "Encoding granulepos: %lld %lld / %d\n",
          op.granulepos,
          op.granulepos >> theora->ti.keyframe_granule_shift,
          op.granulepos & ((1<<theora->ti.keyframe_granule_shift)-1));
#endif
  return gavl_packet_sink_put_packet(theora->psink, &gp);
  }


static gavl_video_sink_t *
init_theora(void * data, gavl_compression_info_t * ci,
            gavl_video_format_t * format,
            gavl_metadata_t * stream_metadata)
  {
  int sub_h, sub_v;
  int arg_i1, arg_i2;
  uint8_t * ptr;
  ogg_packet op;
  int header_packets;
  
  theora_t * theora = data;

  theora->format = format;

  bg_encoder_set_framerate(&theora->fr, format);
  
  /* Set video format */
  theora->ti.frame_width  = ((format->image_width  + 15)/16*16);
  theora->ti.frame_height = ((format->image_height + 15)/16*16);
  theora->ti.pic_width = format->image_width;
  theora->ti.pic_height = format->image_height;
  
  theora->ti.fps_numerator      = format->timescale;
  theora->ti.fps_denominator    = format->frame_duration;
  theora->ti.aspect_numerator   = format->pixel_width;
  theora->ti.aspect_denominator = format->pixel_height;

  format->interlace_mode = GAVL_INTERLACE_NONE;
    
  format->frame_width  = theora->ti.frame_width;
  format->frame_height = theora->ti.frame_height;
  
  if(theora->cbr)
    theora->ti.quality = 0;
  else
    theora->ti.target_bitrate = 0;

  /* Granule shift */
  theora->ti.keyframe_granule_shift = 0;

  while(1 << theora->ti.keyframe_granule_shift < theora->max_keyframe_interval)
    theora->ti.keyframe_granule_shift++;
  
  theora->ti.colorspace=TH_CS_UNSPECIFIED;

  format->pixelformat =
    gavl_pixelformat_get_best(format->pixelformat,
                              supported_pixelformats, NULL);
    
  switch(format->pixelformat)
    {
    case GAVL_YUV_420_P:
      theora->ti.pixel_fmt = TH_PF_420;
      break;
    case GAVL_YUV_422_P:
      theora->ti.pixel_fmt = TH_PF_422;
      break;
    case GAVL_YUV_444_P:
      theora->ti.pixel_fmt = TH_PF_444;
      break;
    default:
      return 0;
    }
  
  /* Initialize encoder */
  if(!(theora->ts = th_encode_alloc(&theora->ti)))
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN,  "th_encode_alloc failed");
    return 0;
    }
  /* Build comment (comments are UTF-8, good for us :-) */

  // build_comment(&theora->tc, metadata);

  /* Call encode CTLs */
  
  // Keyframe frequency

  th_encode_ctl(theora->ts,
                TH_ENCCTL_SET_KEYFRAME_FREQUENCY_FORCE,
                &theora->max_keyframe_interval, sizeof(theora->max_keyframe_interval));

#ifdef THEORA_1_1  
  // Rate flags

  th_encode_ctl(theora->ts,
                TH_ENCCTL_SET_RATE_FLAGS,
                &theora->rate_flags,sizeof(theora->rate_flags));
#endif
  // Maximum speed

  if(th_encode_ctl(theora->ts,
                   TH_ENCCTL_GET_SPLEVEL_MAX,
                   &arg_i1, sizeof(arg_i1)) != TH_EIMPL)
    {
    arg_i2 = (int)((float)arg_i1 * theora->speed + 0.5);

    if(arg_i2 > arg_i1)
      arg_i2 = arg_i1;
    
    th_encode_ctl(theora->ts, TH_ENCCTL_SET_SPLEVEL,
                  &arg_i2, sizeof(arg_i2));
    }
  
  /* Encode initial packets */

  ci->id = GAVL_CODEC_ID_THEORA;
  ci->flags = GAVL_COMPRESSION_HAS_P_FRAMES;
  
  header_packets = 0;

  /* Build global header */
  while(th_encode_flushheader(theora->ts, &theora->tc, &op) > 0)
    {
    ci->global_header = realloc(ci->global_header, 
                                ci->global_header_len + 4 + op.bytes);

    ptr = ci->global_header + ci->global_header_len;

    GAVL_32BE_2_PTR(op.bytes, ptr); ptr += 4;
    memcpy(ptr, op.packet, op.bytes);
    ci->global_header_len += 4 + op.bytes;

    if(header_packets == 1)
      {
      char * vendor;
      int vendor_len;
      
      /* Extract vendor ID */
      ptr = op.packet + 7;
      vendor_len = GAVL_PTR_2_32LE(ptr); ptr += 4;
      vendor = calloc(1, vendor_len + 1);
      memcpy(vendor, ptr, vendor_len);
      gavl_metadata_set_nocpy(stream_metadata, GAVL_META_SOFTWARE, vendor);
      fprintf(stderr, "Got vendor: %s\n", vendor);
      }
    header_packets++;
    }
  
  if(header_packets < 3)
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN,  "Got %d header packets instead of 3", header_packets);
    return 0;
    }

  /* Initialize buffer */
  
  gavl_pixelformat_chroma_sub(theora->format->pixelformat, &sub_h, &sub_v);
  theora->buf[0].width  = theora->format->frame_width;
  theora->buf[0].height = theora->format->frame_height;
  theora->buf[1].width  = theora->format->frame_width  / sub_h;
  theora->buf[1].height = theora->format->frame_height / sub_v;
  theora->buf[2].width  = theora->format->frame_width  / sub_h;
  theora->buf[2].height = theora->format->frame_height / sub_v;
  
  return gavl_video_sink_create(NULL, write_video_frame_theora, theora, theora->format);
  }

#ifdef THEORA_1_1

static int set_video_pass_theora(void * data, int pass, int total_passes,
                                 const char * stats_file)
  {
  char * buf;
  int ret;
  theora_t * theora = data;

  theora->pass = pass;

  if(theora->pass == 1)
    {
    theora->stats_file = fopen(stats_file, "wb");
    /* Get initial header */

    if(!theora->stats_file)
      {
      bg_log(BG_LOG_ERROR, LOG_DOMAIN, "couldn't open stats file %s", stats_file);
      return 0;
      }
    
    ret = th_encode_ctl(theora->ts,
                        TH_ENCCTL_2PASS_OUT,
                        &buf, sizeof(buf));

    if(ret < 0)
      {
      bg_log(BG_LOG_ERROR, LOG_DOMAIN, "getting 2 pass header failed");
      return 0;
      }
    fwrite(buf, 1, ret, theora->stats_file);
    }
  else
    {
    theora->stats_file = fopen(stats_file, "rb");

    if(!theora->stats_file)
      {
      bg_log(BG_LOG_ERROR, LOG_DOMAIN, "couldn't open stats file %s", stats_file);
      return 0;
      }

    fseek(theora->stats_file, 0, SEEK_END);
    theora->stats_size = ftell(theora->stats_file);
    fseek(theora->stats_file, 0, SEEK_SET);
    
    theora->stats_buf = malloc(theora->stats_size);
    if(fread(theora->stats_buf, 1, theora->stats_size, theora->stats_file) <
       theora->stats_size)
      {
      bg_log(BG_LOG_ERROR, LOG_DOMAIN, "couldn't read stats data");
      return 0;
      }
      
    fclose(theora->stats_file);
    theora->stats_file = 0;
    theora->stats_ptr = theora->stats_buf;
    }
  return 1;
  }
#endif




#if 0
static int write_packet_theora(void * data, gavl_packet_t * packet)
  {
  ogg_packet op;
  theora_t * theora;
  theora = data;
  
  memset(&op, 0, sizeof(op));

  op.packet = packet->data;
  op.bytes =  packet->data_len;

  if(theora->frames_since_keyframe < 0)
    {
    if(!(packet->flags & GAVL_PACKET_KEYFRAME))
      {
      bg_log(BG_LOG_ERROR, LOG_DOMAIN,
             "First packet isn't a keyframe");
      return 0;
      }
    theora->frames_since_keyframe = 0;
    theora->last_keyframe = packet->pts / theora->format->frame_duration + 1;
    }
  else if(packet->flags & GAVL_PACKET_KEYFRAME)
    {
    theora->last_keyframe += theora->frames_since_keyframe + 1;
    theora->frames_since_keyframe = 0;
    }
  else
    {
    theora->frames_since_keyframe++;
    }
  
  op.granulepos =
    (theora->last_keyframe << theora->ti.keyframe_granule_shift) +
    theora->frames_since_keyframe;
#if 0
  fprintf(stderr, "Encoding granulepos: %lld %lld / %d\n",
          op.granulepos, 
          theora->last_keyframe, theora->frames_since_keyframe);
#endif
  ogg_stream_packetin(&theora->os,&op);
  if(bg_ogg_flush(&theora->os, theora->output, 0) < 0)
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN,
           "Writing theora packet failed");
    return 0;
    }
  return 1;
  }

#endif

static void convert_packet(bg_ogg_stream_t * s, gavl_packet_t * src, ogg_packet * dst)
  {
  theora_t * theora;
  theora = s->codec_priv;

  if(theora->frames_since_keyframe < 0)
    {
    if(!(src->flags & GAVL_PACKET_KEYFRAME))
      {
      bg_log(BG_LOG_ERROR, LOG_DOMAIN,
             "First packet isn't a keyframe");
      return;
      }
    theora->frames_since_keyframe = 0;
    theora->last_keyframe = src->pts / theora->format->frame_duration + 1;
    }
  else if(src->flags & GAVL_PACKET_KEYFRAME)
    {
    theora->last_keyframe += theora->frames_since_keyframe + 1;
    theora->frames_since_keyframe = 0;
    }
  else
    {
    theora->frames_since_keyframe++;
    }
  
  dst->granulepos =
    (theora->last_keyframe << theora->ti.keyframe_granule_shift) +
    theora->frames_since_keyframe;
  }


static int close_theora(void * data)
  {
  int ret = 1;
  theora_t * theora;
  theora = data;
  
#ifdef THEORA_1_1
  if(theora->stats_file)
    fclose(theora->stats_file);
  if(theora->stats_buf)
    free(theora->stats_buf);
#endif
  
  th_comment_clear(&theora->tc);
  th_info_clear(&theora->ti);
  th_encode_free(theora->ts);
  free(theora);
  return ret;
  }


const bg_ogg_codec_t bg_theora_codec =
  {
    .name =      "theora",
    .long_name = TRS("Theora encoder"),
    .create = create_theora,

    .get_parameters = get_parameters_theora,
    .set_parameter =  set_parameter_theora,
#ifdef THEORA_1_1
    .set_video_pass = set_video_pass_theora,
#endif    
    .init_video =     init_theora,
    .init_video_compressed =     init_compressed_theora,

    .set_packet_sink = set_packet_sink,
    
    .convert_packet = convert_packet,
    .close = close_theora,
  };
