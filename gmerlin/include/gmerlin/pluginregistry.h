/*****************************************************************
 * gmerlin - a general purpose multimedia framework and applications
 *
 * Copyright (c) 2001 - 2012 Members of the Gmerlin project
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

#ifndef __BG_PLUGINREGISTRY_H_
#define __BG_PLUGINREGISTRY_H_

/* Plugin registry */
#include <pthread.h>

#include <gmerlin/plugin.h>
#include <gmerlin/cfg_registry.h>

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
 *  \brief Identifiers for plugin APIs
 */

typedef enum
  {
    BG_PLUGIN_API_GMERLIN = 0, //!< Always 0 so native plugins can leave this empty
    BG_PLUGIN_API_LADSPA,      //!< Ladspa API
    BG_PLUGIN_API_LV,          //!< Libvisual
    BG_PLUGIN_API_FREI0R,      //!< frei0r
  } bg_plugin_api_t;

/** \ingroup plugin_registry
 *  \brief Identifiers for stream types
 */

typedef enum
  {
    BG_STREAM_AUDIO             = (1<<0),
    BG_STREAM_SUBTITLE_TEXT     = (1<<1),
    BG_STREAM_SUBTITLE_OVERLAY  = (1<<2),
    BG_STREAM_VIDEO             = (1<<3),
  } bg_stream_type_t;

/** \ingroup plugin_registry
 *  \brief Typedef for plugin info
 */

typedef struct bg_plugin_info_s  bg_plugin_info_t;

/** \ingroup plugin_registry
 *  \brief Information about a plugin
 */

struct bg_plugin_info_s
  {
  char * gettext_domain; //!< First argument for bindtextdomain(). 
  char * gettext_directory; //!< Second argument for bindtextdomain().
  
  char * name;            //!< unique short name
  char * long_name;       //!< Humanized name
  char * mimetypes;       //!< Mimetypes, this plugin can handle
  char * extensions;      //!< Extensions, this plugin can handle
  char * protocols;       //!< Network protocols, this plugin can handle
  gavl_codec_id_t * compressions; //!< Compressions, this plugin can handle

  char * description;     //!< Description of what the plugin does

  char * module_filename; //!< Path of the shared module
  long   module_time;     //!< Modification time of the shared module, needed internally

  bg_plugin_api_t api;    //!< API of the plugin
  int index;              //!< Index inside the module. Always 0 for native plugins.
  
  bg_plugin_type_t type; //!< Plugin type
  int flags;             //!< Flags (see \ref plugin_flags)
  int priority;          //!< Priority (1..10)
  
  bg_device_info_t * devices; //!< Device list returned by the plugin
  
  bg_plugin_info_t * next; //!< Used for chaining, never touch this

  bg_parameter_info_t * parameters; //!< Parameters, which can be passed to the plugin

  
  int max_audio_streams; //!< For encoders: Maximum number of audio streams (-1 means infinite)
  int max_video_streams; //!< For encoders: Maximum number of video streams (-1 means infinite)
  int max_subtitle_text_streams;//!< For encoders: Maximum number of text subtitle streams (-1 means infinite)
  int max_subtitle_overlay_streams;//!< For encoders: Maximum number of overlay subtitle streams (-1 means infinite)

  bg_parameter_info_t * audio_parameters; //!< Parameters, which can be passed to set_audio_parameter
  bg_parameter_info_t * video_parameters; //!< Parameters, which can be passed to set_video_parameter

  bg_parameter_info_t * subtitle_text_parameters; //!< Parameters, which can be passed to set_subtitle_text_parameter
  bg_parameter_info_t * subtitle_overlay_parameters; //!< Parameters, which can be passed to set_subtitle_overlay_parameter
  
  char * cmp_name; //!< Name used for alphabetical sorting. Not for external use.
  
  };

/** \ingroup plugin_registry
 *  \brief Creation options for a plugin registry
 *
 
 */

typedef struct
  {
  char ** blacklist; //!< Plugins, which should be ignored
  int dont_save;            //!< If 1, don't save the registry after it was created
  } bg_plugin_registry_options_t;

/** \ingroup plugin_registry
 *  \brief Opaque handle for a plugin registry
 *
 *  You don't want to know, what's inside here.
 */

typedef struct bg_plugin_registry_s bg_plugin_registry_t;

/** \ingroup plugin_registry
 *  \brief Typedef for plugin handle
 */

typedef struct bg_plugin_handle_s bg_plugin_handle_t;

/** \ingroup plugin_registry
 *  \brief Handle of a loaded plugin
 *
 *  When you load a plugin, the shared module will be loaded. Then, the
 *  create method of the plugin is called. The pointer obtained from the create method
 *  is stored in the priv member of the returned handle.
 */

struct bg_plugin_handle_s
  {
  /* Private members, should not be accessed! */
    
  void * dll_handle; //!< dll_handle (don't touch)
  pthread_mutex_t mutex; //!< dll_handle (don't touch, use \ref bg_plugin_lock and \ref bg_plugin_unlock)
  int refcount;          //!< Reference counter (don't touch, use \ref bg_plugin_ref and \ref bg_plugin_unref)
  bg_plugin_registry_t * plugin_reg; //!< The plugin registry, from which the plugin was loaded
  
  /* These are for use by applications */
  
  const bg_plugin_common_t * plugin; //!< Common structure, cast this to the derived type (e.g. \ref bg_input_plugin_t).
  bg_plugin_common_t * plugin_nc; //!< Used for dynamic allocation. Never touch this.
  const bg_plugin_info_t * info; //!< Info about this plugin
  void * priv; //!< Private handle, passed as the first argument to most plugin functions

  char * location; //!< Applications can save the argument of an open call here
  bg_edl_t * edl; //!< EDL
  };

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
 * \brief Create a plugin registry with options
 * \param section A configuration section
 * \param opt The options structure
 *
 *  The configuration section will be owned exclusively
 *  by the plugin registry, applications should not touch it.
 */

bg_plugin_registry_t *
bg_plugin_registry_create_with_options(bg_cfg_section_t * section,
                                       const bg_plugin_registry_options_t * opt);



/** \ingroup plugin_registry
 * \brief Scan for pluggable devices
 * \param plugin_reg A plugin registry
 * \param type_mask Mask of all types you want to have the devices scanned for
 * \param flag_mask Mask of all flags you want to have the devices scanned for
 *
 *  Some plugins offer a list of supported devices as parameters. To update these
 *  (e.g. if pluggable devices are among them), call this function right after you
 *  created the plugin registry
 */

void bg_plugin_registry_scan_devices(bg_plugin_registry_t * plugin_reg,
                                     uint32_t type_mask, uint32_t flag_mask);


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
 *  \brief Find a plugin by the compression ID
 *  \param reg A plugin registry
 *  \param typemask Mask of plugin types to be returned
 *  \param flagmask Mask of plugin flags to be returned
 *  \returns A plugin info or NULL
 */

const bg_plugin_info_t *
bg_plugin_find_by_compression(bg_plugin_registry_t * reg,
                              gavl_codec_id_t id,
                              int typemask, int flagmask);


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
 *  GUI menus. Use \ref bg_plugin_find_by_name to get
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
 *  \param callbacks Input callbacks (only for authentication)
 *  \param prefer_edl If 1 EDLs are preferred over raw streams
 *  \returns 1 on success, 0 on error.
 *
 *  This is a convenience function to load an input file. If info is
 *  NULL, the plugin will be autodetected. The handle is stored in ret.
 *  If ret is non-null before the call, the old plugin will be unrefed.
 */

int bg_input_plugin_load(bg_plugin_registry_t * reg,
                         const char * location,
                         const bg_plugin_info_t * info,
                         bg_plugin_handle_t ** ret,
                         bg_input_callbacks_t * callbacks, int prefer_edl);

/** \ingroup plugin_registry
 *  \brief Load and open an edl decoder
 *  \param reg A plugin registry
 *  \param edl The edl to open
 *  \param info Plugin to use (can be NULL for autodetection)
 *  \param ret Will return the plugin handle.
 *  \param callbacks Input callbacks (only for authentication)
 *  \returns 1 on success, 0 on error.
 *
 *  This is a convenience function to load an input file. If info is
 *  NULL, the plugin will be autodetected. The handle is stored in ret.
 *  If ret is non-null before the call, the old plugin will be unrefed.
 */

int bg_input_plugin_load_edl(bg_plugin_registry_t * reg,
                             const bg_edl_t * edl,
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

/** \ingroup plugin_registry
 *  \brief Set a parameter info for selecting and configuring input plugins
 *  \param reg A plugin registry
 *  \param type_mask Mask of all returned types
 *  \param flag_mask Mask of all returned flags
 *  \param ret Where the parameter info will be copied
 *
 */

void bg_plugin_registry_set_parameter_info_input(bg_plugin_registry_t * reg,
                                                 uint32_t type_mask,
                                                 uint32_t flag_mask,
                                                 bg_parameter_info_t * ret);


/** \ingroup plugin_registry
 *  \brief Set a parameter of an input plugin
 *  \param data A plugin registry cast to void
 *  \param name Name
 *  \param val Value
 *
 */

void bg_plugin_registry_set_parameter_input(void * data, const char * name,
                                            const bg_parameter_value_t * val);

int bg_plugin_registry_get_parameter_input(void * data, const char * name,
                                            bg_parameter_value_t * val);


/** \ingroup plugin_registry
 *  \brief Create a parameter array for encoders
 *  \param reg       A plugin registry
 *  \param stream_type_mask Mask of all stream types to be encoded
 *  \param flag_mask Mask of all returned plugin flags
 *  \returns Parameter array for setting up encoders
 *
 *  Free the returned parameters with
 *  \ref bg_parameter_info_destroy_array
 *
 *  If you create a config section from the returned parameters
 *  (with \ref bg_cfg_section_create_from_parameters or \ref bg_cfg_section_create_items)
 *  the resulting encoding section will contain the complete encoder setup.
 *  It can be manipulated through the bg_encoder_section_*() functions.
 */

bg_parameter_info_t *
bg_plugin_registry_create_encoder_parameters(bg_plugin_registry_t * reg,
                                             uint32_t stream_type_mask,
                                             uint32_t flag_mask);

/** \ingroup plugin_registry
 *  \brief Get the name for an encoding plugin
 *  \param plugin_reg A plugin registry
 *  \param s An encoder section (see \ref bg_plugin_registry_create_encoder_parameters)
 *  \param stream_type The stream type to encode
 *  \param stream_mask The mask passed to \ref bg_plugin_registry_create_encoder_parameters
 *  \returns Returns the plugin name or NULL if the stream will be encoded by the video encoder
 */

const char * 
bg_encoder_section_get_plugin(bg_plugin_registry_t * plugin_reg,
                              bg_cfg_section_t * s,
                              bg_stream_type_t stream_type,
                              int stream_mask);

/** \ingroup plugin_registry
 *  \brief Get the plugin configuration for an encoding plugin
 *  \param plugin_reg A plugin registry
 *  \param s An encoder section (see \ref bg_plugin_registry_create_encoder_parameters)
 *  \param stream_type The stream type to encode
 *  \param stream_mask The mask passed to \ref bg_plugin_registry_create_encoder_parameters
 *  \param section_ret If non-null returns the config section for the plugin
 *  \param params_ret If non-null returns the parameters for the plugin
 */

  
void
bg_encoder_section_get_plugin_config(bg_plugin_registry_t * plugin_reg,
                                     bg_cfg_section_t * s,
                                     bg_stream_type_t stream_type,
                                     int stream_mask,
                                     bg_cfg_section_t ** section_ret,
                                     const bg_parameter_info_t ** params_ret);

/** \ingroup plugin_registry
 *  \brief Get the stream configuration for an encoding plugin
 *  \param plugin_reg A plugin registry
 *  \param s An encoder section (see \ref bg_plugin_registry_create_encoder_parameters)
 *  \param stream_type The stream type to encode
 *  \param stream_mask The mask passed to \ref bg_plugin_registry_create_encoder_parameters
 *  \param section_ret If non-null returns the config section for the stream
 *  \param params_ret If non-null returns the parameters for the stream
 */

void
bg_encoder_section_get_stream_config(bg_plugin_registry_t * plugin_reg,
                                     bg_cfg_section_t * s,
                                     bg_stream_type_t stream_type,
                                     int stream_mask,
                                     bg_cfg_section_t ** section_ret,
                                     const bg_parameter_info_t ** params_ret);


/** \ingroup plugin_registry
 *  \brief Get an encoder configuration section from a registry
 *  \param plugin_reg A plugin registry
 *  \param parameters Parameters returned by \ref bg_plugin_registry_create_encoder_parameters
 *  \param type_mask The stream mask passed to \ref bg_plugin_registry_create_encoder_parameters
 *  \param flag_mask The mask passed to \ref bg_plugin_registry_create_encoder_parameters
 *  \returns The encoder section with the values from the registry
*/

bg_cfg_section_t *
bg_encoder_section_get_from_registry(bg_plugin_registry_t * plugin_reg,
                                     const bg_parameter_info_t * parameters,
                                     uint32_t type_mask,
                                     uint32_t flag_mask);

/** \ingroup plugin_registry
 *  \brief Store an encoder configuration in a registry
 *  \param plugin_reg A plugin registry
 *  \param s The encoder section to store
 *  \param parameters Parameters returned by \ref bg_plugin_registry_create_encoder_parameters
 *  \param type_mask The stream mask passed to \ref bg_plugin_registry_create_encoder_parameters
 *  \param flag_mask The mask passed to \ref bg_plugin_registry_create_encoder_parameters
 *  \returns The encoder section with the values from the registry
 */

void
bg_encoder_section_store_in_registry(bg_plugin_registry_t * plugin_reg,
                                     bg_cfg_section_t * s,
                                     const bg_parameter_info_t * parameters,
                                     uint32_t type_mask,
                                     uint32_t flag_mask);


/** \ingroup plugin_registry_defaults 
 *  \brief Set the default for a particular plugin type
 *  \param reg A plugin registry
 *  \param type The type for which the default will be set
 *  \param flag_mask A mask of plugin flags
 *  \param plugin_name Short name of the plugin
 *
 *  Default plugins are stored for various types (recorders, output and
 *  encoders). The default will be stored in the config section.
 */

void bg_plugin_registry_set_default(bg_plugin_registry_t * reg,
                                    bg_plugin_type_t type, uint32_t flag_mask,
                                    const char * plugin_name);

/** \ingroup plugin_registry_defaults
 *  \brief Set the default for a particular plugin type
 *  \param reg A plugin registry
 *  \param type The plugin type
 *  \param flag_mask A mask of plugin flags
 *  \returns A plugin info or NULL
 *
 *  Note, that the registry does not store a default input plugin.
 */
const bg_plugin_info_t * bg_plugin_registry_get_default(bg_plugin_registry_t * reg,
                                                        bg_plugin_type_t type, uint32_t flag_mask);


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

/** \ingroup plugin_registry_defaults
 *  \brief Specify whether visualizations should be enabled
 *  \param reg A plugin registry
 *  \param enable 1 to enable visualizations, 0 else
 */

void bg_plugin_registry_set_visualize(bg_plugin_registry_t * reg,
                                       int enable);

/** \ingroup plugin_registry_defaults
 *  \brief Query whether visualizations should be enabled
 *  \param reg A plugin registry
 *  \returns 1 if visualizations should be enabled, 0 else
 */

int bg_plugin_registry_get_visualize(bg_plugin_registry_t * reg);


/** \ingroup plugin_registry
 *  \brief Add a device to a plugin
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
 *  \param m Returns metadata
 *  \returns The frame, which contains the image
 *
 *  Use gavl_video_frame_destroy to free the
 *  return value
 */

gavl_video_frame_t * bg_plugin_registry_load_image(bg_plugin_registry_t * reg,
                                                   const char * filename,
                                                   gavl_video_format_t * format,
                                                   gavl_metadata_t * m);

/* Same as above for writing. Does implicit pixelformat conversion */

/** \ingroup plugin_registry
 *  \brief Save an image
 *  \param reg A plugin registry
 *  \param filename Image filename
 *  \param frame The frame, which contains the image
 *  \param format Returns format of the image
 *  \param m Metadata
 */

void
bg_plugin_registry_save_image(bg_plugin_registry_t * reg,
                              const char * filename,
                              gavl_video_frame_t * frame,
                              const gavl_video_format_t * format,
                              const gavl_metadata_t * m);


/** \brief Get thumbnail of a movie
 *  \param gml Location (should be a regular file)
 *  \param thumbnail_file If non-null, returns the filename of the thumbnail
 *  \param plugin_reg Plugin registry
 *  \param frame_ret If non-null, returns the video frame
 *  \param format_ret If non-null, the video format of the thumbnail will be
                      copied here
 *  \returns 1 if a unique thumbnail could be generated, 0 if a default thumbnail was returned
 *
 */

int bg_get_thumbnail(const char * gml,
                     bg_plugin_registry_t * plugin_reg,
                     char ** thumbnail_filename_ret,
                     gavl_video_frame_t ** frame_ret,
                     gavl_video_format_t * format_ret);


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
 *  \brief Load a video output plugin
 *  \param reg A plugin registry
 *  \param info The plugin info
 *  \param window_id The window ID or NULL
 *
 *  Load a video output plugin for embedding into an already existing window
 *  and return handle with reference count of 1
 */

bg_plugin_handle_t * bg_ov_plugin_load(bg_plugin_registry_t * reg,
                                       const bg_plugin_info_t * info,
                                       const char * window_id);

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

/** \ingroup plugin_registry
 *  \brief Decrease the reference count without locking
 *  \param h A plugin handle
 *
 *  Use this *only* if you know for sure, that the plugin is
 *  already locked and no other thread waits for the plugin to
 *  be unlocked.
 *  If the reference count gets zero, the plugin will
 *  be destroyed
 */

void bg_plugin_unref_nolock(bg_plugin_handle_t * h);

/** \ingroup plugin_registry
 *  \brief Create a plugin info from a plugin
 *  \param plugin A plugin
 *  \returns A newly allocated plugin info
 *
 *  This is used by internal plugins only.
 */

bg_plugin_info_t * bg_plugin_info_create(const bg_plugin_common_t * plugin);

/** \ingroup plugin_registry
 *  \brief Create an empty plugin handle
 *  \returns A newly allocated plugin handle
 *
 *  Use this function only if you create a plugin handle outside a plugin
 *  registry. Free the returned info with \ref bg_plugin_unref
 */

bg_plugin_handle_t * bg_plugin_handle_create();


#endif // __BG_PLUGINREGISTRY_H_
