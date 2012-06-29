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
    48000
  };

static int get_samplerate(int in_rate)
  {
  int i;
  int diff;
  int min_diff = 1000000;
  int min_i = -1;
  
  for(i = 0; i < sizeof(samplerates)/sizeof(samplerates[0]); i++)
    {
    if(samplerates[i] == in_rate)
      return in_rate;
    else
      {
      diff = abs(in_rate - samplerates[i]);
      if(diff < min_diff)
        {
        min_diff = diff;
        min_i = i;
        }
      }
    }
  if(min_i >= 0)
    {
    return samplerates[min_i];
    }
  else
    return 44100;
  }

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


void bg_lame_init(lame_common_t * com)
  {
  com->vbr_mode = vbr_off;
  }
  
void bg_lame_close(lame_common_t * com)
  {
  if(com->lame)
    {
    lame_close(com->lame);
    com->lame = NULL;
    }

  if(com->output_buffer)
    {
    free(com->output_buffer);
    com->output_buffer = NULL;
    }
  
  }

void bg_lame_set_audio_parameter(void * data, int stream,
                                 const char * name,
                                 const bg_parameter_value_t * v)
  {
  lame_common_t * lame;
  int i;
  lame = data;
  
  if(stream)
    return;
  else if(!name)
    {
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
            get_bitrate(lame->abr_min_bitrate, lame->format.samplerate);
          if(lame->abr_min_bitrate > lame->abr_bitrate)
            {
            lame->abr_min_bitrate = get_bitrate(8, lame->format.samplerate);
            }
          if(lame_set_VBR_min_bitrate_kbps(lame->lame, lame->abr_min_bitrate))
            bg_log(BG_LOG_ERROR, LOG_DOMAIN,
                   "lame_set_VBR_min_bitrate_kbps failed");
          }
        if(lame->abr_max_bitrate)
          {
          lame->abr_max_bitrate =
            get_bitrate(lame->abr_max_bitrate, lame->format.samplerate);
          if(lame->abr_max_bitrate < lame->abr_bitrate)
            {
            lame->abr_max_bitrate = get_bitrate(320, lame->format.samplerate);
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
            get_bitrate(lame->cbr_bitrate, lame->format.samplerate);
        if(lame_set_brate(lame->lame, lame->cbr_bitrate))
          bg_log(BG_LOG_ERROR, LOG_DOMAIN,  "lame_set_brate failed");
        break;
      default:
        break;
      }

    if(lame_init_params(lame->lame) < 0)
      bg_log(BG_LOG_ERROR, LOG_DOMAIN,  "lame_init_params failed");

    return;
    }

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
    
    if(lame_set_bWriteVbrTag(lame->lame, (lame->vbr_mode == vbr_off) ? 0 : 1))
      bg_log(BG_LOG_ERROR, LOG_DOMAIN,  "lame_set_bWriteVbrTag failed");

    //    lame_set_bWriteVbrTag(lame->lame, 0);
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

void bg_lame_open(lame_common_t * com)
  {
  com->lame = lame_init();
  }

int bg_lame_add_audio_stream(void * data,
                             const gavl_metadata_t * m,
                             const gavl_audio_format_t * format)
  {
  lame_common_t * lame;
  lame = data;
  
  /* Copy and adjust format */
  
  gavl_audio_format_copy(&lame->format, format);

  lame->format.sample_format = GAVL_SAMPLE_FLOAT;
  lame->format.interleave_mode = GAVL_INTERLEAVE_NONE;
  lame->format.samplerate = get_samplerate(lame->format.samplerate);
  
  if(lame->format.num_channels > 2)
    {
    lame->format.num_channels = 2;
    lame->format.channel_locations[0] = GAVL_CHID_NONE;
    gavl_set_channel_setup(&lame->format);
    }

  if(lame_set_in_samplerate(lame->lame, lame->format.samplerate))
    bg_log(BG_LOG_ERROR, LOG_DOMAIN,  "lame_set_in_samplerate failed");
  if(lame_set_num_channels(lame->lame,  lame->format.num_channels))
    bg_log(BG_LOG_ERROR, LOG_DOMAIN,  "lame_set_num_channels failed");

  if(lame_set_scale(lame->lame, 32767.0))
    bg_log(BG_LOG_ERROR, LOG_DOMAIN,  "lame_set_scale failed");
    
  
  //  lame_set_out_samplerate(lame->lame, lame->format.samplerate);
  
  return 0;

  }

int bg_lame_write_audio_frame(void * data, gavl_audio_frame_t * frame, int stream)
  {
  int ret = 1;
  int max_out_size, bytes_encoded;
  lame_common_t * lame = data;
  
  max_out_size = (5 * frame->valid_samples) / 4 + 7200;
  if(lame->output_buffer_alloc < max_out_size)
    {
    lame->output_buffer_alloc = max_out_size + 1024;
    lame->output_buffer = realloc(lame->output_buffer,
                                  lame->output_buffer_alloc);
    }

  bytes_encoded = lame_encode_buffer_float(lame->lame,
                                           frame->channels.f[0],
                                           (lame->format.num_channels > 1) ?
                                           frame->channels.f[1] :
                                           frame->channels.f[0],
                                           frame->valid_samples,
                                           lame->output_buffer,
                                           lame->output_buffer_alloc);
  
  if((bytes_encoded > 0) &&
     (lame->write_callback(lame->write_priv,
                           lame->output_buffer, bytes_encoded) < bytes_encoded))
    
    ret = 0;
  
  lame->samples_read += frame->valid_samples;

  return ret;
  }

void bg_lame_get_audio_format(void * data, int stream,
                              gavl_audio_format_t * ret)
  {
  lame_common_t * lame;
  lame = data;
  gavl_audio_format_copy(ret, &lame->format);
  }

int bg_lame_flush(lame_common_t * lame)
  {
  int bytes_encoded;

  if(lame->output_buffer_alloc < 7200)
    {
    lame->output_buffer_alloc = 7200;
    lame->output_buffer = realloc(lame->output_buffer, lame->output_buffer_alloc);
    }
  bytes_encoded = lame_encode_flush(lame->lame, lame->output_buffer, 
                                    lame->output_buffer_alloc);

  if((bytes_encoded > 0) &&
     (lame->write_callback(lame->write_priv,
                           lame->output_buffer, bytes_encoded) < bytes_encoded))
    return 0;
  
  return 1;
  }
