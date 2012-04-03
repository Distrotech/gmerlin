/*****************************************************************
 * gmerlin - a general purpose multimedia framework and applications
 *
 * Copyright (c) 2001 - 2011 Members of the Gmerlin project
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

#include <config.h>
#include <time.h>

/* Flags should not be changed */

#define BG_ALBUM_EXPANDED    (1<<1)
#define BG_ALBUM_ERROR       (1<<2)
#define BG_ALBUM_CAN_EJECT   (1<<3)

typedef struct bg_shuffle_list_s
  {
  bg_album_entry_t * entry;
  bg_album_t       * album;
  struct bg_shuffle_list_s * next;
  struct bg_shuffle_list_s * prev;
  } bg_shuffle_list_t;

void bg_shuffle_list_destroy(bg_shuffle_list_t*l);

/* Types of the album */

typedef struct
  {
  bg_plugin_registry_t * plugin_reg;
  char * filename_template;
  char * directory;

  /* Highest Track ID */
  
  int highest_id;

  bg_album_t       * current_album;
  bg_album_entry_t * current_entry;

  void (*set_current_callback)(void * data, bg_album_t * current_album,
                               const bg_album_entry_t * current_entry);
  void * set_current_callback_data;

  void (*play_callback)(void * data);
  void * play_callback_data;

  int (*userpass_callback)(const char * resource, char ** user, char ** pass, int * save_password,
                           void * data);
  void * userpass_callback_data;
  
  
  bg_plugin_handle_t * load_handle;

  /* Configuration stuff */

  int use_metadata;
  char * metadata_format;

  char * blacklist;
  
  /* Shuffle list */

  bg_shuffle_list_t * shuffle_list;

  /* Favourites album */

  bg_album_t       * favourites;
  bg_input_callbacks_t input_callbacks;

  /* Stored authentication data */
  char * username;
  char * password;
  int save_auth;

  int prefer_edl;

#ifdef HAVE_INOTIFY
  int inotify_fd;
#endif

  } bg_album_common_t;

void bg_album_common_prepare_callbacks(bg_album_common_t*,bg_album_entry_t * entry);

void bg_album_common_set_auth_info(bg_album_common_t*, bg_album_entry_t*);

struct bg_album_s
  {
  bg_album_type_t type;
  bg_album_common_t * com;

  int open_count;
  
  char * name;        /* Name for dialog boxes      */
  char * xml_file;    /* Album filename */
  char * device;      /* Device */
  
  char * disc_name;   /* Set by open_removable */
  char * watch_dir;   /* Directory to watch */
  
  const bg_plugin_info_t * plugin_info;
  bg_plugin_handle_t      * handle;
  
  int    flags;

  struct bg_album_s       * children;
  struct bg_album_s       * next;
  struct bg_album_s       * parent;
  
  bg_album_entry_t * entries;
  
  void (*change_callback)(bg_album_t * a, void * data);
  void * change_callback_data;

  void (*entry_change_callback)(bg_album_t * a, const bg_album_entry_t * e, void * data);
  void * entry_change_callback_data;

  void (*name_change_callback)(bg_album_t * a, const char * name, void * data);
  void * name_change_callback_data;

  /* Current track changed */
  void (*current_change_callback)(bg_album_t * current_album,
                                  const bg_album_entry_t * current_entry, void * data);
  void * current_change_callback_data;

  /* Tracks were deleted */

  void (*delete_callback)(bg_album_t*,
                          int * indices, void * data);
  void * delete_callback_data;

  /* Tracks were inserted */

  void (*insert_callback)(bg_album_t*,
                          int start, int num, void * data);
  void * insert_callback_data;
  
  /* Coordinates in the screen */

  //  int x;
  //  int y;
  //  int width;
  //  int height;
  
  /* Path for loading files */
    
  //  char * open_path;

  /* Here, dialogs can store additional config data */
    
  bg_cfg_section_t * cfg_section;

#ifdef HAVE_INOTIFY
  int inotify_wd;
#endif
  };

/* Returns 0 on systems without inotify */
int bg_album_inotify(bg_album_t * a, uint8_t * event1);

/* album.c */

void bg_album_update_entry(bg_album_t * album,
                           bg_album_entry_t * entry,
                           bg_track_info_t  * track_info, int callback);

int bg_album_get_unique_id(bg_album_t * album);

bg_album_t * bg_album_create(bg_album_common_t * com, bg_album_type_t type,
                             bg_album_t * parent);


void bg_album_set_default_location(bg_album_t * album);

void bg_album_destroy(bg_album_t * album);

void bg_album_load(bg_album_t * album, const char * filename);

/* Notify the album of changes */

void bg_album_changed(bg_album_t * album);
void bg_album_current_changed(bg_album_t * album);

void bg_album_entry_changed(bg_album_t * album, const bg_album_entry_t * entry);

void bg_album_insert_entries_after(bg_album_t * album,
                                   bg_album_entry_t * new_entries,
                                   bg_album_entry_t * before);

void bg_album_insert_entries_before(bg_album_t * album,
                                    bg_album_entry_t * new_entries,
                                    bg_album_entry_t * after);

void bg_album_insert_file_before(bg_album_t * a,
                                 char * file,
                                 const char * plugin,
                                 int prefer_edl,
                                 bg_album_entry_t * after,
                                 time_t mtime);

void bg_album_set_watch_dir(bg_album_t * a,
                            const char * dir);

void bg_album_delete_with_file(bg_album_t * album, const char * filename);

void bg_album_delete_unsync(bg_album_t * album);
int bg_album_num_unsync(bg_album_t * a);


/*
 *   Load a single URL, perform redirection and return
 *   zero (for error) or more entries
 */

bg_album_entry_t * bg_album_load_url(bg_album_t * album,
                                     char * url,
                                     const char * plugin_name, int prefer_edl);

struct bg_mediatree_s
  {
  bg_album_common_t com;

  char * filename;
  char * add_directory_path;
  
  bg_album_t       * children;

  /* The following ones must always be there */

  bg_album_t       * incoming;
  
  bg_plugin_handle_t * load_handle;
  
  void (*change_callback)(bg_media_tree_t*, void*);
  void * change_callback_data;
  
  /* Current entry in shuffle list */

  bg_shuffle_list_t * shuffle_current;
  bg_shuffle_mode_t last_shuffle_mode;

  int purge_directory;

  /* Here, dialogs can store additional config data */
    
  bg_cfg_section_t * cfg_section;
  };

/* Purge the directory (remove unused albums) */

void bg_media_tree_purge_directory(bg_media_tree_t * t);

/* Create a unique filename */

// char * bg_media_tree_new_album_filename(bg_media_tree_t * tree);

void bg_album_entry_destroy(bg_album_entry_t * entry);

bg_album_entry_t * bg_album_entry_create(void);

bg_album_entry_t * bg_album_entry_copy(bg_album_t * a, bg_album_entry_t * e);

