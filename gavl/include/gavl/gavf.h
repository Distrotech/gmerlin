
#ifndef GAVF_H_INCLUDED
#define GAVF_H_INCLUDED


#include <inttypes.h>
#include <gavl/gavl.h>
#include <gavl/compression.h>
#include <gavl/chapterlist.h>
#include <gavl/metadata.h>
#include <gavl/gavldefs.h>
#include <gavl/connectors.h>

#include <stdio.h>

typedef struct gavf_s gavf_t;
typedef struct gavf_options_s gavf_options_t;

#define GAVF_IO_CB_PROGRAM_HEADER 0
#define GAVF_IO_CB_PACKET         1
#define GAVF_IO_CB_METADATA       2

/* I/O Structure */

typedef int (*gavf_read_func)(void * priv, uint8_t * data, int len);
typedef int (*gavf_write_func)(void * priv, const uint8_t * data, int len);
typedef int64_t (*gavf_seek_func)(void * priv, int64_t pos, int whence);
typedef void (*gavf_close_func)(void * priv);
typedef int (*gavf_flush_func)(void * priv);

typedef int (*gavf_io_cb_func)(void * priv, int type, const void * data);

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
int gavf_io_flush(gavf_io_t *);

GAVL_PUBLIC
gavf_io_t * gavf_io_create_file(FILE * f, int wr, int can_seek, int close);

GAVL_PUBLIC
void gavf_io_set_cb(gavf_io_t * io, gavf_io_cb_func cb, void * cb_priv);

GAVL_PUBLIC
int gavf_io_got_error(gavf_io_t * io);

/* Stream information */

typedef enum
  {
    GAVF_STREAM_AUDIO   = 0,
    GAVF_STREAM_VIDEO   = 1,
    GAVF_STREAM_TEXT    = 2,
    GAVF_STREAM_OVERLAY = 3,
  } gavf_stream_type_t;

/* Return short name */
GAVL_PUBLIC
const char * gavf_stream_type_name(gavf_stream_type_t t);

typedef struct
  {
  uint32_t size_min;
  uint32_t size_max;
  int64_t  duration_min;
  int64_t  duration_max;
  int64_t  pts_start;
  int64_t  pts_end;
  } gavf_stream_footer_t;

typedef struct
  {
  gavf_stream_type_t type;
  uint32_t id;
  
  gavl_compression_info_t ci;

  union
    {
    gavl_audio_format_t audio;
    gavl_video_format_t video; // Video and overlay streams

    struct
      {
      uint32_t timescale;
      } text;
    
    } format;

  gavl_metadata_t m;
  gavf_stream_footer_t foot;
  } gavf_stream_header_t;

GAVL_PUBLIC
void gavf_stream_header_dump(const gavf_stream_header_t * h);


typedef struct
  {
  uint32_t num_streams;
  gavf_stream_header_t * streams;
  gavl_metadata_t m;
  } gavf_program_header_t;

GAVL_PUBLIC
void gavf_program_header_dump(gavf_program_header_t * ph);


typedef struct
  {
  uint32_t stream_id;
  } gavf_packet_header_t;

typedef void (*gavf_stream_skip_func)(gavf_t * gavf,
                                      const gavf_stream_header_t * h,
                                      void * priv);

/* Options */

#define GAVF_OPT_FLAG_SYNC_INDEX   (1<<0)
#define GAVF_OPT_FLAG_PACKET_INDEX (1<<1)
#define GAVF_OPT_FLAG_INTERLEAVE   (1<<2)
#define GAVF_OPT_FLAG_BUFFER_READ  (1<<3)

#define GAVF_OPT_FLAG_DUMP_HEADERS (1<<4)
#define GAVF_OPT_FLAG_DUMP_INDICES (1<<5)
#define GAVF_OPT_FLAG_DUMP_PACKETS (1<<6)
#define GAVF_OPT_FLAG_DUMP_METADATA (1<<7)


GAVL_PUBLIC
void gavf_options_set_flags(gavf_options_t *, int flags);

GAVL_PUBLIC
int gavf_options_get_flags(gavf_options_t *);

GAVL_PUBLIC
void gavf_options_set_sync_distance(gavf_options_t *,
                                    gavl_time_t sync_distance);

GAVL_PUBLIC
void gavl_options_set_metadata_callback(gavf_options_t *, 
                                        void (*cb)(void*,const gavl_metadata_t*),
                                        void *cb_priv);

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
int gavf_get_num_streams(gavf_t *, int type);

GAVL_PUBLIC
const gavf_stream_header_t * gavf_get_stream(gavf_t *, int index, int type);

GAVL_PUBLIC
const gavf_packet_header_t * gavf_packet_read_header(gavf_t * gavf);

/* Mark this stream as skipable. */
GAVL_PUBLIC
void gavf_stream_set_skip(gavf_t * gavf, uint32_t id,
                          gavf_stream_skip_func func, void * priv);

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

/* frame must be allocated with the maximum size already! */

GAVL_PUBLIC
void gavf_packet_to_overlay(gavl_packet_t * p,
                            gavl_video_frame_t * frame,
                            const gavl_video_format_t * format);

/* Packet must be allocated with the maximum size already! */
GAVL_PUBLIC
void gavf_overlay_to_packet(gavl_video_frame_t * frame, 
                            gavl_packet_t * p,
                            const gavl_video_format_t * format);

/* These copy *only* the metadata */
GAVL_PUBLIC
void gavf_video_frame_to_packet_metadata(const gavl_video_frame_t * frame,
                                         gavl_packet_t * p);

GAVL_PUBLIC
void gavf_audio_frame_to_packet_metadata(const gavl_audio_frame_t * frame,
                                         gavl_packet_t * p);

GAVL_PUBLIC
void gavf_shrink_audio_frame(gavl_audio_frame_t * f, 
                             gavl_packet_t * p, 
                             const gavl_audio_format_t * format);

/* Write support */

GAVL_PUBLIC
int gavf_open_write(gavf_t * g, gavf_io_t * io,
                    const gavl_metadata_t * m,
                    const gavl_chapter_list_t * cl);

/*
 *  Return value: >= 0 is the stream index passed to gavf_write_packet()
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
                          const gavl_video_format_t * format,
                          const gavl_metadata_t * m);

GAVL_PUBLIC
int gavf_add_text_stream(gavf_t * g,
                         uint32_t timescale,
                         const gavl_metadata_t * m);

GAVL_PUBLIC
int gavf_add_overlay_stream(gavf_t * g,
                            const gavl_compression_info_t * ci,
                            const gavl_video_format_t * format,
                            const gavl_metadata_t * m);


/* Call this after adding all streams and before writing the first packet */
GAVL_PUBLIC
int gavf_start(gavf_t * g);

// GAVL_PUBLIC
// int gavf_write_packet(gavf_t *, int stream, const gavl_packet_t * p);

GAVL_PUBLIC
int gavf_write_video_frame(gavf_t *, int stream, gavl_video_frame_t * frame);

GAVL_PUBLIC
int gavf_write_audio_frame(gavf_t *, int stream, gavl_audio_frame_t * frame);

GAVL_PUBLIC
int gavf_update_metadata(gavf_t *, const gavl_metadata_t * m);

GAVL_PUBLIC gavl_packet_sink_t *
gavf_get_packet_sink(gavf_t *, uint32_t id);

GAVL_PUBLIC gavl_audio_sink_t *
gavf_get_audio_sink(gavf_t *, uint32_t id);

GAVL_PUBLIC gavl_video_sink_t *
gavf_get_video_sink(gavf_t *, uint32_t id);

GAVL_PUBLIC gavl_packet_source_t *
gavf_get_packet_source(gavf_t *, uint32_t id);

GAVL_PUBLIC gavl_audio_source_t *
gavf_get_audio_source(gavf_t *, uint32_t id);

GAVL_PUBLIC gavl_video_source_t *
gavf_get_video_source(gavf_t *, uint32_t id);

/* Utility functions */

GAVL_PUBLIC 
int gavf_get_max_audio_packet_size(const gavl_audio_format_t * fmt,
                                   const gavl_compression_info_t * ci);

GAVL_PUBLIC 
int gavf_get_max_video_packet_size(const gavl_video_format_t * fmt,
                                   const gavl_compression_info_t * ci);



/* Ultra simple image format */

GAVL_PUBLIC
int gavl_image_write_header(gavf_io_t * io,
                            const gavl_metadata_t * m,
                            const gavl_video_format_t * v);

GAVL_PUBLIC
int gavl_image_write_image(gavf_io_t * io,
                           const gavl_video_format_t * v,
                           gavl_video_frame_t * frame);

GAVL_PUBLIC
int gavl_image_read_header(gavf_io_t * io,
                           gavl_metadata_t * m,
                           gavl_video_format_t * v);

GAVL_PUBLIC
int gavl_image_read_image(gavf_io_t * io,
                          gavl_video_format_t * v,
                          gavl_video_frame_t * f);

#endif // GAVF_H_INCLUDED
