/*****************************************************************
 
  bgav.h
 
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

/* Open a file or URL, return handle on success */

/*
 * Timeout will only be used for network connections.
 * It is in milliseconds, 0 is infinity
 */

bgav_t * bgav_open(const char * location, int timeout);

/* Close and free everything */

void bgav_close(bgav_t *);

/***************************************************
 * Get information about the file
 ***************************************************/

/* Query stream numbers */

int bgav_num_audio_streams(bgav_t *);
int bgav_num_video_streams(bgav_t *);

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

gavl_audio_format_t * bgav_get_audio_format(bgav_t*, int stream);
gavl_video_format_t * bgav_get_video_format(bgav_t*, int stream);

/* Return duration, will be 0 if unknown */

gavl_time_t bgav_get_duration(bgav_t*);

/* Returns TRUE if the track is seekable */

int bgav_can_seek(bgav_t *);

/*
 *  Metadata: These will return NULL for the fields,
 *  which are not present in the stream
 */

const char * bgav_get_author(bgav_t*);
const char * bgav_get_title(bgav_t*);
const char * bgav_get_copyright(bgav_t*);
const char * bgav_get_comment(bgav_t*);

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
#define BGAV_STREAM_UNSUPPORTED  3

int bgav_set_audio_stream(bgav_t*, int stream, int action);
int bgav_set_video_stream(bgav_t*, int stream, int action);

/*
 *  Call this function, after you called bgav_set_audio_stream()
 *  and/or bgav_set_video_stream(). After that, you can get the
 *  formats (see above)
 */

void bgav_start_decoders(bgav_t *);


/***************************************************
 * Decoding functions
 ***************************************************/

int bgav_read_video(bgav_t *, gavl_video_frame_t * frame, int stream);
int bgav_read_audio(bgav_t *, gavl_audio_frame_t * frame, int stream);

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
