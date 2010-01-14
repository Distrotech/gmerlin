/*****************************************************************
 * gmerlin-encoders - encoder plugins for gmerlin
 *
 * Copyright (c) 2001 - 2010 Members of the Gmerlin project
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
#include <config.h>
#include <unistd.h> 
#include <pthread.h>
#include <sys/signal.h>

#include <gmerlin/translation.h>
#include <gmerlin/plugin.h>
#include <gmerlin/utils.h>
#include <gmerlin/subprocess.h>
#include <gmerlin/log.h>

#include "mpa_common.h"

#define LOG_DOMAIN "mp2enc"

static const bg_parameter_info_t parameters[] =
  {
    {
      .name =        "bitrate",
      .long_name =   TRS("Bitrate (kbps)"),
      .type =        BG_PARAMETER_INT,
      .val_default = { .val_i =     224 },
      .val_min =     { .val_i =      32 },
      .val_max =     { .val_i =     448 },
    },
    {
      .name =        "layer",
      .long_name =   TRS("Layer (1 or 2)"),
      .type =        BG_PARAMETER_INT,
      .val_default = { .val_i =       2 },
      .val_min =     { .val_i =       1 },
      .val_max =     { .val_i =       2 },
      .help_string = TRS("Audio layer"),
    },
    {
      .name =        "vcd",
      .long_name =   TRS("VCD Compatible"),
      .type =        BG_PARAMETER_CHECKBUTTON,
      .val_default = { .val_i =       1 },
      .help_string = TRS("Make VCD compliant output. This forces layer II, 224 kbps and 44.1 KHz stereo"),
    },
    { /* End of parameters */ }
  };

const bg_parameter_info_t * bg_mpa_get_parameters()
  {
  return parameters;
  }
  

void bg_mpa_set_parameter(void * data, const char * name,
                          const bg_parameter_value_t * val)
  {
  bg_mpa_common_t * com = (bg_mpa_common_t*)data;
  
  if(!name)
    {
    return;
    }
  
  if(!strcmp(name, "bitrate"))
    com->bitrate = val->val_i;
  else if(!strcmp(name, "vcd"))
    com->vcd = val->val_i;
  else if(!strcmp(name, "layer"))
    com->layer = val->val_i;
  }

static char * bg_mpa_make_commandline(bg_mpa_common_t * com,
                               const char * filename)
  {
  char * ret;
  char * mp2enc_path;
  char * tmp_string;

  if(!bg_search_file_exec("mp2enc", &mp2enc_path))
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN,  "Cannot find mp2enc executable");
    return (char*)0;
    }

  /* path & audio format and verbosity */
  ret = bg_sprintf("%s -R %d,%d,16 -v 0", mp2enc_path,
                   com->format.samplerate,com->format.num_channels);

  /* Check for VCD compatibility */

  if(com->vcd)
    tmp_string = bg_sprintf(" -V -b %d", com->bitrate);
  else
    tmp_string = bg_sprintf(" -b %d -l %d -r %d",
                            com->bitrate, com->layer,
                            com->format.samplerate);
  ret = bg_strcat(ret, tmp_string);
  free(tmp_string);

  /* Output file */
  
  tmp_string = bg_sprintf(" -o \"%s\"", filename);
  ret = bg_strcat(ret, tmp_string);
  free(tmp_string);
  
  return ret;
  }

static const int bitrates[2][15] = {
  {0,32,64,96,128,160,192,224,256,288,320,352,384,416,448},
  {0,32,48,56,64,80,96,112,128,160,192,224,256,320,384},
  //  {0,32,40,48,56,64,80,96,112,128,160,192,224,256,320}
};

static int get_bitrate(int in_rate, int layer, int channels,
                       int vcd)
  {
  int i;
  int diff;
  int min_diff = 1000000;
  int min_i = -1;
  int ret = 0;
  
  for(i = 0; i < 15; i++)
    {
    if(bitrates[layer-1][i] == in_rate)
      {
      ret = bitrates[layer-1][i];
      break;
      }
    else
      {
      diff = abs(in_rate - bitrates[layer-1][i]);
      if(diff < min_diff)
        {
        min_diff = diff;
        min_i = i;
        }
      }
    }
  if(!ret)
    {
    ret = bitrates[layer-1][min_i];
    }

  /* Sanity checks */
  if((layer == 2) && (channels == 1) && (ret > 192))
    ret = 192;

  if(vcd)
    {
    if(channels == 2)
      {
      switch(ret)
        {
        case  128:
        case  192:
        case  224:
        case  384:
          break;
        default:
          ret = 224;
        }
      }
    else if(channels == 1)
      {
      switch(ret)
        {
        case 64:
        case 96:
        case 192:
          break;
        default:
          ret = 96;
        }
      }
    }
  if(ret != in_rate)
    {
    bg_log(BG_LOG_WARNING, LOG_DOMAIN,
           "Bitrate %d kbps unsupported, switching to %d kbps", in_rate, ret);
    }
  return ret;
  }

static int samplerates[] =
  {
#if 0
    /* MPEG-2.5 */
    8000,
    11025,
    12000,
    /* MPEG-2 */
    16000,
    22050,
    24000,
#endif
    /* MPEG-1 */
    32000,
    44100,
    48000
  };

static int get_samplerate(int in_rate, int vcd)
  {
  int i;
  int diff;
  int min_diff = 1000000;
  int min_i = -1;
  int ret = 0;
  
  if(vcd)
    ret = 44100;

  else
    {
    for(i = 0; i < sizeof(samplerates)/sizeof(samplerates[0]); i++)
      {
      if(samplerates[i] == in_rate)
        ret = in_rate;
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
    if(!ret)
      ret = samplerates[min_i];
    }

  if(ret != in_rate)
    bg_log(BG_LOG_WARNING, LOG_DOMAIN,
           "Samplerate %d unsupported, switching to %d", in_rate, ret);
  return ret;
  }



static const char * extension_layer1  = "mpa";
static const char * extension_layer2  = "mp2";

const char * bg_mpa_get_extension(bg_mpa_common_t * mpa)
  {
  if(mpa->layer == 1)
    return extension_layer1;
  else
    return extension_layer2;
  }

void bg_mpa_set_format(bg_mpa_common_t * com,
                       const gavl_audio_format_t * format)
  {
  gavl_audio_format_copy(&(com->format), format);

  com->format.sample_format = GAVL_SAMPLE_S16;
  com->format.interleave_mode = GAVL_INTERLEAVE_ALL;

  if(com->format.num_channels > 2)
    {
    com->format.num_channels = 2;
    com->format.channel_locations[0] = GAVL_CHID_NONE;
    gavl_set_channel_setup(&com->format);
    }
  }

void bg_mpa_get_format(bg_mpa_common_t * com, gavl_audio_format_t * format)
  {
  gavl_audio_format_copy(format, &(com->format));
  }

int bg_mpa_start(bg_mpa_common_t * com, const char * filename)
  {
  sigset_t newset;
  char * commandline;

  /* Block SIGPIPE */
  sigemptyset(&newset);
  sigaddset(&newset, SIGPIPE);
  pthread_sigmask(SIG_BLOCK, &newset, &com->oldset);
  
  /* Adjust Samplerate */
  com->format.samplerate = get_samplerate(com->format.samplerate, com->vcd);
    
  /* Adjust bitrate */
  
  com->bitrate = get_bitrate(com->bitrate, com->layer,
                             com->format.num_channels, com->vcd);

  commandline = 
    bg_mpa_make_commandline(com, filename);
  if(!commandline)
    {
    return 0;
    }
  com->mp2enc = bg_subprocess_create(commandline, 1, 0, 0);
  if(!com->mp2enc)
    return 0;
  free(commandline);
  return 1;
  }

int bg_mpa_write_audio_frame(bg_mpa_common_t * com,
                              gavl_audio_frame_t * frame)
  {
  int bytes = 2 * com->format.num_channels * frame->valid_samples;
  if(write(com->mp2enc->stdin_fd, frame->samples.s_16, bytes) < bytes)
    return 0;
  return 1;
  }

int bg_mpa_close(bg_mpa_common_t * com)
  {
  int ret = 1;
  if(com->mp2enc)
    {
    if(bg_subprocess_close(com->mp2enc))
      ret = 0;
    }
  pthread_sigmask(SIG_SETMASK, &com->oldset, NULL);
  return ret;
  }
