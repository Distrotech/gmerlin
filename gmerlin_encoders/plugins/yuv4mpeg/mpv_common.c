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


static bg_parameter_info_t parameters[] =
  {
    {
      name:        "format",
      long_name:   "Format",
      type:        BG_PARAMETER_STRINGLIST,
      val_default: { val_str: "mpeg1" },
      multi_names:  (char*[]){ "mpeg1",          "mpeg2",          "vcd", "svcd", "dvd" },
      multi_labels: (char*[]){ "Generic MPEG-1", "Generic MPEG-2", "VCD", "SVCD", "DVD (for dvdauthor)" },
      help_string:  "Sets the MPEG flavour. Note, that for VCD, SVCD and DVD, you MUST provide valid\
 frame sizes",
    },
    {
      name:        "bframes",
      long_name:   "Number of B-Frames between 2 P/I frames",
      type:        BG_PARAMETER_INT,
      val_default: { val_i: 0 },
      val_min:     { val_i: 0 },
      val_max:     { val_i: 2 },
      help_string: "Specify the number of B-frames between 2 P/I frames. More B-frames slow down encoding and \
increase memory usage, but might result in better compression results. For VCD, this option is ignored, since \
the VCD standard requires 2 B-frames, no matter if you like them or not.",
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
  if(!strcmp(name, "format"))
    {
    SET_ENUM("mpeg1", com->format, FORMAT_MPEG1);
    SET_ENUM("mpeg2", com->format, FORMAT_MPEG2);
    SET_ENUM("vcd",   com->format, FORMAT_VCD);
    SET_ENUM("svcd",  com->format, FORMAT_SVCD);
    SET_ENUM("dvd",   com->format, FORMAT_DVD);
    }
  else if(!strcmp(name, "bframes"))
    com->bframes = val->val_i;
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
  
  /* TODO: More parameters */

  
  /* Output file */

  tmp_string = bg_sprintf(" -o %s", filename);
  ret = bg_strcat(ret, tmp_string);
  free(tmp_string);
  
  return ret;
  }
