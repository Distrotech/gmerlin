/*****************************************************************
 * gmerlin - a general purpose multimedia framework and applications
 *
 * Copyright (c) 2001 - 2010 Members of the Gmerlin project
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

#define BG_MK_FOURCC(a, b, c, d) ((a<<24)|(b<<16)|(c<<8)|d)

typedef struct
  {
  int (*read_callback)(void * priv, uint8_t * data, int len);
  int (*write_callback)(void * priv, const uint8_t * data, int len);
  int64_t (*ftell_callback)(void * priv);
  int64_t (*seek_callback)(void * priv, int64_t pos, int whence);
  void (*close_callback)(void * priv);
  void * data;

  uint8_t * buffer;
  int buffer_alloc;
  } bg_f_io_t;

int bg_f_io_open_stdio_read(bg_f_io_t * io, const char * filename);
int bg_f_io_open_stdio_write(bg_f_io_t * io, const char * filename);

void bg_f_io_close(bg_f_io_t * io);


typedef enum
  {
    CHUNK_TYPE_SIGNATURE           = BG_MK_FOURCC('G','A','V','L'), 
    CHUNK_TYPE_AUDIO_STREAM_HEADER = BG_MK_FOURCC('A','S','T','R'),
    CHUNK_TYPE_VIDEO_STREAM_HEADER = BG_MK_FOURCC('V','S','T','R'),
    CHUNK_TYPE_AUDIO_FORMAT        = BG_MK_FOURCC('A','F','M','T'),
    CHUNK_TYPE_VIDEO_FORMAT        = BG_MK_FOURCC('V','F','M','T'),

    CHUNK_TYPE_AUDIO_FRAME         = BG_MK_FOURCC('A','F','R','M'),
    CHUNK_TYPE_VIDEO_FRAME         = BG_MK_FOURCC('V','F','R','M'),
  } bg_f_chunk_type_t;

typedef enum
  {
    SIG_TYPE_IMAGE               = BG_MK_FOURCC('I','M','A','G'), 
  } bg_f_sig_type_t;

typedef struct
  {
  uint32_t type;
  uint64_t size;
  uint64_t start; /* Set during writing */
  } bg_f_chunk_t;

int bg_f_chunk_read(bg_f_io_t * io, bg_f_chunk_t * ch);
int bg_f_chunk_write_header(bg_f_io_t * io, bg_f_chunk_t * ch, bg_f_chunk_type_t type);
int bg_f_chunk_write_footer(bg_f_io_t * io, bg_f_chunk_t * ch);

typedef struct
  {
  uint32_t type;
  } bg_f_signature_t;

int bg_f_signature_read(bg_f_io_t * io, bg_f_chunk_t * ch, bg_f_signature_t * sig);
int bg_f_signature_write(bg_f_io_t * io, bg_f_signature_t * sig);

int bg_f_audio_format_read(bg_f_io_t * io, bg_f_chunk_t * ch, gavl_audio_format_t * f, int * big_endian);
int bg_f_audio_format_write(bg_f_io_t * io, const gavl_audio_format_t * f);

int bg_f_video_format_read(bg_f_io_t * io, bg_f_chunk_t * ch, gavl_video_format_t * f, int * big_endian);
int bg_f_video_format_write(bg_f_io_t * io, const gavl_video_format_t * f);

int bg_f_audio_frame_read(bg_f_io_t * io, bg_f_chunk_t * ch,
                          const gavl_audio_format_t * format,
                          gavl_audio_frame_t * f, int big_endian,
                          gavl_dsp_context_t * ctx);

int bg_f_audio_frame_write(bg_f_io_t * io,
                           const gavl_audio_format_t * format,
                           const gavl_audio_frame_t * f);

int bg_f_video_frame_read(bg_f_io_t * io, bg_f_chunk_t * ch,
                          const gavl_video_format_t * format,
                          gavl_video_frame_t * f, int big_endian,
                          gavl_dsp_context_t * ctx);

int bg_f_video_frame_write(bg_f_io_t * io,
                           const gavl_video_format_t * format,
                           const gavl_video_frame_t * f);

typedef struct
  {
  uint32_t id;
  uint32_t language;
  uint64_t duration;
  } bg_f_stream_header_t;

