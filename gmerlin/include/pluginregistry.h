/*****************************************************************
 
  pluginregistry.h
 
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

#ifndef __BG_PLUGINREGISTRY_H_
#define __BG_PLUGINREGISTRY_H_

/* Plugin registry */
#include <pthread.h>
#include <plugin.h>
#include <cfg_registry.h>

typedef struct bg_plugin_info_s
  {
  char * name;
  char * long_name;
  char * mimetypes;
  char * extensions;
  char * module_filename;
  long   module_time; /* Modification time of DLL, needed internally */

  int type;
  int flags;
    
  struct bg_plugin_info_s * next;
  } bg_plugin_info_t;

typedef struct bg_plugin_registry_s bg_plugin_registry_t;

typedef struct bg_plugin_handle_s
  {
  void * dll_handle;
  bg_plugin_common_t * plugin;
  const bg_plugin_info_t * info;
  void * priv;
  pthread_mutex_t mutex;
  int refcount;
  } bg_plugin_handle_t;

/*
 *  pluginregistry.c
 */

bg_plugin_registry_t *
bg_plugin_registry_create(bg_cfg_section_t * section);

void bg_plugin_registry_destroy(bg_plugin_registry_t *);

int bg_plugin_registry_get_num_plugins(bg_plugin_registry_t *,
                                       uint32_t type_mask, uint32_t flag_mask);

const bg_plugin_info_t *
bg_plugin_find_by_index(bg_plugin_registry_t *, int index,
                        uint32_t type_mask, uint32_t flag_mask);

const bg_plugin_info_t *
bg_plugin_find_by_name(bg_plugin_registry_t *, const char * name);

const bg_plugin_info_t *
bg_plugin_find_by_filename(bg_plugin_registry_t *, const char * filename, int type_mask);

const bg_plugin_info_t *
bg_plugin_find_by_mimetype(bg_plugin_registry_t *, const char * mimetype);

const bg_plugin_info_t *
bg_plugin_find_by_long_name(bg_plugin_registry_t *, const char * long_name);

/* Set the supported extensions and mimetypes for a plugin */

void bg_plugin_registry_set_extensions(bg_plugin_registry_t *,
                                       const char * plugin_name,
                                       const char * extensions);
void bg_plugin_registry_set_mimetypes(bg_plugin_registry_t *,
                                      const char * plugin_name,
                                      const char * mimetypes);

bg_cfg_section_t *
bg_plugin_registry_get_section(bg_plugin_registry_t *,
                               const char * plugin_name);

void bg_plugin_registry_set_default(bg_plugin_registry_t *,
                                    bg_plugin_type_t type,
                                    const char * name);

const bg_plugin_info_t * bg_plugin_registry_get_default(bg_plugin_registry_t *,
                                                        bg_plugin_type_t type);


/*
 *  Sort the plugin registry
 *  sort_string is a comma separated list of plugin names
 *  Plugins in the registry, which are not
 *  present in the sort string, will be appended to the end
 *
 *  It doesn't make much sense to sort anything other than
 *  input and redirector plugins with this function
 */


void
bg_plugin_registry_sort(bg_plugin_registry_t * r,
                        const char * sort_string);


/*
 *  Convenience function: Load an image.
 *  Use gavl_video_frame_destroy to free the
 *  return value
 */

gavl_video_frame_t * bg_plugin_registry_load_image(bg_plugin_registry_t * r,
                                                   const char * filename,
                                                   gavl_video_format_t * format);

/*
 *  pluginreg_xml.c
 *
 *  These functions load/save a chained list of plugin infos from/to
 *  a file (used by registry.c only)
 */

bg_plugin_info_t * bg_load_plugin_file(const char * filename);

void bg_save_plugin_file(bg_plugin_info_t * , const char * filename);

/*
 *  These are the actual loading/unloading functions
 *  (loader.c)
 */

/* Load a plugin and return handle with reference count of 1 */

bg_plugin_handle_t * bg_plugin_load(bg_plugin_registry_t * reg,
                                    const bg_plugin_info_t * info);

/* Copy a plugin with the same dll handle but with a new instance */

bg_plugin_handle_t * bg_plugin_copy(bg_plugin_registry_t * reg, bg_plugin_handle_t *);

// void bg_plugin_unload(bg_plugin_handle_t *);

void bg_plugin_lock(bg_plugin_handle_t *);
void bg_plugin_unlock(bg_plugin_handle_t *);

/* Reference counting for input plugins */

void bg_plugin_ref(bg_plugin_handle_t *);

/* Plugin will be unloaded when refcount is zero */

void bg_plugin_unref(bg_plugin_handle_t *);

#endif // __BG_PLUGINREGISTRY_H_
