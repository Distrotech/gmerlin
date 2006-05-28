/*****************************************************************
 
  plugin.h
 
  Copyright (c) 2003-2006 by Burkhard Plaum - plaum@ipf.uni-stuttgart.de
 
  http://gmerlin.sourceforge.net
 
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
 
  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111, USA.
 
*****************************************************************/

#ifndef __BG_PLUGIN_H_
#define __BG_PLUGIN_H_

#include <gavl/gavl.h>
#include "parameter.h"
#include "streaminfo.h"

#define BG_PLUGIN_API_VERSION 6

/* Include this into all plugin modules exactly once
   to let the plugin loader obtain the API version */

#define BG_GET_PLUGIN_API_VERSION \
  extern int get_plugin_api_version() { return BG_PLUGIN_API_VERSION; }

#define BG_PLUGIN_PRIORITY_MIN 1
#define BG_PLUGIN_PRIORITY_MAX 10

typedef enum
  {
    BG_STREAM_ACTION_OFF = 0,
    BG_STREAM_ACTION_DECODE,
    
    /*
     *  A/V data will bypass the player. It will only be chosen if the
     *  BG_PLUGIN_BYPASS flag (see below) is present
     */
    
    BG_STREAM_ACTION_BYPASS,
    
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

typedef enum
  {
    BG_PLUGIN_NONE                       = 0,
    BG_PLUGIN_INPUT                      = (1<<0),
    BG_PLUGIN_OUTPUT_AUDIO               = (1<<1),
    BG_PLUGIN_OUTPUT_VIDEO               = (1<<2),
    BG_PLUGIN_RECORDER_AUDIO             = (1<<3),
    BG_PLUGIN_RECORDER_VIDEO             = (1<<4),
    BG_PLUGIN_ENCODER_AUDIO              = (1<<5),
    BG_PLUGIN_ENCODER_VIDEO              = (1<<6),
    BG_PLUGIN_ENCODER_SUBTITLE_TEXT      = (1<<7),
    BG_PLUGIN_ENCODER_SUBTITLE_OVERLAY   = (1<<8),
    BG_PLUGIN_ENCODER                    = (1<<9),  // Encoder for multiple kinds of streams
    BG_PLUGIN_ENCODER_PP                 = (1<<10), // Encoder postprocessor
    BG_PLUGIN_IMAGE_READER               = (1<<11),
    BG_PLUGIN_IMAGE_WRITER               = (1<<12)
  } bg_plugin_type_t;

/* At least one of these must be set */

#define BG_PLUGIN_REMOVABLE    (1<<0)  /* Removable media (CD, DVD etc.) */
#define BG_PLUGIN_FILE         (1<<1)  /* Plugin reads/writes files      */
#define BG_PLUGIN_RECORDER     (1<<2)

#define BG_PLUGIN_URL          (1<<3)
#define BG_PLUGIN_PLAYBACK     (1<<4)  /* Output plugins for playback    */

#define BG_PLUGIN_BYPASS       (1<<5)  /* Plugin can send A/V data directly to the output bypassing
                                          the player engine */

#define BG_PLUGIN_KEEP_RUNNING (1<<6) /* Plugin should not be stopped and restarted if tracks change */

#define BG_PLUGIN_INPUT_HAS_SYNC (1<<7) /* FOR INPUTS ONLY: Plugin will set the time via callback    */

#define BG_PLUGIN_ALL 0xFFFFFFFF

/*
 *  Device info: Returned by the "find_devices()" function
 *  Device arrays are NULL terminated
 */

typedef struct
  {
  char * device; /* Can be passed to the open() method */
  char * name;   /* Might be NULL */
  } bg_device_info_t;

/*
 *  Append device info to an existing array and return the new array.
 *  arr can be NULL
 */

bg_device_info_t * bg_device_info_append(bg_device_info_t * arr,
                                         const char * device,
                                         const char * name);

void bg_device_info_destroy(bg_device_info_t * arr);

/* Common part */

typedef struct bg_plugin_common_s
  {
  char             * name;       /* Unique short name */
  char             * long_name;
  char             * mimetypes;
  char             * extensions;
  bg_plugin_type_t type;
  int              flags;

  /*
   *  If there might be more than one plugin for the same
   *  job, there is a priority (0..10) which is used for the
   *  decision
   */
  
  int              priority;
  
  /*
   *  Open the plugin, return handle, handle is the first
   *  argument for all other functions
   */
  
  void * (*create)();
      
  /*
   * Destroy everything, making it ready for dlclose()
   * This function might also be called on opened plugins,
   * so the plugins should call their close()-function from
   * within the destructor
   */

  void (*destroy)(void* priv);

  /* Get available parameters */

  bg_parameter_info_t * (*get_parameters)(void * priv);

  /* Set configuration parameter */
    
  bg_set_parameter_func set_parameter;

  /* Get parameter is optional, also, it needs to return values for
     all parameters. */
  
  bg_get_parameter_func get_parameter;

  /* OPTIONAL: Return a readable description of the last error */

  const char * (*get_error)(void* priv);

  /* Optional: */
  
  int (*check_device)(const char * device, char ** name);
  
  /*
   *  Even more optional: Scan for available devices (will most likely call
   *  check_device internally)
   */
  
  bg_device_info_t * (*find_devices)();
    
  } bg_plugin_common_t;

/*
 *  Plugin callbacks: Functions called by the
 *  plugin to reflect user input or other changes
 *  Applications might pass NULL callbacks,
 *  so plugins MUST check for valid callbacks structure
 *  before calling any of these functions
 */

/* Input plugin */

typedef struct bg_input_callbacks_s
  {
  /*
   *  Change the track.
   */

  void (*track_changed)(void * data, int track);

  /*
   *  Time changed (only for bypass mode)
   */
  
  void (*time_changed)(void * data, gavl_time_t time);
    
  /*
   *  Update the duration
   */
  void (*duration_changed)(void * data,
                           gavl_time_t duration);
  /* Track name changed */
  
  void (*name_changed)(void * data, const char * name);

  /* Metadata changed */
  
  void (*metadata_changed)(void * data, const bg_metadata_t * m);

  /* Buffering progress */

  void (*buffer_notify)(void * data, float percentage);
  
  /* Get authentication (return values 1: Success, 2: cancel) */
  int (*user_pass)(void * data, const char * resource,
                   char ** username, char ** password);
  
  void * data;
  } bg_input_callbacks_t;

/*************************************************
 * MEDIA INPUT
 *************************************************/

typedef struct bg_input_plugin_s
  {
  bg_plugin_common_t common;
  
  /* Open file/device, return False on failure */
  
  int (*open)(void * priv, const char * arg);

  /* Alternative: Open with filedescriptor (used for http mostly) */

  int (*open_fd)(void * priv, int fd, int64_t total_bytes,
                 const char * mimetype);
  
  /*
   * Set callback functions, which can be called by plugin
   * This can be NULL is the plugin doesn't support any of these
   * callabacks
   */
  
  void (*set_callbacks)(void * priv, bg_input_callbacks_t * callbacks);
  
  /* For file and network plugins, this is NULL */

  int (*get_num_tracks)(void * priv);
  
  /*
   *  Return track information
   *  The following fields MUST be valid after this call:
   *  - num_audio_streams
   *  - num_video_streams
   *  - num_subtitle_streams
   *  - duration
   *  - Name (If NULL, the filename minus the suffix will be used)
   */
  
  bg_track_info_t * (*get_track_info)(void * priv, int track);

  /* Set the track to be played */
    
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
  
  int (*set_audio_stream)(void * priv, int stream, bg_stream_action_t);
  int (*set_video_stream)(void * priv, int stream, bg_stream_action_t);
  int (*set_still_stream)(void * priv, int stream, bg_stream_action_t);
  int (*set_subtitle_stream)(void * priv, int stream, bg_stream_action_t);
  //  int (*set_program)(void * priv, int program);
  
  /*
   *  Start decoding.
   *   
   *  After this call, all remaining members of the track info returned earlier
   *  (audio- and video formats!) must be valid.
   *
   *  From the plugins point of view, this is the last chance to return 0
   *  if something fails
   */
  
  int (*start)(void * priv);

  /* Read one audio frame (returns FALSE on EOF) */
  
  int (*read_audio_samples)(void * priv, gavl_audio_frame_t*, int stream,
                            int num_samples);
  
  /* Read one video frame (returns FALSE on EOF) */

  int (*read_video_frame)(void * priv, gavl_video_frame_t*, int stream);

  int (*has_subtitle)(void * priv, int stream);
    
  /* Read one pixmap subtitle (returns FALSE if no subtitle was
     decoded yet or EOF) */
  
  int (*read_subtitle_overlay)(void * priv,
                               gavl_overlay_t*, int stream);
  
  int (*read_subtitle_text)(void * priv,
                            char ** text, int * text_alloc,
                            int64_t * start_time,
                            int64_t * duration, int stream);
  
  /* The following 3 functions are only meaningful for plugins, which
     have the BG_PLUGIN_BYPASS flag set. */

  /* This will be called periodically during playback so the plugin
     can call the callbacks (to update time and track infos */
    
  int (*bypass)(void * priv);
  
  /*
   *  For plugins, which can be paused
   *  pause = 1: pause, pause = 0: resume
   */

  void (*bypass_set_pause)(void * priv, int pause);

  /* Volume control */

  void (*bypass_set_volume)(void * priv, float volume);
    
  /*
   *  Media streams are supposed to be seekable, if this
   *  function is non-NULL AND the duration field of the track info
   *  is > 0 AND the seekable flag in the track info is nonzero.
   *  The time argument might be changed to the correct value
   */

  void (*seek)(void * priv, gavl_time_t * time);

  /* Stop playback (plugin can be started again after) */

  void (*stop)(void * priv);

  /* Close plugin */

  void (*close)(void * priv);
  
  } bg_input_plugin_t;

/*******************************************
 * AUDIO OUTPUT
 *******************************************/

typedef struct bg_oa_plugin_s
  {
  bg_plugin_common_t common;

  /*
   * Open plugin, format will be adjusted to what can be played
   */

  int (*open)(void * priv, gavl_audio_format_t*);

  /*
   *  Start playback
   */

  int (*start)(void * priv);
    
  /*
   *  Write audio samples
   */
  
  void (*write_frame)(void * priv, gavl_audio_frame_t*);

  /*
   *  Stop playback
   */

  void (*stop)(void * priv);
    
  /*
   *  Close plugin, make it ready to be opened with another format
   */
  
  void (*close)(void * priv);
  
  /*
   *  Get the delay due to buffered audio in _samples_
   *  If this function is NULL, software synchronization will be
   *  used
   */

  int (*get_delay)(void * priv);
  
  } bg_oa_plugin_t;

/*******************************************
 * AUDIO RECORDER
 *******************************************/

typedef struct bg_ra_plugin_s
  {
  bg_plugin_common_t common;
  int (*open)(void * priv, gavl_audio_format_t*);
  void (*close)(void * priv);
  void (*read_frame)(void * priv, gavl_audio_frame_t*,int num_samples);
  } bg_ra_plugin_t;

/*******************************************
 * VIDEO OUTPUT
 *******************************************/

/* Callbacks */

typedef struct
  {
  /* Keyboard callback */
  void (*key_callback)(void * data, int key, int mask);
  /* Button can be 1, 2 or 3, coordinates are in IMAGE space */
  void (*button_callback)(void * data, int x, int y, int button, int mask);

  /* Call this, is the window got (un)-mapped */  
  void (*show_window)(void * data, int show);

  /* These can update OSD, when brightness, saturation or contrast changed */
  void (*brightness_callback)(void * data, float val);
  void (*saturation_callback)(void * data, float val);
  void (*contrast_callback)(void * data, float val);
  
  void * data;
  } bg_ov_callbacks_t;

/* Plugin structure */

typedef struct bg_ov_plugin_s
  {
  bg_plugin_common_t common;
  
  /* Set callback functions, which can be called by plugin */
  
  void (*set_callbacks)(void * priv, bg_ov_callbacks_t * callbacks);

  /*
   *   First Operation mode: Video: Initialize with format,
   *   Put frames and close again
   */
         
  int  (*open)(void *, gavl_video_format_t*, const char * window_title);
  void (*put_video)(void * priv, gavl_video_frame_t*);
  
  /*
   *  Add a stream for transparent overlays.
   *  It's up to the plugin, if they are realized in hardware or
   *  with a gavl_overlay_blend_context_t, but they must be there.
   *  add_overlay_stream() must be called AFTER open()
   */
  
  int (*add_overlay_stream)(void*, const gavl_video_format_t * format);
  void (*set_overlay)(void*, int stream, gavl_overlay_t * ovl);
  
  /* 
   *  Second Operation mode:
   *  Put a still image. This will tell the plugin to remember the frame
   *  and redisplay it when it got an expose event.
   */

  void (*put_still)(void*, gavl_video_frame_t * frame);

  /* Get all events from the queue and handle them */
  
  void (*handle_events)(void*);
  
  void (*close)(void * priv);
  
  /* The following ones are optional */

  gavl_video_frame_t * (*alloc_frame)(void*);
  void (*free_frame)(void*,gavl_video_frame_t*);

  /* Show (raise) the window */
  void (*show_window)(void*, int show);

  } bg_ov_plugin_t;

/*******************************************
 * VIDEO RECORDER
 *******************************************/

typedef struct bg_rv_plugin_s
  {
  bg_plugin_common_t common;

  int (*open)(void *, gavl_video_format_t*);
  void (*close)(void * priv);
  int (*read_frame)(void * priv, gavl_video_frame_t*);
  
  /* The following ones are optional */
  gavl_video_frame_t * (*alloc_frame)(void * priv);
  void (*free_frame)(void * priv,gavl_video_frame_t*);
  } bg_rv_plugin_t;

/*******************************************
 * ENCODER
 *******************************************/

typedef struct bg_encoder_plugin_s
  {
  bg_plugin_common_t common;

  /* Maximum number of audio/video streams. -1 means infinite */

  int max_audio_streams;
  int max_video_streams;
  int max_subtitle_text_streams;
  int max_subtitle_overlay_streams;

  /* Return the file extension, which will be appended to the file */

  const char * (*get_extension)(void * priv);
  
  /* Open a file, filename base is without extension, which
     will be added by the plugin */
  
  int (*open)(void *, const char * filename, bg_metadata_t * metadata);

  /* Return the filename which can be passed to the player.
     This must be implemented only if it's different
     from the filename passed to the open function */

  const char * (*get_filename)(void*);

  /* Return per stream parameters */
    
  bg_parameter_info_t * (*get_audio_parameters)(void * data);
  bg_parameter_info_t * (*get_video_parameters)(void * data);
  bg_parameter_info_t * (*get_subtitle_text_parameters)(void * data);
  bg_parameter_info_t * (*get_subtitle_overlay_parameters)(void * data);
  
  /* Add streams. The formats can be changed, be sure to get the
   * final formats with get_[audio|video]_format after starting the plugin
   * Return value is the index of the added stream.
   */
  
  int (*add_audio_stream)(void *, gavl_audio_format_t * format);
  int (*add_video_stream)(void *, gavl_video_format_t * format);
  int (*add_subtitle_text_stream)(void *, const char * language);
  int (*add_subtitle_overlay_stream)(void *, const char * language,
                                     gavl_video_format_t * format);
  
  /* Set parameters for the streams */
  
  void (*set_audio_parameter)(void * data, int stream, char * name,
                              bg_parameter_value_t * v);
  
  void (*set_video_parameter)(void * data, int stream, char * name,
                              bg_parameter_value_t * v);

  void (*set_subtitle_text_parameter)(void * data, int stream, char * name,
                                      bg_parameter_value_t * v);

  void (*set_subtitle_overlay_parameter)(void * data, int stream, char * name,
                                         bg_parameter_value_t * v);
  
  /* Multipass video encoding. Return FALSE if multipass transcoding is
     not supported and can be ommitted */
  int (*set_video_pass)(void * data, int stream, int pass, int total_passes,
                        const char * stats_file);
  
  
  /*
   *  Optional function for preparing the actual encoding.
   *  It might return FALSE is something went wrong
   */

  int (*start)(void * data);
  
  /*
   *  After setting the parameters, get the formats, you need to deliver the frames in
   */

  void (*get_audio_format)(void * data, int stream, gavl_audio_format_t*ret);
  void (*get_video_format)(void * data, int stream, gavl_video_format_t*ret);
  void (*get_subtitle_overlay_format)(void * data, int stream,
                                      gavl_video_format_t*ret);
    
  /*
   *  Encode audio/video
   */

  void (*write_audio_frame)(void*,gavl_audio_frame_t*,int stream);
  void (*write_video_frame)(void*,gavl_video_frame_t*,int stream);
  
  void (*write_subtitle_text)(void*,const char * text, gavl_time_t start,
                              gavl_time_t duration, int stream);

  void (*write_subtitle_overlay)(void*, gavl_overlay_t * ovl, int stream);
  
  /*
   *  Close it. if do_delete = 1, all output files should be erased
   *  again
   */

  void (*close)(void*, int do_delete);
  } bg_encoder_plugin_t;


/*******************************************
 * ENCODER Postprocessor
 *******************************************/
typedef struct
  {
  void (*action_callback)(void * data, char * action);
  void (*progress_callback)(void * data, float perc);
  void * data;
  } bg_e_pp_callbacks_t;

typedef struct bg_encoder_pp_plugin_s
  {
  bg_plugin_common_t common;

  /* Maximum number of audio/video streams. -1 means infinite */

  int max_audio_streams;
  int max_video_streams;

  /* Set callbacks */

  void (*set_callbacks)(void*,bg_e_pp_callbacks_t*);
  
  /* Initialize */
  
  int (*init)(void *);

  /* Add a transcoded track */
  void (*add_track)(void*, const char * filename,
                    bg_metadata_t * metadata);

  /* Run can be a long operation, it should be called from a separate
     thread launched by the application and the callbacks should be
     used for progress reporting */
  void (*run)(void*, const char * directory, int cleanup);

  /* Stop a previously called run() function. This MUST (of course)
     be thread save */
  void (*stop)(void*);
  } bg_encoder_pp_plugin_t;


/*******************************************
 * Image reader
 *******************************************/

typedef struct bg_image_reader_plugin_s
  {
  bg_plugin_common_t common;

  /*
   * Read the file header, return format (Framerate will be undefined)
   * Return FALSE on error
  */
  
  int (*read_header)(void * priv, const char * filename,
                     gavl_video_format_t * format);
  /*
   *  Read the image, cleanup after so read_header can be calles
   *  again after that. If frame == NULL, do cleanup only
   */
  int (*read_image)(void * priv, gavl_video_frame_t * frame);
  } bg_image_reader_plugin_t;

/*******************************************
 * Image writer
 *******************************************/

typedef struct bg_image_writer_plugin_s
  {
  bg_plugin_common_t common;

  /* Return the file extension */

  const char * (*get_extension)(void * priv);
  
  /*
   *  Write the file header.
   *  Return FALSE on error
   */
  
  int (*write_header)(void * priv, const char * filename,
                      gavl_video_format_t * format);
  
  /*
   *  Read the image, cleanup after so read_header can be calles
   *  again after that. If frame == NULL, do cleanup only
   */
  int (*write_image)(void * priv, gavl_video_frame_t * frame);
  } bg_image_writer_plugin_t;

#endif // __BG_PLUGIN_H_
