#include "gavf.h"

/* Buffer */

typedef struct
  {
  uint8_t * buf;
  int len;
  int alloc;
  int alloc_static;
  
  int pos;
  } gavf_buffer_t;

void gavf_buffer_init(gavf_buffer_t * buf);

void gavf_buffer_init_static(gavf_buffer_t * buf, uint8_t * data, int size);


int gavf_buffer_alloc(gavf_buffer_t * buf,
                      int size);

void gavf_buffer_free(gavf_buffer_t * buf);

void gavf_buffer_reset(gavf_buffer_t * buf);

/* I/O */

struct gavl_io_s
  {
  int (*read_func)(void * priv, uint8_t * data, int len);
  int (*write_func)(void * priv, const uint8_t * data, int len);
  int64_t (*seek_func)(void * priv, int64_t pos, int whence);
  void (*close_func)(void * priv);
  void * priv;

  int64_t position;
  };

void gavf_io_init(gavf_io_t * ret,
                  gavf_read_func  r,
                  gavf_write_func w,
                  gavf_seek_func  s,
                  gavf_close_func c,
                  void * priv);

int64_t gavf_io_seek(gavf_io_t * io, int64_t pos, int whence);


int gavf_io_read_data(gavf_io_t * io, uint8_t * buf, int len);
int gavf_io_write_data(gavf_io_t * io, const uint8_t * buf, int len);

int gavf_io_write_uint64f(gavf_io_t * io, uint64_t num);
int gavf_io_read_uint64f(gavf_io_t * io, uint64_t * num);

int gavf_io_write_uint64v(gavf_io_t * io, uint64_t num);
int gavf_io_read_uint64v(gavf_io_t * io, uint64_t * num);

int gavf_io_write_uint32v(gavf_io_t * io, uint32_t num);
int gavf_io_read_uint32v(gavf_io_t * io, uint32_t * num);

int gavf_io_write_int64v(gavf_io_t * io, int64_t num);
int gavf_io_read_int64v(gavf_io_t * io, int64_t * num);

int gavf_io_write_int32v(gavf_io_t * io, int32_t num);
int gavf_io_read_int32v(gavf_io_t * io, int32_t * num);

int gavf_io_read_string(gavf_io_t * io, char **);
int gavf_io_write_string(gavf_io_t * io, const char * );

int gavf_io_read_buffer(gavf_io_t * io, gavf_buffer_t * ret);
int gavf_io_write_buffer(gavf_io_t * io,  const gavf_buffer_t * buf);

int gavf_io_read_float(gavf_io_t * io, float * num);
int gavf_io_write_float(gavf_io_t * io, float num);

void gavf_io_skip(gavf_io_t * io, int bytes);


/* Buffer as io */

gavf_io_t * gavf_io_create_buf_read(gavf_buffer_t * buf);
gavf_io_t * gavf_io_create_buf_write(gavf_buffer_t * buf);

void gavf_io_init_buf_read(gavf_io_t * io, gavf_buffer_t * buf);
void gavf_io_init_buf_write(gavf_io_t * io, gavf_buffer_t * buf);


/* Streamheader */

void gavf_stream_header_free(gavf_stream_header_t * h);

int gavf_stream_header_read(gavf_io_t * io, gavf_stream_header_t * h);
int gavf_stream_header_write(gavf_io_t * io, const gavf_stream_header_t * h);

void gavf_stream_header_init_audio(gavf_stream_header_t * h);
void gavf_stream_header_init_video(gavf_stream_header_t * h);
void gavf_stream_header_init_text(gavf_stream_header_t * h);

/* Program header */

int gavf_program_header_add_audio_stream(gavf_program_header_t * ph,
                                         const gavl_compression_info_t * ci,
                                         const gavl_audio_format_t * format,
                                         const gavl_metadata_t * m);
int gavf_program_header_add_video_stream(gavf_program_header_t * ph,
                                         const gavl_compression_info_t * ci,
                                         const gavl_video_format_t * format,
                                         const gavl_metadata_t * m);
int gavf_program_header_add_text_stream(gavf_program_header_t * ph,
                                        uint32_t timescale,
                                        const gavl_metadata_t * m);
int gavf_program_header_read(gavf_io_t * io, gavf_program_header_t * ph);
int gavf_program_header_write(gavf_io_t * io,
                              const gavf_program_header_t * ph);
void gavf_program_header_free(gavf_program_header_t * ph);

/* Packetbuffer */

typedef struct gavf_packet_buffer_s gavf_packet_buffer_t;

gavf_packet_buffer_t * gavf_packet_buffer_create(int timescale);

gavl_packet_t * gavf_packet_buffer_get_write(gavf_packet_buffer_t *);
gavl_packet_t * gavf_packet_buffer_get_read(gavf_packet_buffer_t *);

gavl_time_t gavf_packet_buffer_get_min_pts(gavf_packet_buffer_t * b);

void gavf_packet_buffer_destroy(gavf_packet_buffer_t *);

/* Stream */

typedef struct
  {
  gavf_stream_header_t * h;

  /* Secondary variables */
  int timescale;
  int packet_duration;
  int64_t last_sync_pts;
  int64_t next_pts;
  int has_pts;
  
  gavf_packet_buffer_t * pb;

  /* Video stuff */
  int image_size;
  
  } gavf_stream_t;

/* Formats */

int gavf_read_audio_format(gavf_io_t * io, gavl_audio_format_t * format);
int gavf_write_audio_format(gavf_io_t * io, const gavl_audio_format_t * format);

int gavf_read_video_format(gavf_io_t * io, gavl_video_format_t * format);
int gavf_write_video_format(gavf_io_t * io, const gavl_video_format_t * format);

/* Compression info */

int gavf_read_compression_info(gavf_io_t * io, gavl_compression_info_t * ci);
int gavf_write_compression_info(gavf_io_t * io, const gavl_compression_info_t * ci);

/* Metadata */

int gavf_read_metadata(gavf_io_t * io, gavl_metadata_t * ci);
int gavf_write_metadata(gavf_io_t * io, const gavl_metadata_t * ci);

/* Packet */
int gavf_read_gavl_packet(gavf_io_t * io,
                          gavf_stream_t * s,
                          gavl_packet_t * p);

int gavf_write_gavl_packet(gavf_io_t * io,
                           gavf_stream_t * s,
                           const gavl_packet_t * p);


/* Options */

struct gavf_options_s
  {
  uint32_t flags;
  };

/* Extension header */

typedef struct
  {
  uint32_t key;
  uint32_t len;
  } gavl_extension_header_t;

int gavf_extension_header_read(gavf_io_t * io, gavl_extension_header_t * eh);
int gavf_extension_write(gavf_io_t * io, uint32_t key, uint32_t len,
                         uint8_t * data);

/* Known extensions */

/* Audio format */

#define GAVF_EXT_AF_SAMPLESPERFRAME 0
#define GAVF_EXT_AF_SAMPLEFORMAT    1
#define GAVF_EXT_AF_INTERLEAVE      2
#define GAVF_EXT_AF_CENTER_LEVEL    3
#define GAVF_EXT_AF_REAR_LEVEL      4

/* Video format */

#define GAVF_EXT_VF_PIXELFORMAT     0
#define GAVF_EXT_VF_PIXEL_ASPECT    1
#define GAVF_EXT_VF_INTERLACE       2
#define GAVF_EXT_VF_FRAME_SIZE      3

/* Compresson info */

#define GAVF_EXT_CI_GLOBAL_HEADER    0

/* Packet */

#define GAVF_EXT_PK_DURATION         0

/* File index */

#define GAVF_TAG_FILE_INDEX     "GAVFFIDX"
#define GAVF_TAG_PROGRAM_HEADER "GAVFPHDR"
#define GAVF_TAG_SYNC_HEADER    "GAVFSYNC"
#define GAVF_TAG_SYNC_INDEX     "GAVFSIDX"
#define GAVF_TAG_PACKET_INDEX   "GAVFPIDX"

#define GAVF_TAG_PACKET_HEADER    "P"
#define GAVF_TAG_PACKET_HEADER_C  'P'

#define GAVF_TAG_METADATA_HEADER   "M"
#define GAVF_TAG_METADATA_HEADER_C 'M'

typedef struct
  {
  uint32_t num_entries;
  uint32_t entries_alloc;
  
  struct
    {
    uint8_t tag[8];
    uint64_t position;
    } * entries;
  } gavf_file_index_t;

void gavf_file_index_init(gavf_file_index_t * fi, int num_entries);
int gavf_file_index_read(gavf_io_t * io, gavf_file_index_t * fi);
int gavf_file_index_write(gavf_io_t * io, const gavf_file_index_t * fi);
void gavf_file_index_free(gavf_file_index_t * fi);
void gavf_file_index_add(gavf_file_index_t * fi, char * tag, int64_t position);

typedef struct
  {
  uint64_t num_entries;
  uint64_t entries_alloc;
  
  struct
    {
    uint32_t id;
    uint32_t flags;    // Same as the packet flags
    
    uint64_t pos;
    int64_t pts;
    } * entries;
  } gavf_packet_index_t;

void gavf_packet_index_add(gavf_packet_index_t * idx,
                           uint32_t id, uint32_t flags, uint64_t pos,
                           int64_t pts);

int gavf_packet_index_read(gavf_io_t * io, gavf_packet_index_t * idx);
int gavf_packet_index_write(gavf_io_t * io, const gavf_packet_index_t * idx);
void gavf_packet_index_free(gavf_packet_index_t * idx);

typedef struct
  {
  uint64_t num_entries;
  uint64_t entries_alloc;

  struct
    {
    uint64_t pos;
    int64_t * pts;
    } * entries;

  /* Secondary variables (not in the file) */
  int num_streams;
  int pts_len;
  
  } gavf_sync_index_t;

void gavf_sync_index_init(gavf_sync_index_t * idx, int num_streams);


void gavf_sync_index_add(gavf_sync_index_t * idx,
                         uint64_t pos, int64_t * pts);

int gavf_sync_index_read(gavf_io_t * io, gavf_sync_index_t * idx);
int gavf_sync_index_write(gavf_io_t * io, const gavf_sync_index_t * idx);
void gavf_sync_index_free(gavf_sync_index_t * idx);
