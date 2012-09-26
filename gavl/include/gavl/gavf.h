#include <inttypes.h>
#include <gavl/gavl.h>
#include <gavl/compression.h>
#include <gavl/chapterlist.h>
#include <gavl/metadata.h>
#include <gavl/gavldefs.h>

#include <stdio.h>

typedef struct gavf_s gavf_t;
typedef struct gavf_options_s gavf_options_t;

/* I/O Structure */

typedef int (*gavf_read_func)(void * priv, uint8_t * data, int len);
typedef int (*gavf_write_func)(void * priv, const uint8_t * data, int len);
typedef int64_t (*gavf_seek_func)(void * priv, int64_t pos, int whence);
typedef void (*gavf_close_func)(void * priv);
typedef void (*gavf_flush_func)(void * priv);


typedef struct gavl_io_s gavf_io_t;

GAVL_PUBLIC
gavf_io_t * gavf_io_create(gavf_read_func  r,
                           gavf_write_func w,
                           gavf_seek_func  s,
                           gavf_close_func c,
                           gavf_flush_func f,
                           void * data);

GAVL_PUBLIC
void gavf_io_destroy(gavf_io_t *);

GAVL_PUBLIC
void gavf_io_flush(gavf_io_t *);

GAVL_PUBLIC
gavf_io_t * gavf_io_create_file(FILE * f, int wr, int can_seek);

/* Stream information */

#define GAVF_STREAM_AUDIO 0
#define GAVF_STREAM_VIDEO 1
#define GAVF_STREAM_TEXT  2

typedef struct
  {
  uint32_t type;
  uint32_t id;
  
  gavl_compression_info_t ci;

  union
    {
    gavl_audio_format_t audio;
    gavl_video_format_t video;

    struct
      {
      uint32_t timescale;
      } text;

    } format;

  gavl_metadata_t m;
  
  } gavf_stream_header_t;

typedef struct
  {
  uint32_t num_streams;
  gavf_stream_header_t * streams;
  gavl_metadata_t m;
  } gavf_program_header_t;

typedef struct
  {
  uint32_t stream_id;
  } gavf_packet_header_t;

/* Options */

#define GAVF_OPT_FLAG_SYNC_INDEX   (1<<0)
#define GAVF_OPT_FLAG_PACKET_INDEX (1<<1)
#define GAVF_OPT_FLAG_INTERLEAVE   (1<<2)

#define GAVF_OPT_FLAG_DUMP_HEADERS (1<<3)
#define GAVF_OPT_FLAG_DUMP_INDICES (1<<4)
#define GAVF_OPT_FLAG_DUMP_PACKETS (1<<5)


GAVL_PUBLIC
void gavf_options_set_flags(gavf_options_t *, int flags);

GAVL_PUBLIC
void gavf_options_set_sync_distance(gavf_options_t *, gavl_time_t sync_distance);


/* General functions */

GAVL_PUBLIC
gavf_t * gavf_create();

GAVL_PUBLIC
void gavf_close(gavf_t *);

GAVL_PUBLIC
gavf_options_t * gavf_get_options(gavf_t *);


/* Read support */

GAVL_PUBLIC
int gavf_open_read(gavf_t * g, gavf_io_t * io);

GAVL_PUBLIC
const gavl_chapter_list_t * gavf_get_chapter_list(gavf_t * g);

GAVL_PUBLIC
gavf_program_header_t * gavf_get_program_header(gavf_t *);

GAVL_PUBLIC
const gavf_packet_header_t * gavf_packet_read_header(gavf_t * gavf);

GAVL_PUBLIC
void gavf_packet_skip(gavf_t * gavf);

GAVL_PUBLIC
int gavf_packet_read_packet(gavf_t * gavf, gavl_packet_t * p);

GAVL_PUBLIC
int gavf_reset(gavf_t * gavf);

/* Get the first PTS of all streams */

GAVL_PUBLIC
const int64_t * gavf_first_pts(gavf_t * gavf);

/* Get the last PTS of the streams */

GAVL_PUBLIC
const int64_t * gavf_end_pts(gavf_t * gavf);

/* Seek to a specific time. Return the sync timestamps of
   all streams at the current position */

GAVL_PUBLIC
const int64_t * gavf_seek(gavf_t * gavf, int64_t time, int scale);

/* Read uncompressed frames */

GAVL_PUBLIC
void gavf_packet_to_video_frame(gavl_packet_t * p, gavl_video_frame_t * frame,
                                const gavl_video_format_t * format);

GAVL_PUBLIC
void gavf_packet_to_audio_frame(gavl_packet_t * p, gavl_audio_frame_t * frame,
                                const gavl_audio_format_t * format);

/* These copy *only* the metadata */
GAVL_PUBLIC
void gavf_video_frame_to_packet_metadata(const gavl_video_frame_t * frame,
                                         gavl_packet_t * p);

GAVL_PUBLIC
void gavf_audio_frame_to_packet_metadata(const gavl_audio_frame_t * frame,
                                         gavl_packet_t * p);


/* Write support */

GAVL_PUBLIC
int gavf_open_write(gavf_t * g, gavf_io_t * io,
                    const gavl_metadata_t * m,
                    const gavl_chapter_list_t * cl);

/*
 *  Return value: >= 0 is the stream ID passed to gavf_write_packet()
 *  < 0 means error
 */

GAVL_PUBLIC
int gavf_add_audio_stream(gavf_t * g,
                          const gavl_compression_info_t * ci,
                          const gavl_audio_format_t * format,
                          const gavl_metadata_t * m);

GAVL_PUBLIC
int gavf_add_video_stream(gavf_t * g,
                          const gavl_compression_info_t * ci,
                          gavl_video_format_t * format,
                          const gavl_metadata_t * m);

GAVL_PUBLIC
int gavf_add_text_stream(gavf_t * g,
                         uint32_t timescale,
                         const gavl_metadata_t * m);

GAVL_PUBLIC
int gavf_write_packet(gavf_t *, int stream, const gavl_packet_t * p);

GAVL_PUBLIC
int gavf_write_video_frame(gavf_t *, int stream, gavl_video_frame_t * frame);

GAVL_PUBLIC
int gavf_write_audio_frame(gavf_t *, int stream, gavl_audio_frame_t * frame);

GAVL_PUBLIC
int gavf_update_metadata(gavf_t *, gavl_metadata_t * m);

