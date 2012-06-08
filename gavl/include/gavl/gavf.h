#include <inttypes.h>
#include <gavl/gavl.h>
#include <gavl/compression.h>
#include <gavl/metadata.h>

typedef struct gavf_s gavf_t;
typedef struct gavf_options_s gavf_options_t;

/* I/O Structure */

typedef int (*gavf_read_func)(void * priv, uint8_t * data, int len);
typedef int (*gavf_write_func)(void * priv, const uint8_t * data, int len);
typedef int64_t (*gavf_seek_func)(void * priv, int64_t pos, int whence);
typedef void (*gavf_close_func)(void * priv);


typedef struct gavl_io_s gavf_io_t;

gavf_io_t * gavf_io_create(gavf_read_func  r,
                           gavf_write_func w,
                           gavf_seek_func  s,
                           gavf_close_func c,
                           void * data);

void gavf_io_destroy(gavf_io_t *);

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

  /* Secondary variables */
  int timescale;
  int packet_duration;
  int64_t last_sync_pts;
  int64_t last_pts;
  
  int has_pts;
  
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
  uint32_t len;
  } gavf_packet_header_t;


/* General functions */

gavf_t * gavf_create();
void gavf_close(gavf_t *);

gavf_options_t * gavf_get_options(gavf_t *);


/* Read support */

int gavf_open_read(gavf_t * g, gavf_io_t * io);
const gavf_program_header_t * gavf_get_program_header(gavf_t *);
const gavf_packet_header_t * gavf_packet_read_header(gavf_t * gavf);

void gavf_packet_skip(gavf_t * gavf);
void gavf_packet_read_packet(gavf_t * gavf, gavl_packet_t * p);

/* Write support */

int gavf_open_write(gavf_t * g, gavf_io_t * io);

/*
 *  Return value: >= 0 is the stream ID passed to gavf_write_packet()
 *  < 0 means error
 */

int gavf_add_audio_stream(gavf_t * g,
                          const gavl_compression_info_t * ci,
                          const gavl_audio_format_t * format,
                          const gavl_metadata_t * m);

int gavf_add_video_stream(gavf_t * g,
                          const gavl_compression_info_t * ci,
                          const gavl_video_format_t * format,
                          const gavl_metadata_t * m);

int gavf_add_text_stream(gavf_t * g,
                         uint32_t timescale,
                         const gavl_metadata_t * m);

int gavf_write_packet(gavf_t *, int stream, gavl_packet_t * p);
int gavf_update_metadata(gavf_t *, gavl_metadata_t * m);

