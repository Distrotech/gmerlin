/*****************************************************************
 
  gmerlin.h
 
  Copyright (c) 2003-2004 by Burkhard Plaum - plaum@ipf.uni-stuttgart.de
 
  http://gmerlin.sourceforge.net
 
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
 
  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111, USA.
 
*****************************************************************/

#include <gtk/gtk.h>

#include <player.h>
#include <cfg_registry.h>
#include <pluginregistry.h>
#include <tree.h>
#include <gui_gtk/tree.h>
#include <gui_gtk/infowindow.h>

#include <cfg_dialog.h>

typedef struct gmerlin_s gmerlin_t;

#include "display.h"
#include "playerwindow.h"
#include "pluginwindow.h"

/* Volume boundaries */

#define VOLUME_MIN -40.0
#define VOLUME_MAX   0.0

/* Repeat mode */

typedef enum
  {
    REPEAT_MODE_NONE = 0,
    REPEAT_MODE_1    = 1,
    REPEAT_MODE_ALL  = 2,
    NUM_REPEAT_MODES = 3,
  } repeat_mode_t;

typedef struct
  {
  char * directory;
  
  player_window_skin_t playerwindow;
  } gmerlin_skin_t;

void gmerlin_skin_destroy(gmerlin_skin_t * s);

typedef struct gmerlin_skin_browser_s gmerlin_skin_browser_t;

#define PLAYBACK_SKIP_ERROR (1<<0)
#define PLAYBACK_NOADVANCE  (1<<1)

struct gmerlin_s
  {
  int playback_flags;

  repeat_mode_t repeat_mode;
  
  /* Core stuff */
  
  bg_cfg_registry_t    * cfg_reg;
  bg_plugin_registry_t * plugin_reg;
  bg_player_t          * player;
  bg_media_tree_t      * tree;
      
  /* GUI */

  bg_dialog_t * cfg_dialog;
    
  bg_gtk_tree_window_t * tree_window;

  player_window_t * player_window;
  bg_gtk_info_window_t * info_window;
  gmerlin_skin_t skin;

  plugin_window_t * plugin_window;
    
  gmerlin_skin_browser_t * skin_browser;

  int tree_error;

  /* Configuration stuff */

  bg_cfg_section_t * display_section;
  bg_cfg_section_t * tree_section;
  bg_cfg_section_t * general_section;
  bg_cfg_section_t * audio_section;
  bg_cfg_section_t * video_section;

  int show_info_window;
  int show_tree_window;
  };

gmerlin_t * gmerlin_create(bg_cfg_registry_t * cfg_reg);

void gmerlin_destroy(gmerlin_t*);

void gmerlin_run(gmerlin_t*);

/* Skin stuff */

void gmerlin_skin_load(gmerlin_skin_t *, const char * name);
void gmerlin_skin_set(gmerlin_t*);
void gmerlin_skin_free(gmerlin_skin_t*);

/* Skin browser */

gmerlin_skin_browser_t * gmerlin_skin_browser_create(gmerlin_t *);
void gmerlin_skin_browser_destroy(gmerlin_skin_browser_t *);
void gmerlin_skin_browser_show(gmerlin_skin_browser_t *);

/* Run the main config dialog */

void gmerlin_create_dialog(gmerlin_t * g);
void gmerlin_configure(gmerlin_t *);


int gmerlin_play(gmerlin_t * g, int ignore_flags);

/* This is called when the player signals that it wants a new
   track */

void gmerlin_next_track(gmerlin_t * g);


void gmerlin_tree_close_callback(bg_gtk_tree_window_t * win,
                                 void * data);

bg_parameter_info_t * gmerlin_get_parameters(gmerlin_t * g);

void gmerlin_set_parameter(void * data, char * name,
                           bg_parameter_value_t * val);

int gmerlin_get_parameter(void * data, char * name,
                          bg_parameter_value_t * val);

