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
#include <math.h>
#include <pthread.h>
#include <sys/signal.h>

#include <config.h>

#include <gmerlin/translation.h>
#include <gmerlin/plugin.h>
#include <gmerlin/pluginfuncs.h>

#include <gmerlin/utils.h>
#include <gmerlin/log.h>
#define LOG_DOMAIN "mpegvideo"

#include <gmerlin/subprocess.h>

#include <yuv4mpeg.h>

#include "mpv_common.h"

#define BITRATE_AUTO 0
#define BITRATE_VBR  1
#define BITRATE_CBR  2

#define FORMAT_MPEG1 0
#define FORMAT_VCD   1
#define FORMAT_MPEG2 3
#define FORMAT_SVCD  4
#define FORMAT_DVD   8


static const bg_parameter_info_t parameters[] =
  {
    {
      .name =        "format",
      .long_name =   TRS("Format"),
      .type =        BG_PARAMETER_STRINGLIST,
      .val_default = { .val_str = "mpeg1" },
      .multi_names =  (char const *[]){ "mpeg1",
                                        "mpeg2",
                                        "vcd", "svcd", "dvd", NULL },
      .multi_labels = (char const *[]){ TRS("MPEG-1 (generic)"),
                                        TRS("MPEG-2 (generic)"),
                                        TRS("VCD"),
                                        TRS("SVCD"),
                                        TRS("DVD (for dvdauthor)"),
                                        NULL  },
      .help_string =  TRS("Sets the MPEG flavour. Note, that for VCD, SVCD and DVD, you MUST provide valid\
 frame sizes"),
    },
    {
      .name =        "bitrate_mode",
      .long_name =   TRS("Bitrate Mode"),
      .type =        BG_PARAMETER_STRINGLIST,
      .val_default = { .val_str = "auto" },
      .multi_names =  (char const *[]){ "auto", "vbr", "cbr",NULL  },
      .multi_labels = (char const *[]){ TRS("Auto"),
                                        TRS("Variable"),
                                        TRS("Constant"),
                                        NULL  },
      .help_string = TRS("Specify constant or variable bitrate. For \"Auto\", constant bitrate will be \
used for MPEG-1, variable bitrate will be used for MPEG-2. For formats, which require CBR, this option \
is ignored"),
    },
    {
      .name =        "bitrate",
      .long_name =   TRS("Bitrate (kbps)"),
      .type =        BG_PARAMETER_INT,
      .val_default = { .val_i =    1150 },
      .val_min =     { .val_i =     200 },
      .val_max =     { .val_i =   99999 },
      .help_string = TRS("Video bitrate in kbps. For VBR, it's the maximum bitrate. If the format requires a \
fixed bitrate (e.g. VCD) this option is ignored"),
    },
    {
      .name =        "quantization",
      .long_name =   TRS("Quantization"),
      .type =        BG_PARAMETER_INT,
      .val_default = { .val_i =       8 },
      .val_min =     { .val_i =       1 },
      .val_max =     { .val_i =      31 },
      .help_string = TRS("Minimum quantization for VBR. Lower numbers mean higher quality. For CBR, this option is ignored."),
    },
    {
      .name =        "quant_matrix",
      .long_name =   TRS("Quantization matrices"),
      .type =        BG_PARAMETER_STRINGLIST,
      .val_default = { .val_str = "default" },
      .multi_names = (char const *[]){ "default", "kvcd", "tmpgenc",
                              "hi-res", NULL },
      .multi_labels = (char const *[]){ TRS("Default"), TRS("KVCD"),
                               TRS("tmpegenc"), TRS("Hi-Res"),
                               NULL },
    },
    {
      .name =        "bframes",
      .long_name =   TRS("Number of B-Frames"),
      .type =        BG_PARAMETER_INT,
      .val_default = { .val_i = 0 },
      .val_min =     { .val_i = 0 },
      .val_max =     { .val_i = 2 },
      .help_string = TRS("Specify the number of B-frames between 2 P/I frames. More B-frames slow down encoding and \
increase memory usage, but might give better compression results. For VCD, this option is ignored, since \
the VCD standard requires 2 B-frames, no matter if you like them or not."),
    },
    {
      .name =        "user_options",
      .long_name =   TRS("User options"),
      .type =        BG_PARAMETER_STRING,
      .help_string = TRS("Enter further commandline options for mpeg2enc here. Check the mpeg2enc manual page \
for details"),
    },
    BG_ENCODER_FRAMERATE_PARAMS,
    { /* End of parameters */ }
    
  };

const bg_parameter_info_t * bg_mpv_get_parameters()
  {
  return parameters;
  }

/* Must pass a bg_mpv_common_t for parameters */

#define SET_ENUM(str, dst, v) if(!strcmp(val->val_str, str)) dst = v

void bg_mpv_set_parameter(void * data, const char * name, const bg_parameter_value_t * val)
  {
  bg_mpv_common_t * com = data;

  if(!name)
    return;
  else if(bg_encoder_set_framerate_parameter(&com->y4m.fr,
                                             name, val))
    {
    return;
    }
  else if(!strcmp(name, "format"))
    {
    SET_ENUM("mpeg1", com->format, FORMAT_MPEG1);
    SET_ENUM("mpeg2", com->format, FORMAT_MPEG2);
    SET_ENUM("vcd",   com->format, FORMAT_VCD);
    SET_ENUM("svcd",  com->format, FORMAT_SVCD);
    SET_ENUM("dvd",   com->format, FORMAT_DVD);
    }
  else if(!strcmp(name, "bitrate_mode"))
    {
    SET_ENUM("auto", com->bitrate_mode, BITRATE_AUTO);
    SET_ENUM("vbr",  com->bitrate_mode, BITRATE_VBR);
    SET_ENUM("cbr",  com->bitrate_mode, BITRATE_CBR);
    }

  else if(!strcmp(name, "bitrate"))
    com->bitrate = val->val_i;

  else if(!strcmp(name, "quantization"))
    com->quantization = val->val_i;

  else if(!strcmp(name, "bframes"))
    com->bframes = val->val_i;

  else if(!strcmp(name, "user_options"))
    com->user_options = bg_strdup(com->user_options, val->val_str);
  else if(!strcmp(name, "quant_matrix"))
    com->quant_matrix = bg_strdup(com->quant_matrix, val->val_str);
  }

#undef SET_ENUM

static char * bg_mpv_make_commandline(bg_mpv_common_t * com, const char * filename)
  {
  char * ret;
  char * mpeg2enc_path;
  char * tmp_string;

  int mpeg_1;
  
  if(!bg_search_file_exec("mpeg2enc", &mpeg2enc_path))
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN,  "Cannot find mpeg2enc executable");
    return NULL;
    }

  /* Check if we have MPEG-1 */
  if((com->format == FORMAT_VCD) ||
     (com->format == FORMAT_MPEG1))
    mpeg_1 = 1;
  else
    mpeg_1 = 0;
  
  /* path + format */
  ret = bg_sprintf("%s -f %d", mpeg2enc_path, com->format);
  free(mpeg2enc_path);

  /* B-frames */
  
  if(com->format != FORMAT_VCD)
    {
    tmp_string = bg_sprintf(" -R %d", com->bframes);
    ret = bg_strcat(ret, tmp_string);
    free(tmp_string);
    }

  /* Bitrate mode */
  
  if(com->format != FORMAT_VCD)
    {
    if(mpeg_1)
      {
      if(com->bitrate_mode == BITRATE_VBR)
        {
        tmp_string = bg_sprintf(" -q %d", com->quantization);
        ret = bg_strcat(ret, tmp_string);
        free(tmp_string);
        }
      }
    else
      {
      if(com->bitrate_mode == BITRATE_CBR)
        ret = bg_strcat(ret, " --cbr");
      else
        {
        tmp_string = bg_sprintf(" -q %d", com->quantization);
        ret = bg_strcat(ret, tmp_string);
        free(tmp_string);
        }

      tmp_string = bg_sprintf(" -K %s", com->quant_matrix);
      ret = bg_strcat(ret, tmp_string);
      free(tmp_string);
      }
    tmp_string = bg_sprintf(" -b %d", com->bitrate);
    ret = bg_strcat(ret, tmp_string);
    free(tmp_string);
    }
  
  /* TODO: More parameters */

  /* Verbosity level: Too many messages on std[out|err] are not
     useful for GUI applications */
  
  ret = bg_strcat(ret, " -v 0");
  
  /* User options */

  if(com->user_options)
    {
    tmp_string = bg_sprintf(" %s", com->user_options);
    ret = bg_strcat(ret, tmp_string);
    free(tmp_string);
    }
  
  /* Output file */

  tmp_string = bg_sprintf(" -o \"%s\"", filename);
  ret = bg_strcat(ret, tmp_string);
  free(tmp_string);
  
  return ret;
  }



static int bg_mpv_get_chroma_mode(bg_mpv_common_t * com)
  {
  switch(com->format)
    {
    case FORMAT_MPEG1:
    case FORMAT_VCD:
      return Y4M_CHROMA_420JPEG;
      break;
    case FORMAT_MPEG2:
    case FORMAT_SVCD:
    case FORMAT_DVD:
      return Y4M_CHROMA_420MPEG2;
      break;
    default:
      bg_log(BG_LOG_ERROR, LOG_DOMAIN,  "Unknown MPEG format");
    }
  return -1;
  }

static void bg_mpv_adjust_interlacing(gavl_video_format_t * format,
                                      int mpeg_format)
  {
  switch(mpeg_format)
    {
    case FORMAT_MPEG1:
    case FORMAT_VCD:
      format->interlace_mode = GAVL_INTERLACE_NONE;
      break;
    case FORMAT_MPEG2:
    case FORMAT_SVCD:
    case FORMAT_DVD:
      if(format->interlace_mode == GAVL_INTERLACE_MIXED)
        {
        bg_log(BG_LOG_WARNING, LOG_DOMAIN,
               "Mixed interlacing not supported (yet)");
        format->interlace_mode = GAVL_INTERLACE_NONE;
        }
      break;
    default:
      bg_log(BG_LOG_ERROR, LOG_DOMAIN,  "Unknown MPEG format");
    }
  }

static const char * extension_mpeg1  = "m1v";
static const char * extension_mpeg2  = "m2v";

const char * bg_mpv_get_extension(bg_mpv_common_t * mpv)
  {
  if(mpv->ci)
    {
    if(mpv->ci->id == GAVL_CODEC_ID_MPEG2)
      return extension_mpeg2;
    else if(mpv->ci->id == GAVL_CODEC_ID_MPEG1)
      return extension_mpeg1;
    }
  
  switch(mpv->format)
    {
    case FORMAT_MPEG1:
    case FORMAT_VCD:
      return extension_mpeg1;
      break;
    case FORMAT_MPEG2:
    case FORMAT_SVCD:
    case FORMAT_DVD:
      return extension_mpeg2;
    }
  return NULL;
  }


int bg_mpv_open(bg_mpv_common_t * com, const char * filename)
  {
  char * commandline;

  sigset_t newset;

  if(com->ci)
    {
    com->out = fopen(filename, "wb");
    }
  else
    {
    /* Block SIGPIPE */
    sigemptyset(&newset);
    sigaddset(&newset, SIGPIPE);
    pthread_sigmask(SIG_BLOCK, &newset, &com->oldset);
    
    commandline = bg_mpv_make_commandline(com, filename);
    if(!commandline)
      {
      return 0;
      }
    
    com->mpeg2enc = bg_subprocess_create(commandline, 1, 0, 0);
    if(!com->mpeg2enc)
      {
      return 0;
      }


    com->y4m.fd = com->mpeg2enc->stdin_fd;
    
    free(commandline);
    }
  return 1;
  }

static const bg_encoder_framerate_t mpeg_framerates[] =
  {
    { 24000, 1001 },
    {    24,    1 },
    {    25,    1 },
    { 30000, 1001 },
    {    30,    1 },
    {    50,    1 },
    { 60000, 1001 },
    {    60,    1 },
    { /* End of framerates */ }
  };

void bg_mpv_set_format(bg_mpv_common_t * com, const gavl_video_format_t * format)
  {
  gavl_video_format_copy(&com->y4m.format, format);
  }

void bg_mpv_get_format(bg_mpv_common_t * com, gavl_video_format_t * format)
  {
  gavl_video_format_copy(format, &com->y4m.format);
  }

int bg_mpv_write_video_frame(bg_mpv_common_t * com, gavl_video_frame_t * frame)
  {
  return bg_y4m_write_frame(&com->y4m, frame);
  }

gavl_video_sink_t * bg_mpv_get_video_sink(bg_mpv_common_t * com)
  {
  return com->y4m.sink;
  }


static const uint8_t sequence_end[4] = { 0x00, 0x00, 0x01, 0xb7 };

int bg_mpv_close(bg_mpv_common_t * com)
  {
  int ret = 1;
  if(com->mpeg2enc)
    {
    if(bg_subprocess_close(com->mpeg2enc))
      ret = 0;
    
    pthread_sigmask(SIG_SETMASK, &com->oldset, NULL);
    
    bg_y4m_cleanup(&com->y4m);
    if(com->user_options)
      free(com->user_options);
    if(com->quant_matrix)
      free(com->quant_matrix);
    }
  if(com->out)
    {
    if(fwrite(sequence_end, 1, 4, com->out) < 4)
      bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Inserting sequence end code failed");
    fclose(com->out);
    }
  
  return ret;
  }

int bg_mpv_start(bg_mpv_common_t * com)
  {
  int result;

  if(com->ci)
    return 1;
  
  com->y4m.chroma_mode = bg_mpv_get_chroma_mode(com);

  bg_encoder_set_framerate_nearest(&com->y4m.fr,
                                   mpeg_framerates,
                                   &com->y4m.format);
  
  bg_mpv_adjust_interlacing(&com->y4m.format, com->format);
  bg_y4m_set_pixelformat(&com->y4m);
  
  result = bg_y4m_write_header(&com->y4m);
  return result;
  }

void bg_mpv_set_ci(bg_mpv_common_t * com, const gavl_compression_info_t * ci)
  {
  com->ci = ci;
  }

int bg_mpv_write_video_packet(bg_mpv_common_t * com,
                              gavl_packet_t * packet)
  {
  int len = packet->data_len;
  if(packet->sequence_end_pos > 0)
    len = packet->sequence_end_pos;
  if(fwrite(packet->data, 1, len, com->out) < len)
    return 0;
  return 1;
  }
