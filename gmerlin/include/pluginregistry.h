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

#include "plugin.h"
#include "cfg_registry.h"

/** \defgroup plugin_registry Plugin registry
 *  \brief Database of all installed plugins
 *
 *  The plugin registry keeps informations about all installed plugins.
 *  Furthermore, it manages default plugins and some other settings.
 *  Available plugins are cached in the file $HOME/.gmerlin/plugins.xml,
 *  which is used by all applications. Application specific data are
 *  stored in a \ref bg_cfg_section_t.
 *
 *  It allows you to search for plugins according to certain criteria.
 *  You get detailed information about plugins in \ref bg_plugin_info_t
 *  structs.
 */

/** \ingroup plugin_registry
 *  \brief Information about a plugin
 */

typedef struct bg_plugin_info_s
  {
  char * gettext_domain; //!< First argument for bindtextdomain(). 
  char * gettext_directory; //!< Second argument for bindtextdomain().
  
  char * name;            //!< unique short name
  char * long_name;       //!< Humanized name
  char * mimetypes;       //!< Mimetypes, this plugin can handle
  char * extensions;      //!< Extensions, this plugin can handle
  char * protocols;       //!< Network protocols, this plugin can handle

  char * description;     //!< Description of what the plugin does

  char * module_filename; //!< Path of the shared module
  long   module_time;     //!< Modification time of the shared module, needed internally

  
  
  bg_plugin_type_t type; //!< Plugin type
  int flags;             //!< Flags (see \ref plugin_flags)
  int priority;          //!< Priority (1..10)
  
  bg_device_info_t * devices; //!< Device list returned by the plugin
  
  struct bg_plugin_info_s * next; //!< Used for chaining, never touch this

  bg_parameter_info_t * parameters; //!< Parameters, which can be passed to the plugin
  
  int max_audio_streams; //!< For encoders: Maximum number of audio streams (-1 means infinite)
  int max_video_streams; //!< For encoders: Maximum number of video streams (-1 means infinite)
  int max_subtitle_text_streams;//!< For encoders: Maximum number of text subtitle streams (-1 means infinite)
  int max_subtitle_overlay_streams;//!< For encoders: Maximum number of overlay subtitle streams (-1 means infinite)

  bg_parameter_info_t * audio_parameters; //!< Parameters, which can be passed to set_audio_parameter
  bg_parameter_info_t * video_parameters; //!< Parameters, which can be passed to set_video_parameter

  bg_parameter_info_t * subtitle_text_parameters; //!< Parameters, which can be passed to set_subtitle_text_parameter
  bg_parameter_info_t * subtitle_overlay_parameters; //!< Parameters, which can be passed to set_subtitle_overlay_parameter
  
  } bg_plugin_info_t;

/** \ingroup plugin_registry
 *  \brief Opaque handle for a plugin registry
 *
 *  You don't want to know, what's inside here.
 */

typedef struct bg_plugin_registry_s bg_plugin_registry_t;

/** \ingroup plugin_registry
 *  \brief Handle of a loaded plugin
 *
 *  When you load a plugin, the shared module will be loaded. Then, the
 *  create method of the plugin is called. The pointer obtained from the create method
 *  is stored in the priv member of the returned handle.
 */

typedef struct bg_plugin_handle_s
  {
  /* Private members, should not be accessed! */
    
  void * dll_handle; //!< dll_handle (don't touch)
  pthread_mutex_t mutex; //!< dll_handle (don't touch, use \ref bg_plugin_lock and \ref bg_plugin_unlock)
  int refcount;          //!< Reference counter (don't touch, use \ref bg_plugin_ref and \ref bg_plugin_unref)
  bg_plugin_registry_t * plugin_reg; //!< The plugin registry, from which the plugin was loaded
  
  /* These are for use by applications */
  
  bg_plugin_common_t * plugin; //!< Common structure, cast this to the derived type (e.g. \ref bg_input_plugin_t).
  const bg_plugin_info_t * info; //!< Info about this plugin
  void * priv; //!< Private handle, passed as the first argument to most plugin functions
  
  } bg_plugin_handle_t;

/*
 *  pluginregistry.c
 */

/** \ingroup plugin_registry
 * \brief Create a plugin registry
 * \param section A configuration section
 *
 *  The configuration section will be owned exclusively
 *  by the plugin registry, applications should not touch it.
 */

bg_plugin_registry_t *
bg_plugin_registry_create(bg_cfg_section_t * section);

/** \ingroup plugin_registry
 * \brief Destroy a plugin registry
 * \param reg A plugin registry
 */

void bg_plugin_registry_destroy(bg_plugin_registry_t * reg);

/** \ingroup plugin_registry
 *  \brief Count plugins
 *  \param reg A plugin registry
 *  \param type_mask Mask of all types you want to have
 *  \param flag_mask Mask of all flags you want to have
 *  \returns Number of available plugins matching type_mask and flag_mask
 */

int bg_plugin_registry_get_num_plugins(bg_plugin_registry_t * reg,
                                       uint32_t type_mask, uint32_t flag_mask);
/** \ingroup plugin_registry
 *  \brief Find a plugin by index
 *  \param reg A plugin registry
 *  \param index Index
 *  \param type_mask Mask of all types you want to have
 *  \param flag_mask Mask of all flags you want to have
 *  \returns A plugin info or NULL
 *
 *  This function should be called after
 *  \ref bg_plugin_registry_get_num_plugins to get a particular plugin
 */

const bg_plugin_info_t *
bg_plugin_find_by_index(bg_plugin_registry_t * reg, int index,
                        uint32_t type_mask, uint32_t flag_mask);

/** \ingroup plugin_registry
 *  \brief Find a plugin by it's unique short name
 *  \param reg A plugin registry
 *  \param name The name
 *  \returns A plugin info or NULL
 */

const bg_plugin_info_t *
bg_plugin_find_by_name(bg_plugin_registry_t * reg, const char * name);

/** \ingroup plugin_registry
 *  \brief Find a plugin by the file extension
 *  \param reg A plugin registry
 *  \param filename The file, whose extension should match
 *  \param type_mask Mask of plugin types to be returned
 *  \returns A plugin info or NULL
 *
 *  This function returns the first plugin matching type_mask,
 *  whose extensions match filename.
 */
const bg_plugin_info_t *
bg_plugin_find_by_filename(bg_plugin_registry_t * reg,
                           const char * filename, int type_mask);


/** \ingroup plugin_registry
 *  \brief Find an input plugin for a network protocol
 *  \param reg A plugin registry
 *  \param protocol The network protocol (e.g. http)
 *  \returns A plugin info or NULL
 */
const bg_plugin_info_t *
bg_plugin_find_by_protocol(bg_plugin_registry_t * reg,
                           const char * protocol);


/* Another method: Return long names as strings (NULL terminated) */

/** \ingroup plugin_registry
 *  \brief Get a list of plugins
 *  \param reg A plugin registry
 *  \param type_mask Mask of all returned plugin types
 *  \param flag_mask Mask of all returned plugin flags
 *  \returns A NULL-terminated list of plugin names.
 *
 *  This functions returns plugin names suitable for adding to
 *  GUI menus. Use \ref bg_plugin_find_by_name_name to get
 *  the corresponding plugin infos.
 *
 *  Use \ref bg_plugin_registry_free_plugins to free the returned list.
 */

char ** bg_plugin_registry_get_plugins(bg_plugin_registry_t*reg,
                                       uint32_t type_mask,
                                       uint32_t flag_mask);

/** \ingroup plugin_registry
 *  \brief Free a plugin list
 *  \param plugins List returned by \ref bg_plugin_registry_get_plugins
 */
void bg_plugin_registry_free_plugins(char ** plugins);


/*  Finally a version for finding/loading plugins */

/*
 *  info can be NULL
 *  If ret is non NULL before the call, the plugin will be unrefed
 *
 *  Return values are 0 for error, 1 on success
 */

/** \ingroup plugin_registry
 *  \brief Load and open an input plugin
 *  \param reg A plugin registry
 *  \param location Filename or URL
 *  \param info Plugin to use (can be NULL for autodetection)
 *  \param ret Will return the plugin handle.
 *  \param error_msg Might return an error message
 *  \param callbacks Input callbacks (only for authentication)
 *  \returns 1 on success, 0 on error.
 *
 *  This is a convenience function to load an input file. If info is
 *  NULL, the plugin will be autodetected. The handle is stored in ret.
 *  If ret is non-null before the call, the old plugin will be unrefed.
 *  If an error occurred, error_msg might contain a human readable error
 *  message. The error string must be freed by the caller.
 */

int bg_input_plugin_load(bg_plugin_registry_t * reg,
                         const char * location,
                         const bg_plugin_info_t * info,
                         bg_plugin_handle_t ** ret,
                         bg_input_callbacks_t * callbacks);

/* Set the supported extensions and mimetypes for a plugin */

/** \ingroup plugin_registry
 *  \brief Set file extensions for a plugin
 *  \param reg A plugin registry
 *  \param plugin_name Name of the plugin
 *  \param extensions Space separated list of file extensions
 *
 *  The extensions will be saved in the plugin file
 */

void bg_plugin_registry_set_extensions(bg_plugin_registry_t * reg,
                                       const char * plugin_name,
                                       const char * extensions);

/** \ingroup plugin_registry
 *  \brief Set protocols for a plugin
 *  \param reg A plugin registry
 *  \param plugin_name Name of the plugin
 *  \param protocols Space separated list of protocols
 *
 *  The protocols will be saved in the plugin file
 */

void bg_plugin_registry_set_protocols(bg_plugin_registry_t * reg,
                                      const char * plugin_name,
                                      const char * protocols);

/** \ingroup plugin_registry
 *  \brief Set priority for a plugin
 *  \param reg A plugin registry
 *  \param plugin_name Name of the plugin
 *  \param priority Priority (BG_PLUGIN_PRIORITY_MIN..BG_PLUGIN_PRIORITY_MAX, should be 1..10)
 *
 *  The priority will be saved in the plugin file
 */

void bg_plugin_registry_set_priority(bg_plugin_registry_t * reg,
                                     const char * plugin_name,
                                     int priority);


/** \ingroup plugin_registry
 *  \brief Get the config section belonging to a plugin
 *  \param reg A plugin registry
 *  \param plugin_name Short name of the plugin
 *  \returns The config section belonging to the plugin or NULL
 */
bg_cfg_section_t *
bg_plugin_registry_get_section(bg_plugin_registry_t * reg,
                               const char * plugin_name);

/** \ingroup plugin_registry
 *  \brief Set a parameter info for selecting and configuring plugins
 *  \param reg A plugin registry
 *  \param type_mask Mask of all returned types
 *  \param flag_mask Mask of all returned flags
 *  \param ret Where the parameter info will be copied
 *
 */

void bg_plugin_registry_set_parameter_info(bg_plugin_registry_t * reg,
                                           uint32_t type_mask,
                                           uint32_t flag_mask,
                                           bg_parameter_info_t * ret);


/** \ingroup plugin_registry_defaults 
 *  \brief Set the default for a particular plugin type
 *  \param reg A plugin registry
 *  \param type The type for which the default will be set
 *  \param plugin_name Short name of the plugin
 *
 *  Default plugins are stored for various types (recorders, output and
 *  encoders). The default will be stored in the config section.
 */

void bg_plugin_registry_set_default(bg_plugin_registry_t * reg,
                                    bg_plugin_type_t type,
                                    const char * plugin_name);

/** \ingroup plugin_registry_defaults
 *  \brief Set the default for a particular plugin type
 *  \param reg A plugin registry
 *  \param type The plugin type
 *  \returns A plugin info or NULL
 *
 *  Note, that the registry does not store a default input plugin.
 */
const bg_plugin_info_t * bg_plugin_registry_get_default(bg_plugin_registry_t * reg,
                                                        bg_plugin_type_t type);


/** \defgroup plugin_registry_defaults Defaults saved between sessions
 *  \ingroup plugin_registry
 *  \brief Plugin defaults
 *
 *  The registry stores a complete plugin setup for any kind of application. This includes the
 *  default plugins (see \ref bg_plugin_registry_get_default and bg_plugin_registry_set_default),
 *  their parameters,
 *  as well as flags, whether encoded streams should be multiplexed or not.
 *  It's up the the application if these informations are
 *  used or not.
 *
 *  These infos play no role inside the registry, but they are saved and reloaded
 *  between sessions. 
 *
 */

/** \ingroup plugin_registry_defaults
 *  \brief Specify whether audio should be encoded into the same file as the video if possible
 *  \param reg A plugin registry
 *  \param audio_to_video 0 if the audio streams should always go to a separate file, 1 else
 */

void bg_plugin_registry_set_encode_audio_to_video(bg_plugin_registry_t * reg,
                                                  int audio_to_video);

/** \ingroup plugin_registry_defaults
 *  \brief Query whether audio should be encoded into the same file as the video if possible
 *  \param reg A plugin registry
 *  \returns 0 if the audio streams should always go to a separate file, 1 else
 */

int bg_plugin_registry_get_encode_audio_to_video(bg_plugin_registry_t * reg);

/** \ingroup plugin_registry_defaults
 *  \brief Specify whether text subtitles should be encoded into the same file as the video if possible
 *  \param reg A plugin registry
 *  \param subtitle_text_to_video 0 if the text subtitles should always go to a separate file, 1 else
 */

void bg_plugin_registry_set_encode_subtitle_text_to_video(bg_plugin_registry_t * reg,
                                                          int subtitle_text_to_video);

/** \ingroup plugin_registry_defaults
 *  \brief Query whether text subtitles should be encoded into the same file as the video if possible
 *  \param reg A plugin registry
 *  \returns 0 if the text subtitles should always go to a separate file, 1 else
 */

int bg_plugin_registry_get_encode_subtitle_text_to_video(bg_plugin_registry_t * reg);

/** \ingroup plugin_registry_defaults
 *  \brief Specify whether overlay subtitles should be encoded into the same file as the video if possible
 *  \param reg A plugin registry
 *  \param subtitle_overlay_to_video 0 if the overay subtitles should always go to a separate file, 1 else
 */

void bg_plugin_registry_set_encode_subtitle_overlay_to_video(bg_plugin_registry_t * reg,
                                                             int subtitle_overlay_to_video);

/** \ingroup plugin_registry_defaults
 *  \brief Query whether overlay subtitles should be encoded into the same file as the video if possible
 *  \param reg A plugin registry
 *  \returns 0 if the overay subtitles should always go to a separate file, 1 else
 */
int bg_plugin_registry_get_encode_subtitle_overlay_to_video(bg_plugin_registry_t * reg);

/** \ingroup plugin_registry_defaults
 *  \brief Specify whether postprocessing should be done after encoding
 *  \param reg A plugin registry
 *  \param encode_pp 0 if the overay subtitles should always go to a separate file, 1 else
 */

void bg_plugin_registry_set_encode_pp(bg_plugin_registry_t * reg,
                                      int encode_pp);

/** \ingroup plugin_registry_defaults
 *  \brief Query whether postprocessing should be done after encoding
 *  \param reg A plugin registry
 *  \returns 0 if the overay subtitles should always go to a separate file, 1 else
 */
int bg_plugin_registry_get_encode_pp(bg_plugin_registry_t * reg);

/*
 *  Add a device to a plugin
 */

/** \ingroup plugin_registry
 *  \brief
 *  \param reg A plugin registry
 *  \param plugin_name Name of the plugin
 *  \param device Device file
 *  \param name Name for the device
 */

void bg_plugin_registry_add_device(bg_plugin_registry_t * reg,
                                   const char * plugin_name,
                                   const char * device,
                                   const char * name);

/** \ingroup plugin_registry
 *  \brief Change the name of a device
 *  \param reg A plugin registry
 *  \param plugin_name Name of the plugin
 *  \param device Device file name
 *  \param name New name for the device
 *
 *  Usually, plugins are quite smart in getting the name (Vendor, Model etc)
 *  of devices from the OS. In the case, you want to change names of a device
 *  use this function
 */

void bg_plugin_registry_set_device_name(bg_plugin_registry_t * reg,
                                        const char * plugin_name,
                                        const char * device,
                                        const char * name);

/* Rescan the available devices */

/** \ingroup plugin_registry
 *  \brief Let a plugin rescan for devices
 *  \param reg A plugin registry
 *  \param plugin_name Name of the plugin
 *
 *  This will let the plugin rescan for devices. Call this,
 *  after you changed your hardware.
 */

void bg_plugin_registry_find_devices(bg_plugin_registry_t * reg,
                                     const char * plugin_name);

/** \ingroup plugin_registry
 *  \brief Remove a device
 *  \param reg A plugin registry
 *  \param plugin_name Name of the plugin
 *  \param device Device file name
 *  \param name New name for the device
 *
 *  Remove a device from the list of devices. Call this if a
 *  plugin detected a device multiple times.
 */

void bg_plugin_registry_remove_device(bg_plugin_registry_t * reg,
                                      const char * plugin_name,
                                      const char * device,
                                      const char * name);

/** \ingroup plugin_registry
 *  \brief Load an image
 *  \param reg A plugin registry
 *  \param filename Image filename
 *  \param format Returns format of the image
 *  \returns The frame, which contains the image
 *
 *  Use gavl_video_frame_destroy to free the
 *  return value
 */

gavl_video_frame_t * bg_plugin_registry_load_image(bg_plugin_registry_t * reg,
                                                   const char * filename,
                                                   gavl_video_format_t * format);

/* Same as above for writing. Does implicit pixelformat conversion */

/** \ingroup plugin_registry
 *  \brief
 *  \param reg A plugin registry
 */

/** \ingroup plugin_registry
 *  \brief Save an image
 *  \param reg A plugin registry
 *  \param filename Image filename
 *  \param frame The frame, which contains the image
 *  \param format Returns format of the image
 */

void
bg_plugin_registry_save_image(bg_plugin_registry_t * reg,
                              const char * filename,
                              gavl_video_frame_t * frame,
                              const gavl_video_format_t * format);



/*
 *  These are the actual loading/unloading functions
 *  (loader.c)
 */

/* Load a plugin and return handle with reference count of 1 */

/** \ingroup plugin_registry
 *  \brief Load a plugin
 *  \param reg A plugin registry
 *  \param info The plugin info
 *
 *  Load a plugin and return handle with reference count of 1
 */

bg_plugin_handle_t * bg_plugin_load(bg_plugin_registry_t * reg,
                                    const bg_plugin_info_t * info);

/** \ingroup plugin_registry
 *  \brief Lock a plugin
 *  \param h A plugin handle
 */
void bg_plugin_lock(bg_plugin_handle_t * h);

/** \ingroup plugin_registry
 *  \brief Unlock a plugin
 *  \param h A plugin handle
 */
void bg_plugin_unlock(bg_plugin_handle_t * h);

/* Reference counting for input plugins */

/** \ingroup plugin_registry
 *  \brief Increase the reference count
 *  \param h A plugin handle
 */
void bg_plugin_ref(bg_plugin_handle_t * h);

/* Plugin will be unloaded when refcount is zero */

/** \ingroup plugin_registry
 *  \brief Decrease the reference count
 *  \param h A plugin handle
 *
 *  If the reference count gets zero, the plugin will
 *  be destroyed
 */
void bg_plugin_unref(bg_plugin_handle_t * h);

/* Check if 2 plugins handles are equal */

/** \ingroup plugin_registry
 *  \brief Check if 2 plugins are equal
 *  \param h1 A Plugin handle
 *  \param h2 Another plugin handle
 *  \returns 1 if the plugins are equal, 0 else
 */

int bg_plugin_equal(bg_plugin_handle_t * h1,
                    bg_plugin_handle_t * h2);                    

#endif // __BG_PLUGINREGISTRY_H_
