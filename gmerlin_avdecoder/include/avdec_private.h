
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

#include <avdec.h>
#include <stdio.h>

#include "config.h"

#define BGAV_MK_FOURCC(a, b, c, d) ((a<<24)|(b<<16)|(c<<8)|d)

typedef struct bgav_demuxer_s         bgav_demuxer_t;
typedef struct bgav_demuxer_context_s bgav_demuxer_context_t;
typedef struct bgav_packet_s          bgav_packet_t;

typedef struct bgav_input_s           bgav_input_t;
typedef struct bgav_input_context_s   bgav_input_context_t;
typedef struct bgav_audio_decoder_s   bgav_audio_decoder_t;
typedef struct bgav_video_decoder_s   bgav_video_decoder_t;

typedef struct bgav_audio_decoder_context_s bgav_audio_decoder_context_t;
typedef struct bgav_video_decoder_context_s bgav_video_decoder_context_t;

typedef struct bgav_stream_s   bgav_stream_t;

typedef struct bgav_packet_buffer_s   bgav_packet_buffer_t;

/* Decoder structures */

struct bgav_audio_decoder_s
  {
  uint32_t * fourccs;
  const char * name;
  int (*init)(bgav_stream_t*);
  int (*decode)(bgav_stream_t*, gavl_audio_frame_t*, int num_samples);
  void (*close)(bgav_stream_t*);
  void (*clear)(bgav_stream_t*);

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
  void (*clear)(bgav_stream_t*);
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
  int64_t timestamp;
  int keyframe;
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

bgav_packet_t * bgav_packet_buffer_get_packet_write(bgav_packet_buffer_t*);

void bgav_packet_buffer_clear(bgav_packet_buffer_t*);

int bgav_packet_buffer_get_timestamp(bgav_packet_buffer_t*,
                                     gavl_time_t * ret);

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

#define BGAV_STREAM_AUDIO 0
#define BGAV_STREAM_VIDEO 1

/* Stream structure */ 

struct bgav_stream_s
  {
  void * priv;
  int action;
  int stream_id; /* Format specific stream id */
  int type;
  bgav_packet_buffer_t * packet_buffer;
  
  char * ext_data;
  int ext_size;
  
  uint32_t fourcc;

  int64_t position; /* In samples/frames */
  gavl_time_t time; /* Timestamp (used mainly for seeking) */

  /* Where to get data */
  bgav_demuxer_context_t * demuxer;

  /* Some demuxers can only read incomplete packets at once,
     so they can store the packets here */

  bgav_packet_t * packet;
  int             packet_seq;

  char * description;
  
  union
    {
    struct
      {
      gavl_audio_format_t format;
      bgav_audio_decoder_context_t * decoder;
      int bits_per_sample; /* In some cases, this must be set from the
                              Container to distinguish between 8 and 16 bit
                              PCM codecs. For compressed codecss like mp3, this
                              field is nonsense*/
      
      /* The following ones are mainly for Microsoft formats and codecs */
      
      int bitrate;
      int block_align;
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
      } video;
    } data;
  };

/* Overloadable input module */

struct bgav_input_s
  {
  int     (*open)(bgav_input_context_t*, const char * url,
                  int connect_timeout);
  int     (*read)(bgav_input_context_t*, uint8_t * buffer, int len);
  int64_t (*seek_byte)(bgav_input_context_t*, int64_t pos, int whence);
  void    (*close)(bgav_input_context_t*);
  };

struct bgav_input_context_s
  {
  char * get_buffer;
  int    get_buffer_size;
  int    get_buffer_alloc;
  
  void * priv;
  int64_t total_bytes; /* Maybe 0 for non seekable streams */
  int64_t position;    /* Updated also for non seekable streams */
  bgav_input_t * input;
  };

/* input.c */

/* Read functions return FALSE on error */

int bgav_input_read_data(bgav_input_context_t*, uint8_t*, int);
int bgav_input_read_string_pascal(bgav_input_context_t*, char*);

int bgav_input_read_8(bgav_input_context_t*,uint8_t*);
int bgav_input_read_16_le(bgav_input_context_t*,uint16_t*);
int bgav_input_read_32_le(bgav_input_context_t*,uint32_t*);
int bgav_input_read_64_le(bgav_input_context_t*,uint64_t*);

int bgav_input_read_16_be(bgav_input_context_t*,uint16_t*);
int bgav_input_read_24_be(bgav_input_context_t*,uint32_t*);
int bgav_input_read_32_be(bgav_input_context_t*,uint32_t*);
int bgav_input_read_64_be(bgav_input_context_t*,uint64_t*);

int bgav_input_get_data(bgav_input_context_t*, uint8_t*,int);

int bgav_input_get_8(bgav_input_context_t*,uint8_t*);
int bgav_input_get_16_le(bgav_input_context_t*,uint16_t*);
int bgav_input_get_32_le(bgav_input_context_t*,uint32_t*);
int bgav_input_get_64_le(bgav_input_context_t*,uint64_t*);

int bgav_input_get_16_be(bgav_input_context_t*,uint16_t*);
int bgav_input_get_32_be(bgav_input_context_t*,uint32_t*);
int bgav_input_get_64_be(bgav_input_context_t*,uint64_t*);

#define bgav_input_read_fourcc(a,b) bgav_input_read_32_be(a,b)
#define bgav_input_get_fourcc(a,b)  bgav_input_get_32_be(a,b)

bgav_input_context_t * bgav_input_open(const char * url, int milliseconds);
void bgav_input_close(bgav_input_context_t * ctx);

void bgav_input_skip(bgav_input_context_t *, int);

void bgav_input_seek(bgav_input_context_t * ctx,
                     int64_t position,
                     int whence);

/* Input module to read from memory */

bgav_input_context_t * bgav_input_open_memory(uint8_t * data,
                                              uint32_t data_size);


/* Demuxer class */

struct bgav_demuxer_s
  {
  int  (*probe)(bgav_input_context_t*);
  int  (*open)(bgav_demuxer_context_t*);
  int  (*next_packet)(bgav_demuxer_context_t*);

  /*
   *  Seeking sets either the position- or the time
   *  member of the stream
   */
  
  void (*seek)(bgav_demuxer_context_t*, gavl_time_t);
  void (*close)(bgav_demuxer_context_t*);
  };

/* Metadata structure */

typedef struct
  {
  char * author;
  char * title;
  char * comment;
  char * copyright;
  } bgav_metadata_t;

struct bgav_demuxer_context_s
  {
  void * priv;
  bgav_metadata_t metadata;
  bgav_demuxer_t * demuxer;
  bgav_input_context_t * input;
  gavl_time_t duration;
  
  /* Stream table */
  int num_audio_streams;
  int num_video_streams;
  
  bgav_stream_t * audio_streams;
  bgav_stream_t * video_streams;

  /* What is actually supported by us */

  int supported_audio_streams;
  int supported_video_streams;
  
  int * audio_stream_index;
  int * video_stream_index;
  char * stream_description;
  int can_seek;

  };

/* demuxer.c */

bgav_demuxer_context_t * bgav_demuxer_create(bgav_input_context_t * input);

void bgav_demuxer_create_buffers(bgav_demuxer_context_t * demuxer);
void bgav_demuxer_destroy(bgav_demuxer_context_t * demuxer);

bgav_stream_t *
bgav_demuxer_add_audio_stream(bgav_demuxer_context_t * demuxer);

bgav_stream_t *
bgav_demuxer_add_video_stream(bgav_demuxer_context_t * demuxer);

bgav_packet_t *
bgav_demuxer_get_packet_read(bgav_demuxer_context_t * demuxer,
                             bgav_stream_t * s);


void 
bgav_demuxer_done_packet_read(bgav_demuxer_context_t * demuxer,
                              bgav_packet_t *);

void
bgav_demuxer_seek(bgav_demuxer_context_t * demuxer,
                  gavl_time_t time);

// bgav_packet_t *
// bgav_demuxer_get_packet_write(bgav_demuxer_context_t * demuxer, int stream);

bgav_stream_t * bgav_demuxer_find_stream(bgav_demuxer_context_t * ctx,
                                         int stream_id);


/* Actual decoder */

struct bgav_s
  {
  bgav_metadata_t metadata;
  bgav_input_context_t * input;
  bgav_demuxer_context_t * demuxer;
  };

/* Bytestream utilities */

#define BGAV_PTR_2_16LE(p) \
((*(p+1) << 8) | \
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
p[7])

#define BGAV_PTR_2_FOURCC(p) BGAV_PTR_2_32BE(p)

#define BGAV_WAVID_2_FOURCC(id) BGAV_MK_FOURCC(0x00, 0x00, (id>>8), (id&0xff))

/* utils.c */

void bgav_dump_fourcc(uint32_t fourcc);
void bgav_hexdump(uint8_t * data, int len, int linebreak);
char * bgav_sprintf(const char * format,...);
char * bgav_strndup(const char * start, const char * end);


int bgav_url_split(const char * url,
                   char ** protocol,
                   char ** hostname,
                   int * port,
                   char ** path);

/* tcp.c */

int bgav_tcp_connect(const char * url, int port, int millisecondsn);

/* Charset utilities (charset.c) */

char * bgav_convert_string(const char * in_string, int in_len,
                           char * in_charset, char * out_charset);

/* audio.c */

void bgav_audio_dump(bgav_stream_t * s);
int bgav_start_audio_decoders(bgav_t *);
void bgav_close_audio_decoders(bgav_t *);

int bgav_audio_decode(bgav_stream_t * stream, gavl_audio_frame_t * frame,
                      int num_samples);

/* video.c */

void bgav_video_dump(bgav_stream_t * s);
int bgav_start_video_decoders(bgav_t * s);
void bgav_close_video_decoders(bgav_t *);

/* codecs.c */

void bgav_codecs_init();

bgav_audio_decoder_t * bgav_find_audio_decoder(bgav_stream_t*);
bgav_video_decoder_t * bgav_find_video_decoder(bgav_stream_t*);
void bgav_audio_decoder_register(bgav_audio_decoder_t * dec);
void bgav_video_decoder_register(bgav_video_decoder_t * dec);

