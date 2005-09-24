/*****************************************************************

  mpv_common.c

  Copyright (c) 2005 by Burkhard Plaum - plaum@ipf.uni-stuttgart.de

  http://gmerlin.sourceforge.net

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111, USA.

*****************************************************************/

#include <string.h>
#include <math.h>

#include <gmerlin/plugin.h>
#include <gmerlin/utils.h>

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


static bg_parameter_info_t parameters[] =
  {
    {
      name:        "format",
      long_name:   "Format",
      type:        BG_PARAMETER_STRINGLIST,
      val_default: { val_str: "mpeg1" },
      multi_names:  (char*[]){ "mpeg1",          "mpeg2",          "vcd", "svcd", "dvd", (char*)0 },
      multi_labels: (char*[]){ "Generic MPEG-1", "Generic MPEG-2", "VCD", "SVCD", "DVD (for dvdauthor)", (char*)0  },
      help_string:  "Sets the MPEG flavour. Note, that for VCD, SVCD and DVD, you MUST provide valid\
 frame sizes",
    },
    {
      name:        "bitrate_mode",
      long_name:   "Bitrate Mode",
      type:        BG_PARAMETER_STRINGLIST,
      val_default: { val_str: "auto" },
      multi_names:  (char*[]){ "auto", "vbr", "cbr", (char*)0  },
      multi_labels: (char*[]){ "Auto", "Variable", "Constant", (char*)0  },
      help_string: "Specify constant for variable bitrate. For \"Auto\", constant bitrate will be \
used for MPEG-1, variable bitrate will be used for MPEG-2. For formats, which require CBR, this option \
is ignored",
    },
    {
      name:        "bitrate",
      long_name:   "Bitrate (kb/s)",
      type:        BG_PARAMETER_INT,
      val_default: { val_i:    1150 },
      val_min:     { val_i:     200 },
      val_max:     { val_i:   99999 },
      help_string: "Video bitrate in (kb/s). For VBR, it's the maximum bitrate. If the format requires a \
fixed bitrate (e.g. VCD) this option is ignored",
    },
    {
      name:        "quantization",
      long_name:   "Quantization",
      type:        BG_PARAMETER_INT,
      val_default: { val_i:       8 },
      val_min:     { val_i:       1 },
      val_max:     { val_i:      31 },
      help_string: "Minimum quantization for VBR. Lower numbers mean higher quality. For CBR, this option is ignored.",
    },
    {
      name:        "bframes",
      long_name:   "Number of B-Frames",
      type:        BG_PARAMETER_INT,
      val_default: { val_i: 0 },
      val_min:     { val_i: 0 },
      val_max:     { val_i: 2 },
      help_string: "Specify the number of B-frames between 2 P/I frames. More B-frames slow down encoding and \
increase memory usage, but might give better compression results. For VCD, this option is ignored, since \
the VCD standard requires 2 B-frames, no matter if you like them or not.",
    },
    {
      name:        "user_options",
      long_name:   "User options",
      type:        BG_PARAMETER_STRING,
      help_string: "Enter further commandline options for mpeg2enc here. Check the mpeg2enc manual page \
for details",
    },
    { /* End of parameters */ }
    
  };

bg_parameter_info_t * bg_mpv_get_parameters()
  {
  return parameters;
  }

/* Must pass a bg_mpv_common_t for parameters */

#define SET_ENUM(str, dst, v) if(!strcmp(val->val_str, str)) dst = v

void bg_mpv_set_parameter(void * data, char * name, bg_parameter_value_t * val)
  {
  bg_mpv_common_t * com = (bg_mpv_common_t*)data;

  if(!name)
    return;
  else if(!strcmp(name, "format"))
    {
    SET_ENUM("mpeg1", com->format, FORMAT_MPEG1);
    SET_ENUM("mpeg2", com->format, FORMAT_MPEG2);
    SET_ENUM("vcd",   com->format, FORMAT_VCD);
    SET_ENUM("svcd",  com->format, FORMAT_SVCD);
    SET_ENUM("dvd",   com->format, FORMAT_DVD);
    fprintf(stderr, "val->str: %s format: %d\n",
            val->val_str, com->format);
    }
  else if(!strcmp(name, "bitrate_mode"))
    {
    SET_ENUM("auto", com->bitrate_mode, BITRATE_AUTO);
    SET_ENUM("cbr",  com->bitrate_mode, BITRATE_VBR);
    SET_ENUM("vbr",  com->bitrate_mode, BITRATE_CBR);
    }

  else if(!strcmp(name, "bitrate"))
    com->bitrate = val->val_i;

  else if(!strcmp(name, "quanitzation"))
    com->quantization = val->val_i;

  else if(!strcmp(name, "bframes"))
    com->bframes = val->val_i;

  else if(!strcmp(name, "user_options"))
    com->user_options = bg_strdup(com->user_options, val->val_str);
  }

#undef SET_ENUM

char * bg_mpv_make_commandline(bg_mpv_common_t * com, const char * filename)
  {
  char * ret;
  char * mpeg2enc_path;
  char * tmp_string;
  
  if(!bg_search_file_exec("mpeg2enc", &mpeg2enc_path))
    {
    fprintf(stderr, "Cannot find mpeg2enc");
    return (char*)0;
    }

  /* path + format */
  //  fprintf(stderr, "com->format: %d\n", com->format);
  ret = bg_sprintf("%s -f %d", mpeg2enc_path, com->format);
  free(mpeg2enc_path);

  /* B-frames */
  
  if(com->format != FORMAT_VCD)
    {
    tmp_string = bg_sprintf(" -R %d", com->bframes);
    ret = bg_strcat(ret, tmp_string);
    free(tmp_string);
    }

  /* Bitrate */

  if(com->format != FORMAT_VCD)
    {
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

static struct
  {
  int timescale;
  int frame_duration;
  }
mpeg_framerates[] =
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


void bg_mpv_adjust_framerate(gavl_video_format_t * format)
  {
  double rate_d;
  double test_rate_d;
  double min_diff, test_diff;
  int min_index;
  int i;
  
  /* Constant framerate */
  format->framerate_mode = GAVL_FRAMERATE_CONSTANT;
  
  /* Nearest framerate */
  rate_d = (double)format->timescale / (double)format->frame_duration;

  test_rate_d = (double)mpeg_framerates[0].timescale / (double)mpeg_framerates[0].frame_duration;
  
  min_diff = fabs(rate_d - test_rate_d);
  min_index = 0;
  
  i = 1;
  while(mpeg_framerates[i].timescale)
    {
    test_rate_d = (double)mpeg_framerates[i].timescale /
      (double)mpeg_framerates[i].frame_duration;

    test_diff = fabs(rate_d - test_rate_d);
#if 0
    fprintf(stderr, "Rate: %f, test_rate: %f, diff: %f\n",
            rate_d, test_rate_d, test_diff);
#endif 
    if(test_diff < min_diff)
      {
      min_index = i;
      min_diff = test_diff;
      }
    i++;
    }
  format->timescale      = mpeg_framerates[min_index].timescale;
  format->frame_duration = mpeg_framerates[min_index].frame_duration;
  }

int bg_mpv_get_chroma_mode(bg_mpv_common_t * com)
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
      fprintf(stderr, "ERROR: Unknown MPEG format\n");
    }
  return -1;
  }

void bg_mpv_adjust_interlacing(gavl_video_format_t * format,
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
      break;
    default:
      fprintf(stderr, "ERROR: Unknown MPEG format\n");
    }
  }

static const char * extension_mpeg1  = ".m1v";
static const char * extension_mpeg2  = ".m2v";

const char * bg_mpv_get_extension(bg_mpv_common_t * mpv)
  {
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
  return (char*)0;
  }

static ssize_t write_func(void * data, const void *buf, size_t len)
  {
  size_t result;
  result = fwrite(buf, 1, len, (FILE*)(data));
  
  if(result != len)
    return -len;
  return 0;
  }

int bg_mpv_open(bg_mpv_common_t * com, const char * filename)
  {
  char * commandline;

  commandline = bg_mpv_make_commandline(com, filename);
  if(!commandline)
    {
    return 0;
    }

  fprintf(stderr, "launching %s...", commandline);
  
  com->y4m.file = popen(commandline, "w");
  if(!com->y4m.file)
    {
    fprintf(stderr, "failed\n");
    return 0;
    }
  fprintf(stderr, "done\n");

  free(commandline);
  
  /* Set up writer */
  com->y4m.writer.data  = com->y4m.file;
  com->y4m.writer.write = write_func;

  return 1;
  
  }

void bg_mpv_set_format(bg_mpv_common_t * com, const gavl_video_format_t * format)
  {
  gavl_video_format_copy(&(com->y4m.format), format);
  com->y4m.chroma_mode = bg_mpv_get_chroma_mode(com);
  bg_mpv_adjust_framerate(&(com->y4m.format));
  bg_mpv_adjust_interlacing(&(com->y4m.format), com->format);
  bg_y4m_set_pixelformat(&com->y4m);
  }

void bg_mpv_get_format(bg_mpv_common_t * com, gavl_video_format_t * format)
  {
  gavl_video_format_copy(format, &(com->y4m.format));
  }

void bg_mpv_write_video_frame(bg_mpv_common_t * com, gavl_video_frame_t * frame)
  {
  bg_y4m_write_frame(&com->y4m, frame);
  }

void bg_mpv_close(bg_mpv_common_t * com)
  {
  pclose(com->y4m.file);
  bg_y4m_cleanup(&com->y4m);
  if(com->user_options)
    free(com->user_options);
  
  }

int bg_mpv_start(bg_mpv_common_t * com)
  {
  return bg_y4m_write_header(&com->y4m);
  }
