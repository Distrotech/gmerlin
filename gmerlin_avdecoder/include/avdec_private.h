/*****************************************************************
 * gmerlin-avdecoder - a general purpose multimedia decoding library
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

#include "config.h"


#include <avdec.h>

#include <stdio.h> /* Needed for fileindex stuff */

#include <libintl.h>

#define BGAV_MK_FOURCC(a, b, c, d) ((a<<24)|(b<<16)|(c<<8)|d)

typedef struct bgav_edl_dec_s bgav_edl_dec_t;

typedef struct bgav_demuxer_s         bgav_demuxer_t;
typedef struct bgav_demuxer_context_s bgav_demuxer_context_t;

typedef struct bgav_redirector_s         bgav_redirector_t;
typedef struct bgav_redirector_context_s bgav_redirector_context_t;

typedef struct bgav_packet_s          bgav_packet_t;
typedef struct bgav_file_index_s          bgav_file_index_t;

typedef struct bgav_input_s                    bgav_input_t;
typedef struct bgav_input_context_s            bgav_input_context_t;
typedef struct bgav_audio_decoder_s            bgav_audio_decoder_t;
typedef struct bgav_video_decoder_s            bgav_video_decoder_t;
typedef struct bgav_subtitle_overlay_decoder_s bgav_subtitle_overlay_decoder_t;
typedef struct bgav_subtitle_reader_s bgav_subtitle_reader_t;
typedef struct bgav_subtitle_reader_context_s bgav_subtitle_reader_context_t;

typedef struct bgav_audio_decoder_context_s bgav_audio_decoder_context_t;
typedef struct bgav_video_decoder_context_s bgav_video_decoder_context_t;
typedef struct bgav_subtitle_overlay_decoder_context_s
bgav_subtitle_overlay_decoder_context_t;

typedef struct bgav_stream_s   bgav_stream_t;

typedef struct bgav_packet_buffer_s   bgav_packet_buffer_t;

typedef struct bgav_charset_converter_s bgav_charset_converter_t;

typedef struct bgav_track_s bgav_track_t;

typedef struct bgav_timecode_table_s bgav_timecode_table_t;

#include <id3.h>
#include <yml.h>

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
  void (*parse)(bgav_stream_t*);
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
  void (*parse)(bgav_stream_t*, int flush);
  bgav_video_decoder_t * next;
  };

struct bgav_subtitle_overlay_decoder_s
  {
  uint32_t * fourccs;
  const char * name;
  int (*init)(bgav_stream_t*);

  /* Query if a subtitle is available */
  int (*has_subtitle)(bgav_stream_t*);
  /*
   *  Decodes one frame. If frame is NULL;
   *  the frame is skipped
   */
  int (*decode)(bgav_stream_t*, gavl_overlay_t*);
  void (*close)(bgav_stream_t*);
  void (*resync)(bgav_stream_t*);
  void (*parse)(bgav_stream_t*);
  bgav_subtitle_overlay_decoder_t * next;
  };


struct bgav_audio_decoder_context_s
  {
  void * priv;
  const bgav_audio_decoder_t * decoder;
  };

struct bgav_video_decoder_context_s
  {
  void * priv;
  const bgav_video_decoder_t * decoder;
  };

struct bgav_subtitle_overlay_decoder_context_s
  {
  void * priv;
  const bgav_subtitle_overlay_decoder_t * decoder;
  };

/* Packet */

struct bgav_packet_s
  {
  /* For superindex files, it's the index position, for all other files,
     it's the file position */
  int64_t position;
  int valid;
  int data_size;
  int data_alloc;
  uint8_t * data;
  
  int64_t pts; /* In stream timescale tics */
  int64_t dts; /* In stream timescale tics */
  
  int64_t duration;  /* For text subtitles and VFR video only! */
  int keyframe;
  bgav_stream_t * stream; /* The stream this packet belongs to */

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

void bgav_packet_set_text_subtitle(bgav_packet_t * p,
                                   const char * text,
                                   int len,
                                   int64_t start,
                                   int64_t duration);

void bgav_packet_get_text_subtitle(bgav_packet_t * p,
                                   char ** text,
                                   int * text_alloc,
                                   gavl_time_t * start,
                                   gavl_time_t * duration);


/* packetbuffer.c */

struct bgav_packet_buffer_s
  {
  bgav_packet_t * packets;
  bgav_packet_t * read_packet;
  bgav_packet_t * write_packet;
  };

bgav_packet_buffer_t * bgav_packet_buffer_create();
void bgav_packet_buffer_destroy(bgav_packet_buffer_t*);

bgav_packet_t *
bgav_packet_buffer_get_packet_read(bgav_packet_buffer_t*, int get_duration);
bgav_packet_t *
bgav_packet_buffer_peek_packet_read(bgav_packet_buffer_t*, int get_duration);

bgav_packet_t *
bgav_packet_buffer_get_packet_write(bgav_packet_buffer_t*, bgav_stream_t * s);

void bgav_packet_buffer_clear(bgav_packet_buffer_t*);

int bgav_packet_buffer_is_empty(bgav_packet_buffer_t * b);

int bgav_packet_buffer_total_bytes(bgav_packet_buffer_t * b);

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
    BGAV_STREAM_SUBTITLE_TEXT,
    BGAV_STREAM_SUBTITLE_OVERLAY,
  } bgav_stream_type_t;


/* Stream structure */ 

#define BGAV_BITRATE_VBR -1

#define BGAV_ENDIANESS_NONE   0 // Unspecified
#define BGAV_ENDIANESS_BIG    1
#define BGAV_ENDIANESS_LITTLE 2

struct bgav_stream_s
  {
  void * priv;

  int initialized; /* Mostly means, that the format is valid */
  
  const bgav_options_t * opt;

  bgav_stream_action_t action;
  int stream_id; /* Format specific stream id */
  bgav_stream_type_t type;
  bgav_packet_buffer_t * packet_buffer;
  
  uint8_t * ext_data;
  int ext_size;
  
  uint32_t fourcc;

  uint32_t subformat; /* Real flavors, sub_ids.... */
  
  int64_t in_position;  /* In packets */
  
  /*
   *  Support for custom timescales
   *  Default timescales are the samplerate for audio
   *  streams and the timescale from the format for video streams.
   *  Demuxers can, however, define other timescales.
   */
  int timescale;
  int64_t in_time; /* Demuxer time in demuxer timescale */

  int64_t out_time; /* In codec timescale */
  
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
 
  char * description;   /* Technical description of the stream */
  char * info;          /* Info about the stream (e.g. Directors comments) */
  
  /* Language (ISO 639-3 B code) */

  char language[4];
    
  /*
   *  Sometimes, the bitrates important for codecs 
   *  and the bitrates set in the container format
   *  differ, so we save both here
   */
  
  int container_bitrate;
  int codec_bitrate;

  /*
   *  EOF reached
   *  This is *only* used by the core, never by demuxers or codecs
   */ 
  int eof;
  
  /*
   *  This indicates, that the demuxer doesn't produce packets,
   *  which are identical to frames.
   */

  int not_aligned;
  
  int64_t first_timestamp;

  /* The track, where this stream belongs */
  bgav_track_t * track;
  bgav_file_index_t * file_index;

  void (*process_packet)(bgav_stream_t * s, bgav_packet_t * p);
  int64_t duration;
  
  /* Set for INDEX_MODE_SI_PARSE for all streams, which need parsing */
  int index_mode;

  /* timecode table (for video streams only for now) */
  bgav_timecode_table_t * timecode_table;
  
  int has_codec_timecode;
  gavl_timecode_t codec_timecode;
  
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

      /* Number of *samples* which must be decoded before
         the next frame after seeking. Codecs set this, demuxers
         can honour it when seeking. */
      
      int preroll;
      
      } audio;
    struct
      {
      int depth;
      int planes;     /* For M$ formats only */
      int image_size; /* For M$ formats only */
      int flip_y;
      
      bgav_video_decoder_context_t * decoder;
      gavl_video_format_t format;
      int palette_size;
      bgav_palette_entry_t * palette;
      int palette_changed;
      /*
       *  Codecs should update this field, even if they are
       *  skipping frames. Units are timescale tics from the CODEC timescale
       *  By using these, we can do more accurate seeks
       */
      
      int64_t last_frame_time;
      int     last_frame_duration;

      /* These are set by the demuxer during seeking (and optionally
         by the codec during resync. They are invalid except during the
         seek process */

      int     next_frame_duration;

      int still_mode; /* Don't always have the next picture
                         during seeking */
      
/* How to get frame times and durations */
#define BGAV_FRAMETIME_CONSTANT  0 /* Strictly constant framerate */
#define BGAV_FRAMETIME_PACKET    1 /* Packets contain complete frames and have correct duration fields */
#define BGAV_FRAMETIME_PTS       2 /* Packets contain complete frames but have only correct PTS field  */
#define BGAV_FRAMETIME_CODEC     3 /* PTS and duration can only be generated by the codec              */
#define BGAV_FRAMETIME_CODEC_PTS 4 /* Codec takes PTS and duration from demuxer but sets times itself  */
      int frametime_mode;
      int wrong_b_timestamps;
      } video;
    struct
      {
      /* Video format for overlays */
      gavl_video_format_t format;
      /* Characterset converter for text subtitles */
      bgav_charset_converter_t * cnv;

      bgav_subtitle_overlay_decoder_context_t * decoder;
      bgav_subtitle_reader_context_t * subreader;
      
      char * charset;
      
      /* The video stream, onto which the subtitles will be
         displayed */
      bgav_stream_t * video_stream;
      
      } subtitle;
    } data;
  };

/* stream.c */

int bgav_stream_start(bgav_stream_t * stream);
void bgav_stream_stop(bgav_stream_t * stream);
void bgav_stream_create_packet_buffer(bgav_stream_t * stream);
void bgav_stream_init(bgav_stream_t * stream, const bgav_options_t * opt);
void bgav_stream_free(bgav_stream_t * stream);
void bgav_stream_dump(bgav_stream_t * s);

bgav_packet_t * bgav_stream_get_packet_write(bgav_stream_t * s);

int bgav_stream_get_index(bgav_stream_t * s);

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

int bgav_stream_skipto(bgav_stream_t * s, int64_t * time, int scale);

/*
 *  Chapter list
 */

typedef struct
  {
  int num_chapters;
  int timescale;
  struct
    {
    int64_t time;
    char * name;
    } * chapters;
  } bgav_chapter_list_t;

bgav_chapter_list_t * bgav_chapter_list_create(int timescale,
                                               int num_chapters);

void bgav_chapter_list_dump(bgav_chapter_list_t * list);
void bgav_chapter_list_destroy(bgav_chapter_list_t * list);

struct bgav_track_s
  {
  char * name;
  gavl_time_t duration;
  bgav_metadata_t metadata;

  int num_audio_streams;
  int num_video_streams;
  int num_subtitle_streams;

  bgav_stream_t * audio_streams;
  bgav_stream_t * video_streams;
  bgav_stream_t * subtitle_streams;
  
  bgav_chapter_list_t * chapter_list;
  
  void * priv; /* For storing private data */  
  
  int has_file_index;
  int sample_accurate;
  };

/* track.c */

bgav_stream_t *
bgav_track_add_audio_stream(bgav_track_t * t, const bgav_options_t * opt);

bgav_stream_t *
bgav_track_add_video_stream(bgav_track_t * t, const bgav_options_t * opt);

void
bgav_track_remove_audio_stream(bgav_track_t * track, int stream);

void
bgav_track_remove_video_stream(bgav_track_t * track, int stream);

void
bgav_track_remove_subtitle_stream(bgav_track_t * track, int stream);

bgav_stream_t *
bgav_track_add_subtitle_stream(bgav_track_t * t, const bgav_options_t * opt,
                               int text, const char * encoding);

bgav_stream_t *
bgav_track_attach_subtitle_reader(bgav_track_t * t,
                                  const bgav_options_t * opt,
                                  bgav_subtitle_reader_context_t * r);

bgav_stream_t *
bgav_track_find_stream(bgav_demuxer_context_t * t, int stream_id);

/* Clear all buffers (call BEFORE seeking) */

void bgav_track_clear(bgav_track_t * track);

/* Call after the track becomes current */
void bgav_track_mute(bgav_track_t * t);

/* Resync the decoders, return the latest time, where we can start decoding again */

gavl_time_t bgav_track_resync_decoders(bgav_track_t*, int scale);

/* Skip to a specific point */

int bgav_track_skipto(bgav_track_t*, int64_t * time, int scale);

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

/* Calculate duration from stream durations */
void bgav_track_calc_duration(bgav_track_t * t);

/* Tracktable */

typedef struct
  {
  int num_tracks;
  bgav_track_t * tracks;
  bgav_track_t * cur;
  int refcount;
  } bgav_track_table_t;

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
  /* Try sample accurate processing */
  int sample_accurate;
  int cache_time;
  int cache_size;
  
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

  /* Default character set for text subtitles */
  char * default_subtitle_encoding;
  
  int audio_dynrange;
  int seamless;
  int seek_subtitles;

  /* Postprocessing level (0..6) */
  
  int pp_level;

  char * dvb_channels_file;

  /* Prefer ffmpeg demuxers over native demuxers */
  int prefer_ffmpeg_demuxers;

  int dv_datetime;
  /* Callbacks */

  bgav_log_callback log_callback;
  void * log_callback_data;
  
  bgav_name_change_callback name_change_callback;
  void * name_change_callback_data;
  
  bgav_metadata_change_callback metadata_change_callback;
  void * metadata_change_callback_data;

  bgav_track_change_callback track_change_callback;
  void * track_change_callback_data;

  bgav_buffer_callback buffer_callback;
  void * buffer_callback_data;

  bgav_user_pass_callback user_pass_callback;
  void * user_pass_callback_data;

  bgav_aspect_callback aspect_callback;
  void * aspect_callback_data;
  
  bgav_index_callback index_callback;
  void * index_callback_data;

  };

void bgav_options_set_defaults(bgav_options_t*opt);
void bgav_options_free(bgav_options_t*opt);

/* Overloadable input module */

struct bgav_input_s
  {
  const char * name;
  int     (*open)(bgav_input_context_t*, const char * url, char ** redirect_url);
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

  /* Time based seek function for media, which are not stored
     stricktly linear. Time is changed to the actual seeked time */
  void (*seek_time)(bgav_input_context_t*, int64_t time, int scale);
  
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
  const bgav_input_t * input;

  /* Some input modules already fire up a demuxer */
    
  bgav_demuxer_context_t * demuxer;

  /*
   *  These can be NULL. If not, they might be used to
   *  choose the right demultiplexer for sources, which
   *  don't support format detection by content
   */
  
  char * filename;
  char * url;
  char * mimetype;
  char * disc_name;
  
  /* For reading textfiles */
  char * charset;
  bgav_charset_converter_t * cnv;
  
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
  
  // Stream ID, which will be used for syncing (for DVB)
  int sync_id;
  
  /* Inputs set this, if indexing is supported */
  char * index_file;
  
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

int bgav_input_read_float_32_be(bgav_input_context_t * ctx, float * ret);
int bgav_input_read_float_32_le(bgav_input_context_t * ctx, float * ret);

int bgav_input_read_double_64_be(bgav_input_context_t * ctx, double * ret);
int bgav_input_read_double_64_le(bgav_input_context_t * ctx, double * ret);

int bgav_input_get_data(bgav_input_context_t*, uint8_t*,int);

int bgav_input_get_8(bgav_input_context_t*,uint8_t*);
int bgav_input_get_16_le(bgav_input_context_t*,uint16_t*);
int bgav_input_get_24_le(bgav_input_context_t*,uint32_t*);
int bgav_input_get_32_le(bgav_input_context_t*,uint32_t*);
int bgav_input_get_64_le(bgav_input_context_t*,uint64_t*);

int bgav_input_get_16_be(bgav_input_context_t*,uint16_t*);
int bgav_input_get_32_be(bgav_input_context_t*,uint32_t*);
int bgav_input_get_64_be(bgav_input_context_t*,uint64_t*);

int bgav_input_get_float_32_be(bgav_input_context_t * ctx, float * ret);
int bgav_input_get_float_32_le(bgav_input_context_t * ctx, float * ret);

int bgav_input_get_double_64_be(bgav_input_context_t * ctx, double * ret);
int bgav_input_get_double_64_le(bgav_input_context_t * ctx, double * ret);


#define bgav_input_read_fourcc(a,b) bgav_input_read_32_be(a,b)
#define bgav_input_get_fourcc(a,b)  bgav_input_get_32_be(a,b)

/*
 *  Read one line from the input. Linebreak characters
 *  (\r, \n) are NOT stored in the buffer, return values is
 *  the number of characters in the line
 */

int bgav_input_read_line(bgav_input_context_t*,
                         char ** buffer, int * buffer_alloc, int buffer_offset, int * len);

void bgav_input_detect_charset(bgav_input_context_t*);

int bgav_input_read_convert_line(bgav_input_context_t*,
                                 char ** buffer, int * buffer_alloc,
                                 int * len);

int bgav_input_read_sector(bgav_input_context_t*, uint8_t*);

int bgav_input_open(bgav_input_context_t *, const char * url);

void bgav_input_close(bgav_input_context_t * ctx);
void bgav_input_destroy(bgav_input_context_t * ctx);

void bgav_input_skip(bgav_input_context_t *, int64_t);

/* Reopen  the input. Not all inputs can do this */
int bgav_input_reopen(bgav_input_context_t*);

bgav_input_context_t * bgav_input_create(const bgav_options_t * opt);

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

void bgav_input_ensure_buffer_size(bgav_input_context_t * ctx, int len);

/* Input module to read from memory */

bgav_input_context_t * bgav_input_open_memory(uint8_t * data,
                                              uint32_t data_size,
                                              const bgav_options_t*);

/* Reopen a memory input with new data and minimal CPU overhead */

bgav_input_context_t * bgav_input_open_as_buffer(bgav_input_context_t * input);

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
    int64_t time;  /* Time is scaled with the timescale of the stream */
    int duration;  /* In timescale tics, can be 0 if unknown */
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
                                int keyframe, int duration);

void bgav_superindex_seek(bgav_superindex_t * idx,
                          bgav_stream_t * s,
                          int64_t time, int scale);

void bgav_superindex_dump(bgav_superindex_t * idx);

void bgav_superindex_set_durations(bgav_superindex_t * idx, bgav_stream_t * s);

void bgav_superindex_merge_fileindex(bgav_superindex_t * idx, bgav_stream_t * s);
void bgav_superindex_set_size(bgav_superindex_t * ret, int size);

/*
 * File index
 *
 * fileindex.c
 */

typedef struct
  {
  int keyframe;     /* 1 if the frame is a keyframe   */
  /*
   * Seek positon:
   * 
   * For 1-layer muxed files, it's the
   * fseek() position, where the demuxer
   * can start to parse the packet header.
   *
   * For 2-layer muxed files, it's the
   * fseek() position of the lowest level
   * paket inside which the subpacket *starts*
   */
  
  uint64_t position; 

  /*
   *  Presentation time of the frame in
   *  format-based timescale (*not* in
   *  stream timescale)
   */

  uint64_t time;
  
  } bgav_file_index_entry_t;

struct bgav_file_index_s
  {
  uint32_t num_entries;
  uint32_t entries_alloc;
  bgav_file_index_entry_t * entries;
  };

bgav_file_index_t * bgav_file_index_create();
void bgav_file_index_destroy(bgav_file_index_t *);

int bgav_demuxer_next_packet_fileindex(bgav_demuxer_context_t * ctx);

void bgav_file_index_dump(bgav_t * b);


void
bgav_file_index_append_packet(bgav_file_index_t * idx,
                              int64_t position,
                              int64_t time,
                              int keyframe);

int bgav_file_index_read_header(const char * filename,
                                bgav_input_context_t * input,
                                int * num_tracks);

void bgav_file_index_write_header(const char * filename,
                                  FILE * output,
                                  int num_tracks);

int bgav_read_file_index(bgav_t*);

void bgav_write_file_index(bgav_t*);

int bgav_build_file_index(bgav_t * b, gavl_time_t * time_needed);

/* Demuxer class */

struct bgav_demuxer_s
  {
  int  (*probe)(bgav_input_context_t*);
  int  (*probe_yml)(bgav_yml_node_t*);

  int  (*open)(bgav_demuxer_context_t * ctx);
  int  (*open_yml)(bgav_demuxer_context_t * ctx,
                   bgav_yml_node_t * node);
  
  int  (*next_packet)(bgav_demuxer_context_t*);

  /*
   *  Seeking sets either the position- or the time
   *  member of the stream
   */
  
  void (*seek)(bgav_demuxer_context_t*, int64_t time, int scale);
  void (*close)(bgav_demuxer_context_t*);

  /* Some demuxers support multiple tracks. This can fail e.g.
     if the input must be seekable but isn't */

  int (*select_track)(bgav_demuxer_context_t*, int track);

  /* Some demuxers have their own magic to build a file index */
  //  void (*build_index)(bgav_demuxer_context_t*);

  /* Demuxers might need this function to update the internal state
     after seeking with the fileindex */
  
  void (*resync)(bgav_demuxer_context_t*, bgav_stream_t * s);
  };

/* Demuxer flags */

#define BGAV_DEMUXER_CAN_SEEK             (1<<0)
#define BGAV_DEMUXER_SEEK_ITERATIVE       (1<<1) /* If 1, perform iterative seeking */
#define BGAV_DEMUXER_PEEK_FORCES_READ     (1<<2) /* This is set if only subtitle streams are read */
#define BGAV_DEMUXER_SI_SEEKING           (1<<3) /* Demuxer is seeking */
//#define BGAV_DEMUXER_SI_NON_INTERLEAVED   (1<<4) /* Track is non-interleaved */
#define BGAV_DEMUXER_SI_PRIVATE_FUNCS     (1<<5) /* We have a suprindex but use private seek/demux funcs */
#define BGAV_DEMUXER_HAS_TIMESTAMP_OFFSET (1<<6) /* Timestamp offset (from input) is valid */
#define BGAV_DEMUXER_EOF                  (1<<7) /* Report EOF just once and not for each stream */
#define BGAV_DEMUXER_HAS_DATA_START       (1<<8) /* Has data start */

#define BGAV_DEMUXER_BUILD_INDEX          (1<<9) /* We're just building
                                                    an index */

#define INDEX_MODE_NONE   0 /* Default: No sample accuracy */
/* Packets have precise timestamps and durations and are adjacent in the file */
#define INDEX_MODE_SIMPLE 1
/* Packets have precise timestamps (but no durations) and are adjacent in the file */
#define INDEX_MODE_PTS    2
/* MPEG Program/transport stream: Needs complete parsing */
#define INDEX_MODE_MPEG   3
/* For PCM soundfiles: Sample accuracy is already there */
#define INDEX_MODE_PCM    4
/* File has a global index and codecs, which allow sample accuracy */
#define INDEX_MODE_SI_SA  5
/* File has a global index but codecs, which need complete parsing */
#define INDEX_MODE_SI_PARSE  6

/* Stream must be completely parsed, streams can have
   INDEX_MODE_SIMPLE, INDEX_MODE_MPEG or INDEX_MODE_PTS */
#define INDEX_MODE_MIXED  7

// #define INDEX_MODE_CUSTOM 4 /* Demuxer builds index */


#define DEMUX_MODE_STREAM 0
#define DEMUX_MODE_SI_I   1 /* Interleaved with superindex */
#define DEMUX_MODE_SI_NI  2 /* Non-interleaved with superindex */
#define DEMUX_MODE_FI     3 /* Non-interleaved with file-index */

struct bgav_demuxer_context_s
  {
  const bgav_options_t * opt;
  void * priv;
  const bgav_demuxer_t * demuxer;
  bgav_input_context_t * input;
  
  bgav_track_table_t * tt;
  char * stream_description;

  int index_mode;
  int demux_mode;
  uint32_t flags;
  
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
  
  /* Timestamp offset: By definition, timestamps for a track
     start at 0. The offset can be set either by the demuxer or
     by some inputs (DVD). */

  int64_t timestamp_offset;

  /* Data start (set in intially to -1, maybe set to
     other values by demuxers). It can be used by the core to
     seek to the position, where the demuxer can start working
  */
  int64_t data_start;

  /* Used by MPEG style fileindex for catching
     packets, inside which no frame starts */
  
  int64_t next_packet_pos;

  /* EDL */

  bgav_edl_t * edl;
  
  bgav_redirector_context_t * redirector;
  
  };

/* demuxer.c */

/*
 *  Create a demuxer.
 */

bgav_demuxer_context_t *
bgav_demuxer_create(const bgav_options_t * opt,
                    const bgav_demuxer_t * demuxer,
                    bgav_input_context_t * input);

const bgav_demuxer_t * bgav_demuxer_probe(bgav_input_context_t * input,
                                          bgav_yml_node_t ** yml);

void bgav_demuxer_create_buffers(bgav_demuxer_context_t * demuxer);
void bgav_demuxer_destroy(bgav_demuxer_context_t * demuxer);

bgav_packet_t *
bgav_demuxer_get_packet_read(bgav_demuxer_context_t * demuxer,
                             bgav_stream_t * s);


bgav_packet_t *
bgav_demuxer_peek_packet_read(bgav_demuxer_context_t * demuxer,
                              bgav_stream_t * s, int force);

void 
bgav_demuxer_done_packet_read(bgav_demuxer_context_t * demuxer,
                              bgav_packet_t *);


void
bgav_demuxer_seek(bgav_demuxer_context_t * demuxer,
                  int64_t time, int scale);

int
bgav_demuxer_next_packet(bgav_demuxer_context_t * demuxer);

/*
 *  Start a demuxer. Some demuxers (most notably quicktime)
 *  can contain nothing but urls for the real streams.
 *  In this case, redir (if not NULL) will contain the
 *  redirector context
 */

int bgav_demuxer_start(bgav_demuxer_context_t * ctx,
                       bgav_yml_node_t * yml);
void bgav_demuxer_stop(bgav_demuxer_context_t * ctx);


// bgav_packet_t *
// bgav_demuxer_get_packet_write(bgav_demuxer_context_t * demuxer, int stream);

// bgav_stream_t * bgav_track_find_stream(bgav_track_t * ctx, int stream_id);

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

  const bgav_options_t * opt;
  };

void bgav_redirector_destroy(bgav_redirector_context_t*r);
const bgav_redirector_t * bgav_redirector_probe(bgav_input_context_t * input);


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

  bgav_edl_dec_t * edl_dec;
  
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

#define BGAV_24LE_2_PTR(i, p) \
(p)[0] = (i) & 0xff; \
(p)[1] = ((i)>>8) & 0xff; \
(p)[2] = ((i)>>16) & 0xff;

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
char * bgav_sprintf(const char * format,...)   __attribute__ ((format (printf, 1, 2)));
char * bgav_strndup(const char * start, const char * end);
char * bgav_strdup(const char * str);

char * bgav_strncat(char * old, const char * start, const char * end);

void bgav_dprintf(const char * format, ...) __attribute__ ((format (printf, 1, 2)));
void bgav_diprintf(int indent, const char * format, ...)
  __attribute__ ((format (printf, 2, 3)));

int bgav_url_split(const char * url,
                   char ** protocol,
                   char ** user,
                   char ** password,
                   char ** hostname,
                   int * port,
                   char ** path);

char ** bgav_stringbreak(const char * str, char sep);
void bgav_stringbreak_free(char ** str);

int bgav_slurp_file(const char * location,
                    uint8_t ** ret,
                    int * ret_alloc,
                    int * size,
                    const bgav_options_t * opt);

char * bgav_search_file_write(const bgav_options_t * opt,
                              const char * directory, const char * file);

char * bgav_search_file_read(const bgav_options_t * opt,
                             const char * directory, const char * file);

int bgav_match_regexp(const char * str, const char * regexp);
  

/* Check if file exist and is readable */

int bgav_check_file_read(const char * filename);



/* Read a single line from a filedescriptor */

int bgav_read_line_fd(int fd, char ** ret, int * ret_alloc, int milliseconds);

int bgav_read_data_fd(int fd, uint8_t * ret, int size, int milliseconds);

/* tcp.c */

int bgav_tcp_connect(const bgav_options_t * opt,
                     const char * host, int port);

int bgav_tcp_send(const bgav_options_t * opt,
                  int fd, uint8_t * data, int len);

/* Charset utilities (charset.c) */


bgav_charset_converter_t *
bgav_charset_converter_create(const bgav_options_t * opt,
                              const char * in_charset,
                              const char * out_charset);

void bgav_charset_converter_destroy(bgav_charset_converter_t *);

char * bgav_convert_string(bgav_charset_converter_t *,
                           const char * in_string, int in_len,
                           int * out_len);

int bgav_convert_string_realloc(bgav_charset_converter_t * cnv,
                                const char * str, int len,
                                int * out_len,
                                char ** ret, int * ret_alloc);

/* audio.c */

void bgav_audio_dump(bgav_stream_t * s);

int bgav_audio_start(bgav_stream_t * s);
void bgav_audio_stop(bgav_stream_t * s);


void bgav_audio_resync(bgav_stream_t * stream);

/* Skip to a point in the stream, return 0 on EOF */

int bgav_audio_skipto(bgav_stream_t * stream, int64_t * t, int scale);

/* video.c */

void bgav_video_dump(bgav_stream_t * s);

int bgav_video_start(bgav_stream_t * s);
void bgav_video_stop(bgav_stream_t * s);

void bgav_video_resync(bgav_stream_t * stream);

int bgav_video_skipto(bgav_stream_t * stream, int64_t * t, int scale);

/* subtitle.c */

void bgav_subtitle_dump(bgav_stream_t * s);

int bgav_subtitle_start(bgav_stream_t * s);
void bgav_subtitle_stop(bgav_stream_t * s);

void bgav_subtitle_resync(bgav_stream_t * stream);

int bgav_subtitle_skipto(bgav_stream_t * stream, int64_t * t, int scale);


/* codecs.c */

void bgav_codecs_init(bgav_options_t * opt);

bgav_audio_decoder_t * bgav_find_audio_decoder(bgav_stream_t*);
bgav_video_decoder_t * bgav_find_video_decoder(bgav_stream_t*);
bgav_subtitle_overlay_decoder_t * bgav_find_subtitle_overlay_decoder(bgav_stream_t*);

void bgav_audio_decoder_register(bgav_audio_decoder_t * dec);
void bgav_video_decoder_register(bgav_video_decoder_t * dec);
void bgav_subtitle_overlay_decoder_register(bgav_subtitle_overlay_decoder_t * dec);

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

/* For debugging only */

void bgav_device_info_dump(bgav_device_info_t * arr);

/* languages.c */

const char * bgav_lang_from_name(const char * name);

const char * bgav_lang_from_twocc(const char * twocc);

const char * bgav_lang_name(const char * lang);

void bgav_correct_language(char * lang);

/* subreader.c */

struct bgav_subtitle_reader_context_s
  {
  bgav_input_context_t * input;
  const bgav_subtitle_reader_t * reader;

  char * info; /* Derived from filename difference */
  char * filename; /* Name of the subtitle file */
  bgav_packet_t * p;
  gavl_overlay_t  ovl;

  char * line;
  int line_alloc;
  
  /* 1 if the packet or overlay contains a not yet read
     subtitle */
  
  int has_subtitle; 

  /* Some formats have a header... */
  int64_t data_start;
  
  /* bgav_subtitle_reader_open returns a chained list */
  bgav_subtitle_reader_context_t * next;

  /* Private data */
  void * priv;
  };

struct bgav_subtitle_reader_s
  {
  char * extensions;
  char * name;

  int (*probe)(char * line);
  
  int (*init)(bgav_stream_t*);
  void (*close)(bgav_stream_t*);
  void (*seek)(bgav_stream_t*,int64_t time, int scale);
  
  int (*read_subtitle_text)(bgav_stream_t*);
  int (*read_subtitle_overlay)(bgav_stream_t*);
  };

bgav_subtitle_reader_context_t *
bgav_subtitle_reader_open(bgav_input_context_t * input_ctx);


int bgav_subtitle_reader_start(bgav_stream_t *);

void bgav_subtitle_reader_stop(bgav_stream_t *);
void bgav_subtitle_reader_destroy(bgav_stream_t *);

void bgav_subtitle_reader_seek(bgav_stream_t *,
                               int64_t time, int scale);

int bgav_subtitle_reader_has_subtitle(bgav_stream_t * s);

bgav_packet_t * bgav_subtitle_reader_read_text(bgav_stream_t *);

int bgav_subtitle_reader_read_overlay(bgav_stream_t *, gavl_overlay_t * ovl);

/* log.c */

void bgav_log(const bgav_options_t * opt,
              bgav_log_level_t level,
              const char * domain, const char * format, ...)
  __attribute__ ((format (printf, 4, 5)));

/* bytebuffer.c */

typedef struct
  {
  uint8_t * buffer;
  int size;
  int alloc;
  } bgav_bytebuffer_t;

void bgav_bytebuffer_append(bgav_bytebuffer_t * b, bgav_packet_t * p, int padding);
void bgav_bytebuffer_remove(bgav_bytebuffer_t * b, int bytes);
void bgav_bytebuffer_free(bgav_bytebuffer_t * b);
void bgav_bytebuffer_flush(bgav_bytebuffer_t * b);

/* sampleseek.c */
int bgav_set_sample_accurate(bgav_t * b);

/* edl.c */

bgav_edl_t * bgav_edl_create();

bgav_edl_track_t * bgav_edl_add_track(bgav_edl_t * e);

bgav_edl_stream_t * bgav_edl_add_audio_stream(bgav_edl_track_t * t);

bgav_edl_stream_t * bgav_edl_add_video_stream(bgav_edl_track_t * t);

bgav_edl_stream_t * bgav_edl_add_subtitle_text_stream(bgav_edl_track_t * t);

bgav_edl_stream_t * bgav_edl_add_subtitle_overlay_stream(bgav_edl_track_t * t);

bgav_edl_segment_t * bgav_edl_add_segment(bgav_edl_stream_t * s);

bgav_edl_t * bgav_edl_copy(const bgav_edl_t * e);

void bgav_edl_destroy(bgav_edl_t * e);


/* Translation specific stuff */

void bgav_translation_init();

/* For dynamic strings */
#define TRD(s) dgettext(PACKAGE, (s))

/* For static strings */
#define TRS(s) (s)

/* timecode.c */

typedef struct
  {
  int64_t pts;
  gavl_timecode_t timecode;
  } bgav_timecode_table_entry_t;

struct bgav_timecode_table_s
  {
  int num_entries;
  bgav_timecode_table_entry_t * entries;
  };

bgav_timecode_table_t *
bgav_timecode_table_create(int num);

void
bgav_timecode_table_destroy(bgav_timecode_table_t *);

gavl_timecode_t
bgav_timecode_table_get_timecode(bgav_timecode_table_t * table,
                                 int64_t pts);
