/*****************************************************************
 * gmerlin - a general purpose multimedia framework and applications
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

#include <gtk/gtk.h>

#include <gmerlin/player.h>
#include <gmerlin/cfg_registry.h>
#include <gmerlin/pluginregistry.h>
#include <gmerlin/tree.h>
#include <gui_gtk/tree.h>
#include <gui_gtk/infowindow.h>
#include <gui_gtk/logwindow.h>
#include <gmerlin/lcdproc.h>

#include <gmerlin/cfg_dialog.h>

#include <gmerlin/remote.h>

typedef struct gmerlin_s gmerlin_t;

#include "display.h"
#include "playerwindow.h"

/* Class hints */

#define WINDOW_NAME  "Gmerlin"
#define WINDOW_CLASS "gmerlin"

/* Repeat mode */

typedef enum
  {
    REPEAT_MODE_NONE = 0,
    REPEAT_MODE_1    = 1,
    REPEAT_MODE_ALL  = 2,
    NUM_REPEAT_MODES = 3,
  } repeat_mode_t;

/* Accelerators */

#define ACCEL_VOLUME_DOWN             1
#define ACCEL_VOLUME_UP               2
#define ACCEL_SEEK_BACKWARD           3
#define ACCEL_SEEK_FORWARD            4
#define ACCEL_SEEK_START              5
#define ACCEL_PAUSE                   6
#define ACCEL_MUTE                    7
#define ACCEL_NEXT_CHAPTER            8
#define ACCEL_PREV_CHAPTER            9

#define ACCEL_NEXT                   10
#define ACCEL_PREV                   11
#define ACCEL_QUIT                   12
#define ACCEL_OPTIONS                13
#define ACCEL_GOTO_CURRENT           15
#define ACCEL_CURRENT_TO_FAVOURITES  16

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

  repeat_mode_t  repeat_mode;
  bg_shuffle_mode_t shuffle_mode;
  
  /* Core stuff */
  
  bg_cfg_registry_t    * cfg_reg;
  bg_plugin_registry_t * plugin_reg;
  bg_player_t          * player;
  bg_media_tree_t      * tree;
  
  /* GUI */

  bg_dialog_t * cfg_dialog;
  bg_dialog_t * audio_dialog;
  bg_dialog_t * audio_filter_dialog;

  bg_dialog_t * video_dialog;
  bg_dialog_t * video_filter_dialog;
  
  bg_dialog_t * subtitle_dialog;
  
  bg_dialog_t * visualization_dialog;
  
  bg_gtk_tree_window_t * tree_window;

  player_window_t * player_window;
  bg_gtk_info_window_t * info_window;
  bg_gtk_log_window_t * log_window;
  
  gmerlin_skin_t skin;
  char * skin_dir;
  
  gmerlin_skin_browser_t * skin_browser;

  int tree_error;

  /* Configuration stuff */

  bg_cfg_section_t * display_section;
  bg_cfg_section_t * tree_section;
  bg_cfg_section_t * general_section;
  bg_cfg_section_t * audio_section;
  bg_cfg_section_t * audio_filter_section;
  bg_cfg_section_t * video_section;
  bg_cfg_section_t * video_filter_section;
  bg_cfg_section_t * subtitle_section;
  bg_cfg_section_t * osd_section;
  bg_cfg_section_t * input_section;
  bg_cfg_section_t * lcdproc_section;
  bg_cfg_section_t * remote_section;
  bg_cfg_section_t * logwindow_section;
  bg_cfg_section_t * infowindow_section;
  bg_cfg_section_t * visualization_section;

  bg_parameter_info_t * input_plugin_parameters;
  bg_parameter_info_t * image_reader_parameters;
  
  int show_info_window;
  int show_log_window;
  int show_tree_window;

  bg_lcdproc_t * lcdproc;

  /* Remote control */
  bg_remote_server_t * remote;
  
  int player_state;

  /* For all windows */
  GtkAccelGroup *accel_group;
  };

gmerlin_t * gmerlin_create(bg_cfg_registry_t * cfg_reg);

/* Right after creating, urls can be added */

void gmerlin_add_locations(gmerlin_t * g, char ** locations);
void gmerlin_play_locations(gmerlin_t * g, char ** locations);

void gmerlin_open_device(gmerlin_t * g, char * device);
void gmerlin_play_device(gmerlin_t * g, char * device);

void gmerlin_destroy(gmerlin_t*);

void gmerlin_run(gmerlin_t*, char ** locations);

/* Skin stuff */

/* Load a skin from directory. Return the default dierectory if the
   skin could not be found */

char * gmerlin_skin_load(gmerlin_skin_t *, char * directory);


void gmerlin_skin_set(gmerlin_t*);
void gmerlin_skin_free(gmerlin_skin_t*);

/* Skin browser */

gmerlin_skin_browser_t * gmerlin_skin_browser_create(gmerlin_t *);
void gmerlin_skin_browser_destroy(gmerlin_skin_browser_t *);
void gmerlin_skin_browser_show(gmerlin_skin_browser_t *);

/* Run the main config dialog */

void gmerlin_create_dialog(gmerlin_t * g);

void gmerlin_configure(gmerlin_t * g);

int gmerlin_play(gmerlin_t * g, int ignore_flags);
void gmerlin_pause(gmerlin_t * g);

/* This is called when the player signals that it wants a new
   track */

void gmerlin_next_track(gmerlin_t * g);

/* Check if the next track in the tree has the index
   specified by track */

void gmerlin_check_next_track(gmerlin_t * g, int track);


void gmerlin_tree_close_callback(bg_gtk_tree_window_t * win,
                                 void * data);

const bg_parameter_info_t * gmerlin_get_parameters(gmerlin_t * g);

void gmerlin_set_parameter(void * data, const char * name,
                           const bg_parameter_value_t * val);

int gmerlin_get_parameter(void * data, const char * name,
                          bg_parameter_value_t * val);


/* Handle remote command */

void gmerlin_handle_remote(gmerlin_t * g, bg_msg_t * msg);
