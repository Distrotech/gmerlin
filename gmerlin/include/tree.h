/*****************************************************************
 
  tree.h
 
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

#ifndef __BG_TREE_H_
#define __BG_TREE_H_

#include <pluginregistry.h>

/* Entry flags                 */
/* Flags should not be changed */

#define BG_ALBUM_ENTRY_ERROR    (1<<0)
#define BG_ALBUM_ENTRY_SELECTED (1<<1)

typedef struct bg_album_entry_s
  {
  char * name;
  void * location;
  char * plugin;
  gavl_time_t duration;

  int num_audio_streams;
  int num_video_streams;
  int num_subpicture_streams;
  int num_programs;

  /*
   *  Track index for multi track files/plugins
   */
  
  int index; 
  int total_tracks;

  /* Runtime ID unique in the whole tree */

  int id;
    
  int flags;
  struct bg_album_entry_s * next;
  } bg_album_entry_t;

typedef struct bg_album_s bg_album_t;
typedef struct bg_mediatree_s bg_media_tree_t;

void bg_album_open(bg_album_t *);
void bg_album_close(bg_album_t *);

void bg_album_select_error_tracks(bg_album_t*);


int                bg_album_get_num_entries(bg_album_t*);
bg_album_entry_t * bg_album_get_entry(bg_album_t*, int);

void bg_album_set_change_callback(bg_album_t * a,
                                  void (*change_callback)(bg_album_t * a, void * data),
                                  void * change_callback_data);


/* Return the current entry (might be NULL) */

bg_album_entry_t * bg_album_get_current_entry(bg_album_t*);

int bg_album_next(bg_album_t*, int wrap);
int bg_album_previous(bg_album_t*, int wrap);

void bg_album_lock(bg_album_t *);
void bg_album_unlock(bg_album_t *);

void bg_album_set_current(bg_album_t * a, const bg_album_entry_t * e);

int bg_album_is_current(bg_album_t*);
int bg_album_entry_is_current(bg_album_t*, bg_album_entry_t*);

int          bg_album_get_num_children(bg_album_t *);
bg_album_t * bg_album_get_child(bg_album_t *, int);

char * bg_album_get_name(bg_album_t * a);
char * bg_album_get_location(bg_album_t * a);

void bg_album_set_name(bg_album_t * a, const char *);

int bg_album_is_removable(bg_album_t * a);

/*
 *  Mark an album as expanded i.e. itself and all the
 *  children are visible
 */

void bg_album_set_expanded(bg_album_t * a, int expanded);
int bg_album_get_expanded(bg_album_t * a);
int bg_album_is_open(bg_album_t * a);

/* Add items */

void bg_album_insert_urls_before(bg_album_t * a,
                                 char ** urls,
                                 const char * plugin,
                                 bg_album_entry_t * after);

void bg_album_insert_urls_after(bg_album_t * a,
                                char ** urls,
                                const char * plugin,
                                bg_album_entry_t * before);

/* Inserts an xml-string */

void bg_album_insert_xml_before(bg_album_t * a, const char * xml_string,
                                int length, bg_album_entry_t * after);

void bg_album_insert_xml_after(bg_album_t * a, const char * xml_string,
                               int length, bg_album_entry_t * before);

/* Inserts a string of the type text/uri-list into the album */

void bg_album_insert_urilist_after(bg_album_t * a, const char * xml_string,
                                   int length, bg_album_entry_t * before);

void bg_album_insert_urilist_before(bg_album_t * a, const char * xml_string,
                                    int length, bg_album_entry_t * after);

/*
 *  Play the current track. This works only, if a tree exists and
 *  has a play callback set
 */

void bg_album_play(bg_album_t * a);

void bg_album_set_coords(bg_album_t * a, int x, int y, int width, int height);

void bg_album_get_coords(bg_album_t * a, int * x, int * y,
                         int * width, int * height);

/* The Open Path of an album can be stored as well */

void bg_album_set_open_path(bg_album_t * a, const char * path);
const char * bg_album_get_open_path(bg_album_t * a);

char ** bg_album_get_plugins(bg_album_t * a,
                             uint32_t type_mask,
                             uint32_t flag_mask);

void bg_album_free_plugins(bg_album_t * a, char ** plugins);


/* album_xml.c */

void bg_album_insert_albums_before(bg_album_t * a,
                                   char ** locations,
                                   bg_album_entry_t * after);

void bg_album_insert_albums_after(bg_album_t * a,
                                  char ** locations,
                                  bg_album_entry_t * after);

void bg_album_save(bg_album_t * a, const char * filename);

/* Create a struing containing the xml code for the album */
/* Can be used for Drag & Drop for example                */

/* Delete entries */

void bg_album_delete_selected(bg_album_t * album);

/* Refresh entries */

void bg_album_refresh_selected(bg_album_t * album);

/* Move entries */

void bg_album_move_selected_up(bg_album_t * album);
void bg_album_move_selected_down(bg_album_t * album);
void bg_album_sort(bg_album_t * album);

void bg_album_rename_track(bg_album_t * album,
                           const bg_album_entry_t * entry,
                           const char * name);
/* Return value should be free()d                         */

char * bg_album_save_to_memory(bg_album_t * a, int * len);
char * bg_album_save_selected_to_memory(bg_album_t * a, int * len);

/* Media tree */

bg_media_tree_t * bg_media_tree_create(const char * filename,
                                       bg_plugin_registry_t * plugin_reg);

void bg_media_tree_set_change_callback(bg_media_tree_t *,
                                       void (*change_callback)(bg_media_tree_t*, void*),
                                       void*);

void bg_media_tree_set_play_callback(bg_media_tree_t *,
                                     void (*change_callback)(bg_media_tree_t*, void*),
                                     void*);

void bg_media_tree_destroy(bg_media_tree_t *);

int bg_media_tree_get_num_albums(bg_media_tree_t *);

/* Gets a root album */

bg_album_t * bg_media_tree_get_album(bg_media_tree_t *, int);

/*
 *  Returns an array of indices to the given album
 *  terminated with -1 and to be freed with free()
 */

int * bg_media_tree_get_path(bg_media_tree_t *, bg_album_t*);

/* Manage the tree */

bg_album_t * bg_media_tree_append_album(bg_media_tree_t *,
                                        bg_album_t * parent);

void bg_media_tree_remove_album(bg_media_tree_t *,
                                bg_album_t * album);

/* Check if we can move an album */

int bg_media_tree_check_move_album_before(bg_media_tree_t *,
                                          bg_album_t * album,
                                          bg_album_t * after);

int bg_media_tree_check_move_album_after(bg_media_tree_t *,
                                         bg_album_t * album,
                                         bg_album_t * before);

int bg_media_tree_check_move_album(bg_media_tree_t *,
                                   bg_album_t * album,
                                   bg_album_t * parent);

/* Move an album inside the tree */

void bg_media_tree_move_album_before(bg_media_tree_t *,
                                     bg_album_t * album,
                                     bg_album_t * after);

void bg_media_tree_move_album_after(bg_media_tree_t *,
                                    bg_album_t * album,
                                    bg_album_t * before);

void bg_media_tree_move_album(bg_media_tree_t *,
                              bg_album_t * album,
                              bg_album_t * parent);

bg_album_t * bg_media_tree_get_current_album(bg_media_tree_t*);

/* tree_xml.c */

void bg_media_tree_load(bg_media_tree_t *);
void bg_media_tree_save(bg_media_tree_t *);

/* Set and get entries for playback */

void bg_media_tree_set_current(bg_media_tree_t *,
                               bg_album_t * album,
                               const bg_album_entry_t * entry);

/* Set the next and previous track */

int bg_media_tree_next(bg_media_tree_t *, int wrap);
int bg_media_tree_previous(bg_media_tree_t *, int wrap);

bg_plugin_handle_t *
bg_media_tree_get_current_track(bg_media_tree_t *, int * index);

const char * bg_media_tree_get_current_track_name(bg_media_tree_t *);

void
bg_media_tree_set_coords(bg_media_tree_t * t,
                         int x, int y,
                         int width, int height);

void bg_media_tree_get_coords(bg_media_tree_t * t,
                              int * x, int * y,
                              int * width, int * height);


/* Configuration stuff */

bg_parameter_info_t * bg_media_tree_get_parameters(bg_media_tree_t*);
void bg_media_tree_set_parameter(void * priv, char * name, bg_parameter_value_t * val);

/* Mark the current entry as error */

void bg_media_tree_mark_error(bg_media_tree_t * , int err);


#endif //  __BG_TREE_H_

