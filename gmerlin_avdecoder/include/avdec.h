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

#include <gavl/gavl.h>

/***************************************************
 *  This is the opaque structure of a bgav decoder
 *  You don't want to know, whats inside here
 ***************************************************/

typedef struct bgav_s bgav_t;

/***************************************************
 * Housekeeping Functions
 ***************************************************/

bgav_t * bgav_create();

/***************************************************
 * Set parameters
 ***************************************************/

/*
 * Timeout will only be used for network connections.
 * It is in milliseconds, 0 is infinity
 */

void bgav_set_connect_timeout(bgav_t *, int timeout);
void bgav_set_read_timeout(bgav_t *, int timeout);

/*
 *  Set network bandwidth (in bits per second)
 */

void bgav_set_network_bandwidth(bgav_t *, int bandwidth);

void bgav_set_network_buffer_size(bgav_t *, int size);


/* HTTP Options */

void bgav_set_http_use_proxy(bgav_t*, int use_proxy);
void bgav_set_http_proxy_host(bgav_t*, const char *);
void bgav_set_http_proxy_port(bgav_t*, int);
void bgav_set_http_shoutcast_metadata(bgav_t*, int);

/* Set callbacks */

void
bgav_set_name_change_callback(bgav_t*,
                              void (callback)(void*data, const char * name),
                              void * data);

void
bgav_set_track_change_callback(bgav_t*,
                               void (callback)(void*data, int track),
                               void * data);

void
bgav_set_buffer_callback(bgav_t*,
                         void (callback)(void*data, float percentage),
                         void * data);

/******************************************************
 * Open
 ******************************************************/

/* Open a file or URL, return handle on success */

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

/*
 *  Metadata: These will return NULL for the fields,
 *  which are not present in the stream
 */

const char * bgav_get_author(bgav_t*, int track);
const char * bgav_get_title(bgav_t*, int track);
const char * bgav_get_copyright(bgav_t*, int track);
const char * bgav_get_comment(bgav_t*, int track);

char * bgav_get_album(bgav_t*, int track);
char * bgav_get_artist(bgav_t*, int track);
char * bgav_get_genre(bgav_t*, int track);
char * bgav_get_date(bgav_t*, int track);
int bgav_get_track(bgav_t*, int track);


void bgav_select_track(bgav_t *, int);

/* Description of the current track */

const char * bgav_get_description(bgav_t * b);

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
#define BGAV_STREAM_UNSUPPORTED  3

int bgav_set_audio_stream(bgav_t*, int stream, int action);
int bgav_set_video_stream(bgav_t*, int stream, int action);

/*
 *  Call this function, after you called bgav_set_audio_stream()
 *  and/or bgav_set_video_stream(). After that, you can get the
 *  formats (see above)
 */

void bgav_start(bgav_t *);


/***************************************************
 * Decoding functions
 ***************************************************/

int bgav_read_video(bgav_t *, gavl_video_frame_t * frame, int stream);
int bgav_read_audio(bgav_t *, gavl_audio_frame_t * frame, int stream,
                    int num_samples);

/***************************************************
 * Seek to a timestanmp. This also resyncs all streams
 ***************************************************/

void bgav_seek(bgav_t *, gavl_time_t);

/***************************************************
 * Codec path handling
 ***************************************************/

/*
 *  Note: call these BEFORE the first call to bgav_open()
 *  or bgav_codecs_dump().
 */

/*
 *   Win32 dlls MUST reside in /usr/lib/win32,
 *   for other codecs you can change the dll path globally
 */

/* Default is /usr/lib/RealPlayer8/Codecs/ */

const char * bgav_get_dll_path_real();
void bgav_set_dll_path_real(const char*);

/* Default is to read the XANMIM_MOD_DIR environment variable */

void bgav_set_dll_path_xanim(const char*);
const char * bgav_get_dll_path_xanim();

/***************************************************
 * Debugging functions
 ***************************************************/

/* Dump all information about the stream to stderr */

void bgav_dump(bgav_t * bgav);

/* Dump infos about the installed codecs */

void bgav_codecs_dump();
