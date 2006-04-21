/*****************************************************************
 
  e_pp_vcdimager.c
 
  Copyright (c) 2006 by Burkhard Plaum - plaum@ipf.uni-stuttgart.de
 
  http://gmerlin.sourceforge.net
 
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
 
  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111, USA.
 
*****************************************************************/

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <config.h>
#include <plugin.h>
#include "cdrdao_common.h"

typedef struct
  {
  char * vcd_version;
  char * error_msg;

  char * bin_file;
  char * cue_file;
  } vcdimager_t;

static void * create_vcdimager()
  {
  vcdimager_t * ret;
  ret = calloc(1, sizeof(*ret));
  return ret;
  }

static void destroy_vcdimager(void * priv)
  {
  vcdimager_t * vcdimager;
  vcdimager = (vcdimager_t*)priv;


  free(vcdimager);
  }

static const char * get_error_vcdimager(void * priv)
  {
  vcdimager_t * vcdimager;
  vcdimager = (vcdimager_t*)priv;
  return vcdimager->error_msg;
  }


static bg_parameter_info_t parameters[] =
  {
    {
      name: "vcdimager",
      long_name: "VCD options",
      type: BG_PARAMETER_SECTION,
    },
    {
      name: "version",
      long_name: "Format",
      type: BG_PARAMETER_STRINGLIST,
      val_default: { val_str: "vcd2" },
      multi_names: (char*[]){ "vcd11", "vcd2", "svcd", "hqsvcd", (char*)0 },
      multi_labels: (char*[]){ "VCD 1.1", "VCD 2.0", "SVCD", "HQSVCD", (char*)0 },
    },
    {
      name: "bin_file",
      long_name: "Bin file",
      type: BG_PARAMETER_STRING,
      val_default: { val_str: "videocd.bin" },
    },
    {
      name: "cue_file",
      long_name: "Cue file",
      type: BG_PARAMETER_STRING,
      val_default: { val_str: "videocd.cue" },
    },
    CDRDAO_PARAMS,
    { /* End of parameters */ },
  };

static bg_parameter_info_t * get_parameters_vcdimager(void * data)
  {
  return parameters;
  }

static void set_parameter_vcdimager(void * data, char * name, bg_parameter_value_t * v)
  {

  }

static void set_callbacks_vcdimager(void * data, bg_e_pp_callbacks_t * callbacks)
  {

  }

static int init_vcdimager(void * data)
  {
  return 0;
  }

void add_track_vcdimager(void * data, const char * filename,
                         bg_metadata_t * metadata)
  {
  
  }

static void run_vcdimager(void * data, const char * directory, int cleanup)
  {

  }


bg_encoder_pp_plugin_t the_plugin =
  {
    common:
    {
      name:              "e_pp_vcdimager", /* Unique short name */
      long_name:         "VCD image generator/burner",
      mimetypes:         NULL,
      extensions:        "mpg",
      type:              BG_PLUGIN_ENCODER_PP,
      flags:             0,
      create:            create_vcdimager,
      destroy:           destroy_vcdimager,
      get_error:         get_error_vcdimager,
      get_parameters:    get_parameters_vcdimager,
      set_parameter:     set_parameter_vcdimager,
      priority:          1,
    },
    max_audio_streams:   1,
    max_video_streams:   0,

    set_callbacks:       set_callbacks_vcdimager,
    init:                init_vcdimager,
    add_track:           add_track_vcdimager,
    run:                 run_vcdimager,
    
  };

/* Include this into all plugin modules exactly once
   to let the plugin loader obtain the API version */
BG_GET_PLUGIN_API_VERSION;
