/*****************************************************************
 * gmerlin-encoders - encoder plugins for gmerlin
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


#include <string.h>
#include <stdlib.h>

#include <config.h>

#include <gmerlin/translation.h>
#include <gmerlin/plugin.h>
#include <gmerlin/pluginfuncs.h>
#include <gmerlin/utils.h>
#include <gmerlin/log.h>
#define LOG_DOMAIN "oggtheora"

#include <theora/theoraenc.h>

#include "ogg_common.h"

#ifdef TH_ENCCTL_2PASS_IN
#define HAVE_2_PASS
#endif

typedef struct
  {
  /* Ogg theora stuff */
    
  ogg_stream_state os;
  
  th_info        ti;
  th_comment     tc;
  th_enc_ctx   * ts;
  
  long serialno;
  bg_ogg_encoder_t * output;

  gavl_video_format_t * format;
  int have_packet;
  int cbr;
  int max_keyframe_interval;
  int rate_flags;
  
  th_ycbcr_buffer buf;

  float speed;
  
  int64_t frame_counter;
  
#ifdef HAVE_2_PASS
  int pass;
  FILE * stats_file;
  
  char * stats_buf;
  char * stats_ptr;
  int stats_size;
#endif
  
  bg_encoder_framerate_t fr;
  } theora_t;

static void * create_theora(bg_ogg_encoder_t * output, long serialno)
  {
  theora_t * ret;
  ret = calloc(1, sizeof(*ret));
  ret->serialno = serialno;
  ret->output = output;
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
  }


static void build_comment(th_comment * vc, bg_metadata_t * metadata)
  {
  char * tmp_string;
  
  th_comment_init(vc);
  
  if(metadata->artist)
    th_comment_add_tag(vc, "ARTIST", metadata->artist);
  
  if(metadata->title)
    th_comment_add_tag(vc, "TITLE", metadata->title);

  if(metadata->album)
    th_comment_add_tag(vc, "ALBUM", metadata->album);
    
  if(metadata->genre)
    th_comment_add_tag(vc, "GENRE", metadata->genre);

  if(metadata->date)
    th_comment_add_tag(vc, "DATE", metadata->date);
  
  if(metadata->copyright)
    th_comment_add_tag(vc, "COPYRIGHT", metadata->copyright);

  if(metadata->track)
    {
    tmp_string = bg_sprintf("%d", metadata->track);
    th_comment_add_tag(vc, "TRACKNUMBER", tmp_string);
    free(tmp_string);
    }

  if(metadata->comment)
    th_comment_add(vc, metadata->comment);
  }

static const gavl_pixelformat_t supported_pixelformats[] =
  {
    GAVL_YUV_420_P,

    /* Of course 2-pass and 4:2:2/4:4:4 are completely unrelated.
       But both appeared in libtheora 1.1.0 and we have no other
       way to check the version at compile time */
#ifdef HAVE_2_PASS
    GAVL_YUV_422_P,
    GAVL_YUV_444_P,
#endif
    GAVL_PIXELFORMAT_NONE,
  };

static int init_theora(void * data, gavl_video_format_t * format, bg_metadata_t * metadata)
  {
  int sub_h, sub_v;
  int arg_i1, arg_i2;
  
  ogg_packet op;
  int header_packets;
  
  theora_t * theora = (theora_t *)data;

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

    
  // format->framerate_mode = GAVL_FRAMERATE_CONSTANT;
  
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
  
  ogg_stream_init(&theora->os, theora->serialno);

  /* Initialize encoder */
  if(!(theora->ts = th_encode_alloc(&theora->ti)))
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN,  "th_encode_alloc failed");
    return 0;
    }
  /* Build comment (comments are UTF-8, good for us :-) */

  build_comment(&theora->tc, metadata);

  /* Call encode CTLs */
  
  // Keyframe frequency

  th_encode_ctl(theora->ts,
                TH_ENCCTL_SET_KEYFRAME_FREQUENCY_FORCE,
                &theora->max_keyframe_interval, sizeof(theora->max_keyframe_interval));

  // Rate flags

  th_encode_ctl(theora->ts,
                TH_ENCCTL_SET_RATE_FLAGS,
                &theora->rate_flags,sizeof(theora->rate_flags));
  
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

  header_packets = 0;
  
  while(th_encode_flushheader(theora->ts, &theora->tc, &op) > 0)
    {
    ogg_stream_packetin(&theora->os,&op);

    if(!header_packets)
      {
      /* And stream them out */
      if(!bg_ogg_flush_page(&theora->os, theora->output, 1))
        {
        bg_log(BG_LOG_ERROR, LOG_DOMAIN,  "Got no Theora ID page");
        return 0;
        }
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
  
  return 1;
  }

#ifdef HAVE_2_PASS

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

static int flush_header_pages_theora(void*data)
  {
  theora_t * theora = data;
  if(bg_ogg_flush(&theora->os, theora->output, 1) <= 0)
    return 0;
  return 1;
  }


static int write_video_frame_theora(void * data, gavl_video_frame_t * frame)
  {
  theora_t * theora;
  int result;
  int i;
  ogg_packet op;
  
  theora = data;
  
  if(theora->have_packet)
    {
    if(!th_encode_packetout(theora->ts, 0, &op))
      {
      bg_log(BG_LOG_ERROR, LOG_DOMAIN,
             "Theora encoder produced no packet");
      return 0;
      }
    
    ogg_stream_packetin(&theora->os,&op);
    if(bg_ogg_flush(&theora->os, theora->output, 0) < 0)
      {
      bg_log(BG_LOG_ERROR, LOG_DOMAIN,
             "Writing theora packet failed");
      return 0;
      }
    theora->have_packet = 0;
    }
 
  for(i = 0; i < 3; i++)
    {
    theora->buf[i].stride = frame->strides[i];
    theora->buf[i].data   = frame->planes[i];
    }

#ifdef HAVE_2_PASS
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
        return 0;
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

#ifdef HAVE_2_PASS
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
      return 0;
      }
    fwrite(buf, 1, ret, theora->stats_file);
    }
#endif
  
  theora->have_packet = 1;
  return 1;
  }

static int close_theora(void * data)
  {
  int ret = 1;
  theora_t * theora;
  ogg_packet op;
  theora = data;

  if(theora->have_packet)
    {
    if(!th_encode_packetout(theora->ts, 1, &op))
      {
      bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Theora encoder produced no packet");
      ret = 0;
      }

    if(ret)
      {
      ogg_stream_packetin(&theora->os,&op);
      if(bg_ogg_flush(&theora->os, theora->output, 1) <= 0)
        {
        bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Writing packet failed");
        ret = 0;
        }
      }
    theora->have_packet = 0;

#ifdef HAVE_2_PASS
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
        return 0;
        }
      fseek(theora->stats_file, 0, SEEK_SET);
      fwrite(buf, 1, ret, theora->stats_file);
      }
#endif
    
    }

#ifdef HAVE_2_PASS
  if(theora->stats_file)
    fclose(theora->stats_file);
  if(theora->stats_buf)
    free(theora->stats_buf);
#endif
  
  ogg_stream_clear(&theora->os);
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
#ifdef HAVE_2_PASS
    .set_video_pass = set_video_pass_theora,
#endif    
    .init_video =     init_theora,
    
    .flush_header_pages = flush_header_pages_theora,
    
    .encode_video = write_video_frame_theora,
    .close = close_theora,
  };
