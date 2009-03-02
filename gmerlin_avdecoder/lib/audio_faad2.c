/*****************************************************************
 * gmerlin-avdecoder - a general purpose multimedia decoding library
 *
 * Copyright (c) 2001 - 2008 Members of the Gmerlin project
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <config.h>
#include <avdec_private.h>
#include <codecs.h>

#ifdef HAVE_NEAACDEC_H
#include <neaacdec.h>

/*
 *  Backwards compatibility names (currently in neaacdec.h,
 *  but might be removed in future versions)
 */
#ifndef faacDecHandle
/* structs */
#define faacDecHandle                  NeAACDecHandle
#define faacDecConfiguration           NeAACDecConfiguration
#define faacDecConfigurationPtr        NeAACDecConfigurationPtr
#define faacDecFrameInfo               NeAACDecFrameInfo
/* functions */
#define faacDecGetErrorMessage         NeAACDecGetErrorMessage
#define faacDecSetConfiguration        NeAACDecSetConfiguration
#define faacDecGetCurrentConfiguration NeAACDecGetCurrentConfiguration
#define faacDecInit                    NeAACDecInit
#define faacDecInit2                   NeAACDecInit2
#define faacDecInitDRM                 NeAACDecInitDRM
#define faacDecPostSeekReset           NeAACDecPostSeekReset
#define faacDecOpen                    NeAACDecOpen
#define faacDecClose                   NeAACDecClose
#define faacDecDecode                  NeAACDecDecode
#define AudioSpecificConfig            NeAACDecAudioSpecificConfig
#endif

#else
#include <faad.h>
#endif

#define LOG_DOMAIN "faad2"

// #define DUMP_DECODE

typedef struct
  {
  bgav_bytebuffer_t buf;
  faacDecHandle dec;
  float * sample_buffer;
  
  int64_t last_position; /* For parsing only */
  
  } faad_priv_t;

static int get_data(bgav_stream_t * s)
  {
  faad_priv_t * priv;
  bgav_packet_t * p;
  
  priv = (faad_priv_t *)(s->data.audio.decoder->priv);

  
  p = bgav_demuxer_get_packet_read(s->demuxer, s);
  if(!p)
    return 0;

  /* If we know the number of samples (i.e. when the packet comes
     from an mp4/mov container), flush the buffer to remove previous
     padding bytes */
  if(p->duration)
    bgav_bytebuffer_flush(&priv->buf);
  
  bgav_bytebuffer_append(&priv->buf, p, 0);
  bgav_demuxer_done_packet_read(s->demuxer, p);
  
  return 1;
  }

static const struct
  {
  int faad_channel;
  gavl_channel_id_t gavl_channel;
  }
channels[] =
  {
    { FRONT_CHANNEL_CENTER,  GAVL_CHID_FRONT_CENTER },
    { FRONT_CHANNEL_LEFT,    GAVL_CHID_FRONT_LEFT },
    { FRONT_CHANNEL_RIGHT,   GAVL_CHID_FRONT_RIGHT },
    { SIDE_CHANNEL_LEFT,     GAVL_CHID_SIDE_LEFT },
    { SIDE_CHANNEL_RIGHT,    GAVL_CHID_SIDE_RIGHT },
    { BACK_CHANNEL_LEFT,     GAVL_CHID_REAR_LEFT },
    { BACK_CHANNEL_RIGHT,    GAVL_CHID_REAR_RIGHT },
    { BACK_CHANNEL_CENTER,   GAVL_CHID_REAR_CENTER },
    { LFE_CHANNEL,           GAVL_CHID_LFE },
    { UNKNOWN_CHANNEL,       GAVL_CHID_NONE }
  };

static gavl_channel_id_t get_channel(int ch)
  {
  int i;
  for(i = 0; i < sizeof(channels)/sizeof(channels[0]); i++)
    {
    if(channels[i].faad_channel == ch)
      return channels[i].gavl_channel;
    }
  return GAVL_CHID_AUX;
  }

static void set_channel_setup(faacDecFrameInfo * frame_info,
                              gavl_audio_format_t * format)
  {
  int i;

  /* This method fails for HE-AAC streams. So we
     use our own detection for mono and stereo */
  if(format->num_channels > 2)
    {
    for(i = 0; i < format->num_channels; i++)
      format->channel_locations[i] =
        get_channel(frame_info->channel_position[i]);
    }
  else
    gavl_set_channel_setup(format);
  
  }

static int decode_frame_faad2(bgav_stream_t * s)
  {
  faacDecFrameInfo frame_info;
  faad_priv_t * priv;
  int parse = (s->action == BGAV_STREAM_PARSE);
    
  priv = (faad_priv_t *)(s->data.audio.decoder->priv);

  memset(&frame_info, 0, sizeof(&frame_info));
  
  if(priv->buf.size < FAAD_MIN_STREAMSIZE)
    if(parse || !get_data(s))
      return 0;
  
  while(1)
    {
#ifdef DUMP_DECODE
    bgav_dprintf("faacDecDecode %d bytes\n", priv->buf.size);
    bgav_hexdump(priv->buf.buffer, 7, 7);
#endif
   
    s->data.audio.frame->samples.f = faacDecDecode(priv->dec,
                                                   &frame_info,
                                                   priv->buf.buffer,
                                                   priv->buf.size);
    
#ifdef DUMP_DECODE
    bgav_dprintf("Used %d bytes, ptr: %p, samples: %d\n",
                 frame_info.bytesconsumed, priv->frame->samples.f,
                 frame_info.samples);
#endif
   
    bgav_bytebuffer_remove(&priv->buf, frame_info.bytesconsumed);
    
    if(!s->data.audio.frame->samples.f)
      {
      if(frame_info.error == 14) /* Too little data */
        {
        if(parse || !get_data(s))
          return 0;
        }
      else
        {
        bgav_log(s->opt, BGAV_LOG_ERROR, LOG_DOMAIN,
                 "faacDecDecode failed %s",
                 faacDecGetErrorMessage(frame_info.error));
        bgav_bytebuffer_flush(&priv->buf);
        if(parse || !get_data(s))
          return 0;
        //    priv->data_size = 0;
        //    priv->frame->valid_samples = 0;
        // return 0; /* Recatching the stream is doomed to failure, so we end here */
        }
      }
    else
      break;
    }

  /* If decoding didn't fail, we might have some silent samples.
     faad2 seems to be reporting 0 samples in this case :( */
  
  if(!frame_info.samples)
    s->data.audio.frame->valid_samples = s->data.audio.format.samples_per_frame;
  else
    s->data.audio.frame->valid_samples = frame_info.samples  / s->data.audio.format.num_channels;
  
  if(s->data.audio.format.channel_locations[0] == GAVL_CHID_NONE)
    {
    set_channel_setup(&frame_info,
                      &(s->data.audio.format));
    }
  if(!s->description)
    {
    switch(frame_info.object_type)
      {
      case MAIN:
        s->description = bgav_sprintf("%s", "AAC Main profile");
        break;
      case LC:
        s->description = bgav_sprintf("%s", "AAC Low Complexity profile (LC)");
        break;
      case SSR:
        s->description = bgav_sprintf("%s", "AAC Scalable Sample Rate profile (SSR)");
        break;
      case LTP:
        s->description = bgav_sprintf("%s", "AAC Long Term Prediction (LTP)");
        break;
      case HE_AAC:
        s->description = bgav_sprintf("%s", "HE-AAC");
        break;
      case ER_LC:
      case ER_LTP:
      case LD:
      case DRM_ER_LC: /* special object type for DRM */
        s->description = bgav_sprintf("%s", "MPEG_2/4 AAC");
        break;
      }
    }

    
  return 1;
  }

static int init_faad2(bgav_stream_t * s)
  {
  faad_priv_t * priv;
  unsigned long samplerate = 0;
  unsigned char channels;
  char result;
  faacDecConfigurationPtr cfg;
  
  priv = calloc(1, sizeof(*priv));
  priv->dec = faacDecOpen();
  s->data.audio.decoder->priv = priv;
  
  /* Init the library using a DecoderSpecificInfo */
  
  if(!s->ext_size)
    {
    if(!get_data(s))
      return 0;

    result = faacDecInit(priv->dec, priv->buf.buffer,
                         priv->buf.size,
                         &samplerate, &channels);
    bgav_bytebuffer_remove(&priv->buf, result);
    }
  else
    {
    result = faacDecInit2(priv->dec, s->ext_data,
                          s->ext_size,
                          &samplerate, &channels);
    }

  /* Some mp4 files have a wrong samplerate in the sample description,
     so we correct it here */
  if(samplerate == 2 * s->data.audio.format.samplerate)
    {
    fprintf(stderr, "Detected HE-AAC\n");
    bgav_hexdump(s->ext_data, s->ext_size, 16);
    
    s->data.audio.format.samples_per_frame = 2048;
    if(s->duration)
      s->duration *= 2;
    }
  else
    {
    s->data.audio.format.samples_per_frame = 1024;
    fprintf(stderr, "Detected NO HE-AAC\n");
    bgav_hexdump(s->ext_data, s->ext_size, 16);
    }
  s->data.audio.format.samplerate = samplerate;
  
  s->data.audio.format.num_channels = channels;
  s->data.audio.format.sample_format = GAVL_SAMPLE_FLOAT;
  //  s->data.audio.format.sample_format = GAVL_SAMPLE_S16;
  s->data.audio.format.interleave_mode = GAVL_INTERLEAVE_ALL;
  
  cfg = faacDecGetCurrentConfiguration(priv->dec);
  cfg->outputFormat = FAAD_FMT_FLOAT;
  // cfg->outputFormat = FAAD_FMT_16BIT;
  faacDecSetConfiguration(priv->dec, cfg);

  if(s->action != BGAV_STREAM_PARSE)
    {
    /* Decode a first frame to get the channel setup and the description */
    if(!decode_frame_faad2(s))
      return 0;
    }
  return 1;
  }


// static int frame_number = 0;


static void close_faad2(bgav_stream_t * s)
  {
  faad_priv_t * priv;
  priv = (faad_priv_t *)(s->data.audio.decoder->priv);
  if(priv->dec)
    faacDecClose(priv->dec);

  bgav_bytebuffer_free(&priv->buf);
  free(priv);
  }

static void resync_faad2(bgav_stream_t * s)
  {
  faad_priv_t * priv;
  priv = (faad_priv_t *)(s->data.audio.decoder->priv);

  bgav_bytebuffer_flush(&priv->buf);
  }

static void parse_faad2(bgav_stream_t * s)
  {
  bgav_packet_t * p;
  faad_priv_t * priv;
  int old_buffer_size;
  int64_t position;

  priv = s->data.audio.decoder->priv;
  
  while(bgav_demuxer_peek_packet_read(s->demuxer, s, 0))
    {
    /* Get the packet and append data to the buffer */
    p = bgav_demuxer_get_packet_read(s->demuxer, s);

    old_buffer_size = priv->buf.size;

    bgav_bytebuffer_append(&priv->buf, p, 0);
    position = p->position;
    bgav_demuxer_done_packet_read(s->demuxer, p);
    
    
    while(priv->buf.size >= FAAD_MIN_STREAMSIZE)
      {
      if(!decode_frame_faad2(s))
        break;
      
      /* If frame starts in the previous packet,
         use the previous index */
      if(old_buffer_size)
        {
        bgav_file_index_append_packet(s->file_index,
                                      priv->last_position,
                                      s->duration,
                                      PACKET_FLAG_KEY);
        old_buffer_size = 0;
        }
      else
        bgav_file_index_append_packet(s->file_index,
                                      position,
                                      s->duration,
                                      PACKET_FLAG_KEY);
      
      s->duration += s->data.audio.frame->valid_samples;
      }
    priv->last_position = position;
    }
  }


static bgav_audio_decoder_t decoder =
  {
    .name =   "FAAD AAC audio decoder",
    .fourccs = (uint32_t[]){ BGAV_MK_FOURCC('m','p','4','a'),
                           BGAV_MK_FOURCC('a','a','c',' '),
                           BGAV_MK_FOURCC('A','A','C',' '),
                           BGAV_MK_FOURCC('A','A','C','P'),
                           BGAV_MK_FOURCC('r','a','a','c'),
                           BGAV_MK_FOURCC('r','a','c','p'),
                           BGAV_WAVID_2_FOURCC(0x00ff),
                      0x0 },
    
    .init =   init_faad2,
    .decode_frame = decode_frame_faad2,
    .parse = parse_faad2,
    .close =  close_faad2,
    .resync =  resync_faad2
  };

void bgav_init_audio_decoders_faad2()
  {
  bgav_audio_decoder_register(&decoder);
  }

