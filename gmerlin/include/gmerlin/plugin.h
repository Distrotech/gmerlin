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

#ifndef __BG_PLUGIN_H_
#define __BG_PLUGIN_H_

#include <gavl/gavl.h>
#include <gmerlin/parameter.h>
#include <gmerlin/streaminfo.h>
#include <gmerlin/accelerator.h>
#include <gmerlin/edl.h>

/** \defgroup plugin Plugins
 *  \brief Plugin types and associated functions
 *
 *  Gmerlin plugins are structs which contain function pointers and
 *  other data. The API looks a bit complicated, but many functions are
 *  optional, so plugins can, in prinpiple, be very simple.
 *  All plugins are based on a common struct (\ref bg_plugin_common_t),
 *  which contains an identifier for the plugin type. The bg_plugin_common_t
 *  pointer can be casted to the derived plugin types.
 *
 *  The application calls the functions in the order, in which they are
 *  defined. Some functions are mandatory from the plugin view (i.e. they
 *  must be non-null), some functions are mandatory for the application
 *  (i.e. the application must check for them and call them if they are
 *  present.
 *
 *  The configuration of the plugins works entirely through the
 *  parameter passing mechanisms (see \ref parameter). Configurable
 *  plugins only need to define get_parameters and set_parameter methods.
 *  Encoding plugins have an additional layer, which allows setting
 *  parameters individually for each stream.
 *  
 *  Events, which are caught by the plugins (e.g. song name changes or
 *  mouse clicks) are propagated through optional callbacks.
 *  These are passed from the application to the plugin with the
 *  set_callbacks function.
 *  Applications should not rely on any callback to be actually supported by
 *  a plugin. Plugins should not rely on the presence of any callback
 */


/** \ingroup plugin
 *  \brief Generic prototype for reading audio
 *  \param priv Private data
 *  \param frame Audio frame
 *  \param stream Stream index (0 is only one stream)
 *  \param samples Samples to read
 *  \returns The number of samples read
 *
 *  This is a generic prototype for reading audio. It's shared between
 *  input pluigns, recorders and filters to enable arbitrary chaining.
 */

typedef int (*bg_read_audio_func_t)(void * priv, gavl_audio_frame_t* frame, int stream,
                                    int num_samples);

/** \ingroup plugin
 *  \brief Generic prototype for reading video
 *  \param priv Private data
 *  \param frame Video frame
 *  \param stream Stream index (0 is only one stream)
 *  \returns 1 if a frame could be read, 0 else
 *
 *  This is a generic prototype for reading audio. It's shared between
 *  input pluigns, recorders and filters to enable arbitrary chaining.
 */

typedef int (*bg_read_video_func_t)(void * priv, gavl_video_frame_t* frame, int stream);

/** \defgroup plugin_flags Plugin flags
 *  \ingroup plugin
 *  \brief Macros for the plugin flags
 *
 *
 *  All plugins must have at least one flag set.
 *  @{
 */

#define BG_PLUGIN_REMOVABLE        (1<<0)  //!< Plugin handles removable media (CD, DVD etc.)
#define BG_PLUGIN_FILE             (1<<1)  //!< Plugin reads/writes files
#define BG_PLUGIN_RECORDER         (1<<2)  //!< Plugin does hardware recording
#define BG_PLUGIN_URL              (1<<3)  //!< Plugin can load URLs
#define BG_PLUGIN_PLAYBACK         (1<<4)  //!< Plugin is an audio or video driver for playback
#define BG_PLUGIN_STDIN            (1<<8)  //!< Plugin can read from stdin ("-")
#define BG_PLUGIN_TUNER            (1<<9)  //!< Plugin has some kind of tuner. Channels will be loaded as tracks.
#define BG_PLUGIN_FILTER_1        (1<<10)  //!< Plugin acts as a filter with one input
#define BG_PLUGIN_EMBED_WINDOW    (1<<11)  //!< Plugin can embed it's window into another application
#define BG_PLUGIN_VISUALIZE_FRAME (1<<12)  //!< Visualization plugin outputs video frames
#define BG_PLUGIN_VISUALIZE_GL    (1<<13)  //!< Visualization plugin outputs via OpenGL
#define BG_PLUGIN_PP              (1<<14)  //!< Postprocessor
#define BG_PLUGIN_CALLBACKS       (1<<15)  //!< Plugin can be opened from callbacks
#define BG_PLUGIN_BROADCAST       (1<<16)  //!< Plugin can broadcasts (e.g. webstreams)
#define BG_PLUGIN_DEVPARAM        (1<<17)  //!< Plugin has pluggable devices as parameters, which must be updated regurarly

#define BG_PLUGIN_UNSUPPORTED     (1<<24)  //!< Plugin is not supported. Only for a foreign API plugins


#define BG_PLUGIN_ALL 0xFFFFFFFF //!< Mask of all possible plugin flags

/** @}
 */

#define BG_PLUGIN_API_VERSION 23

/* Include this into all plugin modules exactly once
   to let the plugin loader obtain the API version */

#define BG_GET_PLUGIN_API_VERSION \
  int get_plugin_api_version() __attribute__ ((visibility("default"))); \
  int get_plugin_api_version() { return BG_PLUGIN_API_VERSION; }

#define BG_PLUGIN_PRIORITY_MIN 1
#define BG_PLUGIN_PRIORITY_MAX 10

/** \defgroup plugin_i Media input
 *  \ingroup plugin
 *  \brief Media input
 */ 

/** \ingroup plugin_i
 *  \brief Stream actions
 *
 *  These describe how streams should be handled by the input
 *  plugin. Note that by default, each stream is switched off.
 */

typedef enum
  {
    BG_STREAM_ACTION_OFF = 0, //!< Stream is switched off and will be ignored
    BG_STREAM_ACTION_DECODE,  //!< Stream is switched on and will be decoded
    
    /*
     */
    
    /*
     *  Future support for compressed frames
     *  must go here
     */

    /* BG_STREAM_ACTION_READRAW */
    
  } bg_stream_action_t;

/***************************************************
 * Plugin API
 *
 * Plugin dlls contain a symbol "the_plugin",
 * which points to one of the structures below.
 * The member functions are described below.
 *
 ***************************************************/

/*
 * Plugin types
 */

/** \ingroup plugin
 *  \brief Plugin types
 */

typedef enum
  {
    BG_PLUGIN_NONE                       = 0,      //!< None or undefined
    BG_PLUGIN_INPUT                      = (1<<0), //!< Media input
    BG_PLUGIN_OUTPUT_AUDIO               = (1<<1), //!< Audio output
    BG_PLUGIN_OUTPUT_VIDEO               = (1<<2), //!< Video output
    BG_PLUGIN_RECORDER_AUDIO             = (1<<3), //!< Audio recorder
    BG_PLUGIN_RECORDER_VIDEO             = (1<<4), //!< Video recorder
    BG_PLUGIN_ENCODER_AUDIO              = (1<<5), //!< Encoder for audio only
    BG_PLUGIN_ENCODER_VIDEO              = (1<<6), //!< Encoder for video only
    BG_PLUGIN_ENCODER_SUBTITLE_TEXT      = (1<<7), //!< Encoder for text subtitles only
    BG_PLUGIN_ENCODER_SUBTITLE_OVERLAY   = (1<<8), //!< Encoder for overlay subtitles only
    BG_PLUGIN_ENCODER                    = (1<<9), //!< Encoder for multiple kinds of streams
    BG_PLUGIN_ENCODER_PP                 = (1<<10),//!< Encoder postprocessor (e.g. CD burner)
    BG_PLUGIN_IMAGE_READER               = (1<<11),//!< Image reader
    BG_PLUGIN_IMAGE_WRITER               = (1<<12), //!< Image writer
    BG_PLUGIN_FILTER_AUDIO               = (1<<13), //!< Audio filter
    BG_PLUGIN_FILTER_VIDEO               = (1<<14), //!< Video filter
    BG_PLUGIN_VISUALIZATION              = (1<<15), //!< Visualization
    BG_PLUGIN_AV_RECORDER                = (1<<16),  //!< Audio/Video recorder
  } bg_plugin_type_t;

/** \ingroup plugin
 *  \brief Device description
 *
 *  The find_devices() function of a plugin returns
 *  a NULL terminated array of devices. It's used mainly for input plugins,
 *  which access multiple drives. For output plugins, devices are normal parameters.
 */

typedef struct
  {
  char * device; //!< String, which can be passed to the open() method
  char * name;   //!< More humanized description, might be NULL
  } bg_device_info_t;

/** \ingroup plugin
 *  \brief Append device info to an existing array and return the new array.
 *  \param arr An array (can be NULL)
 *  \param device Device string
 *  \param name Humanized description (can be NULL)
 *  \returns Newly allocated array. The source array is freed.
 *
 *  This is used mainly by the device detection routines of the plugins
 */

bg_device_info_t * bg_device_info_append(bg_device_info_t * arr,
                                         const char * device,
                                         const char * name);

/** \ingroup plugin
 *  \brief Free an array of device descriptions
 *  \param arr a device array
 */

void bg_device_info_destroy(bg_device_info_t * arr);

/* Common part */

/** \ingroup plugin
 *  \brief Typedef for base structure common to all plugins
 */

typedef struct bg_plugin_common_s bg_plugin_common_t;

/** \ingroup plugin
 *  \brief Base structure common to all plugins
 */

struct bg_plugin_common_s
  {
  char * gettext_domain; //!< First argument for bindtextdomain().
  char * gettext_directory; //!< Second argument for bindtextdomain().
  
  char             * name;       //!< Unique short name
  char             * long_name;  //!< Humanized name for GUI widgets
  bg_plugin_type_t type;  //!< Type
  int              flags;  //!< Flags (see defines)
  
  char             * description; //!< Textual description 
  
  /*
   *  If there might be more than one plugin for the same
   *  job, there is a priority (0..10) which is used for the
   *  decision
   */
  
  int              priority; //!< Priority (between 1 and 10).
  
  /** \brief Create the instance, return handle.
   *  \returns A private handle, which is the first argument to all subsequent functions.
   */
  
  void * (*create)();
      
  /** \brief Destroy plugin instance
   *  \param priv The handle returned by the create() method
   *
   * Destroy everything, making it ready for dlclose()
   * This function might also be called on opened plugins,
   * so the plugins should call their close()-function from
   * within the destroy method.
   */

  void (*destroy)(void* priv);

  /** \brief Get available parameters
   *  \param priv The handle returned by the create() method
   *  \returns a NULL terminated parameter array.
   *
   *  The returned array is owned (an should be freed) by the plugin.
   */

  const bg_parameter_info_t * (*get_parameters)(void * priv);

  /** \brief Set configuration parameter (optional)
   */
    
  bg_set_parameter_func_t set_parameter;

  /** \brief Get configuration parameter (optional)
   *
   *  This must only return parameters, which are changed internally
   *  by the plugins.
   */
  
  bg_get_parameter_func_t get_parameter;
  
  /** \brief Check, if a device can be opened by the plugin (optional)
   *  \param device The device as passed to the open() method
   *  \param name Returns the name if available
   *  \returns 1 if the device is supported, 0 else
   *
   *  The name should be set to NULL before this call, and must be freed
   *  if it's non-NULL after the call.
   */
  
  int (*check_device)(const char * device, char ** name);
  

  /** \brief Get an array of all supported devices found on the system
   *  \returns A NULL terminated device array
   *
   *  The returned array must be freed with \ref bg_device_info_destroy by
   *  the caller.
   */
  
  bg_device_info_t * (*find_devices)();
    
  };

/*
 *  Plugin callbacks: Functions called by the
 *  plugin to reflect user input or other changes
 *  Applications might pass NULL callbacks,
 *  so plugins MUST check for valid callbacks structure
 *  before calling any of these functions
 */

/* Input plugin */

/** \ingroup plugin_i
 *  \brief typedef for input callbacks
 *
 */

typedef struct bg_input_callbacks_s bg_input_callbacks_t;

/** \ingroup plugin_i
 *  \brief Callbacks for input plugins
 *
 *  Passing the callback structure to the plugin is optional. Futhermore,
 *  any of the callback functions is optional (i.e. can be NULL). The plugin
 *  might use the callbacks for propagating events.
 */

struct bg_input_callbacks_s
  {
  /** \brief Duration changed
   *  \param data The data member of this bg_input_callbacks_s struct
   *  \param time The new duration
   */
  
  void (*duration_changed)(void * data, gavl_time_t duration);

  /** \brief Name changed
   *  \param data The data member of this bg_input_callbacks_s struct
   *  \param time The new name
   *
   *  This is for web-radio stations, which send song-names.
   */
  
  void (*name_changed)(void * data, const char * name);

  /** \brief Metadata changed
   *  \param data The data member of this bg_input_callbacks_s struct
   *  \param m The new metadata
   *
   *  This is for web-radio stations, which send metadata for each song.
   */
  
  void (*metadata_changed)(void * data, const bg_metadata_t * m);

  /** \brief Buffer callback
   *  \param data The data member of this bg_input_callbacks_s struct
   *  \param percentage The buffer fullness (0.0..1.0)
   *
   *  For network connections, plugins might buffer data. This takes some time,
   *  so they notify the API about the progress with this callback.
   */
  
  void (*buffer_notify)(void * data, float percentage);

  /** \brief Authentication callback
   *  \param data The data member of this bg_input_callbacks_s struct
   *  \param resource Name of the resource (e.g. server name)
   *  \param username Returns the username 
   *  \param password Returns the password
   *  \returns 1 if username and password are available, 0 else.
   *
   *  For sources, which require authentication (like FTP servers), this
   *  function is called by the plugin to get username and password.
   *  Username and password must be returned in newly allocated strings, which
   *  will be freed by the plugin.
   */

  int (*user_pass)(void * data, const char * resource,
                   char ** username, char ** password);

  /** \brief Aspect ratio change callback
   *  \param data The data member of this bg_input_callbacks_s struct
   *  \param stream Video stream index (starts with 0)
   *  \param pixel_width  New pixel width
   *  \param pixel_height New pixel height
   *
   *  Some streams (e.g. DVB) change the pixel aspect ratio on the fly.
   *  If the input plugin detects such a change, it should call this
   *  callback.
   */
  
  void (*aspect_changed)(void * data, int stream,
                         int pixel_width, int pixel_height);
  
  
  void * data; //!< Application specific data passed as the first argument to all callbacks.
  
  };

/*************************************************
 * MEDIA INPUT
 *************************************************/

/** \ingroup plugin_i
 *  \brief Typedef for input plugin
 */

typedef struct bg_input_plugin_s bg_input_plugin_t;


/** \ingroup plugin_i
 *  \brief Input plugin
 *
 *  This is for all kinds of media inputs (files, disks, urls, etc), except recording from
 *  hardware devices (see \ref plugin_r).
 *
 *
 */

struct bg_input_plugin_s
  {
  bg_plugin_common_t common; //!< Infos and functions common to all plugin types

  /** \brief Get supported protocols
   *  \param priv The handle returned by the create() method
   *  \returns A space separated list of protocols
   */
  
  const char * (*get_protocols)(void * priv);
  /** \brief Get supported mimetypes
   *  \param priv The handle returned by the create() method
   *  \returns A space separated list of mimetypes
   */
  const char * (*get_mimetypes)(void * priv);

  /** \brief Get supported extensions
   *  \param priv The handle returned by the create() method
   *  \returns A space separated list of extensions
   */
  const char * (*get_extensions)(void * priv);
  
  /** \brief Set callbacks
   *  \param priv The handle returned by the create() method
   *  \param callbacks Callback structure initialized by the caller before
   *
   * Set callback functions, which will be called by the plugin.
   * Defining as well as calling this function is optional. Any of the
   * members of callbacks can be NULL.
   */
  
  void (*set_callbacks)(void * priv, bg_input_callbacks_t * callbacks);
  
  /** \brief Open file/url/device
   *  \param priv The handle returned by the create() method
   *  \param arg Filename, URL or device name
   *  \returns 1 on success, 0 on failure
   */
  int (*open)(void * priv, const char * arg);

  /** \brief Open plugin from filedescriptor (optional)
   *  \param priv The handle returned by the create() method
   *  \param fd Open filedescriptor
   *  \param total_bytes Totally available bytes or 0 if unknown
   *  \param mimetype Mimetype from http header (or NULL)
   *  \returns 1 on success, 0 on failure
   */

  int (*open_fd)(void * priv, int fd, int64_t total_bytes,
                 const char * mimetype);

  /** \brief Open plugin with callbacks (optional)
   *  \param priv The handle returned by the create() method
   *  \param read_callback Callback for reading data
   *  \param seek_callback Callback for seeking
   *  \param cb_priv Private argument for the callbacks
   *  \param filename The filename of the input or NULL if this info is not known.
   *  \param mimetype The mimetype of the input or NULL if this info is not known.
   *  \param total_bytes total number of bytes or 0 if this info is not known.
   *  \returns 1 on success, 0 on failure
   */

  int (*open_callbacks)(void * priv,
                        int (*read_callback)(void * priv, uint8_t * data, int len),
                        int64_t (*seek_callback)(void * priv, uint64_t pos, int whence),
                        void * cb_priv, const char * filename, const char * mimetype,
                        int64_t total_bytes);
  
  /** \brief Get the edl (optional)
   *  \param priv The handle returned by the create() method
   *  \returns The edl if any
   */

  const bg_edl_t * (*get_edl)(void * priv);
    
  /** \brief Get the disc name (optional)
   *  \param priv The handle returned by the create() method
   *  \returns The name of the disc if any
   *
   *  This is only for plugins, which access removable discs (e.g. CDs).
   */
  
  const char * (*get_disc_name)(void * priv);

  /** \brief Eject disc (optional)
   *  \param priv The handle returned by the create() method
   *  \returns 1 if eject was successful
   *
   *  This is only for plugins, which access removable discs (e.g. CDs).
   *  \todo This function doesn't work reliably now. Either fix or remove this
   */
  
  int (*eject_disc)(const char * device);
  
  /** \brief Get the number of tracks
   *  \param priv The handle returned by the create() method
   *  \returns The number of tracks
   *
   *  This can be NULL for plugins, which support just one track.
   */
  
  int (*get_num_tracks)(void * priv);
  
  /** \brief Return information about a track
   *  \param priv The handle returned by the create() method
   *  \param track Track index starting with 0
   *  \returns The track info
   *
   *  The following fields MUST be valid after this call:
   *  - num_audio_streams
   *  - num_video_streams
   *  - num_subtitle_streams
   *  - duration
   *  - Name (If NULL, the filename minus the suffix will be used)
   *
   * Other data, especially audio and video formats, will become valid after the
   * start() call (see below).
   */
  
  bg_track_info_t * (*get_track_info)(void * priv, int track);

  /** \brief Set the track to be played
   *  \param priv The handle returned by the create() method
   *  \param track Track index starting with 0
   *
   *  This has to be defined even if the plugin doesn't support
   *  multiple tracks. In addition to selecting the track,
   *  the plugin should also reset it's internal state such
   *  that streams can newly be selected.
   */
  
  int (*set_track)(void * priv, int track);
    
  /*
   *  These functions set the audio- video- and subpicture streams
   *  as well as programs (== DVD Angles). All these start with 0
   *
   *  Arguments for actions are defined in the enum bg_stream_action_t
   *  above. Plugins must return FALSE on failure (e.g. no such stream)
   *
   *  Functions must be defined only, if the corresponding stream
   *  type is supported by the plugin and can be switched.
   *  Single stream plugins can leave these NULL
   *  Gmerlin will never try to call these functions on nonexistent streams
   */

  /** \brief Setup audio stream
   *  \param priv The handle returned by the create() method
   *  \param stream Stream index starting with 0
   *  \param action What to do with the stream
   *  \returns 1 on success, 0 on failure
   */
  
  int (*set_audio_stream)(void * priv, int stream, bg_stream_action_t action);

  /** \brief Setup video stream
   *  \param priv The handle returned by the create() method
   *  \param stream Stream index starting with 0
   *  \param action What to do with the stream
   *  \returns 1 on success, 0 on failure
   */

  int (*set_video_stream)(void * priv, int stream, bg_stream_action_t action);
  
  /** \brief Setup subtitle stream
   *  \param priv The handle returned by the create() method
   *  \param stream Stream index starting with 0
   *  \param action What to do with the stream
   *  \returns 1 on success, 0 on failure
   */

  int (*set_subtitle_stream)(void * priv, int stream, bg_stream_action_t action);
  
  /** \brief Start decoding
   *  \param priv The handle returned by the create() method
   *  \returns 1 on success, 0 on error
   *   
   *  After this call, all remaining members of the track info returned earlier
   *  (especially audio- and video formats) must be valid.
   *
   *  From the plugins point of view, this is the last chance to return 0
   *  if something fails
   */
  
  int (*start)(void * priv);

  /** \brief Get frame table
   *  \param priv The handle returned by the create() method
   *  \param stream Stream index (starting with 0)
   *  \returns A newly allocated frame table or NULL
   *
   *  The returned frame table must be freed with
   *  \ref gavL_frame_table_destroy.
   */

  gavl_frame_table_t * (*get_frame_table)(void * priv, int stream);
    
  /** \brief Read audio samples
   *  \param priv The handle returned by the create() method
   *  \param frame The frame, where the samples will be copied
   *  \param stream Stream index starting with 0
   *  \param num_samples Number of samples
   *  \returns The number of decoded samples, 0 means EOF.
   *
   *  The num_samples argument can be larger than the samples_per_frame member of the
   *  video format. This means, that all audio decoding plugins must have an internal
   *  buffering mechanism.
   */

  bg_read_audio_func_t read_audio;

  /** \brief Check is a still image is available
   *  \param priv The handle returned by the create() method
   *  \param stream Stream index starting with 0
   *  \returns 1 if a still frame can be decoded, 0 else.
   *
   *  If EOF occurs in still streams, this function will return
   *  1, but the subsequent call to read_video returns 0
   */

  int (*has_still)(void * priv, int stream);
  
  /** \brief Read a video frame
   *  \param priv The handle returned by the create() method
   *  \param frame The frame, where the image will be copied
   *  \param stream Stream index starting with 0
   *  \returns 1 if a frame was decoded, 0 means EOF.
   */
  
  bg_read_video_func_t read_video;

  /** \brief Skip frames in a video stream
      \param stream Stream index (starting with 0)
      \param time The time to skip to (will be changed to the true time)
      \param scale Scale by which the time is scaled
      \param exact 1 if an exact skip should be done, 0 for faster approximate skip
      
      Use this function if it turns out, that the machine is too weak to
      decode all frames. Set exact to 0 to make the skipping even faster
      but less accurate.

  */

  void (*skip_video)(void * priv, int stream, int64_t * time, int scale, int exact);
    
  /** \brief Query if a new subtitle is available
   *  \param priv The handle returned by the create() method
   *  \param stream Stream index starting with 0
   *  \returns 1 if a subtitle is available, 0 else.
   */
  
  int (*has_subtitle)(void * priv, int stream);
    
  /** \brief Read one pixmap subtitle
   *  \param priv The handle returned by the create() method
   *  \param ovl Where the overlay will be copied
   *  \param stream Stream index starting with 0
   *  \returns 1 if a subtitle was decoded, 0 else
   *
   *  EOF in a graphical subtitle stream is reached if
   *  - has_subtitle() returned 1 and
   *  - read_subtitle_overlay() returned 0
   */
  
  int (*read_subtitle_overlay)(void * priv,
                               gavl_overlay_t*ovl, int stream);

  /** \brief Read one text subtitle
   *  \param priv The handle returned by the create() method
   *  \param text Where the text will be copied, the buffer will be realloc()ed.
   *  \param text_alloc Allocated bytes for text. Will be updated by the function.
   *  \param start_time Returns the start time of the subtitle
   *  \param duration Returns the duration of the subtitle
   *  \param stream Stream index starting with 0
   *  \returns 1 if a subtitle was decoded, 0 else
   *
   *  EOF in a text subtitle stream is reached if
   *  - has_subtitle() returned 1 and
   *  - read_subtitle_text() returned 0
   *
   *  This function automatically handles the text buffer (and text_alloc).
   *  Just set both to zero before the first call and free() the text buffer
   *  after the last call (if non-NULL).
   */
  
  int (*read_subtitle_text)(void * priv,
                            char ** text, int * text_alloc,
                            int64_t * start_time,
                            int64_t * duration, int stream);
  
  /** \brief Seek within a media track
   *  \param priv The handle returned by the create() method
   *  \param time Time to seek to
   *  \param scale Timescale
   *  
   *  Media streams are supposed to be seekable, if this
   *  function is non-NULL AND the duration field of the track info
   *  is > 0 AND the seekable flag in the track info is nonzero.
   *  The time argument might be changed to the correct value
   */
  
  void (*seek)(void * priv, int64_t * time, int scale);
  
  /** \brief Stop playback
   *  \param priv The handle returned by the create() method
   *  
   * This is used for plugins in bypass mode to stop playback.
   * The plugin can be started again after
   */

  void (*stop)(void * priv);
  
  /** \brief Close plugin
   *  \param priv The handle returned by the create() method
   *
   *  Close the file/device/url.
   */

  void (*close)(void * priv);
  
  };

/** \defgroup plugin_oa Audio output
 *  \ingroup plugin
 *  \brief Audio output
 */ 

/** \ingroup plugin_oa
 *  \brief typedef for audio output callbacks
 *
 */

typedef struct bg_oa_callbacks_s bg_oa_callbacks_t;

/** \ingroup plugin_oa
 *  \brief Callbacks for audio output plugins
 *
 */

struct bg_oa_callbacks_s
  {
  bg_read_audio_func_t read_audio;
  };

/** \ingroup plugin_oa
 *  \brief Typedef for audio output plugin
 */

typedef struct bg_oa_plugin_s bg_oa_plugin_t;

/** \ingroup plugin_oa
 *  \brief Audio output plugin
 *
 *  This plugin type implements audio playback through a soundcard.
 */

struct bg_oa_plugin_s
  {
  bg_plugin_common_t common; //!< Infos and functions common to all plugin types

  /** \brief Open plugin
   *  \param priv The handle returned by the create() method
   *  \param format The format of the media source
   *
   *  The format will be changed to the nearest format, which is supported
   *  by the plugin. To convert the source format to the output format,
   *  use a \ref gavl_audio_converter_t
   */

  int (*open)(void * priv, gavl_audio_format_t* format);

  /** \brief Start playback
   *  \param priv The handle returned by the create() method
   *
   *  Notify the plugin, that audio playback is about to begin.
   */

  int (*start)(void * priv);
    
  /** \brief Write audio samples
   *  \param priv The handle returned by the create() method
   *  \param frame The audio frame to write.
   */
  
  void (*write_audio)(void * priv, gavl_audio_frame_t* frame);

  /** \brief Get the number of buffered audio samples
   *  \param priv The handle returned by the create() method
   *  \returns The number of buffered samples (both soft- and hardware)
   *  
   *  This function is used for A/V synchronization with the soundcard. If this
   *  function is NULL, software synchronization will be used
   */

  int (*get_delay)(void * priv);
  
  /** \brief Stop playback
   *  \param priv The handle returned by the create() method
   *
   * Notify the plugin, that playback will stop. Playback can be starzed again with
   * start().
   */

  void (*stop)(void * priv);
    
  /** \brief Close plugin
   *  \param priv The handle returned by the create() method
   *
   * Close the plugin. After this call, the plugin can be opened with another format
   */
  
  void (*close)(void * priv);
  };

/*******************************************
 * AUDIO RECORDER
 *******************************************/

/** \defgroup plugin_r Recorder
 *  \ingroup plugin
 *  \brief Recorder
 */ 

/** \ingroup plugin_r
 *  \brief Typedef for recorder
 */

typedef struct bg_recorder_plugin_s bg_recorder_plugin_t;

/** \ingroup plugin_ra
 *  \brief Recorder
 *
 *  Recording support from hardware devices
 */

struct bg_recorder_plugin_s
  {
  bg_plugin_common_t common; //!< Infos and functions common to all plugin types

  /** \brief Open plugin
   *  \param priv The handle returned by the create() method
   *  \param format The desired format
   *
   *  The format will be changed to the nearest format, which is supported
   *  by the plugin. To convert the source format to the output format,
   *  use a \ref gavl_audio_converter_t
   */
  
  int (*open)(void * priv, gavl_audio_format_t * audio_format, gavl_video_format_t * video_format);
  
  /** \brief Read audio samples
   */
  bg_read_audio_func_t read_audio;

  /** \brief Read video frame
   */
  bg_read_video_func_t read_video;
  
  /** \brief Close plugin
   *  \param priv The handle returned by the create() method
   */
  
  void (*close)(void * priv);
  };

/*******************************************
 * VIDEO OUTPUT
 *******************************************/

/* Callbacks */

/** \defgroup plugin_ov Video output
 *  \ingroup plugin
 *  \brief Video output
 */ 

/** \ingroup plugin_ov
 * \brief Typedef for callbacks for the video output plugin
 *
 */

typedef struct bg_ov_callbacks_s bg_ov_callbacks_t;

/** \ingroup plugin_ov
 * \brief Callbacks for the video output plugin
 *
 */

struct bg_ov_callbacks_s
  {
  /** \brief Accelerator map
   *
   *  These contain accelerator keys, which get reported
   *  through the accel_callback
   */

  const bg_accelerator_map_t * accel_map;
  
  /** \brief Keyboard callback
   *  \param data The data member of this bg_ov_callbacks_s struct
   *  \param id The accelerator ID
   */
  
  int (*accel_callback)(void * data, int id);
  
  /** \brief Keyboard callback
   *  \param data The data member of this bg_ov_callbacks_s struct
   *  \param key Key code (see \ref keycodes) 
   *  \param mask Modifier mask (see \ref keycodes)
   *  \returns 1 if the event should further be processed, 0 else
   *
   *  Although key_callback and accel_callback can be used at the same
   *  time, accelerator_callback is preferred, because it allows registering
   *  keyboard shortcuts in advance. This makes things more reliable, if
   *  different modules (e.g. embedded visualization plugins) also want to
   *  receive keybords events·
   */
  
  int (*key_callback)(void * data, int key, int mask);

  /** \brief Keyboard release callback
   *  \param data The data member of this bg_ov_callbacks_s struct
   *  \param key Key code (see \ref keycodes) 
   *  \param mask Modifier mask (see \ref keycodes)
   *  \returns 1 if the event should further be processed, 0 else
   */
  
  int (*key_release_callback)(void * data, int key, int mask);
  
  /** \brief Mouse button callback
   *  \param data The data member of this bg_ov_callbacks_s struct
   *  \param x Horizontal cursor position in image coordinates
   *  \param y Vertical cursor position in image coordinates
   *  \param button Number of the mouse button, which was pressed (starting with 1)
   *  \param mask State mask
   *  \returns 1 if the event should further be processed, 0 else
   */
  
  int (*button_callback)(void * data, int x, int y, int button, int mask);

  /** \brief Mouse button release callback
   *  \param data The data member of this bg_ov_callbacks_s struct
   *  \param x Horizontal cursor position in image coordinates
   *  \param y Vertical cursor position in image coordinates
   *  \param button Number of the mouse button, which was pressed (starting with 1)
   *  \param mask State mask
   *  \returns 1 if the event should further be processed, 0 else
   */
  
  int (*button_release_callback)(void * data, int x, int y, int button, int mask);
  
  /** \brief Motion callback
   *  \param data The data member of this bg_ov_callbacks_s struct
   *  \param x Horizontal cursor position in image coordinates
   *  \param y Vertical cursor position in image coordinates
   *  \param mask State mask
   *  \returns 1 if the event should further be processed, 0 else
   */
  
  int (*motion_callback)(void * data, int x, int y, int mask);
  
  /** \brief Show/hide callback
   *  \param data The data member of this bg_ov_callbacks_s struct
   *  \param show 1 if the window is shown now, 0 if it is hidden.
   */
   
  void (*show_window)(void * data, int show);

  /** \brief Brightness change callback
   *  \param data The data member of this bg_ov_callbacks_s struct
   *  \param val New value (0.0..1.0)
   *
   *  This callback can be used to update OSD when the brightness changed.
   */
  
  void (*brightness_callback)(void * data, float val);

  /** \brief Saturation change callback
   *  \param data The data member of this bg_ov_callbacks_s struct
   *  \param val New value (0.0..1.0)
   *
   *  This callback can be used to update OSD when the saturation changed.
   */

  void (*saturation_callback)(void * data, float val);

  /** \brief Contrast change callback
   *  \param data The data member of this bg_ov_callbacks_s struct
   *  \param val New value (0.0..1.0)
   *
   *  This callback can be used to update OSD when the contrast changed.
   */

  void (*contrast_callback)(void * data, float val);

  /** \brief Hue change callback
   *  \param data The data member of this bg_ov_callbacks_s struct
   *  \param val New value (0.0..1.0)
   *
   *  This callback can be used to update OSD when the hue changed.
   */
  
  void (*hue_callback)(void * data, float val);
  
  void * data;//!< Application specific data passed as the first argument to all callbacks.
  };

/* Plugin structure */

/** \ingroup plugin_ov
 * \brief Typedef for video output plugin
 */

typedef struct bg_ov_plugin_s bg_ov_plugin_t;

/** \ingroup plugin_ov
 * \brief Video output plugin
 *
 * This handles video output and still-image display.
 * In a window based system, it will typically open a new window,
 * which is owned by the plugin.
 */

struct bg_ov_plugin_s
  {
  bg_plugin_common_t common; //!< Infos and functions common to all plugin types

  /** \brief Set window
   *  \param priv The handle returned by the create() method
   *  \param window Window identifier
   *
   *  Call this immediately after creation of the plugin to embed 
   *  video output into a foreign application. 
   *  For X11, the window identifier has the form
   *  \<display_name\>:\<normal_id\>:\<fullscreen_id\>. 
   */
  
  void (*set_window)(void * priv, const char * window_id);
  
  /** \brief Get window
   *  \param priv The handle returned by the create() method
   *  \returns Window identifier
   */
  
  const char * (*get_window)(void * priv);
  
  /** \brief Set window class
   *  \param priv The handle returned by the create() method
   *  \param name The name of the window
   *  \param klass The class of the window
   *
   *  This makes sense probably only in an X11 environment.
   *  If the klass argument is the same for all windows of an
   *  application, they, might be grouped together in the
   *  window list. On X11 this results in a call to
   *  XSetClassHint().
   */

  void (*set_window_options)(void * priv, const char * name, 
                             const char * klass, 
                             const gavl_video_frame_t * icon,
                             const gavl_video_format_t * icon_format);

  /** \brief Set window title
   *  \param priv The handle returned by the create() method
   *  \param title The title for the window
   */

  void (*set_window_title)(void * priv, const char * title);
  

  /** \brief Set callbacks
   *  \param priv The handle returned by the create() method
   *  \param callbacks Callback structure initialized by the caller before
   */

  void (*set_callbacks)(void * priv, bg_ov_callbacks_t * callbacks);
  
  /** \brief Open plugin
   *  \param priv The handle returned by the create() method
   *  \param format Video format
   *  \param window_title Window title
   *
   *  The format will be changed to the nearest format, which is supported
   *  by the plugin. To convert the source format to the output format,
   *  use a \ref gavl_video_converter_t
   */
  
  int  (*open)(void * priv, gavl_video_format_t * format, int keep_aspect);
  
  /** \brief Allocate a video frame
   *  \param priv The handle returned by the create() method
   *  \returns a newly allocated video frame
   *
   *  This optional method allocates a video frame in a plugin specific 
   *  manner (e.g. in a shared memory segment). If this funtion is 
   *  defined, all frames which are passed to the plugin, must be allocated
   *  by this function. Before the plugin is closed, all created frames must
   *  be freed with the destroy_frame() method.
   */
  
  gavl_video_frame_t * (*create_frame)(void * priv);
  
  /** \brief Add a stream for transparent overlays
   *  \param priv The handle returned by the create() method
   *  \param format Format of the overlays
   *  \returns The index of the overlay stream
   *
   *  It's up to the plugin, if they are realized in hardware or
   *  with a gavl_overlay_blend_context_t, but they must be there.
   *  add_overlay_stream() must be called after open()
   *
   *  An application can have more than one overlay stream. Typical
   *  is one for subtitles and one for OSD.
   */
  
  int (*add_overlay_stream)(void * priv, gavl_video_format_t * format);

  /** \brief Allocate an overlay
   *  \param priv The handle returned by the create() method
   *  \param id The id returned by the add_overlay_stream() method
   *  \returns a newly allocated overlay
   *
   *  This optional method allocates an overlay in a plugin specific manner
   *  (e.g. in a shared memory segment). If this funtion is defined, all overlays
   *  which are passed to the plugin, must be allocated by this function.
   *  Before the plugin is closed, all created overlays must be freed with
   *  the destroy_overlay() method.
   */
  
  gavl_overlay_t * (*create_overlay)(void * priv, int id);
  
  /** \brief Set an overlay for a specific stream
   *  \param priv The handle returned by the create() method
   *  \param stream Stream index returned by add_overlay_stream()
   *  \param ovl New overlay or NULL
   */
  
  void (*set_overlay)(void * priv, int stream, gavl_overlay_t * ovl);
  
  /** \brief Display a frame of a video stream
   *  \param priv The handle returned by the create() method
   *  \param frame Frame to display
   *  
   *  This is for video playback
   */

  void (*put_video)(void * priv, gavl_video_frame_t*frame);

  /** \brief Display a still image
   *  \param priv The handle returned by the create() method
   *  \param frame Frame to display
   *  
   *  This function is like put_video() with the diffderence, that
   *  the frame will be remembered and redisplayed, when an expose event
   *  is received.
   */
  
  void (*put_still)(void * priv, gavl_video_frame_t*frame);

  /** \brief Get all events from the queue and handle them
   *  \param priv The handle returned by the create() method
   *  
   *  This function  processes and handles all events, which were
   *  received from the windowing system. It calls mouse and key-callbacks,
   *  and redisplays the image when in still mode.
   */
  
  void (*handle_events)(void * priv);

  /** \brief Update aspect ratio
   *  \param priv The handle returned by the create() method
   *  \param pixel_width New pixel width
   *  \param pixel_height New pixel height
   */
  
  void (*update_aspect)(void * priv, int pixel_width, int pixel_height);
    
  /** \brief Free a frame created with the create_frame() method.
   *  \param priv The handle returned by the create() method
   *  \param frame The frame to be freed
   */

  void (*destroy_frame)(void * priv, gavl_video_frame_t * frame);

  /** \brief Free an overlay created with the create_overlay() method.
   *  \param priv The handle returned by the create() method
   *  \param id The id returned by the add_overlay_stream() method
   *  \param ovl The overlay to be freed
   */
  
  void (*destroy_overlay)(void * priv, int id, gavl_overlay_t * ovl);

  /** \brief Close the plugin
   *  \param priv The handle returned by the create() method
   *
   *  Close everything so the plugin can be opened with a differtent format
   *  after.
   */
  
  void (*close)(void * priv);

  /** \brief Show or hide the window
   *  \param priv The handle returned by the create() method
   *  \param show 1 for showing, 0 for hiding
   */
  void (*show_window)(void * priv, int show);
  };

/*******************************************
 * ENCODER
 *******************************************/

/** \defgroup plugin_e Encoder
 *  \ingroup plugin
 *  \brief Encoder
 */ 

/** \ingroup plugin_ov
 * \brief Typedef for callbacks for the encoder plugin
 *
 */

typedef struct bg_encoder_callbacks_s bg_encoder_callbacks_t;

/** \ingroup plugin_ov
 * \brief Callbacks for the encoder plugin
 *
 */

struct bg_encoder_callbacks_s
  {
  
  /** \brief   Output file callback
   *  \param   data The data member of this bg_ov_callbacks_s struct
   *  \param   filename Name of the created file
   *  \returns 1 if the file may be created, 0 else
   *
   *  This is called whenever an output file is created.
   */
  
  int (*create_output_file)(void * data, const char * filename);

  /** \brief   Temp file callback
   *  \param   data The data member of this bg_ov_callbacks_s struct
   *  \param   filename Name of the created file
   *  \returns 1 if the file may be created, 0 else
   *
   *  This is called whenever a temporary file is created.
   */

  int (*create_temp_file)(void * data, const char * filename);
  
  void * data;//!< Application specific data passed as the first argument to all callbacks.
  };


/** \ingroup plugin_e
 *  \brief Typedef for encoder plugin
 */

typedef struct bg_encoder_plugin_s bg_encoder_plugin_t;


/** \ingroup plugin_e
 *  \brief Encoder plugin
 */

struct bg_encoder_plugin_s
  {
  bg_plugin_common_t common; //!< Infos and functions common to all plugin types
  
  int max_audio_streams;  //!< Maximum number of audio streams. -1 means infinite
  int max_video_streams;  //!< Maximum number of video streams. -1 means infinite
  int max_subtitle_text_streams;//!< Maximum number of text subtitle streams. -1 means infinite
  int max_subtitle_overlay_streams;//!< Maximum number of overlay subtitle streams. -1 means infinite
  
  /** \brief Set callbacks
   *  \param priv The handle returned by the create() method
   *  \param cb Callback structure
   */
  
  void (*set_callbacks)(void * priv, bg_encoder_callbacks_t * cb);
  
  /** \brief Open a file
   *  \param priv The handle returned by the create() method
   *  \param filename Name of the file to be opened (without extension!)
   *  \param metadata Metadata to be written to the file
   *  \param chapter_list Chapter list (optional, can be NULL)
   *
   *  The extension is added automatically by the plugin.
   *  To keep track of the written files, use the \ref bg_encoder_callbacks_t.
   */
  
  int (*open)(void * data, const char * filename,
              const bg_metadata_t * metadata,
              const bg_chapter_list_t * chapter_list);
  
  /** \brief Return the filename, which can be passed to the player
   *  \param priv The handle returned by the create() method
   *
   *  This must be implemented only if the plugin creates
   *  files with names different from the the filename passed to
   *  the open() function
   */
  
  const char * (*get_filename)(void*);

  /* Return per stream parameters */

  /** \brief Get audio related parameters
   *  \param priv The handle returned by the create() method
   *  \returns NULL terminated array of parameter descriptions
   *
   *  The returned parameters are owned by the plugin and must not be freed.
   */
  
  const bg_parameter_info_t * (*get_audio_parameters)(void * priv);

  /** \brief Get video related parameters
   *  \param priv The handle returned by the create() method
   *  \returns NULL terminated array of parameter descriptions
   *
   *  The returned parameters are owned by the plugin and must not be freed.
   */

  const bg_parameter_info_t * (*get_video_parameters)(void * priv);

  /** \brief Get text subtitle related parameters
   *  \param priv The handle returned by the create() method
   *  \returns NULL terminated array of parameter descriptions
   *
   *  The returned parameters are owned by the plugin and must not be freed.
   */

  const bg_parameter_info_t * (*get_subtitle_text_parameters)(void * priv);

  /** \brief Get overlay subtitle related parameters
   *  \param priv The handle returned by the create() method
   *  \returns NULL terminated array of parameter descriptions
   *
   *  The returned parameters are owned by the plugin and must not be freed.
   */

  const bg_parameter_info_t * (*get_subtitle_overlay_parameters)(void * priv);
  
  /* Add streams. The formats can be changed, be sure to get the
   * final formats with get_[audio|video]_format after starting the plugin
   * Return value is the index of the added stream.
   */

  /** \brief Add an audio stream
   *  \param priv The handle returned by the create() method
   *  \param language as ISO 639-2 code (3 characters+'\\0') or NULL
   *  \param format Format of the source
   *  \returns Index of this stream (starting with 0)
   *  
   *  The format might be changed to the nearest format supported by
   *  the plugin. Use \ref get_audio_format to get the actual format
   *  needed by the plugin, after \ref start() was called.
   */
  
  int (*add_audio_stream)(void * priv, const char * language,
                          const gavl_audio_format_t * format);

  /** \brief Add a video stream
   *  \param priv The handle returned by the create() method
   *  \param format Format of the source
   *  \returns Index of this stream (starting with 0)
   *  
   *  The format might be changed to the nearest format supported by
   *  the plugin. Use \ref get_video_format to get the actual format
   *  needed by the plugin, after \ref start() was called.
   */
  
  int (*add_video_stream)(void * priv, const gavl_video_format_t * format);

  /** \brief Add a text subtitle stream
   *  \param priv The handle returned by the create() method
   *  \param language as ISO 639-2 code (3 characters+'\\0') or NULL
   *  \returns Index of this stream (starting with 0)
   */
  
  int (*add_subtitle_text_stream)(void * priv, const char * language,
                                  int * timescale);
  
  /** \brief Add a text subtitle stream
   *  \param priv The handle returned by the create() method
   *  \param language as ISO 639-2 code (3 characters+'\\0') or NULL
   *  \param format Format of the source
   *  \returns Index of this stream (starting with 0)
   *
   *  The format might be changed to the nearest format supported by
   *  the plugin. Use \ref get_subtitle_overlay_format
   *  to get the actual format
   *  needed by the plugin, after \ref start was called.
   */
  
  int (*add_subtitle_overlay_stream)(void * priv, const char * language,
                                     const gavl_video_format_t * format);
  
  /* Set parameters for the streams */

  /** \brief Set audio encoding parameter
   *  \param priv The handle returned by the create() method
   *  \param stream Stream index (starting with 0)
   *  \param name Name of the parameter
   *  \param v Value
   *
   *  Use this function with parameters obtained by
   *  \ref get_audio_parameters.
   */
  
  void (*set_audio_parameter)(void * priv, int stream, const char * name,
                              const bg_parameter_value_t * v);

  /** \brief Set video encoding parameter
   *  \param priv The handle returned by the create() method
   *  \param stream Stream index (starting with 0)
   *  \param name Name of the parameter
   *  \param v Value
   *
   *  Use this function with parameters obtained by
   *  \ref get_video_parameters.
   */

  
  void (*set_video_parameter)(void * priv, int stream, const char * name,
                              const bg_parameter_value_t * v);

  /** \brief Set text subtitle encoding parameter
   *  \param priv The handle returned by the create() method
   *  \param stream Stream index (starting with 0)
   *  \param name Name of the parameter
   *  \param v Value
   *
   *  Use this function with parameters obtained by
   *  \ref get_subtitle_text_parameters.
   */
  
  void (*set_subtitle_text_parameter)(void * priv, int stream,
                                      const char * name,
                                      const bg_parameter_value_t * v);

  /** \brief Set text subtitle encoding parameter
   *  \param priv The handle returned by the create() method
   *  \param stream Stream index (starting with 0)
   *  \param name Name of the parameter
   *  \param v Value
   *
   *  Use this function with parameters obtained by
   *  \ref get_subtitle_overlay_parameters.
   */
  
  void (*set_subtitle_overlay_parameter)(void * priv, int stream,
                                         const char * name,
                                         const bg_parameter_value_t * v);
  
  /** \brief Setup multipass video encoding.
   *  \param priv The handle returned by the create() method
   *  \param stream Stream index (starting with 0)
   *  \param pass Number of this pass (starting with 1)
   *  \param total_passes Number of total passes
   *  \param stats_file Name of a file, which can be used for multipass statistics
   *  \returns 0 if multipass transcoding is not supported and can be ommitted, 1 else
   */
  int (*set_video_pass)(void * priv, int stream, int pass, int total_passes,
                        const char * stats_file);
  
  /** \brief Set up all codecs and prepare for encoding
   *  \param priv The handle returned by the create() method
   *  \returns 0 on error, 1 on success
   *
   *  Optional function for preparing the actual encoding. Applications must
   *  check for this function and call it when available.
   */
  
  int (*start)(void * priv);
  
  /*
   *  After setting the parameters, get the formats, you need to deliver the frames in
   */

  /** \brief Get audio format
   *  \param priv The handle returned by the create() method
   *  \param stream Stream index (starting with 0)
   *  \param ret Returns format
   *
   *  Call this after calling \ref start() if it's defined.
   */
  
  void (*get_audio_format)(void * priv, int stream, gavl_audio_format_t*ret);

  /** \brief Get video format
   *  \param priv The handle returned by the create() method
   *  \param stream Stream index (starting with 0)
   *  \param ret Returns format
   *
   *  Call this after calling \ref start() if it's defined.
   */

  void (*get_video_format)(void * priv, int stream, gavl_video_format_t*ret);

  /** \brief Get video format of an overlay subtitle stream
   *  \param priv The handle returned by the create() method
   *  \param stream Stream index (starting with 0)
   *  \param ret Returns format
   *
   *  Call this after calling \ref start() if it's defined.
   */

  void (*get_subtitle_overlay_format)(void * priv, int stream,
                                      gavl_video_format_t*ret);

  /*
   *  Encode audio/video
   */

  /** \brief Write audio samples
   *  \param priv The handle returned by the create() method
   *  \param frame Frame with samples
   *  \param stream Stream index (starting with 0)
   *  \returns 1 is the data was successfully written, 0 else
   *
   *  The actual number of samples must be stored in the valid_samples member of
   *  the frame.
   */
  
  int (*write_audio_frame)(void * data,gavl_audio_frame_t * frame, int stream);

  /** \brief Write video frame
   *  \param priv The handle returned by the create() method
   *  \param frame Frame
   *  \param stream Stream index (starting with 0)
   *  \returns 1 is the data was successfully written, 0 else
   */

  int (*write_video_frame)(void * data,gavl_video_frame_t * frame, int stream);

  /** \brief Write a text subtitle
   *  \param priv The handle returned by the create() method
   *  \param frame The text
   *  \param start Start of the subtitle
   *  \param duration Duration of the subtitle
   *  \param stream Stream index (starting with 0)
   *  \returns 1 is the data was successfully written, 0 else
   */
  
  int (*write_subtitle_text)(void * data,const char * text,
                             int64_t start,
                             int64_t duration, int stream);
  
  /** \brief Write an overlay subtitle
   *  \param priv The handle returned by the create() method
   *  \param ovl An overlay
   *  \param stream Stream index (starting with 0)
   *  \returns 1 is the data was successfully written, 0 else
   */
  
  int (*write_subtitle_overlay)(void * data, gavl_overlay_t * ovl, int stream);
  
  /** \brief Close encoder
   *  \param priv The handle returned by the create() method
   *  \param do_delete Set this to 1 to delete all created files
   *  \returns 1 is the file was sucessfully closed, 0 else
   *
   *  After calling this function, the plugin should be destroyed.
   */

  int (*close)(void * data, int do_delete);
  };


/*******************************************
 * ENCODER Postprocessor
 *******************************************/

/** \defgroup plugin_e_pp Encoding postprocessor
 *  \ingroup plugin
 *  \brief Encoding postprocessor
 *
 *  The postprocessing plugins take one or more files
 *  for further processing them. There are postprocessors
 *  for creating and (optionally) burning audio CDs and VCDs.
 */

/** \ingroup plugin_e_pp
 *  \brief Callbacks for postprocessing
 *
 */

typedef struct
  {
  /** \brief Callback describing the current action
   *  \param data The data member of this bg_ov_callbacks_s struct
   *  \param action A string describing the current action
   *
   *  Action can be something like "Burning track 1/10".
   */
  void (*action_callback)(void * data, char * action);

  /** \brief Callback describing the progress of the current action
   *  \param data The data member of this bg_ov_callbacks_s struct
   *  \param perc Percentage (0.0 .. 1.0)
   *
   *  This is exclusively for updating progress bars in
   *  GUI applications. Note, that some postprocessors
   *  reset the progress during postprocessing.
   */
  
  void (*progress_callback)(void * data, float perc);

  void * data; //!< Application specific data passed as the first argument to all callbacks.

  } bg_e_pp_callbacks_t;

/** \ingroup plugin_e_pp
 *  \brief Typedef for encoding postprocessor
 *
 */

typedef struct bg_encoder_pp_plugin_s bg_encoder_pp_plugin_t;

/** \ingroup plugin_e_pp
 *  \brief Encoding postprocessor
 *
 */

struct bg_encoder_pp_plugin_s
  {
  bg_plugin_common_t common; //!< Infos and functions common to all plugin types
  
  int max_audio_streams;  //!< Maximum number of audio streams. -1 means infinite
  int max_video_streams;  //!< Maximum number of video streams. -1 means infinite

  char * supported_extensions; //!< Supported file extensions (space separated)
  
  /** \brief Set callbacks
   *  \param priv The handle returned by the create() method
   *  \param callbacks Callback structure initialized by the caller before
   *
   */

  void (*set_callbacks)(void * priv,bg_e_pp_callbacks_t * callbacks);
  
  /** \brief Initialize
   *  \param priv The handle returned by the create() method
   *
   *  This functions clears all tracks and makes the plugin ready to
   *  add new tracks.
   */
  
  int (*init)(void * priv);

  /** \brief Add a transcoded track
   *  \param priv The handle returned by the create() method
   *  \param filename Name of the media file
   *  \param metadata Metadata for the track
   *  \param pp_only Set this to 1 if this file was not encoded and is only postprocessed
   *
   *  Send a track to the postprocessor. This plugin will store all necessary data until
   *  the \ref run() method is called. Some postprocessors might do some sanity tests
   *  on the file e.g. the audio CD burner will reject anything except WAV-files.
   *  
   *  Usually, it is assumed, that files were encoded for the only purpose of postprocessing
   *  them. This means, they will be deleted if the cleanup flag is passed to the \ref run()
   *  function. If you set the pp_only argument to 1, you tell, that this file should be
   *  kept after postprocessing.
   */
  
  void (*add_track)(void * priv, const char * filename,
                    bg_metadata_t * metadata, int pp_only);
  
  /** \brief Start postprocessing
   *  \param priv The handle returned by the create() method
   *  \param directory The directory, where output files can be written
   *  \param cleanup Set this to 1 to erase all source files and temporary files after finishing
   * Run can be a long operation, it should be called from a separate
   * thread launched by the application and the callbacks should be
   * used for progress reporting
   */
  
  void (*run)(void * priv, const char * directory, int cleanup);

  /** \brief Stop postprocessing
   *  \param priv The handle returned by the create() method
   *
   *  Call this function to cancel a previously called \ref run() function.
   *  The plugin must implement the stop mechanism in a thread save manner,
   *  i.e. the stop method must savely be callable from another thread than the
   *  one, which called \ref run().
   */
  
  void (*stop)(void * priv);
  };


/** \defgroup plugin_ir Image support
 *  \ingroup plugin
 *  \brief Read and write image files
 *
 *  These support reading and writing of images in a variety
 *  of formats
 *
 *  @{
 */

/** \brief Typedef for image reader plugin
 */

typedef struct bg_image_reader_plugin_s bg_image_reader_plugin_t;

/** \brief Image reader plugin
 */

struct bg_image_reader_plugin_s
  {
  bg_plugin_common_t common; //!< Infos and functions common to all plugin types
  const char * extensions; //!< Supported file extensions (space separated)
  
  /** \brief Read the file header
   *  \param priv The handle returned by the create() method
   *  \param filename Filename
   *  \param format Returns the format of the image
   *  \returns 1 on success, 0 on error.
   */
  
  int (*read_header)(void * priv, const char * filename,
                     gavl_video_format_t * format);
  
  const bg_metadata_t * (*get_metadata)(void * priv);
  
  /** \brief Get file infos
   *  \param priv The handle returned by the create() method
   *  \returns 
   */
  
  
  char ** (*get_info)(void * priv);
  
  /** \brief Read the image
   *  \param priv The handle returned by the create() method
   *  \param frame The frame, where the image will be copied
   *  \returns 1 if the image was read, 0 else
   *  
   *  After reading the image the plugin is cleaned up, so \ref read_header()
   *  can be called again after that. If frame is NULL, no image is read,
   *  and the plugin is reset.
   */
  int (*read_image)(void * priv, gavl_video_frame_t * frame);
  };

/**  
 * \brief Typedef for callbacks for the image writer plugin
 */

typedef struct bg_iw_callbacks_s bg_iw_callbacks_t;

struct bg_iw_callbacks_s
  {
  
  /** \brief   Output file callback
   *  \param   data The data member of this bg_ov_callbacks_s struct
   *  \param   filename Name of the created file
   *  \returns 1 if the file may be created, 0 else
   *
   *  This is called whenever an output file is created.
   */
  
  int (*create_output_file)(void * data, const char * filename);
  
  void * data;//!< Application specific data passed as the first argument to all callbacks.
  };

/** \brief Typedef for image writer plugin
 *
 */

typedef struct bg_image_writer_plugin_s bg_image_writer_plugin_t;

/** \brief Image writer plugin
 *
 */

struct bg_image_writer_plugin_s
  {
  bg_plugin_common_t common; //!< Infos and functions common to all plugin types
  const char * extensions; //!< File extensions (space separated)
  
  /** \brief Set callbacks
   *  \param priv The handle returned by the create() method
   *  \param cb Callback structure
   */
  
  void (*set_callbacks)(void * priv, bg_iw_callbacks_t * cb);
  
  /** \brief Write the file header
   *  \param priv The handle returned by the create() method
   *  \param format Video format
   *  \returns 1 on success, 0 on error.
   *
   *  The format will be changed to the nearest format, which is supported
   *  by the plugin. To convert the source format to the output format,
   *  use a \ref gavl_video_converter_t
   */
  
  int (*write_header)(void * priv, const char * filename,
                      gavl_video_format_t * format, const bg_metadata_t * m);
  
  /** \brief Write the image
   *  \param priv The handle returned by the create() method
   *  \param frame The frame containing the image
   *  \returns 1 on success, 0 on error.
   *  
   *  After writing the image the plugin is cleaned up, so \ref write_header()
   *  can be called again after that. If frame is NULL, no image is read,
   *  and the plugin is reset.
   */

  int (*write_image)(void * priv, gavl_video_frame_t * frame);
  } ;

/**
 *  @}
 */


/** \defgroup plugin_filter A/V Filters
 *  \ingroup plugin
 *  \brief A/V Filters
 
 *  These can apply additional effects to uncomporessed A/V data.
 *  The API follows an asynchronous pull approach: You pass a callback
 *  for reading input frames. Then you call the read method to read one
 *  output frame. The plugin will read input data via the callback.
 *
 *  This mechanism allows filters, where the numbers of input frames/samples
 *  differs from the numbers of output frames/samples (e.g. when the filter
 *  does framerate conversion).
 *
 *  In principle, the API also supports filters with multiple input ports,
 *  but currently, only plugins with one input are available.
 *
 *  Not all filters support all formats. Applications should build filter chains
 *  with format converters between them. There are, however, some standard formats,
 *  which are supported by almost all filters, so the overall conversion overhead
 *  is usually zero in real-life situations.
 *
 *  @{
 */


/* Filters */

/** \brief Typedef for audio filter plugin
 *
 */

typedef struct bg_fa_plugin_s bg_fa_plugin_t;

/** \brief Audio filter plugin
 *
 */

struct bg_fa_plugin_s
  {
  bg_plugin_common_t common; //!< Infos and functions common to all plugin types

  /** \brief Set input callback
   *  \param priv The handle returned by the create() method
   *  \param func The function to call
   *  \param data The private handle to pass to func
   *  \param stream The stream argument to pass to func
   *  \param port The input port of the plugin
   */
  
  void (*connect_input_port)(void * priv, bg_read_audio_func_t func,
                             void * data,
                             int stream, int port);

  /** \brief Set input format
   *  \param priv The handle returned by the create() method
   *  \param format Format
   *  \param port The input port of the plugin
   *
   *  The input format can be changed by the plugin. Make sure that you have a format
   *  converter before the filter.
   */
  
  void (*set_input_format)(void * priv, gavl_audio_format_t * format, int port);


  /** \brief Reset
   *  \param priv The handle returned by the create() method
   *
   *  Optional, resets internal state, as if no frame has been processed before.
   */

  void (*reset)(void * priv);

  /** \brief Get output format
   *  \param priv The handle returned by the create() method
   *  \param format Returns the output format
   *
   *  These must be called after init().
   */

  void (*get_output_format)(void * priv, gavl_audio_format_t * format);

  /** \brief Report, if the plugin must be reinitialized
   *  \param priv The handle returned by the create() method
   *  \returns 1 if the plugin must be reinitialized, 0 else
   *
   *  Optional, must be called after set_parameter() to check, if the
   *  filter must be reinitialized. Note, that the input and output formats can
   *  be changed in this case as well.
   */
  
  int (*need_restart)(void * priv);

  /** \brief Read audio samples from the plugin
   */
 
  bg_read_audio_func_t read_audio;
    
  };

/** \brief Typedef for video filter plugin
 *
 */

typedef struct bg_fv_plugin_s bg_fv_plugin_t;

/** \brief Video filter plugin
 *
 */

struct bg_fv_plugin_s
  {
  bg_plugin_common_t common; //!< Infos and functions common to all plugin types

  /** \brief Get gavl options
   *  \param priv The handle returned by the create() method
   *  \returns Video conversion options
   *
   *  This optional function returns the gavl options. You can configure them
   *  like you do it in plain gavl.
   */
  
  gavl_video_options_t * (*get_options)(void * priv);
  
  /** \brief Set input callback
   *  \param priv The handle returned by the create() method
   *  \param func The function to call
   *  \param data The private handle to pass to func
   *  \param stream The stream argument to pass to func
   *  \param port The input port of the plugin
   */
  
  void (*connect_input_port)(void * priv,
                             bg_read_video_func_t func,
                             void * data, int stream, int port);

  /** \brief Set input format
   *  \param priv The handle returned by the create() method
   *  \param format Format
   *  \param port The input port of the plugin
   */
  
  void (*set_input_format)(void * priv, gavl_video_format_t * format, int port);
  
  /** \brief Reset
   *  \param priv The handle returned by the create() method
   *
   *  Optional, resets internal state, as if no frame has been processed before.
   */

  void (*reset)(void * priv);

  /** \brief Get output format
   *  \param priv The handle returned by the create() method
   *  \param format Returns the output format
   *
   *  These must be called after init().
   */
  
  void (*get_output_format)(void * priv, gavl_video_format_t * format);
  
  /** \brief Report, if the plugin must be reinitialized
   *  \param priv The handle returned by the create() method
   *  \returns 1 if the plugin must be reinitialized, 0 else
   *
   *  Optional, must be called after set_parameter() to check, if the
   *  filter must be reinitialized. Note, that the input and output formats can
   *  be changed in this case as well.
   */
  
  int (*need_restart)(void * priv);

  /** \brief Read a video frame from the plugin
   */

  bg_read_video_func_t read_video;
    
  };


/**
 *  @}
 */


/** \defgroup plugin_visualization Audio Visualization plugins
 *  \ingroup plugin
 *  \brief Audio Visualization plugins
 *
 *
 *
 *  @{
 */

/** \brief Typedef for audio visualization plugin
 *
 */

typedef struct bg_visualization_plugin_s bg_visualization_plugin_t;


/** \brief Audio visualization plugin
 *
 *  These plugins get audio samples and run visualizations of them.
 *  Output can be either into a \ref gavl_video_frame_t or directly
 *  via OpenGL. Which method is used is denoted by the
 *  \ref BG_PLUGIN_VISUALIZE_FRAME and \ref BG_PLUGIN_VISUALIZE_GL
 *  flags.
 *
 *  For OpenGL, you need to pass a window ID to the
 *  plugin. The plugin is then responsible for creating Subwindows and
 *  setting up an OpenGL context. In General, it's stronly recommended to
 *  use the \ref bg_visualizer_t module to use visualizations.
 */

struct bg_visualization_plugin_s
  {
  bg_plugin_common_t common; //!< Infos and functions common to all plugin types
  /** \brief return callback \param priv The handle returned by the
   *  \param priv The handle returned by the create() method
   *
   *  create() method \returns The callbacks or NULL
   */
  
  bg_ov_callbacks_t * (*get_callbacks)(void * priv);

  /** \brief Open a frame based visualization plugin
   *  \param priv The handle returned by the create() method
   *  \param audio_format Audio format
   *  \param video_format Video format
   *
   *  The audio format parameter will most likely changed to the
   *  nearest supported format. In the video format parameter, you
   *  usually pass the desired render size. Everything else
   *  (except the framerate) will be set up by the plugin.
   */
  
  int (*open_ov)(void * priv, gavl_audio_format_t * audio_format,
                 gavl_video_format_t * video_format);
  
  /** \brief Open a window based visualization plugin
   *  \param priv The handle returned by the create() method
   *  \param audio_format Audio format
   *  \param window_id A window ID
   *
   *  The audio format parameter will most likely changed to the
   *  nearest supported format. For the window id, use strings formatted
   *  like the one returned by \ref bg_ov_plugin_t
   
   */
  int (*open_win)(void * priv, gavl_audio_format_t * audio_format,
                  const char * window_id);

  /** \brief Send audio data to the plugin
   *  \param priv The handle returned by the create() method
   *  \param frame Audio frame
   *
   *  This updates the plugin with new audio samples. After that, we may
   *  or may not call the draw_frame function below to actually draw an
   *  image. Note, that visualization plugins are not required to do
   *  internal audio buffering, so it's wise to pass a frame with as many
   *  samples as the samples_per_frame member of the audio format returned
   *  by \ref open_ov or \ref open_win.
   */
  
  void (*update)(void * priv, gavl_audio_frame_t * frame);

  /** \brief Draw an image
   *  \param priv The handle returned by the create() method
   *  \param The video frame to draw to
   *
   *  For OpenGL plugins, frame is NULL. For frame based plugins,
   *  the image will be drawn into the frame you pass. You must display
   *  the frame on your own then.
   */
  
  void (*draw_frame)(void * priv, gavl_video_frame_t * frame);
  
  /** \brief Show the image
   *  \param priv The handle returned by the create() method
   *
   *  This function is needed only for OpenGL plugins. Under X11,
   *  it will typically call glXSwapBuffers and process events on the
   *  X11 window.
   */
  
  void (*show_frame)(void * priv);

  /** \brief Close a plugin
   *  \param priv The handle returned by the create() method
   */
  
  void (*close)(void * priv);
  
  };



/**
 *  @}
 */

#endif // __BG_PLUGIN_H_
