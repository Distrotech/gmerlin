/*****************************************************************
 
  treeprivate.h
 
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

/* Flags should not be changed */

#define BG_ALBUM_EXPANDED    (1<<1)
#define BG_ALBUM_ERROR       (1<<2)

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

  bg_plugin_handle_t * load_handle;

  /* Configuration stuff */

  int use_metadata;
  char * metadata_format;
  } bg_album_common_t;

struct bg_album_s
  {
  bg_album_type_t type;

  bg_album_common_t * com;

  int open_count;
  
  char * name;        /* Name for dialog boxes      */
  char * location;    /* Album filename or device   */

  const bg_plugin_info_t * plugin_info;
  bg_plugin_handle_t      * handle;
  
  int    flags;

  struct bg_album_s       * children;
  struct bg_album_s       * next;
  struct bg_album_s       * parent;
  
  bg_album_entry_t * entries;
  
  //  bg_media_tree_t         * tree;
  pthread_mutex_t         mutex;


  void (*change_callback)(bg_album_t * a, void * data);
  void * change_callback_data;

  /* Coordinates in the screen */

  int x;
  int y;
  int width;
  int height;
  
  /* Path for loading files */
    
  char * open_path;

  };

/* album.c */

void bg_album_update_entry(bg_album_t * album,
                           bg_album_entry_t * entry,
                           bg_track_info_t  * track_info);

int bg_album_get_unique_id(bg_album_t * album);

bg_album_t * bg_album_create(bg_album_common_t * com, bg_album_type_t type,
                             bg_album_t * parent);


void bg_album_destroy(bg_album_t * album);

void bg_album_load(bg_album_t * album, const char * filename);

/* Notify the album of changes */

void bg_album_changed(bg_album_t * album);

void bg_album_insert_entries_after(bg_album_t * album,
                                   bg_album_entry_t * new_entries,
                                   bg_album_entry_t * before);

void bg_album_insert_entries_before(bg_album_t * album,
                                    bg_album_entry_t * new_entries,
                                    bg_album_entry_t * after);

int bg_album_refresh_entry(bg_album_t *,
                           bg_album_entry_t * entry);

/*
 *   Load a single URL, perform redirection and return
 *   zero (for error) or more entries
 */

bg_album_entry_t * bg_album_load_url(bg_album_t * album,
                                     char * url,
                                     const char * plugin_long_name);

struct bg_mediatree_s
  {
  bg_album_common_t com;

  char * filename;
  
  bg_album_t       * children;
  
  bg_plugin_handle_t * load_handle;
  
  void (*change_callback)(bg_media_tree_t*, void*);
  void * change_callback_data;
    
  void (*error_callback)(bg_media_tree_t*, void*, const char*);
  void * error_callback_data;

  /* Coordinates in the screen */

  int x;
  int y;
  int width;
  int height;
  
  };



/* Create a unique filename */

// char * bg_media_tree_new_album_filename(bg_media_tree_t * tree);

void bg_album_entry_destroy(bg_album_entry_t * entry);

bg_album_entry_t * bg_album_entry_create(bg_album_t * album);

/* Configzuration stuff */

