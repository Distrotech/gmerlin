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

#include <gmerlin/plugin.h>
#include <gmerlin/utils.h>

#include "mpv_common.h"

#define BITRATE_AUTO 0
#define BITRATE_VBR  1
#define BITRATE_CBR  2

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
    }
  else if(!strcmp(name, "bitrate_mode"))
    {
    SET_ENUM("auto", com->format, BITRATE_AUTO);
    SET_ENUM("cbr",  com->format, BITRATE_VBR);
    SET_ENUM("vbr",  com->format, BITRATE_CBR);
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

  /* User options */

  if(com->user_options)
    {
    tmp_string = bg_sprintf(" %s", com->user_options);
    ret = bg_strcat(ret, tmp_string);
    free(tmp_string);
    }
  
  /* Output file */

  tmp_string = bg_sprintf(" -o %s", filename);
  ret = bg_strcat(ret, tmp_string);
  free(tmp_string);
  
  return ret;
  }

void bg_mpv_cleanup(bg_mpv_common_t * com)
  {
  if(com->user_options)
    free(com->user_options);
  }
