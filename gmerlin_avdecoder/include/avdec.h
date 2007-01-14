/*****************************************************************
 
  avdec.h
 
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

/* Public entry points */

/**
 * @file avdec.h
 * external api header.
 */

#include <gavl/gavl.h>

/** \defgroup decoding Decoding of multimedia streams
 *
 *  Setting up a decoder is a multi-step process. It might be a bit complicated,
 *  but it provides support for many different multimedia sources through a single
 *  API.
 *
 *  First you need to create a decoder instance (see \ref decoder). Then you might want to
 *  change some options and set callbacks (see \ref options). The opening can happen with
 *  different functions (see \ref opening). If you also open URLs, it can happen, that
 *  you opened a redirector (see \ref redirector).
 *
 *  If you have no redirector, the decoder can have an arbitrary number of tracks. You
 *  can query various informations about the tracks and must select the track you want
 *  (see \ref track).
 *
 *  Once a track is opened, you must set up the streams (See \ref streams). To tell the decoder,
 *  that you are done, call \ref bgav_start. This will start all codecs. Once the codecs are initialized,
 *  you can get informations about the streams (see \ref stream_info).
 *  Then you are ready to decode media frames (see \ref decode).
 *  In many streams you can also seek  (see \ref seeking).
 *  
 */ 

/** \defgroup decoder Creating and destroying a decoder
 *  \ingroup decoding
 */

/*! \ingroup decoder
 * \brief Opaque decoder structure
 *
 * You don't want to know, what's inside here
 */

typedef struct bgav_s bgav_t;

/*! \ingroup decoder
 * \brief Create a decoder instance
 * \returns A newly allocated decoder instance
 */

bgav_t * bgav_create();

/** \defgroup options Configuration of a decoder
 *  \ingroup decoding
 *
 *  These functions are used to configure a decoder and to set callback functions.
 *  After you created a \ref bgav_t object, get it's options
 *  with \ref bgav_get_options. All options are returned in an option container
 *  (\ref bgav_options_t), and the bgav_options_set_* functions allow you
 *  to change single options. 
 *  As an alternative, you can create an independent option container with
 *  \ref bgav_options_create, change it as you like and use \ref bgav_options_copy to
 *  copy them to the container you got with \ref bgav_get_options.
 *
 *  All options become valid with (and should not be changed after) the
 *  call to one of the bgav_open* functions.
 */

/** \defgroup opening Opening a media stream
 *  \ingroup decoding
 *  \brief Various methods of opening a media stream
 */

/** \defgroup metadata Metadata
 */

/** \ingroup metadata
 * \brief Opaque metadata container
 *
 * This structure is returned by \ref bgav_get_metadata
 */

typedef struct bgav_metadata_s bgav_metadata_t;

/** \ingroup metadata
 * \brief Get the author (or composer) of the track
 * \param metadata Metadata container
 * \returns The author of the track in UTF-8 or NULL
 */

const char * bgav_metadata_get_author(const bgav_metadata_t*metadata);

/** \ingroup metadata
 * \brief Get the title of the track
 * \param metadata Metadata container
 * \returns The title of the track in UTF-8 or NULL
 */

const char * bgav_metadata_get_title(const bgav_metadata_t * metadata);

/** \ingroup metadata
 * \brief Get an additional comment of the track
 * \param metadata Metadata container
 * \returns The comment in UTF-8 or NULL
 */

const char * bgav_metadata_get_comment(const bgav_metadata_t * metadata);

/** \ingroup metadata
 * \brief Get the copyright notice of the track
 * \param metadata Metadata container
 * \returns The copyright notice in UTF-8 or NULL
 */

const char * bgav_metadata_get_copyright(const bgav_metadata_t * metadata);

/** \ingroup metadata
 * \brief Get the album this track comes from
 * \param metadata Metadata container
 * \returns The album in UTF-8 or NULL
 */
const char * bgav_metadata_get_album(const bgav_metadata_t * metadata);

/** \ingroup metadata
 * \brief Get the artist (or performer) of this track
 * \param metadata Metadata container
 * \returns The artist in UTF-8 or NULL
 */
const char * bgav_metadata_get_artist(const bgav_metadata_t * metadata);

/** \ingroup metadata
 * \brief Get the genre this track belongs to
 * \param metadata Metadata container
 * \returns The genre in UTF-8 or NULL
 */

const char * bgav_metadata_get_genre(const bgav_metadata_t * metadata);

/** \ingroup metadata
 * \brief Get the date of the recording
 * \param metadata Metadata container
 * \returns The date in UTF-8 or NULL
 */

const char * bgav_metadata_get_date(const bgav_metadata_t * metadata);

/** \ingroup metadata
 * \brief Get the track index
 * \param metadata Metadata container
 * \returns The track index or 0
 */

int bgav_metadata_get_track(const bgav_metadata_t * metadata);

/***************************************************
 * Housekeeping Functions
 ***************************************************/

/***************************************************
 * Set parameters
 ***************************************************/


/** \ingroup options
 * \brief Opaque option container
 */

typedef struct bgav_options_s bgav_options_t;

/** \ingroup options
 * \brief Get the options of a decoder instance
 * \returns Options
 *
 * Use this to get the options container. You can use the bgav_options_set_* functions
 * to change the options. Options will become valid when you call one of the
 * bgav_open*() functions.
 */

bgav_options_t * bgav_get_options(bgav_t * bgav);

/** \ingroup options
 *  \brief Create an options container
 *  \returns A newly allocated option container
 *
 * This function returns a newly allocated option container
 * filled with default values. Use this, if you need to store
 * decoder options independently from decoder instances.
 * To destroy the container again, use \ref bgav_options_destroy.
 *
 * To pass the options to a decoder structure, use \ref bgav_get_options
 * and \ref bgav_options_copy.
 *
 */
bgav_options_t * bgav_options_create();

/** \ingroup options
 *  \brief Destroy option cotainer.
 *  \param opt option cotainer
 *
 * Use the only to destroy options, you created with \ref bgav_options_create.
 * Options returned by \ref bgav_get_options are owned by the \ref bgav_t instance,
 * and must not be freed by you.
 */

void bgav_options_destroy(bgav_options_t * opt);

/** \ingroup options
 *  \brief Copy options
 *  \param dst Destination
 *  \param src Source
 */

void bgav_options_copy(bgav_options_t * dst, const bgav_options_t * src);

/** \ingroup options
 *  \brief Set connect timeout
 *  \param opt Option container
 *  \param timeout Timeout in milliseconds.
 *
 *  This timeout will be used until the connection is ready to
 *  receive media data.
 */

void bgav_options_set_connect_timeout(bgav_options_t * opt, int timeout);

/** \ingroup options
 *  \brief Set read timeout
 *  \param opt Option container
 *  \param timeout Timeout in milliseconds.
 *
 *  This timeout will be used during streaming to detect server/connection
 *  failures.
 */

void bgav_options_set_read_timeout(bgav_options_t * opt, int timeout);

/** \ingroup options
 *  \brief Set network bandwidth
 *  \param opt Option container
 *  \param bandwidth Bandwidth of your internet connection (in bits per second)
 *
 *  Some network protocols allow
 *  to select among multiple streams according to the speed of the internet connection.
 */

void bgav_options_set_network_bandwidth(bgav_options_t * opt, int bandwidth);

/** \ingroup options
 *  \brief Set network buffer size
 *  \param opt Option container
 *  \param size Buffer size in bytes
 *
 *  Set the size of the network buffer.
 *  \todo Make buffer size depend on the stream bitrate.
 */

void bgav_options_set_network_buffer_size(bgav_options_t * opt, int size);

/* HTTP Options */

/** \ingroup options
 *  \brief Set proxy usage
 *  \param opt Option container
 *  \param enable Set 1 of you use a http proxy, 0 else.
 *
 *  Set usage of a http proxy. If you enable proxies, you must specify the proxy
 *  host with \ref bgav_options_set_http_proxy_host and the proxy port with
 *  \ref bgav_options_set_http_proxy_port.
 */

void bgav_options_set_http_use_proxy(bgav_options_t* opt, int enable);

/** \ingroup options
 *  \brief Set proxy host
 *  \param opt Option container
 *  \param host Hostname of the proxy.
 *
 *  Note that you must enable proxies with \ref bgav_options_set_http_use_proxy before
 *  it's actually used.
 */

void bgav_options_set_http_proxy_host(bgav_options_t* opt, const char * host);

/** \ingroup options
 *  \brief Set proxy port
 *  \param opt Option container
 *  \param port Port of the proxy.
 *
 *  Note that you must enable proxies with \ref bgav_options_set_http_use_proxy before
 *  it's actually used.
 */

void bgav_options_set_http_proxy_port(bgav_options_t* opt, int port);

/** \ingroup options
 *  \brief Enable or disable proxy authentication.
 *  \param opt Option container
 *  \param enable Set 1 if your http proxy needs authentication, 0 else.
 *
 *  If you enable http proxy authentication, you must specify the username
 *  host with \ref bgav_options_set_http_proxy_user and the proxy password with
 *  \ref bgav_options_set_http_proxy_pass.
 */

void bgav_options_set_http_proxy_auth(bgav_options_t* opt, int enable);

/** \ingroup options
 *  \brief Set proxy username
 *  \param opt Option container
 *  \param user The username for the proxy.
 *
 *  Note that you must enable proxy authentication with \ref bgav_options_set_http_proxy_auth.
 */

void bgav_options_set_http_proxy_user(bgav_options_t* opt, const char * user);

/** \ingroup options
 *  \brief Set proxy password
 *  \param opt Option container
 *  \param pass The password for the proxy.
 *
 *  Note that you must enable proxy authentication with \ref bgav_options_set_http_proxy_auth.
 */

void bgav_options_set_http_proxy_pass(bgav_options_t* opt, const char * pass);

/** \ingroup options
 *  \brief Enable or disable shoutcast metadata streaming
 *  \param opt Option container
 *  \param enable Set to 1 if the decoder should try to get shoutcast metadata, 0 else
 *
 * Normally it's ok to enable shoutcast metadata streaming even if we connect to non-shoutcast servers.
 */

void bgav_options_set_http_shoutcast_metadata(bgav_options_t* opt, int enable);

/* Set FTP options */

/** \ingroup options
 *  \brief Enable or disable anonymous ftp login
 *  \param opt Option container
 *  \param enable Set to 1 if the decoder should try log anonymously into ftp servers, 0 else
 */

void bgav_options_set_ftp_anonymous(bgav_options_t* opt, int enable);

/** \ingroup options
 *  \brief Set anonymous password
 *  \param opt Option container
 *  \param pass
 *
 *  Note that you must enable anonymous ftp login with \ref bgav_options_set_ftp_anonymous.
 */

void bgav_options_set_ftp_anonymous_password(bgav_options_t* opt, const char* pass);

/** \ingroup options
 *  \brief Set default subtitle encoding
 *  \param opt Option container
 *  \param encoding Encoding
 *
 *  This sets the default encoding for text subtitles, when the right
 *  encoding is unknown. It must be a character set name recognized by
 *  iconv. Type "iconv -l" at the commandline for a list of supported
 *  encodings).
 */

void bgav_options_set_default_subtitle_encoding(bgav_options_t* opt,
                                                const char* encoding);

/** \ingroup options
 *  \brief Handle DVD chapters as tracks
 *  \param opt Option container
 *  \param chapters_as_tracks 1 to handle DVD chapters like individual tracks, 0 else
 */

void bgav_options_set_dvd_chapters_as_tracks(bgav_options_t* opt,
                                             int chapters_as_tracks);

/** \ingroup options
 *  \brief Enable dynamic range control
 *  \param opt Option container
 *  \param audio_dynrange 1 for enabling dynamic range control.
 *
 *  This enables dynamic range control for codecs, which supports it.
 *  By default dynamic range control is enabled. Use this function to switch it
 *  off for the case, that you have a better dynamic range control in your processing pipe.
 */

void bgav_options_set_audio_dynrange(bgav_options_t* opt,
                                     int audio_dynrange);


/** \ingroup options
 *  \brief Enable seamless playback
 *  \param opt Option container
 *  \param seamless 1 for enabling seamless playback
 *  \note This function does nothing for now
 *
 *  If a source has multiple tracks, we can sometimes switch to the
 *  next track seamlessly (i.e. without closing and reopening the
 *  codecs). If you set this option, the decoder will not signal EOF at the
 *  end of a track if a seamless transition to the next track is possible.
 *  Instead, the track change is signalled with the
 *  \ref bgav_track_change_callback (if available) and decoding continues.
 *  \todo Seamless playback isn't implemented yet. This function might be removed in future versions.
 */

void bgav_options_set_seamless(bgav_options_t* opt,
                               int seamless);

/** \ingroup options
 *  \brief Enable external subtitle files
 *  \param opt Option container
 *  \param seek_subtitles If 1, subtitle files will be seeked for video files. If 2, subtitles
        will be seeked for all files.
 *
 *  If the input is a regular file, gmerlin_avdecoder can scan the
 *  directory for matching subtitle files. For a file movie.mpg, possible
 *  subtitle files are e.g. movie_english.srt, movie_german.srt. The
 *  rule is, that the first part of the filename of the subtitle file
 *  must be equal to the movie filename up to the extension (hiere: .mpg).
 *  Furthermore, the subtitle filename must have an extension supported by
 *  any of the subtitle readers.
 */

void bgav_options_set_seek_subtitles(bgav_options_t* opt,
                                    int seek_subtitles);

/** \ingroup options
 *  \brief Set postprocessing level
 *  \param opt Option container
 *  \param Value between 0 (no postprocessing) and 6 (maximum postprocessing)
 */

void bgav_options_set_pp_level(bgav_options_t* opt,
                               int pp_level);

/** \ingroup options
 *  \brief Set DVB channels file
 *  \param opt Option container
 *  \param Name of the channel configurations file
 *
 *  The channels file must have the format of the dvb-utils
 *  programs (like szap, tzap). If you don't set this file,
 *  several locations like $HOME/.tzap/channels.conf will be
 *  searched.
 */

void bgav_options_set_dvb_channels_file(bgav_options_t* opt,
                                        const char * file);


/** \ingroup options
 *  \brief Enumeration for log levels
 *
 * These will be called from within log callbacks
 */

typedef enum
  {
    BGAV_LOG_DEBUG,
    BGAV_LOG_WARNING,
    BGAV_LOG_ERROR,
    BGAV_LOG_INFO
  } bgav_log_level_t;

/** \ingroup options
 *  \brief Function to be called for loggins messages
 *  \param data The data you passed to \ref bgav_options_set_log_callback.
 *  \param level The log level
 *  \param log_domain A string describing the module from which the message comes
 *  \param message The message itself
 */
 
typedef void (*bgav_log_callback)(void*data, bgav_log_level_t level,
                                  const char * log_domain,
                                  const char * message);

/** \ingroup options
 *  \brief Set the callback for log messages
 *  \param opt Option container
 *  \param callback The callback
 *  \param data Some data you want to get passed to the callback
 */

void
bgav_options_set_log_callback(bgav_options_t* opt,
                              bgav_log_callback callback,
                              void * data);


/* Set callbacks */

/** \ingroup options
 *  \brief Function to be called if the track name changes
 *  \param data The data you passed to \ref bgav_options_set_name_change_callback.
 *  \param name The new name of the track
 *
 *  This function will be called whenever the name of the track changed. Such changes
 *  can have different reasons, the most obvious one is a song change in a webradio stream.
 */
 
typedef void (*bgav_name_change_callback)(void*data, const char * name);

/** \ingroup options
 *  \brief Set the callback for name change events
 *  \param opt Option container
 *  \param callback The callback
 *  \param data Some data you want to get passed to the callback
 */

void
bgav_options_set_name_change_callback(bgav_options_t* opt,
                                      bgav_name_change_callback callback,
                                      void * data);

/** \ingroup options
 *  \brief Function to be called if the metadata change
 *  \param data The data you passed to \ref bgav_options_set_metadata_change_callback.
 *  \param metadata The new metadata of the track
 *
 *  This function will be called whenever the metadata of the track changed. Such changes
 *  can have different reasons, the most obvious one is a song change in a webradio stream.
 */


typedef void (*bgav_metadata_change_callback)(void*data, const bgav_metadata_t * metadata);

/** \ingroup options
 *  \brief Set the callback for metadata change events
 *  \param opt Option container
 *  \param callback The callback
 *  \param data Some data you want to get passed to the callback
 */

void
bgav_options_set_metadata_change_callback(bgav_options_t* opt,
                                          bgav_metadata_change_callback callback,
                                          void * data);

/** \ingroup options
 *  \brief Function to be called if the track number changes
 *  \param data The data you passed to \ref bgav_options_set_track_change_callback.
 *  \param track The new track number
 *
 *  This function will be called if the decoder has multiple tracks, but allows
 *  seamless playback of the tracks and a track change occurred.
 *
 */

typedef void (*bgav_track_change_callback)(void*data, int track);

/** \ingroup options
 *  \brief Set the callback for track change events
 *  \param opt Option container
 *  \param callback The callback
 *  \param data Some data you want to get passed to the callback
 */

void
bgav_options_set_track_change_callback(bgav_options_t* opt,
                                       bgav_track_change_callback callback,
                                       void * data);

/** \ingroup options
 *  \brief Function to be called if the input module is buffering data
 *  \param data The data you passed to \ref bgav_options_set_buffer_callback.
 *  \param percentage The percentage done so far (0.0 .. 1.0)
 *
 *  This function will be called when the input module fills a buffer.
 *  It is called multiple times with increasing percentage.
 */

typedef void (*bgav_buffer_callback)(void*data, float percentage);

/** \ingroup options
 *  \brief Set the callback for buffering notification
 *  \param opt Option container
 *  \param callback The callback
 *  \param data Some data you want to get passed to the callback
 */
  
void
bgav_options_set_buffer_callback(bgav_options_t* opt,
                                 bgav_buffer_callback callback,
                                 void * data);

/** \ingroup options
 *  \brief Function to be called if the input module needs authentication data
 *  \param data The data you passed to \ref bgav_options_set_user_pass_callback.
 *  \param resource A string describing the resource (e.g. the hostname of the server)
 *  \param username Returns the username.
 *  \param password Returns the password.
 *  \returns 1 of the username and password are valid, 0 if the user cancelled the authentication.
 *
 *  This is called during one of the bgav_open* calls.
 */

typedef int (*bgav_user_pass_callback)(void*data, const char * resource,
                                       char ** username, char ** password);
  
/** \ingroup options
 *  \brief Set the callback for user authentication
 *  \param opt Option container
 *  \param callback The callback
 *  \param data Some data you want to get passed to the callback
 *
 *  Note that there is no default authentication built into the library.
 *  Therefore you MUST set a callback, otherwise urls which require the user to
 *  enter authentication data, cannot be openend.
 */

void
bgav_options_set_user_pass_callback(bgav_options_t* opt,
                                    bgav_user_pass_callback callback,
                                    void * data);

/** \ingroup options
 *  \brief Function to be called if a change of the aspect ratio was detected
 *  \param data The data you passed to \ref bgav_options_set_user_pass_callback.
 *  \param stream Index of the video stream (starts with 0)
 *  \param pixel_width New pixel width
 *  \param pixel_height New pixel height
 *
 *  This is called during \ref bgav_read_video.
 */

typedef void (*bgav_aspect_callback)(void*data, int stream,
                                     int pixel_width, int pixel_height);


/** \ingroup options
 *  \brief Set aspect ratio change callback
 *  \param opt Option container
 *  \param callback The callback
 *  \param data Some data you want to get passed to the callback
 */

void
bgav_options_set_aspect_callback(bgav_options_t* opt,
                                 bgav_aspect_callback callback,
                                 void * data);


/* Device description */

/** \defgroup devices Device description
 *  \brief Device description
 *
 *  The input modules, which access special hardware devices (e.g. CD drives),
 *  have autoscanning functionality built in. You can call one of the bgav_find_devices_* functions
 *  to get a list of supported devices (see \ref bgav_device_info_t). The created list must be
 *  freed with \ref bgav_device_info_destroy when it's no longer used.
 */

/** \ingroup devices
 *  \brief Info structure for a device.
 *
 * This contains information about a device: The device node name (which can be passed to
 * the bgav_open_* function) and, if available, a human readable description. The last element
 * in the array has the device string set to NULL and terminates the array.
 */

typedef struct
  {
  char * device; /*!< Can be passed to the bgav_open_*() function */
  char * name;   /*!< Device name, might be NULL */
  } bgav_device_info_t;

/* Scan for devices */

/** \ingroup devices
 *  \brief Scan for VCD capable devices
 *  \returns A \ref bgav_device_info_t array or NULL
 *
 *  Free the returned array with \ref bgav_device_info_destroy
 */

bgav_device_info_t * bgav_find_devices_vcd();

/** \ingroup devices
 *  \brief Test if a device is VCD capable
 *  \param device The device node name
 *  \param name Returns a human readable decription in a newly allocated string or NULL
 *  \returns 1 if the device can play VCDs, 0 else.
*/

int bgav_check_device_vcd(const char * device, char ** name);

/** \ingroup devices
 *  \brief Scan for DVD capable devices
 *  \returns A \ref bgav_device_info_t array or NULL
 *
 *  Free the returned array with \ref bgav_device_info_destroy
 */

bgav_device_info_t * bgav_find_devices_dvd();

/** \ingroup devices
 *  \brief Test if a device is DVD capable
 *  \param device The device node name
 *  \param name Returns a human readable decription in a newly allocated string or NULL
 *  \returns 1 if the device can play DVDs, 0 else.
*/

int bgav_check_device_dvd(const char * device, char ** name);

/** \ingroup devices
 *  \brief Scan for DVB capable devices
 *  \returns A \ref bgav_device_info_t array or NULL
 *
 *  Free the returned array with \ref bgav_device_info_destroy
 */

bgav_device_info_t * bgav_find_devices_dvb();

/** \ingroup devices
 *  \brief Test if a device is DVB capable
 *  \param device The directory (e.g. /dev/dvb/adaptor0)
 *  \param name Returns a human readable decription in a newly allocated string or NULL
 *  \returns 1 if the device is ready to receive DVB streams, 0 else.
*/

int bgav_check_device_dvb(const char * device, char ** name);

/** \ingroup devices
 *  \brief Destroy a device info array
 *  \param arr A device info returned by \ref bgav_find_devices_dvd, \ref bgav_find_devices_dvb or \ref bgav_find_devices_dvd
 */


void bgav_device_info_destroy(bgav_device_info_t * arr);

/** \ingroup devices
 *  \brief Eject a disc
 *  \param device Device name
 *  \return 1 if the disc could be ejected, 0 else
 */

int bgav_eject_disc(const char * device);

/** \ingroup devices
 *  \brief Get the name of a disc
 *  \param bgav A decoder instance
 *  \return The name of the disc, or NULL if it's not known or irrelevant
 */

const char * bgav_get_disc_name(bgav_t * bgav);

/******************************************************
 * Open
 ******************************************************/

/* Open a file or URL, return 1 on success */

/** \ingroup opening
 *  \brief Open a file or URL
 *  \param bgav A decoder instance
 *  \param location The URL or path to open
 *  \returns 1 if the location was successfully openend, 0 else.
 */

int bgav_open(bgav_t * bgav, const char * location);

/** \ingroup opening
 *  \brief Open a VCD device
 *  \param bgav A decoder instance
 *  \param location The device node
 *  \returns 1 if the VCD device was successfully openend, 0 else.
 */

int bgav_open_vcd(bgav_t * bgav, const char * location);

/** \ingroup opening
 *  \brief Open a DVD device
 *  \param bgav A decoder instance
 *  \param location The device node
 *  \returns 1 if the DVD device was successfully openend, 0 else.
 */

int bgav_open_dvd(bgav_t * bgav, const char * location);

/** \ingroup opening
 *  \brief Open a DVB device
 *  \param bgav A decoder instance
 *  \param location The device directory
 *  \returns 1 if the DVB device was successfully openend, 0 else.
 *
 *  This function will search your system for channel configuration files,
 *  which are created by other tools. The channels are then available as
 *  normal tracks.
 */

int bgav_open_dvb(bgav_t * bgav, const char * location);


/** \ingroup opening
 *  \brief Open a decoder from a filedescriptor
 *  \param bgav A decoder instance
 *  \param fd The filedescriptor
 *  \param total_size The total number of available bytes or 0 if this info is not known.
 *  \param mimetype The mimetype of the input or NULL if this info is not known.
 *  \returns 1 if the filedescriptor was successfully openend, 0 else.
 */

int bgav_open_fd(bgav_t * bgav, int fd,
                 int64_t total_size,
                 const char * mimetype);

/**\ingroup decoder
 * \brief Get an error description
 * \param bgav A decoder instance
 * \returns A description of the error which occurred (or NULL if no desciption is available).
 *
 * This function can be called after a failed call of one of the open functions (see \ref opening).
 */

const char * bgav_get_error(bgav_t * bgav);

/* Close and destroy everything */

/** \ingroup decoder
 *  \brief Close a decoder and free all associated memory
 *  \param bgav A decoder instance
 */

void bgav_close(bgav_t * bgav);

/***************************************************
 * Check for redirecting: You MUST check if you opened
 * a redirector, because reading data from redirectors
 * crashes
 * After you read the URLs, close the bgav_t object
 * and open a new one with one of the URLs.
 ***************************************************/

/** \defgroup redirector Redirector support
 *  \ingroup decoding
 *  \brief Redirector support
 *
 *  After opening the decoder with \ref bgav_open, you must check if the
 *  opened stream is actually a redirector with \ref bgav_is_redirector.
 *  If this returns 1, get the number of urls with \ref bgav_redirector_get_num_urls.
 *  Then, for each URL, get the address with \ref bgav_redirector_get_url and the
 *  title of the URL with \ref bgav_redirector_get_name.
 *  After you have this info, close and destroy the decoder with \ref bgav_close and
 *  open a newly created decoder with one of the URLs you got.
 */

/** \ingroup redirector
 *  \brief Query if the decoder opened a redirector
 *  \param bgav A decoder instance
 *  \returns 1 if the decoder opened a redirector, 0 else
 *
 *  This function MUST be called after \ref bgav_open to query if we
 *  openend a redirector.
 */

int bgav_is_redirector(bgav_t * bgav);

/** \ingroup redirector
 *  \brief Get the number of URLs found in the redirector
 *  \param bgav A decoder instance
 *  \returns The number of URLs.
 */

int bgav_redirector_get_num_urls(bgav_t * bgav);

/** \ingroup redirector
 *  \brief Get the address of an URL
 *  \param bgav A decoder instance
 *  \param index Index of the url (starting with 0)
 *  \returns The URL (can be passed to a subsequent \ref bgav_open)
 */

const char * bgav_redirector_get_url(bgav_t * bgav, int index);

/** \ingroup redirector
 *  \brief Get the address of an URL
 *  \param bgav A decoder instance
 *  \param index Index of the url (starting with 0)
 *  \returns The name of the stream or NULL if this information is not present.
 */

const char * bgav_redirector_get_name(bgav_t * bgav, int index);

/***************************************************
 * Get information about the file
 ***************************************************/

/** \defgroup track Query and select tracks
 *  \ingroup decoding
 *
 * Each opened decoder can have multiple tracks. You must select the track you wish to decode before
 * you can do anything else.
 */

/** \ingroup track
 *  \brief Get the number of tracks
 *  \param bgav A decoder instance
 *  \returns The number of tracks.
 */

int bgav_num_tracks(bgav_t * bgav);

/** \ingroup track
 *  \brief Get a technical description of the format
 *  \param bgav A decoder instance
 *  \returns Description
 */

const char * bgav_get_description(bgav_t * bgav);

/** \ingroup track
 *  \brief Get the duration of a track
 *  \param bgav A decoder instance
 *  \param track Track index (starting with 0)
 *  \returns The duration of a track or \ref GAVL_TIME_UNDEFINED if the duration is not known.
 */

gavl_time_t bgav_get_duration(bgav_t * bgav, int track);

/* Query stream numbers */

/** \ingroup track
 *  \brief Get the number of audio streams of a track
 *  \param bgav A decoder instance
 *  \param track Track index (starting with 0)
 *  \returns The number of audio streams
 */

int bgav_num_audio_streams(bgav_t * bgav, int track);

/** \ingroup track
 *  \brief Get the number of video streams of a track
 *  \param bgav A decoder instance
 *  \param track Track index (starting with 0)
 *  \returns The number of video streams
 */

int bgav_num_video_streams(bgav_t * bgav, int track);

/** \ingroup track
 *  \brief Get the number of subtitle streams of a track
 *  \param bgav A decoder instance
 *  \param track Track index (starting with 0)
 *  \returns The number of subtitle streams
 */

int bgav_num_subtitle_streams(bgav_t * bgav, int track);


/** \ingroup track
 *  \brief Get the name a track
 *  \param bgav A decoder instance
 *  \param track Track index (starting with 0)
 *  \returns The track name if present or NULL.
 */

const char * bgav_get_track_name(bgav_t * bgav, int track);

/** \ingroup track
 *  \brief Get metadata for a track.
 *  \param bgav A decoder instance
 *  \param track Track index (starts with 0)
 *  \returns A metadata container (see \ref metadata)
 */

const bgav_metadata_t * bgav_get_metadata(bgav_t * bgav,int track);

/** \ingroup track
 *  \brief Select a track
 *  \param bgav A decoder instance
 *  \param track Track index (starts with 0)
 *  \returns 0 if there was no such track, 1 else
 *
 *  Select the track. All subsequent function calls will refer to the track you
 *  selected.
 */


int bgav_select_track(bgav_t * bgav, int track);

/** \ingroup track
 *  \brief Get the number of chapters
 *  \param bgav A decoder instance
 *  \param track Track index (starts with 0)
 *  \param timescale Returns the timescale of the seekpoints
 *  \returns The number of chapters or 0 if the format doesn't support chapters
 *
 *  Chapters are simply named seekpoints. Use
 *  \ref bgav_get_chhapter_time and \ref bgav_get_chapter_name
 *  to query the chapters.
 */

int bgav_get_num_chapters(bgav_t * bgav, int track, int * timescale);

/** \ingroup track
 *  \brief Get the name of a chapter
 *  \param bgav A decoder instance
 *  \param track Track index (starts with 0)
 *  \returns The name of the chapter or NULL
 */

const char *
bgav_get_chapter_name(bgav_t * bgav, int track, int chapter);

/** \ingroup track
 *  \brief Get the name of a chapter
 *  \param bgav A decoder instance
 *  \param track Track index (starts with 0)
 *  \returns The time of the chapter
 */

int64_t bgav_get_chapter_time(bgav_t * bgav, int track, int chapter);

/** \defgroup streams Query and select streams
 * \ingroup decoding
 *
 * The numbers of streams are already known before selecting a track (see \ref bgav_num_audio_streams and
 * \ref bgav_num_video_streams. Note that by default, all streams are muted, which means that you cannot
 * skip the stream selection in your application.
 */

/** \ingroup streams
 *  \brief Get the language of an audio stream
 *  \param bgav A decoder instance
 *  \param stream Audio stream index (starting with 0)
 *  \returns A language string.
 */

const char * bgav_get_audio_language(bgav_t * bgav, int stream);

/** \ingroup streams
 *  \brief Get the language of an audio stream
 *  \param bgav A decoder instance
 *  \param stream Subtitle stream index (starting with 0)
 *  \returns A language string.
 */

const char * bgav_get_subtitle_language(bgav_t * bgav, int stream);

/** \ingroup streams
 */

typedef enum
  {
    BGAV_STREAM_MUTE = 0,  /*!< Stream is switched off */
    BGAV_STREAM_DECODE = 1 /*!< Stream is switched on and will be decoded */
  }
bgav_stream_action_t;

/** \ingroup streams
 * \brief Select mode for an audio stream
 * \param bgav A decoder instance
 * \param stream Stream index (starting with 0)
 * \param action The stream action.
 *
 * Note that the default stream action is BGAV_STREAM_MUTE, which means that
 * all streams are switched off by default.
 */

int bgav_set_audio_stream(bgav_t * bgav, int stream, bgav_stream_action_t action);

/** \ingroup streams
 * \brief Select mode for a video stream
 * \param bgav A decoder instance
 * \param stream Stream index (starting with 0)
 * \param action The stream action.
 *
 * Note that the default stream action is BGAV_STREAM_MUTE, which means that
 * all streams are switched off by default.
 */

int bgav_set_video_stream(bgav_t * bgav, int stream, bgav_stream_action_t action);

/** \ingroup streams
 * \brief Select mode for a subtitle stream
 * \param bgav A decoder instance
 * \param stream Stream index (starting with 0)
 * \param action The stream action.
 *
 * Note that the default stream action is BGAV_STREAM_MUTE, which means that
 * all streams are switched off by default.
 */

int bgav_set_subtitle_stream(bgav_t * bgav, int stream, bgav_stream_action_t action);

/***************************************************
 * Stream handling functions
 ***************************************************/

/*
 *  You MUST these, if you want to decode anything.
 *  After bgav_open(), all streams are switched off by
 *  default
 */


/** \defgroup start Start codecs
 *  \ingroup decoding
 */

/**  \ingroup start
 *  \brief Start all codecs.
 *  \param bgav A decoder instance
 *  \returns 1 if everything went well, 0 if an error occurred.
 *
 *  Call this function, after you called \ref bgav_set_audio_stream
 *  and/or \ref bgav_set_video_stream.
 */

int bgav_start(bgav_t * bgav);

/** \defgroup stream_info Information about the streams
    \ingroup decoding
 */

/** \ingroup stream_info
 *  \brief Get the format of an audio stream
 *  \param bgav A decoder instance
 *  \param stream Stream index (starting with 0)
 *  \returns The format
 *
 *  Note, that you can trust the return value of this function only, if you enabled
 *  the stream (see \ref bgav_set_audio_stream) and started the decoders
 *  (see \ref bgav_start).
 */

const gavl_audio_format_t * bgav_get_audio_format(bgav_t * bgav, int stream);

/** \ingroup stream_info
 *  \brief Get the format of a video stream
 *  \param bgav A decoder instance
 *  \param stream Stream index (starting with 0)
 *  \returns The format
 *
 *  Note, that you can trust the return value of this function only, if you enabled
 *  the stream (see \ref bgav_set_video_stream) and started the decoders
 *  (see \ref bgav_start).
 */

const gavl_video_format_t * bgav_get_video_format(bgav_t * bgav, int stream);

/** \ingroup stream_info
 *  \brief Get the video format of a subtitle stream
 *  \param bgav A decoder instance
 *  \param stream Stream index (starting with 0)
 *  \returns The format
 *
 *  Note, that you can trust the return value of this function only, if you enabled
 *  the stream (see \ref bgav_set_subtitle_stream) and started the decoders
 *  (see \ref bgav_start). For overlay subtitles, this is the video format of the decoded
 *  overlays. For text subtitles, it's the format of the associated video stream.
 */

const gavl_video_format_t * bgav_get_subtitle_format(bgav_t * bgav, int stream);

/** \ingroup stream_info
 *  \brief Check if a subtitle is text or graphics based
 *  \param bgav A decoder instance
 *  \param stream Stream index (starting with 0)
 *  \returns 1 for text subtitles, 0 for graphic subtitles
 *
 *  If this function returns 1, you must use \ref bgav_read_subtitle_text
 *  to decode subtitles, else use \ref bgav_read_subtitle_overlay
 */

int bgav_subtitle_is_text(bgav_t * bgav, int stream);

/** \ingroup stream_info
 *  \brief Get the description of an audio stream
 *  \param bgav A decoder instance
 *  \param stream Stream index (starting with 0)
 *  \returns A technical decription of the stream 
 *
 *  Note, that you can trust the return value of this function only, if you enabled
 *  the stream (see \ref bgav_set_audio_stream) and started the decoders
 *  (see \ref bgav_start).
 */

const char * bgav_get_audio_description(bgav_t * bgav, int stream);

/** \ingroup stream_info
 *  \brief Get additional info about an audio stream
 *  \param bgav A decoder instance
 *  \param stream Stream index (starting with 0)
 *  \returns An info string about the stream or NULL
 *
 *  This returns additional information about the audio stream e.g. if it contains
 *  directors comments, audio for the visually impaired or whatever.
 *
 *  Note, that you can trust the return value of this function only, if you enabled
 *  the stream (see \ref bgav_set_audio_stream) and started the decoders
 *  (see \ref bgav_start).
 */

const char * bgav_get_audio_info(bgav_t * bgav, int stream);


/** \ingroup stream_info
 *  \brief Get the description of a video stream
 *  \param bgav A decoder instance
 *  \param stream Stream index (starting with 0)
 *  \returns A technical decription of the stream 
 *
 *  Note, that you can trust the return value of this function only, if you enabled
 *  the stream (see \ref bgav_set_video_stream) and started the decoders
 *  (see \ref bgav_start).
 */

const char * bgav_get_video_description(bgav_t * bgav, int stream);

/** \ingroup stream_info
 *  \brief Get the description of a subtitle stream
 *  \param bgav A decoder instance
 *  \param stream Stream index (starting with 0)
 *  \returns A technical decription of the stream 
 *
 *  Note, that you can trust the return value of this function only, if you enabled
 *  the stream (see \ref bgav_set_subtitle_stream) and started the decoders
 *  (see \ref bgav_start).
 */

const char * bgav_get_subtitle_description(bgav_t * bgav, int stream);


/** \ingroup stream_info
 *  \brief Get additional info about a subtitle stream
 *  \param bgav A decoder instance
 *  \param stream Stream index (starting with 0)
 *  \returns An info string about the stream or NULL
 *
 *  This returns additional information about the subtitle stream e.g. if it contains
 *  directors comments or whatever.
 *
 *  Note, that you can trust the return value of this function only, if you enabled
 *  the stream (see \ref bgav_set_subtitle_stream) and started the decoders
 *  (see \ref bgav_start).
 */

const char * bgav_get_subtitle_info(bgav_t * bgav, int stream);



/***************************************************
 * Decoding functions
 ***************************************************/

/** \defgroup decode Decode media frames
 *  \ingroup decoding
 */

/** \ingroup decode
    \brief Decode a video frame
    \param bgav A decoder instance
    \param frame The frame to which the image will be decoded.
    \param stream Stream index (starting with 0)
    \returns 1 if a frame could be decoded, 0 for EOF.
*/

int bgav_read_video(bgav_t * bgav, gavl_video_frame_t * frame, int stream);

/** \ingroup decode
    \brief Decode audio samples
    \param bgav A decoder instance
    \param frame The frame to which the samples will be decoded.
    \param stream Stream index (starting with 0)
    \param num_samples Number of samples to be decoded
    \returns The number of actually decoded samples, which can be smaller than num_samples. 0 means EOF.
*/

int bgav_read_audio(bgav_t * bgav, gavl_audio_frame_t * frame, int stream,
                    int num_samples);

/** \ingroup decode
    \brief Check, if a new subtitle is available
    \param bgav A decoder instance
    \param stream Stream index (starting with 0)
    \returns 1 if a subtitle is available.

    Use this function to check in advance, if it would make sense to call
    \ref bgav_read_subtitle_overlay or \ref bgav_read_subtitle_text.

*/

int bgav_has_subtitle(bgav_t * bgav, int stream);

/** \ingroup decode
    \brief Decode an overlay subtitle
    \param bgav A decoder instance
    \param ovl The overlay to which the subtitle will be decoded.
    \param stream Stream index (starting with 0)
    \returns 1 if a subtitle could be decoded, 0 else

    If this function returns 1, a subtitle was decoded. If this function returns
    0 and \ref bgav_has_subtitle returned 0 before as well, there is no subtitle
    yet available, but there might come others at a later point in the stream.
    If this function returns 0 and \ref bgav_has_subtitle returned 1 before,
    it means, that you reached the end of the subtitle stream.

*/

int bgav_read_subtitle_overlay(bgav_t * bgav, gavl_overlay_t * ovl, int stream);

/** \ingroup decode
    \brief Decode a text subtitle
    \param bgav       A decoder instance
    \param ret        The string, where the text will be stored.
    \param ret_alloc  The number of allocated bytes for ret
    \param start_time Returns the start time
    \param duration   Returns the duration
    \param stream     Stream index (starting with 0)
    \returns 1 if a subtitle could be decoded, 0 else

    If this function returns 1, a subtitle was decoded. If this function returns
    0 and \ref bgav_has_subtitle returned 0 before as well, there is no subtitle
    yet available, but there might come others at a later point in the stream.
    If this function returns 0 and \ref bgav_has_subtitle returned 1 before,
    it means, that you reached the end of the subtitle stream.
*/

int bgav_read_subtitle_text(bgav_t * bgav, char ** ret, int *ret_alloc,
                            gavl_time_t * start_time, gavl_time_t * duration, int stream);

/***************************************************
 * Seek to a timestamp. This also resyncs all streams
 ***************************************************/

/** \defgroup seeking Seeking
 *  \ingroup decoding
 */

/** \ingroup seeking
 *  \brief Check if a track is seekabkle
 *  \param bgav A decoder handle
 *  \returns 1 if the track is seekable, 0 else.
 */

int bgav_can_seek(bgav_t * bgav);

/** \ingroup seeking
 *  \brief Seek to a specific time
 *  \param bgav A decoder handle
 *  \param time The time to seek to. 
 *  \returns 1 if the track is seekable, 0 else.
 *
 * The time argument is changed to the actually seeked time, which can be different.
 */

void bgav_seek(bgav_t * bgav, gavl_time_t * time);

/***************************************************
 * Debugging functions
 ***************************************************/

/** \defgroup debugging Debugging utilities
 */

/** \ingroup debugging
 *  \brief Dump informations of all tracks to stderr
 *  \param bgav A decoder handle
*/

void bgav_dump(bgav_t * bgav);

/* Dump infos about the installed codecs */

/** \ingroup debugging
 *  \brief Dump informations about all available codecs to stderr
 *
 * The output format is html. The main purpose of this function is to generate a feature list
 * for the webpage.
 */

void bgav_codecs_dump();

/* Dump known media formats */

/** \ingroup debugging
 *  \brief Dump informations about all available format demuxers to stderr
 *
 * The output format is html. The main purpose of this function is to generate a feature list
 * for the webpage.
 */

void bgav_formats_dump();

/** \ingroup debugging
 *  \brief Dump informations about all available input modules to stderr
 *
 * The output format is html. The main purpose of this function is to generate a feature list
 * for the webpage.
 */

void bgav_inputs_dump();

/** \ingroup debugging
 *  \brief Dump informations about all available redirectors to stderr
 *
 * The output format is html. The main purpose of this function is to generate a feature list
 * for the webpage.
 */

void bgav_redirectors_dump();

/** \ingroup debugging
 *  \brief Dump informations about all available subtitle readers to stderr
 *
 * The output format is html. The main purpose of this function is to generate a feature list
 * for the webpage.
 */

void bgav_subreaders_dump();
