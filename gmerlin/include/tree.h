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

#define BG_ALBUM_ENTRY_ERROR      (1<<0)
#define BG_ALBUM_ENTRY_SELECTED   (1<<1)
#define BG_ALBUM_ENTRY_PRIVNAME   (1<<2)
#define BG_ALBUM_ENTRY_REDIRECTOR (1<<3)
#define BG_ALBUM_ENTRY_SAVE_AUTH  (1<<4)

/*
 *  Shuffle mode passed to bg_media_tree_next() and
 *  bg_media_tree_previous()
 */

typedef enum
  {
    BG_SHUFFLE_MODE_OFF     = 0,
    BG_SHUFFLE_MODE_CURRENT = 1,
    BG_SHUFFLE_MODE_ALL     = 2,
    BG_NUM_SHUFFLE_MODES    = 3,
  } bg_shuffle_mode_t;

typedef struct bg_album_entry_s
  {
  char * name;
  void * location;
  char * plugin;
  gavl_time_t duration;

  int num_audio_streams;
  int num_still_streams;
  int num_video_streams;
  int num_subtitle_streams;

  /*
   *  Track index for multi track files/plugins
   */
  
  int index; 
  int total_tracks;

  /* Authentication data */

  char * username;
  char * password;
    
  /* Runtime ID unique in the whole tree */

  int id;
    
  int flags;
  struct bg_album_entry_s * next;
  } bg_album_entry_t;

typedef struct bg_album_s bg_album_t;
typedef struct bg_mediatree_s bg_media_tree_t;

/* Can return 0 if CD is missing or so */

typedef enum
  {
    /* Regular album */
    BG_ALBUM_TYPE_REGULAR    = 0,
    /* Drive for removable media */
    BG_ALBUM_TYPE_REMOVABLE  = 1,
    /* Hardware plugin (Subalbums are devices) */
    BG_ALBUM_TYPE_PLUGIN     = 2,
    /* Incoming album: Stuff from the commandline and the remote will go there */
    BG_ALBUM_TYPE_INCOMING   = 3,
    /* Favourites */
    BG_ALBUM_TYPE_FAVOURITES = 4,
  } bg_album_type_t;

bg_album_type_t bg_album_get_type(bg_album_t *); 

int bg_album_open(bg_album_t *);
void bg_album_close(bg_album_t *);

void bg_album_select_error_tracks(bg_album_t*);

void bg_album_get_times(bg_album_t * a,
                        gavl_time_t * duration_before,
                        gavl_time_t * duration_current,
                        gavl_time_t * duration_after);


int                bg_album_get_num_entries(bg_album_t*);
bg_album_entry_t * bg_album_get_entry(bg_album_t*, int);

void bg_album_set_change_callback(bg_album_t * a,
                                  void (*change_callback)(bg_album_t * a,
                                                          void * data),
                                  void * change_callback_data);

void bg_album_set_name_change_callback(bg_album_t * a,
                                       void (*name_change_callback)(bg_album_t * a,
                                                                    const char * name,
                                                                    void * data),
                                       void * name_change_callback_data);

void bg_album_move_selected_to_favourites(bg_album_t * a);
void bg_album_copy_selected_to_favourites(bg_album_t * a);

/* Return the current entry (might be NULL) */

bg_album_entry_t * bg_album_get_current_entry(bg_album_t*);

int bg_album_next(bg_album_t*, int wrap);
int bg_album_previous(bg_album_t*, int wrap);

void bg_album_set_current(bg_album_t * a, const bg_album_entry_t * e);

int bg_album_is_current(bg_album_t*);
int bg_album_entry_is_current(bg_album_t*, bg_album_entry_t*);

int          bg_album_get_num_children(bg_album_t *);
bg_album_t * bg_album_get_child(bg_album_t *, int);

char * bg_album_get_name(bg_album_t * a);
char * bg_album_get_location(bg_album_t * a);

void bg_album_set_error(bg_album_t * a, int err);
int  bg_album_get_error(bg_album_t * a);

void bg_album_rename(bg_album_t * a, const char *);

void bg_album_append_child(bg_album_t * parent, bg_album_t * child);

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

/* Load an xml string and make album entries from it */

bg_album_entry_t *
bg_album_entries_new_from_xml(const char * xml_string,
                             int length);

/* Destroy the list returned from the function above */

void bg_album_entries_destroy(bg_album_entry_t*);

/* Check, how many they are */
int bg_album_entries_count(bg_album_entry_t*);

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

/* Get the config section */

bg_cfg_section_t * bg_album_get_cfg_section(bg_album_t * album);

bg_plugin_registry_t * bg_album_get_plugin_registry(bg_album_t * album);

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
void bg_album_sort_entries(bg_album_t * album);
void bg_album_sort_children(bg_album_t * album);

void bg_album_rename_track(bg_album_t * album,
                           const bg_album_entry_t * entry,
                           const char * name);
/* Return value should be free()d                         */

char * bg_album_save_to_memory(bg_album_t * a, int * len);
char * bg_album_save_selected_to_memory(bg_album_t * a, int * len, int preserve_current);

/*
 *  tree.c
 */

/* Media tree */

bg_media_tree_t * bg_media_tree_create(const char * filename,
                                       bg_plugin_registry_t * plugin_reg);

bg_plugin_registry_t *
bg_media_tree_get_plugin_registry(bg_media_tree_t *);

bg_cfg_section_t *
bg_media_tree_get_cfg_section(bg_media_tree_t *);


bg_album_t * bg_media_tree_get_incoming(bg_media_tree_t *);

void
bg_media_tree_set_change_callback(bg_media_tree_t *,
                                  void (*change_callback)(bg_media_tree_t*,
                                                          void*),
                                  void*);

void bg_media_tree_set_userpass_callback(bg_media_tree_t *,
                                         int (*userpass_callback)(const char * resource,
                                                                  char ** user, char ** pass,
                                                                  int * save_password,
                                                                  void * data),
                                         void * userpass_callback_data);

void bg_media_tree_set_play_callback(bg_media_tree_t *,
                                     void (*play_callback)(void*),
                                     void*);

/* Will be called if an error occured while opening a track */

void bg_media_tree_set_error_callback(bg_media_tree_t *,
                                      void (*error_callback)(bg_media_tree_t*, void*,
                                                             const char*),
                                      void*);

void bg_media_tree_destroy(bg_media_tree_t *);

int bg_media_tree_get_num_albums(bg_media_tree_t *);

bg_album_t * bg_media_tree_get_current_album(bg_media_tree_t *);

void bg_media_tree_add_directory(bg_media_tree_t * t, bg_album_t * parent,
                                 const char * directory,
                                 int recursive,
                                 int subdirs_to_subalbums,
                                 const char * plugin);

/* Gets a root album */

bg_album_t * bg_media_tree_get_album(bg_media_tree_t *, int);

void bg_album_set_devices(bg_album_t * a);

void bg_album_find_devices(bg_album_t * a);


void bg_album_add_device(bg_album_t * a,
                          const char * device,
                          const char * name);

void bg_album_remove_from_parent(bg_album_t * album);


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


/* tree_xml.c */

void bg_media_tree_load(bg_media_tree_t *);
void bg_media_tree_save(bg_media_tree_t *);

/* Set and get entries for playback */

void bg_media_tree_set_current(void * data,
                               bg_album_t * album,
                               const bg_album_entry_t * entry);

/* Set the next and previous track */

int bg_media_tree_next(bg_media_tree_t *, int wrap,
                       bg_shuffle_mode_t shuffle_mode);
int bg_media_tree_previous(bg_media_tree_t *, int wrap,
                           bg_shuffle_mode_t shuffle_mode);

bg_plugin_handle_t *
bg_media_tree_get_current_track(bg_media_tree_t *, int * index);

const char * bg_media_tree_get_current_track_name(bg_media_tree_t *);

/* Configuration stuff */

bg_parameter_info_t * bg_media_tree_get_parameters(bg_media_tree_t*);
void bg_media_tree_set_parameter(void * priv, char * name, bg_parameter_value_t * val);

/* Mark the current entry as error */

void bg_media_tree_mark_error(bg_media_tree_t * , int err);


#endif //  __BG_TREE_H_

