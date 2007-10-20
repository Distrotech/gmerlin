/*****************************************************************
 
  vis_lemuria.c
 
  Copyright (c) 2007 by Burkhard Plaum - plaum@ipf.uni-stuttgart.de
 
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

#include <config.h>
#include <gmerlin/translation.h>
#include <gmerlin/plugin.h>
#include <gmerlin/utils.h>
#include <gmerlin/log.h>


#include <lemuria.h>

#define LOG_DOMAIN "vis_lemuria"

typedef struct
  {
  lemuria_engine_t * e;
  } lemuria_priv_t;

static void * create_lemuria()
  {
  lemuria_priv_t * ret;
  ret = calloc(1, sizeof(*ret));
  return ret;
  }

static void destroy_lemuria(void * priv)
  {
  lemuria_priv_t * vp;
  vp = (lemuria_priv_t *)priv;
  free(vp);
  }

#if 0
static bg_parameter_info_t parameters[] =
  {
    {  }
  };

static bg_parameter_info_t * get_parameters_lemuria(void * priv)
  {
  return parameters;
  }

static void
set_parameter_lemuria(void * priv, const char * name,
                    const bg_parameter_value_t * val)
  {
  if(!name)
    return;

  }
#endif

static int
open_lemuria(void * priv, gavl_audio_format_t * audio_format,
             const char * window_id)
  {
  lemuria_priv_t * vp;
  vp = (lemuria_priv_t *)priv;
  
  vp->e = lemuria_create(window_id, 256, 256);
  
  lemuria_adjust_format(vp->e, audio_format);
  return 1;
  }

static void draw_frame_lemuria(void * priv, gavl_video_frame_t * frame)
  {
  lemuria_priv_t * vp;
  vp = (lemuria_priv_t *)priv;
  lemuria_draw_frame(vp->e);
  }

static void update_lemuria(void * priv, gavl_audio_frame_t * frame)
  {
  lemuria_priv_t * vp;
  vp = (lemuria_priv_t *)priv;
  lemuria_update_audio(vp->e, frame);
  }

static void close_lemuria(void * priv)
  {
  lemuria_priv_t * vp;
  vp = (lemuria_priv_t *)priv;
  lemuria_destroy(vp->e);
  fprintf(stderr, "close lemuria\n");
  }

static void show_frame_lemuria(void * priv)
  {
  lemuria_priv_t * vp;
  vp = (lemuria_priv_t *)priv;
  lemuria_flash_frame(vp->e);
  lemuria_check_events(vp->e);
  }

bg_visualization_plugin_t the_plugin = 
  {
    common:
    {
      BG_LOCALE,
      name:      "vis_lemuria",
      long_name: TRS("Lemuria"),
      description: TRS("Lemuria"),
      type:     BG_PLUGIN_VISUALIZATION,
      flags:    BG_PLUGIN_VISUALIZE_GL,
      create:   create_lemuria,
      destroy:   destroy_lemuria,
      //      get_parameters:   get_parameters_lemuria,
      //      set_parameter:    set_parameter_lemuria,
      priority:         1,
    },
    open_win: open_lemuria,
    update: update_lemuria,
    draw_frame: draw_frame_lemuria,

    show_frame: show_frame_lemuria,
    
    close: close_lemuria
  };

/* Include this into all plugin modules exactly once
   to let the plugin loader obtain the API version */
BG_GET_PLUGIN_API_VERSION;
