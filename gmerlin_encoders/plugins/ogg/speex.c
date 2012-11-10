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

#include <gmerlin/translation.h>
#include <gmerlin/plugin.h>
#include <gmerlin/utils.h>
#include <gmerlin/log.h>
#define LOG_DOMAIN "oggspeex"

#include <gavl/metatags.h>


#include <speex/speex.h>
#include <speex/speex_header.h>
#include <speex/speex_stereo.h>
#include <speex/speex_callbacks.h>

#include <ogg/ogg.h>
#include "ogg_common.h"

/* Newer speex version (1.1.x) don't have this */
#ifndef MAX_BYTES_PER_FRAME
#define MAX_BYTES_PER_FRAME 2000
#endif

/* Way too large but save */
#define BUFFER_SIZE (MAX_BYTES_PER_FRAME*10)

typedef struct
  {
  gavl_audio_format_t * format;
  gavl_audio_frame_t * frame;

  int modeID;

  int bitrate;
  int abr_bitrate;
  int quality;
  int complexity;
  int vbr;
  int vad;
  int dtx;
  int nframes;

  void * enc;
  SpeexBits bits;
  int lookahead;
  int lookahead_start;
  
  int frames_encoded;

  uint8_t buffer[BUFFER_SIZE];

  gavl_packet_sink_t * psink;
  SpeexHeader header;
  
  int64_t pts;
  int64_t duration;
  } speex_t;


static void * create_speex()
  {
  speex_t * ret;
  ret = calloc(1, sizeof(*ret));
  return ret;
  }

static const bg_parameter_info_t parameters[] =
  {
    {
      .name =        "mode",
      .long_name =   TRS("Speex mode"),
      .type =        BG_PARAMETER_STRINGLIST,
      .val_default = { .val_str = "auto" },
      .multi_names =  (char const *[]){ "auto", "nb",         "wb",       "uwb",            NULL },
      .multi_labels = (char const *[]){ TRS("Auto"), TRS("Narrowband"), TRS("Wideband"),
                               TRS("Ultra-wideband"), NULL },
      .help_string = TRS("Encoding mode. If you select Auto, the mode will be taken from the samplerate.")
    },
    {
      .name =      "quality",
      .long_name = TRS("Quality (10: best)"),
      .type =      BG_PARAMETER_SLIDER_INT,
      .val_min =     { .val_i = 0 },
      .val_max =     { .val_i = 10 },
      .val_default = { .val_i = 3 },
    },
    {
      .name =      "complexity",
      .long_name = TRS("Encoding complexity"),
      .type =      BG_PARAMETER_SLIDER_INT,
      .val_min =     { .val_i = 0 },
      .val_max =     { .val_i = 10 },
      .val_default = { .val_i = 3 },
    },
    {
      .name =      "nframes",
      .long_name = TRS("Frames per Ogg packet"),
      .type =      BG_PARAMETER_SLIDER_INT,
      .val_min =     { .val_i = 1 },
      .val_max =     { .val_i = 10 },
      .val_default = { .val_i = 1 },
    },
    {
      .name =        "bitrate",
      .long_name =   TRS("Bitrate (kbps)"),
      .type =        BG_PARAMETER_INT,
      .val_min =     { .val_i = 0 },
      .val_max =     { .val_i = 128 },
      .val_default = { .val_i = 8 },
      .help_string = TRS("Bitrate (in kbps). Set to 0 for seleting the standard bitrates for the encoding mode."),
    },
    {
      .name =        "vbr",
      .long_name =   TRS("Variable bitrate"),
      .type =        BG_PARAMETER_CHECKBUTTON,
    },
    {
      .name =        "abr_bitrate",
      .long_name =   TRS("Average bitrate (kbps)"),
      .type =        BG_PARAMETER_INT,
      .val_min =     { .val_i = 0 },
      .val_max =     { .val_i = 128 },
      .val_default = { .val_i = 0 },
      .help_string = TRS("Average bitrate (in kbps). Set to 0 for disabling ABR."),
    },
    {
      .name =        "vad",
      .long_name =   TRS("Use voice activity detection"),
      .type =        BG_PARAMETER_CHECKBUTTON,
    },
    {
      .name =        "dtx",
      .long_name =   TRS("Enable file-based discontinuous transmission"),
      .type =        BG_PARAMETER_CHECKBUTTON,
    },
    { /* End of parameters */ }
  };


static const bg_parameter_info_t * get_parameters_speex()
  {
  return parameters;
  }

static void set_parameter_speex(void * data, const char * name,
                                 const bg_parameter_value_t * v)
  {
  speex_t * speex;
  speex = data;
  
  if(!name)
    {
    return;
    }
  else if(!strcmp(name, "mode"))
    {
    if(!strcmp(v->val_str, "auto"))
      speex->modeID = -1;
    else if(!strcmp(v->val_str, "nb"))
      speex->modeID = SPEEX_MODEID_NB;
    else if(!strcmp(v->val_str, "wb"))
      speex->modeID = SPEEX_MODEID_WB;
    else if(!strcmp(v->val_str, "uwb"))
      speex->modeID = SPEEX_MODEID_UWB;
    }
  
  else if(!strcmp(name, "bitrate"))
    {
    speex->bitrate = v->val_i * 1000;
    }
  else if(!strcmp(name, "abr_bitrate"))
    {
    speex->abr_bitrate = v->val_i * 1000;
    }
  else if(!strcmp(name, "quality"))
    {
    speex->quality = v->val_i;
    }
  else if(!strcmp(name, "complexity"))
    {
    speex->complexity = v->val_i;
    }
  else if(!strcmp(name, "vbr"))
    {
    speex->vbr = v->val_i;
    }
  else if(!strcmp(name, "vad"))
    {
    speex->vad = v->val_i;
    }
  else if(!strcmp(name, "dtx"))
    {
    speex->dtx = v->val_i;
    }
  else if(!strcmp(name, "nframes"))
    {
    speex->nframes = v->val_i;
    }
  }

static int init_compressed_speex(bg_ogg_stream_t * s)
  {
  ogg_packet op;
  memset(&op, 0, sizeof(op));
  op.packet = (unsigned char *)s->ci.global_header;
  op.bytes = s->ci.global_header_len;
  if(!bg_ogg_stream_write_header_packet(s, &op))
    return 0;
  
  bg_ogg_create_comment_packet(NULL, 0, &s->m_stream, s->m_global, 0, &op);

  if(!bg_ogg_stream_write_header_packet(s, &op))
    return 0;
  
  bg_ogg_free_comment_packet(&op);
  return 1;
  }

static int flush_packet(speex_t * speex)
  {
  gavl_packet_t p;
  gavl_packet_init(&p);

  //  fprintf(stderr, "Flush packet\n");
  
  /* Flush packet */
  p.data_len  = speex_bits_write(&speex->bits, (char*)speex->buffer,
                                 BUFFER_SIZE);
  p.data = speex->buffer;

  p.pts = speex->pts;
  p.duration = speex->duration;

  speex->pts += speex->duration;
  speex->duration = 0;
  
  if(gavl_packet_sink_put_packet(speex->psink, &p) != GAVL_SINK_OK)
    return 0;
  speex_bits_reset(&speex->bits);
  
  //  fprintf(stderr, "Flush packet done\n");
  return 1;
  }

static int encode_frame(speex_t * speex)
  {
  //  fprintf(stderr, "Encode frame\n");

  if(speex->format->num_channels == 2)
    speex_encode_stereo_int(speex->frame->samples.s_16,
                            speex->format->samples_per_frame,
                            &speex->bits);
  
  speex_encode_int(speex->enc, speex->frame->samples.s_16, &speex->bits);
  
  speex->duration += speex->frame->valid_samples;
  
  gavl_audio_frame_mute(speex->frame, speex->format);
  speex->frame->valid_samples = 0;
  
  speex->frames_encoded++;

  if(speex->frames_encoded == speex->nframes)
    {
    if(!flush_packet(speex))
      return 0;
    speex->frames_encoded = 0;
    }
  //  fprintf(stderr, "Encode frame done\n");

  return 1;
  }

static int flush(speex_t * speex)
  {
  if(!speex->frame->valid_samples)
    return 1;
  
  if(!encode_frame(speex))
    return 0;

  if(!speex->frames_encoded)
    return 1;
  
  while(speex->frames_encoded < speex->nframes)
    {
    speex_bits_pack(&speex->bits, 15, 5);
    speex->frames_encoded++;
    }

  if(!flush_packet(speex))
    return 0;
  
  return 1;
  }

static void convert_packet(bg_ogg_stream_t * s, gavl_packet_t * src, ogg_packet * dst)
  {
  speex_t * speex = s->codec_priv;
  dst->granulepos -= speex->lookahead;
  }

static gavl_sink_status_t
write_audio_frame_speex(void * data, gavl_audio_frame_t * frame)
  {
  int samples_read = 0;
  int samples_copied;
  speex_t * speex = data;

  /* Insert lookahead samples at the beginning */
  while(speex->lookahead_start)
    {

    speex->frame->valid_samples = speex->lookahead_start;
    if(speex->frame->valid_samples > speex->format->samples_per_frame)
      speex->frame->valid_samples = speex->format->samples_per_frame;

    speex->lookahead_start -= speex->frame->valid_samples;
    
    if(speex->frame->valid_samples == speex->format->samples_per_frame)
      {
      if(!encode_frame(speex))
        return GAVL_SINK_ERROR;
      }
    }
  
  while(samples_read < frame->valid_samples)
    {
    samples_copied =
      gavl_audio_frame_copy(speex->format,
                            speex->frame,
                            frame,
                            speex->frame->valid_samples, /* dst_pos */
                            samples_read,                /* src_pos */
                            speex->format->samples_per_frame -
                            speex->frame->valid_samples, /* dst_size */
                            frame->valid_samples - samples_read /* src_size */ );
    speex->frame->valid_samples += samples_copied;
    samples_read += samples_copied;

    if(speex->frame->valid_samples == speex->format->samples_per_frame)
      {
      if(!encode_frame(speex))
        return GAVL_SINK_ERROR;
      }
    }
  return GAVL_SINK_OK;
  }


static gavl_audio_sink_t * init_speex(void * data,
                                      gavl_audio_format_t * format,
                                      gavl_metadata_t * stream_metadata,
                                      gavl_compression_info_t * ci)
  {
  float quality_f;
  const SpeexMode *mode=NULL;
  int header_len;

  char * vendor_string;
  char * version;
  
  speex_t * speex = data;

  speex->format = format;

  /* Adjust the format */

  speex->format->interleave_mode = GAVL_INTERLEAVE_ALL;
  speex->format->sample_format = GAVL_SAMPLE_S16;
  if(speex->format->samplerate > 48000)
    speex->format->samplerate = 48000;
  if(speex->format->samplerate < 6000)
    speex->format->samplerate = 6000;

  if(speex->format->num_channels > 2)
    {
    speex->format->num_channels = 2;
    speex->format->channel_locations[0] = GAVL_CHID_NONE;
    gavl_set_channel_setup(speex->format);
    }
  
  /* Decide encoding mode */
  
  if(speex->modeID == -1)
    {
    if(speex->format->samplerate > 25000)
      speex->modeID = SPEEX_MODEID_UWB;
    else if(speex->format->samplerate > 12500)
      speex->modeID = SPEEX_MODEID_WB;
    else
      speex->modeID = SPEEX_MODEID_NB;
    }

  /* Setup header and mode */
  
  mode = speex_lib_get_mode(speex->modeID);
  
  speex_init_header(&speex->header, speex->format->samplerate, 1, mode);
  speex->header.frames_per_packet=speex->nframes;
  speex->header.vbr=speex->vbr;
  speex->header.nb_channels = speex->format->num_channels;

  /* Initialize encoder structs */
  
  speex->enc = speex_encoder_init(mode);
  speex_bits_init(&speex->bits);
  
  /* Setup encoding parameters */

  speex_encoder_ctl(speex->enc, SPEEX_SET_COMPLEXITY,
                    &speex->complexity);
  speex_encoder_ctl(speex->enc, SPEEX_SET_SAMPLING_RATE,
                    &speex->format->samplerate);

  if(speex->vbr)
    {
    quality_f = (float)(speex->quality);
    speex_encoder_ctl(speex->enc, SPEEX_SET_VBR_QUALITY, &quality_f);
    }
  else
    speex_encoder_ctl(speex->enc, SPEEX_SET_QUALITY, &speex->quality);

  if(speex->bitrate)
    speex_encoder_ctl(speex->enc, SPEEX_SET_BITRATE, &speex->bitrate);
  if(speex->vbr)
    speex_encoder_ctl(speex->enc, SPEEX_SET_VBR, &speex->vbr);
  else if(speex->vad)
    speex_encoder_ctl(speex->enc, SPEEX_SET_VAD, &speex->vad);
  if(speex->dtx)
    speex_encoder_ctl(speex->enc, SPEEX_SET_VAD, &speex->dtx);
  if(speex->abr_bitrate)
    speex_encoder_ctl(speex->enc, SPEEX_SET_ABR, &speex->abr_bitrate);
  
  speex_encoder_ctl(speex->enc, SPEEX_GET_FRAME_SIZE,
                    &speex->format->samples_per_frame);
  speex_encoder_ctl(speex->enc, SPEEX_GET_LOOKAHEAD,
                    &speex->lookahead);

  speex->lookahead_start = speex->lookahead;
  
  /* Allocate temporary frame */

  speex->frame = gavl_audio_frame_create(speex->format);
  gavl_audio_frame_mute(speex->frame, speex->format);
  
  /* Build header */

  ci->global_header = (unsigned char *)speex_header_to_packet(&speex->header,
                                                              &header_len);
  ci->global_header_len = header_len;
  ci->id = GAVL_CODEC_ID_SPEEX;
  
  bg_hexdump(ci->global_header, ci->global_header_len, 16); 
  
  /* Extract vendor string */
  
  speex_lib_ctl(SPEEX_LIB_GET_VERSION_STRING, &version);
  vendor_string = bg_sprintf("Speex %s", version);
  gavl_metadata_set_nocpy(stream_metadata, GAVL_META_SOFTWARE,
                          vendor_string);
  
  return gavl_audio_sink_create(NULL, write_audio_frame_speex, speex,
                                speex->format);
  }

static void set_packet_sink(void * data, gavl_packet_sink_t * psink)
  {
  speex_t * speex = data;
  speex->psink = psink;
  }

static int close_speex(void * data)
  {
  int ret = 1;
  speex_t * speex;
  speex = data;

  if(!flush(speex))
    ret = 0;
  
  gavl_audio_frame_destroy(speex->frame);
  speex_encoder_destroy(speex->enc);
  speex_bits_destroy(&speex->bits);
  
  free(speex);
  return ret;
  }


const bg_ogg_codec_t bg_speex_codec =
  {
    .name =      "speex",
    .long_name = TRS("Speex encoder"),
    .create = create_speex,

    .get_parameters = get_parameters_speex,
    .set_parameter =  set_parameter_speex,
    
    .init_audio =     init_speex,
    
    //  int (*init_video)(void*, gavl_video_format_t * format);

    .init_audio_compressed = init_compressed_speex,
    .set_packet_sink = set_packet_sink,

    .convert_packet = convert_packet,
    
    .close = close_speex,
  };
