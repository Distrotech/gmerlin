/*****************************************************************
 
  avdec_private.h
 
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

#include "config.h"

#include <avdec.h>
// #include <stdio.h>

#define BGAV_MK_FOURCC(a, b, c, d) ((a<<24)|(b<<16)|(c<<8)|d)

typedef struct bgav_demuxer_s         bgav_demuxer_t;
typedef struct bgav_demuxer_context_s bgav_demuxer_context_t;

typedef struct bgav_redirector_s         bgav_redirector_t;
typedef struct bgav_redirector_context_s bgav_redirector_context_t;

typedef struct bgav_packet_s          bgav_packet_t;

typedef struct bgav_input_s           bgav_input_t;
typedef struct bgav_input_context_s   bgav_input_context_t;
typedef struct bgav_audio_decoder_s   bgav_audio_decoder_t;
typedef struct bgav_video_decoder_s   bgav_video_decoder_t;

typedef struct bgav_audio_decoder_context_s bgav_audio_decoder_context_t;
typedef struct bgav_video_decoder_context_s bgav_video_decoder_context_t;

typedef struct bgav_stream_s   bgav_stream_t;

typedef struct bgav_packet_buffer_s   bgav_packet_buffer_t;

#include "id3.h"

struct bgav_metadata_s
  {
  char * author;
  char * title;
  char * comment;
  char * copyright;
  char * album;
  char * artist;
  char * genre;
  char * date;
  int track;
  };

void bgav_metadata_dump(bgav_metadata_t*m);

void bgav_metadata_merge(bgav_metadata_t * dst,
                         bgav_metadata_t * src1,
                         bgav_metadata_t * src2);

void bgav_metadata_merge2(bgav_metadata_t * dst,
                          bgav_metadata_t * src);

void bgav_metadata_free(bgav_metadata_t*);

/* Decoder structures */

struct bgav_audio_decoder_s
  {
  uint32_t * fourccs;
  const char * name;
  int (*init)(bgav_stream_t*);
  int (*decode)(bgav_stream_t*, gavl_audio_frame_t*, int num_samples);
  void (*close)(bgav_stream_t*);
  void (*resync)(bgav_stream_t*);

  bgav_audio_decoder_t * next;
  };

struct bgav_video_decoder_s
  {
  uint32_t * fourccs;
  const char * name;
  int (*init)(bgav_stream_t*);
  /*
   *  Decodes one frame. If frame is NULL;
   *  the frame is skipped
   */
  int (*decode)(bgav_stream_t*, gavl_video_frame_t*);
  void (*close)(bgav_stream_t*);
  void (*resync)(bgav_stream_t*);
  bgav_video_decoder_t * next;
  };

struct bgav_audio_decoder_context_s
  {
  void * priv;
  bgav_audio_decoder_t * decoder;
  };

struct bgav_video_decoder_context_s
  {
  void * priv;
  bgav_video_decoder_t * decoder;
  };

/* Packet */

struct bgav_packet_s
  {
  int valid;
  int data_size;
  int data_alloc;
  uint8_t * data;
  int64_t timestamp_scaled;
  int keyframe;
  bgav_stream_t * stream; /* The stream this packet belongs to */
  int samples; /* Optional */

  gavl_audio_frame_t * audio_frame; /* For demuxers, which deliver audio
                                       frames directly */

  gavl_video_frame_t * video_frame; /* For demuxers, which deliver video
                                       frames directly */
  
  struct bgav_packet_s * next;
  };

/* packet.c */

bgav_packet_t * bgav_packet_create();
void bgav_packet_destroy(bgav_packet_t*);
void bgav_packet_alloc(bgav_packet_t*, int size);

void bgav_packet_done_write(bgav_packet_t *);

/* packetbuffer.c */

bgav_packet_buffer_t * bgav_packet_buffer_create();
void bgav_packet_buffer_destroy(bgav_packet_buffer_t*);

bgav_packet_t * bgav_packet_buffer_get_packet_read(bgav_packet_buffer_t*);
bgav_packet_t * bgav_packet_buffer_peek_packet_read(bgav_packet_buffer_t * b);

bgav_packet_t * bgav_packet_buffer_get_packet_write(bgav_packet_buffer_t*, bgav_stream_t * s);

void bgav_packet_buffer_clear(bgav_packet_buffer_t*);

int bgav_packet_buffer_is_empty(bgav_packet_buffer_t * b);



/* Palette support */

typedef struct
  {
  uint16_t r;
  uint16_t g;
  uint16_t b;
  uint16_t a;
  } bgav_palette_entry_t;

/* These map a palette entry to a gavl format frame */

#define BGAV_PALETTE_2_RGB24(pal, dst) \
dst[0] = pal.r >> 8;\
dst[1] = pal.g >> 8;\
dst[2] = pal.b >> 8;

#define BGAV_PALETTE_2_RGBA32(pal, dst) \
dst[0] = pal.r >> 8;\
dst[1] = pal.g >> 8;\
dst[2] = pal.b >> 8;\
dst[3] = pal.a >> 8;

#define BGAV_PALETTE_2_BGR24(pal, dst) \
dst[2] = pal.r >> 8;\
dst[1] = pal.g >> 8;\
dst[0] = pal.b >> 8;

/* Stream types */

typedef enum
  {
    BGAV_STREAM_UNKNOWN = 0,
    BGAV_STREAM_AUDIO,
    BGAV_STREAM_VIDEO,
  } bgav_stream_type_t;


/* Stream structure */ 

#define BGAV_BITRATE_VBR -1

#define BGAV_ENDIANESS_NONE   0 // Unspecified
#define BGAV_ENDIANESS_BIG    1
#define BGAV_ENDIANESS_LITTLE 2

struct bgav_stream_s
  {
  void * priv;
  bgav_stream_action_t action;
  int stream_id; /* Format specific stream id */
  bgav_stream_type_t type;
  bgav_packet_buffer_t * packet_buffer;
  
  uint8_t * ext_data;
  int ext_size;
  
  uint32_t fourcc;

  uint32_t subformat; /* Real flavors, sub_ids.... */
  
  int64_t position; /* In samples/frames */

  /*
   *  Support for custom timescales
   *  Default timescales are the samplerate for audio
   *  streams and the timescale from the format for video streams.
   *  Demuxers can, however, define other timescales.
   */

  int64_t time_scaled;
  int timescale;

  /* Positions in the superindex */
  
  int first_index_position;
  int last_index_position;
  int index_position;
  
  /* Where to get data */
  bgav_demuxer_context_t * demuxer;

  /* Some demuxers can only read incomplete packets at once,
     so they can store the packets here */

  bgav_packet_t * packet;
  int             packet_seq;

  /* Description of the stream */
  
  char * description;

  /* Language */

  char * language;
    
  /*
   *  Sometimes, the bitrates important for codecs 
   *  and the bitrates set in the container format
   *  differ, so we save both here
   */
  
  int container_bitrate;
  int codec_bitrate;

  /*
   *  Indicator for streams, which have no packets (e.g. type 1 DV AVIs,
   *  where the audio is inside the videopackets
   */

  int no_packets;
  bgav_stream_t * sync_stream;
    
  union
    {
    struct
      {
      gavl_audio_format_t format;
      bgav_audio_decoder_context_t * decoder;
      int bits_per_sample; /* In some cases, this must be set from the
                              Container to distinguish between 8 and 16 bit
                              PCM codecs. For compressed codecs like mp3, this
                              field is nonsense*/
      
      /* The following ones are mainly for Microsoft formats and codecs */
      int block_align;

      /* This is ONLY used for codecs, which can be both little-
         and big-endian. In this case, the endianess is set by
         the demuxer */
      
      int endianess;
      } audio;
    struct
      {
      int depth;
      int planes;     /* For M$ formats only */
      int image_size; /* For M$ formats only */
            
      bgav_video_decoder_context_t * decoder;
      gavl_video_format_t format;
      int palette_size;
      bgav_palette_entry_t * palette;

      /*
       *  Codecs should update this field, even if they are
       *  skipping frames. Units are timescale tics from the CODEC timescale
       *  By using these, we can do more accurate seeks
       */
      
      int64_t last_frame_time;
      int     last_frame_duration;
      } video;
    } data;
  };

/* stream.c */

int bgav_stream_start(bgav_stream_t * stream);
void bgav_stream_stop(bgav_stream_t * stream);
void bgav_stream_alloc(bgav_stream_t * stream);
void bgav_stream_free(bgav_stream_t * stream);
void bgav_stream_dump(bgav_stream_t * s);

/* Which timestamp would come if we would decode right now? */

gavl_time_t bgav_stream_next_timestamp(bgav_stream_t *);

/* Clear the packet buffer, called before seeking */

void bgav_stream_clear(bgav_stream_t * s);

/* Resynchronize the stream to the next point
   where decoding can start again (this is a nop for
   many decoders) Called AFTER seeking */

void bgav_stream_resync_decoder(bgav_stream_t * s);

/*
 * Skip to a specific point which must be larger than the current stream time
 */

int bgav_stream_skipto(bgav_stream_t * s, gavl_time_t * time);

typedef struct
  {
  char * name;
  gavl_time_t duration;
  bgav_metadata_t metadata;

  int num_audio_streams;
  int num_video_streams;

  bgav_stream_t * audio_streams;
  bgav_stream_t * video_streams;

  void * priv; /* For storing private data */  
  } bgav_track_t;

/* track.c */

bgav_stream_t *
bgav_track_add_audio_stream(bgav_track_t * t);

bgav_stream_t *
bgav_track_add_video_stream(bgav_track_t * t);

void
bgav_track_remove_audio_stream(bgav_track_t * track, int stream);

void
bgav_track_remove_video_stream(bgav_track_t * track, int stream);



bgav_stream_t *
bgav_track_find_stream(bgav_track_t * t, int stream_id);

/* Clear all buffers (call BEFORE seeking) */

void bgav_track_clear(bgav_track_t * track);

/* Resync the decoders, return the latest time, where we can start decoding again */

gavl_time_t bgav_track_resync_decoders(bgav_track_t*);

/* Skip to a specific point */

int bgav_track_skipto(bgav_track_t*, gavl_time_t * time);

/* Find stream among ALL streams, also switched off ones */

bgav_stream_t *
bgav_track_find_stream_all(bgav_track_t * t, int stream_id);

int bgav_track_start(bgav_track_t * t, bgav_demuxer_context_t * demuxer);
void bgav_track_stop(bgav_track_t * t);

/* Remove unsupported streams */

void bgav_track_remove_unsupported(bgav_track_t * t);

void bgav_track_free(bgav_track_t * t);

void bgav_track_dump(bgav_t * b, bgav_track_t * t);

int bgav_track_has_sync(bgav_track_t * t);


typedef struct
  {
  int num_tracks;
  bgav_track_t * tracks;
  bgav_track_t * current_track;
  int refcount;
  } bgav_track_table_t;

/* Tracktable */

bgav_track_table_t * bgav_track_table_create(int num_tracks);
bgav_track_t * bgav_track_table_append_track(bgav_track_table_t * t);

void bgav_track_table_unref(bgav_track_table_t*);
void bgav_track_table_ref(bgav_track_table_t*);

void bgav_track_table_select_track(bgav_track_table_t*,int);
void bgav_track_table_dump(bgav_track_table_t*);

void bgav_track_table_merge_metadata(bgav_track_table_t*,
                                     bgav_metadata_t * m);

void bgav_track_table_remove_unsupported(bgav_track_table_t * t);

/* Options (shared between inputs, demuxers and decoders) */

struct bgav_options_s
  {
  /* Generic network options */
  int connect_timeout;
  int read_timeout;

  int network_bandwidth;
  int network_buffer_size;

  /* http options */

  int http_use_proxy;
  char * http_proxy_host;
  int http_proxy_port;

  int    http_proxy_auth;
  char * http_proxy_user;
  char * http_proxy_pass;
  
  int http_shoutcast_metadata;

  /* ftp options */
    
  char * ftp_anonymous_password;
  int ftp_anonymous;
  /* Callbacks */
  
  void (*name_change_callback)(void * data, const char * name);
  void * name_change_callback_data;

  void (*metadata_change_callback)(void * data, const bgav_metadata_t * m);
  void * metadata_change_callback_data;

  void (*track_change_callback)(void * data, int track);
  void * track_change_callback_data;

  void (*buffer_callback)(void * data, float percentage);
  void * buffer_callback_data;

  int (*user_pass_callback)(void * data, const char * resource, char ** user, char ** password);
  void * user_pass_callback_data;
  };

void bgav_options_set_defaults(bgav_options_t*opt);
void bgav_options_free(bgav_options_t*opt);

/* Overloadable input module */

struct bgav_input_s
  {
  const char * name;
  int     (*open)(bgav_input_context_t*, const char * url);
  int     (*read)(bgav_input_context_t*, uint8_t * buffer, int len);

  /* Attempts to read data but returns immediately if there is nothing
     available */
    
  int     (*read_nonblock)(bgav_input_context_t*, uint8_t * buffer, int len);
  
  int64_t (*seek_byte)(bgav_input_context_t*, int64_t pos, int whence);
  void    (*close)(bgav_input_context_t*);

  /* Some inputs support multiple tracks */

  void    (*select_track)(bgav_input_context_t*, int);

  /* Alternate API: Sector based read and seek access */

  int (*read_sector)(bgav_input_context_t*,uint8_t* buffer);
  int64_t (*seek_sector)(bgav_input_context_t*, int64_t sector);
  
  /* Some inputs autoscan the available devices */
  bgav_device_info_t (*find_devices)();
  };

struct bgav_input_context_s
  {
  /* ID3V2 tags can be prepended to many types of files,
     so we read them globally */

  bgav_id3v2_tag_t * id3v2;
  
  uint8_t * buffer;
  int    buffer_size;
  int    buffer_alloc;
  
  void * priv;
  int64_t total_bytes; /* Maybe 0 for non seekable streams */
  int64_t position;    /* Updated also for non seekable streams */
  bgav_input_t * input;

  /* Some input modules already fire up a demuxer */
    
  bgav_demuxer_context_t * demuxer;

  /*
   *  These can be NULL. If not, they might be used to
   *  fire up the right demultiplexer
   */
  
  char * filename;
  char * url;
  char * mimetype;

  /* For multiple track support */

  bgav_track_table_t * tt;

  bgav_metadata_t metadata;

  /* This is set by the modules to signal that we
     need to prebuffer data */
    
  int do_buffer;

  /* For sector based access */

  int sector_size;
  int sector_size_raw;
  int sector_header_size;
  int64_t total_sectors;
  int64_t sector_position;

  const bgav_options_t * opt;
  
  char * error_msg;
  };

/* input.c */

/* Read functions return FALSE on error */

int bgav_input_read_data(bgav_input_context_t*, uint8_t*, int);
int bgav_input_read_string_pascal(bgav_input_context_t*, char*);

int bgav_input_read_8(bgav_input_context_t*,uint8_t*);
int bgav_input_read_16_le(bgav_input_context_t*,uint16_t*);
int bgav_input_read_24_le(bgav_input_context_t*,uint32_t*);
int bgav_input_read_32_le(bgav_input_context_t*,uint32_t*);
int bgav_input_read_64_le(bgav_input_context_t*,uint64_t*);

int bgav_input_read_16_be(bgav_input_context_t*,uint16_t*);
int bgav_input_read_24_be(bgav_input_context_t*,uint32_t*);
int bgav_input_read_32_be(bgav_input_context_t*,uint32_t*);
int bgav_input_read_64_be(bgav_input_context_t*,uint64_t*);

int bgav_input_get_data(bgav_input_context_t*, uint8_t*,int);

int bgav_input_get_8(bgav_input_context_t*,uint8_t*);
int bgav_input_get_16_le(bgav_input_context_t*,uint16_t*);
int bgav_input_get_24_le(bgav_input_context_t*,uint32_t*);
int bgav_input_get_32_le(bgav_input_context_t*,uint32_t*);
int bgav_input_get_64_le(bgav_input_context_t*,uint64_t*);

int bgav_input_get_16_be(bgav_input_context_t*,uint16_t*);
int bgav_input_get_32_be(bgav_input_context_t*,uint32_t*);
int bgav_input_get_64_be(bgav_input_context_t*,uint64_t*);

#define bgav_input_read_fourcc(a,b) bgav_input_read_32_be(a,b)
#define bgav_input_get_fourcc(a,b)  bgav_input_get_32_be(a,b)

/*
 *  Read one line from the input. Linebreak characters
 *  (\r, \n) are NOT stored in the buffer, return values is
 *  the number of characters in the line
 */

int bgav_input_read_line(bgav_input_context_t*,
                         char ** buffer, int * buffer_alloc, int buffer_offset);

int bgav_input_read_sector(bgav_input_context_t*, uint8_t*);

int bgav_input_open(bgav_input_context_t *, const char * url);

void bgav_input_close(bgav_input_context_t * ctx);
void bgav_input_destroy(bgav_input_context_t * ctx);

void bgav_input_skip(bgav_input_context_t *, int64_t);

bgav_input_context_t * bgav_input_create(bgav_options_t * opt);

/* For debugging purposes only: if you encounter data,
   hexdump them to stderr and skip them */

void bgav_input_skip_dump(bgav_input_context_t *, int);

void bgav_input_get_dump(bgav_input_context_t *, int);

void bgav_input_seek(bgav_input_context_t * ctx,
                     int64_t position,
                     int whence);

void bgav_input_seek_sector(bgav_input_context_t * ctx,
                            int64_t sector);


void bgav_input_buffer(bgav_input_context_t * ctx);

/* Input module to read from memory */

bgav_input_context_t * bgav_input_open_memory(uint8_t * data,
                                              uint32_t data_size);

/* Reopen a memory input with new data and minimal CPU overhead */

void bgav_input_reopen_memory(bgav_input_context_t * ctx,
                              uint8_t * data,
                              uint32_t data_size);


/* Input module to read from a filedescriptor */

bgav_input_context_t *
bgav_input_open_fd(int fd, int64_t total_bytes, const char * mimetype);

/*
 *  Some demuxer will create a superindex. If this is the case,
 *  generic next_packet() and seek() functions will be used
 */

typedef struct 
  {
  int num_entries;
  int entries_alloc;

  int current_position;
  
  struct
    {
    int64_t offset;
    uint32_t size;
    int stream_id;
    int keyframe;
    int64_t time; /* Time is scaled with the timescale of the stream */
    int samples;  /* Can be 0 if unknown */
    } * entries;
  } bgav_superindex_t;

/* Create superindex, nothing will be allocated if size == 0 */

bgav_superindex_t * bgav_superindex_create(int size);
void bgav_superindex_destroy(bgav_superindex_t *);

void bgav_superindex_add_packet(bgav_superindex_t * idx,
                                bgav_stream_t * s,
                                int64_t offset,
                                uint32_t size,
                                int stream_id,
                                int64_t timestamp,
                                int keyframe, int samples);

void bgav_superindex_seek(bgav_superindex_t * idx,
                          bgav_stream_t * s,
                          gavl_time_t time);

void bgav_superindex_dump(bgav_superindex_t * idx);

/* Demuxer class */

struct bgav_demuxer_s
  {
  /* If 1, perform iterative seeking */
  int seek_iterative;
  int  (*probe)(bgav_input_context_t*);

  int  (*open)(bgav_demuxer_context_t * ctx,
               bgav_redirector_context_t ** redir);
  int  (*next_packet)(bgav_demuxer_context_t*);

  /*
   *  Seeking sets either the position- or the time
   *  member of the stream
   */
  
  void (*seek)(bgav_demuxer_context_t*, gavl_time_t);
  void (*close)(bgav_demuxer_context_t*);

  /* Some demuxer support multiple tracks */

  void (*select_track)(bgav_demuxer_context_t*, int track);
  };

struct bgav_demuxer_context_s
  {
  int eof; /* Report EOF just once and not for each stream */
  const bgav_options_t * opt;
  void * priv;
  bgav_demuxer_t * demuxer;
  bgav_input_context_t * input;
  
  bgav_track_table_t * tt;
  char * stream_description;
  
  int can_seek;

  /*
   *  The stream, which requested the next_packet function.
   *  Can come handy sometimes
   */
  bgav_stream_t * request_stream;

  /*
   *  If demuxer creates a superindex, generic get_packet() and
   *  seek() functions will be used
   */
  bgav_superindex_t * si;
  int seeking;
  int non_interleaved;
  
  /* Some demuxers have a custom read packet function */
  int (*read_packet)(bgav_demuxer_context_t * ctx, int size);

  char * error_msg;
  
  };

/* demuxer.c */

/*
 *  Create a demuxer.
 */

bgav_demuxer_context_t *
bgav_demuxer_create(const bgav_options_t * opt,
                    bgav_demuxer_t * demuxer,
                    bgav_input_context_t * input);

bgav_demuxer_t * bgav_demuxer_probe(bgav_input_context_t * input);

void bgav_demuxer_create_buffers(bgav_demuxer_context_t * demuxer);
void bgav_demuxer_destroy(bgav_demuxer_context_t * demuxer);

bgav_packet_t *
bgav_demuxer_get_packet_read(bgav_demuxer_context_t * demuxer,
                             bgav_stream_t * s);


void 
bgav_demuxer_done_packet_read(bgav_demuxer_context_t * demuxer,
                              bgav_packet_t *);


void
bgav_demuxer_seek(bgav_demuxer_context_t * demuxer,
                  gavl_time_t time);

/*
 *  Start a demuxer. Some demuxers (most notably quicktime)
 *  can contain nothing but urls for the real streams.
 *  In this case, redir (if not NULL) will contain the
 *  redirector context
 */

int bgav_demuxer_start(bgav_demuxer_context_t * ctx,
                       bgav_redirector_context_t ** redir);
void bgav_demuxer_stop(bgav_demuxer_context_t * ctx);


// bgav_packet_t *
// bgav_demuxer_get_packet_write(bgav_demuxer_context_t * demuxer, int stream);

bgav_stream_t * bgav_track_find_stream(bgav_track_t * ctx, int stream_id);

/* Redirector */

struct bgav_redirector_s
  {
  const char * name;
  int (*probe)(bgav_input_context_t*);
  int (*parse)(bgav_redirector_context_t*);
  };

typedef struct
  {
  char * url;
  char * name;
  } bgav_url_info_t;

struct bgav_redirector_context_s
  {
  bgav_redirector_t * redirector;
  bgav_input_context_t * input;

  int parsed;
  int num_urls;
  bgav_url_info_t * urls;
  };

void bgav_redirector_destroy(bgav_redirector_context_t*r);
bgav_redirector_t * bgav_redirector_probe(bgav_input_context_t * input);


/* Actual decoder */

struct bgav_s
  {
  char * location;
  
  /* Configuration parameters */

  bgav_options_t opt;
  
  bgav_input_context_t * input;
  bgav_demuxer_context_t * demuxer;
  bgav_redirector_context_t * redirector;

  bgav_track_table_t * tt;
  
  int is_running;
  
  bgav_metadata_t metadata;

  /* Set by the seek function */

  int eof;

  char * error_msg;
  };

/* bgav.c */

void bgav_stop(bgav_t * b);
int bgav_init(bgav_t * b);


/* Bytestream utilities */

/* ptr -> integer */

#define BGAV_PTR_2_16LE(p) \
((*(p+1) << 8) | \
*(p))

#define BGAV_PTR_2_24LE(p) \
((*(p+2) << 16) | \
(*(p+1) << 8) | \
*(p))

#define BGAV_PTR_2_32LE(p) \
((*(p+3) << 24) | \
(*(p+2) << 16) | \
(*(p+1) << 8) | \
*(p))

#define BGAV_PTR_2_64LE(p) \
(((int64_t)(*(p+7)) << 56) | \
((int64_t)(*(p+6)) << 48) | \
((int64_t)(*(p+5)) << 40) | \
((int64_t)(*(p+4)) << 32) | \
((int64_t)(*(p+3)) << 24) | \
((int64_t)(*(p+2)) << 16) | \
((int64_t)(*(p+1)) << 8) | \
*(p))

#define BGAV_PTR_2_16BE(p) \
((*(p) << 8) | \
*(p+1))

#define BGAV_PTR_2_32BE(p) \
((*(p) << 24) | \
(*(p+1) << 16) | \
(*(p+2) << 8) | \
*(p+3))

#define BGAV_PTR_2_24BE(p) \
((*(p) << 16) | \
(*(p+1) << 8) | \
(*(p+2)))

#define BGAV_PTR_2_64BE(p) \
(((int64_t)p[0] << 56) | \
((int64_t)p[1] << 48) | \
((int64_t)p[2] << 40) | \
((int64_t)p[3] << 32) | \
((int64_t)p[4] << 24) | \
((int64_t)p[5] << 16) | \
((int64_t)p[6] << 8) | \
(p)[7])


/* integer -> ptr */

#define BGAV_16LE_2_PTR(i, p) \
(p)[0] = (i) & 0xff; \
(p)[1] = ((i)>>8) & 0xff

#define BGAV_32LE_2_PTR(i, p) \
(p)[0] = (i) & 0xff; \
(p)[1] = ((i)>>8) & 0xff; \
(p)[2] = ((i)>>16) & 0xff; \
(p)[3] = ((i)>>24) & 0xff

#define BGAV_64LE_2_PTR(i, p) \
(p)[0] = (i) & 0xff; \
(p)[1] = ((i)>>8) & 0xff; \
(p)[2] = ((i)>>16) & 0xff; \
(p)[3] = ((i)>>24) & 0xff; \
(p)[4] = ((i)>>32) & 0xff; \
(p)[5] = ((i)>>40) & 0xff; \
(p)[6] = ((i)>>48) & 0xff; \
(p)[7] = ((i)>>56) & 0xff

#define BGAV_16BE_2_PTR(i, p) \
(p)[1] = (i) & 0xff; \
(p)[0] = ((i)>>8) & 0xff

#define BGAV_32BE_2_PTR(i, p) \
(p)[3] = (i) & 0xff; \
(p)[2] = ((i)>>8) & 0xff; \
(p)[1] = ((i)>>16) & 0xff; \
(p)[0] = ((i)>>24) & 0xff;

#define BGAV_64BE_2_PTR(i, p) \
(p)[7] = (i) & 0xff; \
(p)[6] = ((i)>>8) & 0xff; \
(p)[5] = ((i)>>16) & 0xff; \
(p)[4] = ((i)>>24) & 0xff; \
(p)[3] = ((i)>>32) & 0xff; \
(p)[2] = ((i)>>40) & 0xff; \
(p)[1] = ((i)>>48) & 0xff; \
(p)[0] = ((i)>>56) & 0xff

#define BGAV_PTR_2_FOURCC(p) BGAV_PTR_2_32BE(p)

#define BGAV_WAVID_2_FOURCC(id) BGAV_MK_FOURCC(0x00, 0x00, (id>>8), (id&0xff))

#define BGAV_FOURCC_2_WAVID(f) (f & 0xffff)

/* utils.c */

void bgav_dump_fourcc(uint32_t fourcc);
void bgav_hexdump(uint8_t * data, int len, int linebreak);
char * bgav_sprintf(const char * format,...);
char * bgav_strndup(const char * start, const char * end);

char * bgav_strncat(char * old, const char * start, const char * end);

int bgav_url_split(const char * url,
                   char ** protocol,
                   char ** user,
                   char ** password,
                   char ** hostname,
                   int * port,
                   char ** path);

char ** bgav_stringbreak(const char * str, char sep);
void bgav_stringbreak_free(char ** str);


/* Read a single line from a filedescriptor */

int bgav_read_line_fd(int fd, char ** ret, int * ret_alloc, int milliseconds);

int bgav_read_data_fd(int fd, uint8_t * ret, int size, int milliseconds);

/* tcp.c */

int bgav_tcp_connect(const char * host, int port, int milliseconds, char ** error_msg);
int bgav_tcp_send(int fd, uint8_t * data, int len, char ** error_msg);

/* Charset utilities (charset.c) */

typedef struct bgav_charset_converter_s bgav_charset_converter_t;

bgav_charset_converter_t *
bgav_charset_converter_create(const char * in_charset,
                              const char * out_charset);

void bgav_charset_converter_destroy(bgav_charset_converter_t *);

char * bgav_convert_string(bgav_charset_converter_t *,
                           const char * in_string, int in_len,
                           int * out_len);

/* audio.c */

void bgav_audio_dump(bgav_stream_t * s);

int bgav_audio_start(bgav_stream_t * s);
void bgav_audio_stop(bgav_stream_t * s);

int bgav_audio_decode(bgav_stream_t * stream, gavl_audio_frame_t * frame,
                      int num_samples);

void bgav_audio_resync(bgav_stream_t * stream);

/* Skip to a point in the stream, return 0 on EOF */

int bgav_audio_skipto(bgav_stream_t * stream, gavl_time_t * t);

/* video.c */

void bgav_video_dump(bgav_stream_t * s);

int bgav_video_start(bgav_stream_t * s);
void bgav_video_stop(bgav_stream_t * s);

void bgav_video_resync(bgav_stream_t * stream);

int bgav_video_skipto(bgav_stream_t * stream, gavl_time_t * time);


/* codecs.c */

void bgav_codecs_init();

bgav_audio_decoder_t * bgav_find_audio_decoder(bgav_stream_t*);
bgav_video_decoder_t * bgav_find_video_decoder(bgav_stream_t*);
void bgav_audio_decoder_register(bgav_audio_decoder_t * dec);
void bgav_video_decoder_register(bgav_video_decoder_t * dec);

/* base64.c */

int bgav_base64encode(const unsigned char *input, int input_length,
                      unsigned char *output, int output_length);

int bgav_base64decode(const char *input,
                      int input_length,
                      unsigned char *output, int output_length);

/* device.c */

/*
 *  Append device info to an existing array and return the new array.
 *  arr can be NULL
 */

bgav_device_info_t * bgav_device_info_append(bgav_device_info_t * arr,
                                             const char * device,
                                             const char * name);

/* For debuggins only */

void bgav_device_info_dump(bgav_device_info_t * arr);
