/*****************************************************************
 
  avdec.h
 
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

/* Public entry points */

/**
 * @file avdec.h
 * external api header.
 */

#include <gavl/gavl.h>

/** \defgroup decoder Decoder
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

/** \defgroup options Decoder configuration
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
 * \brief Opaque options container
 */

typedef struct bgav_options_s bgav_options_t;

/** \ingroup options
 * \brief Get the options of a decoder instance
 * \returns Options
 *
 * Use this to get the options container. You can use the bgav_set_* functions
 * to change the options. Options will become valid when you call one of the
 * bgav_open*() functions.
 */

bgav_options_t * bgav_get_options(bgav_t* bgav);

/*
 * Timeout will only be used for network connections.
 * It is in milliseconds, 0 is infinity
 */

void bgav_set_connect_timeout(bgav_options_t *, int timeout);
void bgav_set_read_timeout(bgav_options_t *, int timeout);

/*
 *  Set network bandwidth (in bits per second)
 */

void bgav_set_network_bandwidth(bgav_options_t *, int bandwidth);
void bgav_set_network_buffer_size(bgav_options_t *, int size);

/* HTTP Options */

void bgav_set_http_use_proxy(bgav_options_t*, int use_proxy);
void bgav_set_http_proxy_host(bgav_options_t*, const char *);
void bgav_set_http_proxy_port(bgav_options_t*, int);
void bgav_set_http_shoutcast_metadata(bgav_options_t*, int);

/* Set FTP options */

void bgav_set_ftp_anonymous_password(bgav_options_t*, const char*);

/* Set callbacks */

void
bgav_set_name_change_callback(bgav_t*,
                              void (callback)(void*data, const char * name),
                              void * data);

void
bgav_set_metadata_change_callback(bgav_t*,
                                  void (callback)(void*data, const bgav_metadata_t * name),
                                  void * data);

void
bgav_set_track_change_callback(bgav_t*,
                               void (callback)(void*data, int track),
                               void * data);

void
bgav_set_buffer_callback(bgav_t*,
                         void (callback)(void*data, float percentage),
                         void * data);

/* These will become active, when the next file is opened */

void bgav_set_dll_path_real(const char * path);
void bgav_set_dll_path_xanim(const char * path);
void bgav_set_dll_path_win32(const char * path);

/* Device description */

/*
 *  Device info: Returned by the "find_devices()" function
 *  Device arrays are NULL terminated
 */

typedef struct
  {
  char * device; /* Can be passed to the open() method */
  char * name;   /* Might be NULL */
  } bgav_device_info_t;

void bgav_device_info_destroy(bgav_device_info_t * arr);

/* Scan for devices */

bgav_device_info_t * bgav_find_devices_vcd();
int bgav_check_device_vcd(const char * device, char ** name);

/******************************************************
 * Open
 ******************************************************/

/* Open a file or URL, return 1 on success */

int bgav_open(bgav_t *, const char * location);

/* Open VCD Device */

int bgav_open_vcd(bgav_t *, const char * location);

/*
 *  Open with existing file descriptor (e.g. http socket)
 *  total size must be 0 if unknown
 */

int bgav_open_fd(bgav_t *, int fd,
                 int64_t total_size,
                 const char * mimetype);

/* If either open call fails, get the reason with: */

const char * bgav_get_error(bgav_t *);

/* Close and destroy everything */

void bgav_close(bgav_t *);

/***************************************************
 * Check for redirecting: You MUST check if you opened
 * a redirector, because reading data from redirectors
 * crashes
 * After you read the URLs, close the bgav_t object
 * and open a new one with one of the URLs.
 ***************************************************/

int bgav_is_redirector(bgav_t*);

int bgav_redirector_get_num_urls(bgav_t*);
const char * bgav_redirector_get_url(bgav_t*, int);
const char * bgav_redirector_get_name(bgav_t*, int);

/***************************************************
 * Get information about the file
 ***************************************************/

/*
 *  Multiple tracks support
 *  (like VCD tracks, ALBW titles and so on)
 */

int bgav_num_tracks(bgav_t *);

/* Return duration, will be 0 if unknown */

gavl_time_t bgav_get_duration(bgav_t*, int track);

/* Query stream numbers */

int bgav_num_audio_streams(bgav_t *, int track);
int bgav_num_video_streams(bgav_t *, int track);

const char * bgav_get_track_name(bgav_t *, int track);

/* Returns TRUE if the track is seekable */

int bgav_can_seek(bgav_t *);


const bgav_metadata_t * bgav_get_metadata(bgav_t*,int track);

void bgav_select_track(bgav_t *, int);

/*
 * Get formats
 * NOTE: The returned formats aren't completely valid, because
 *       some informations arent available before the decoder
 *       is started. It is wise to call these functions only
 *       after bgav_start_decoders() (see below), for streams,
 *       which are switched on by calling bgav_set_audio_stream()
 *       or bgav_set_video_stream() with the arguments
 *       BGAV_STREAM_SYNC or BGAV_STREAM_DECODE.
 */

const gavl_audio_format_t * bgav_get_audio_format(bgav_t*, int stream);
const gavl_video_format_t * bgav_get_video_format(bgav_t*, int stream);

/*
 *  Get textual description of single streams as well as
 *  the total track
 */

const char * bgav_get_audio_description(bgav_t * b, int stream);
const char * bgav_get_video_description(bgav_t * b, int stream);
const char * bgav_get_description(bgav_t * b);

/***************************************************
 * Stream handling functions
 ***************************************************/

/*
 *  You MUST these, if you want to decode anything.
 *  After bgav_open(), all streams are switched off by
 *  default
 */

/*
 *  Set action for a particular stream:
 *  BGAV_STREAM_MUTE: Stream is switched off
 *  BGAV_STREAM_SYNC: Stream is not decoded but kept sync
 *  BGAV_STREAM_DECODE: Stream is decoded
 */  

#define BGAV_STREAM_MUTE         0
#define BGAV_STREAM_SYNC         1
#define BGAV_STREAM_DECODE       2

int bgav_set_audio_stream(bgav_t*, int stream, int action);
int bgav_set_video_stream(bgav_t*, int stream, int action);

/*
 *  Call this function, after you called bgav_set_audio_stream()
 *  and/or bgav_set_video_stream(). After that, you can get the
 *  formats (see above)
 */

int bgav_start(bgav_t *);

/***************************************************
 * Decoding functions
 ***************************************************/

int bgav_read_video(bgav_t *, gavl_video_frame_t * frame, int stream);
int bgav_read_audio(bgav_t *, gavl_audio_frame_t * frame, int stream,
                    int num_samples);

/***************************************************
 * Seek to a timestamp. This also resyncs all streams
 ***************************************************/

void bgav_seek(bgav_t *, gavl_time_t*);

void bgav_set_dll_path_xanim(const char*);
const char * bgav_get_dll_path_xanim();

/***************************************************
 * Debugging functions
 ***************************************************/

/* Dump all information about the stream to stderr */

void bgav_dump(bgav_t * bgav);

/* Dump infos about the installed codecs */

void bgav_codecs_dump();

/* Dump known media formats */

void bgav_formats_dump();

/* Dump all inputs in html */

void bgav_inputs_dump();

/* Dump all redirectors */

void bgav_redirectors_dump();
