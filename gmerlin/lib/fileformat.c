/*****************************************************************
 * gmerlin - a general purpose multimedia framework and applications
 *
 * Copyright (c) 2001 - 2012 Members of the Gmerlin project
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

#include <stdio.h>
#include <stdlib.h>

#include <config.h>
#include <gavl/gavl.h>
#include <gavl/gavldsp.h>
#include <gmerlin/fileformat.h>
#include <gmerlin/serialize.h>



static int read_stdio(void * priv, uint8_t * data, int len)
  {
  FILE * f = priv;
  return fread(data, 1, len, f);
  }

static int write_stdio(void * priv, const uint8_t * data, int len)
  {
  FILE * f = priv;
  return fwrite(data, 1, len, f);
  }

static int64_t ftell_stdio(void * priv)
  {
  FILE * f = priv;
  return ftell(f);
  }

static int64_t seek_stdio(void * priv, int64_t pos, int whence)
  {
  FILE * f = priv;
  fseek(f, pos, whence);
  return ftell(f);
  }

static void close_stdio(void * priv)
  {
  FILE * f = priv;
  if(f)
    fclose(f);
  }

int bg_f_io_open_stdio_read(bg_f_io_t * io, const char * filename)
  {
  FILE * f;
  f = fopen(filename, "r");
  if(!f)
    return 0;
  io->data = f;
  io->read_callback = read_stdio;
  io->seek_callback = seek_stdio;
  io->ftell_callback = ftell_stdio;
  io->close_callback = close_stdio;
  return 1;
  }

int bg_f_io_open_stdio_write(bg_f_io_t * io, const char * filename)
  {
  FILE * f;
  f = fopen(filename, "w");
  if(!f)
    return 0;
  io->data = f;
  io->write_callback = write_stdio;
  io->seek_callback = seek_stdio;
  io->ftell_callback = ftell_stdio;
  io->close_callback = close_stdio;
  return 1;
  }

void  bg_f_io_close(bg_f_io_t * io)
  {
  if(io->close_callback)
    io->close_callback(io->data);
  if(io->buffer)
    free(io->buffer);
  
  }

static void bg_f_io_buffer_alloc(bg_f_io_t * io, int len)
  {
  if(len <= io->buffer_alloc)
    return;
  io->buffer_alloc = len + 512;
  io->buffer = realloc(io->buffer, io->buffer_alloc);
  }

static inline int read_32(bg_f_io_t * io, uint32_t * val)
  {
  uint8_t data[4];
  if(io->read_callback(io->data, data, 4) < 4)
    return 0;
  *val = ((data[0] << 24) | (data[1] << 16) | (data[2] << 8) | data[3]);
  return 1;
  }

static inline int read_64(bg_f_io_t * io, uint64_t * val)
  {
  uint8_t data[8];
  if(io->read_callback(io->data, data, 8) < 8)
    return 0;
  *val  = data[0]; *val <<= 8;
  *val |= data[1]; *val <<= 8;
  *val |= data[2]; *val <<= 8;
  *val |= data[3]; *val <<= 8;
  *val |= data[4]; *val <<= 8;
  *val |= data[5]; *val <<= 8;
  *val |= data[6]; *val <<= 8;
  *val |= data[7];
  return 1;
  }

static inline int write_32(bg_f_io_t * io, uint32_t val)
  {
  uint8_t data[4];
  data[0] = (val & 0xff000000) >> 24;
  data[1] = (val & 0xff0000) >> 16;
  data[2] = (val & 0xff00) >> 8;
  data[3] = (val & 0xff);
  
  return io->write_callback(io->data, data, 4);
  }

static inline int write_64(bg_f_io_t * io, uint64_t val)
  {
  uint8_t data[8];
  data[0] = (val & 0xff00000000000000LL) >> 56;
  data[1] = (val & 0x00ff000000000000LL) >> 48;
  data[2] = (val & 0x0000ff0000000000LL) >> 40;
  data[3] = (val & 0x000000ff00000000LL) >> 32;
  data[4] = (val & 0x00000000ff000000LL) >> 24;
  data[5] = (val & 0x0000000000ff0000LL) >> 16;
  data[6] = (val & 0x000000000000ff00LL) >> 8;
  data[7] = (val & 0x00000000000000ffLL);
  return io->write_callback(io->data, data, 8);
  }

int bg_f_chunk_read(bg_f_io_t * io, bg_f_chunk_t * ch)
  {
  ch->start = io->ftell_callback(io->data);
  if(!read_32(io, &ch->type) ||
     !read_64(io, &ch->size))
    return 0;
  return 1;
  }

int bg_f_chunk_write_header(bg_f_io_t * io, bg_f_chunk_t * ch, bg_f_chunk_type_t type)
  {
  ch->type = type;
  ch->start = io->ftell_callback(io->data);
  if(!write_32(io, ch->type) ||
     !write_64(io, ch->size))
    return 0;
  return 1;
  }

int bg_f_chunk_write_footer(bg_f_io_t * io, bg_f_chunk_t * ch)
  {
  int64_t end_pos = io->ftell_callback(io->data);
  io->seek_callback(io->data, ch->start + 4, SEEK_SET);
  ch->size = end_pos - ch->start - 12;
  if(!write_64(io, ch->size))
    return 0;
  io->seek_callback(io->data, end_pos, SEEK_SET);
  return 1;
  }

int bg_f_signature_read(bg_f_io_t * io, bg_f_chunk_t * ch, bg_f_signature_t * sig)
  {
  if(!read_32(io, &sig->type))
    return 0;
  return 1;
  }

int bg_f_signature_write(bg_f_io_t * io, bg_f_signature_t * sig)
  {
  bg_f_chunk_t ch;
  if(!bg_f_chunk_write_header(io, &ch, CHUNK_TYPE_SIGNATURE))
    return 0;
  if(!write_32(io, sig->type))
    return 0;
  if(!bg_f_chunk_write_footer(io, &ch))
    return 0;
  return 1;
  }

int bg_f_audio_format_read(bg_f_io_t * io, bg_f_chunk_t * ch, gavl_audio_format_t * f,
                           int * big_endian)
  {
  bg_f_io_buffer_alloc(io, ch->size);
  if(io->read_callback(io->data, io->buffer, ch->size) < ch->size)
    return 0;
  bg_deserialize_audio_format(f, io->buffer, ch->size, big_endian);
  return 1;
  }

int bg_f_audio_format_write(bg_f_io_t * io, const gavl_audio_format_t * f)
  {
  bg_f_chunk_t ch;
  int len;
  
  if(!bg_f_chunk_write_header(io, &ch, CHUNK_TYPE_AUDIO_FORMAT))
    return 0;

  len = bg_serialize_audio_format(f, NULL, 0);
  bg_f_io_buffer_alloc(io, len);
  len = bg_serialize_audio_format(f, io->buffer, len);

  if(io->write_callback(io->data, io->buffer, len) < len)
    return 0;

  if(!bg_f_chunk_write_footer(io, &ch))
    return 0;
  return 1;
  }

int bg_f_video_format_read(bg_f_io_t * io, bg_f_chunk_t * ch, gavl_video_format_t * f,
                           int * big_endian)
  {
  bg_f_io_buffer_alloc(io, ch->size);
  if(io->read_callback(io->data, io->buffer, ch->size) < ch->size)
    return 0;
  bg_deserialize_video_format(f, io->buffer, ch->size, big_endian);
  return 1;

  }

int bg_f_video_format_write(bg_f_io_t * io, const gavl_video_format_t * f)
  {
  bg_f_chunk_t ch;
  int len;
  
  if(!bg_f_chunk_write_header(io, &ch, CHUNK_TYPE_VIDEO_FORMAT))
    return 0;

  len = bg_serialize_video_format(f, NULL, 0);
  bg_f_io_buffer_alloc(io, len);
  len = bg_serialize_video_format(f, io->buffer, len);

  if(io->write_callback(io->data, io->buffer, len) < len)
    return 0;

  if(!bg_f_chunk_write_footer(io, &ch))
    return 0;
  return 1;
  }

int bg_f_audio_frame_read(bg_f_io_t * io, bg_f_chunk_t * ch,
                          const gavl_audio_format_t * format,
                          gavl_audio_frame_t * f, int big_endian,
                          gavl_dsp_context_t * ctx)
  {
  int len;
  len = bg_serialize_audio_frame_header(format, f, NULL, 0);
  bg_f_io_buffer_alloc(io, len);
  if(io->read_callback(io->data, io->buffer, len) < len)
    return 0;
  bg_deserialize_audio_frame_header(format, f, io->buffer, len);

  return bg_deserialize_audio_frame(ctx,
                                    format, f, io->read_callback,
                                    io->data, big_endian);  
  }

int bg_f_audio_frame_write(bg_f_io_t * io,
                           const gavl_audio_format_t * format,
                           const gavl_audio_frame_t * f)
  {
  int len;
  bg_f_chunk_t ch;
  
  if(!bg_f_chunk_write_header(io, &ch, CHUNK_TYPE_AUDIO_FRAME))
    return 0;
  
  len = bg_serialize_audio_frame_header(format, f, NULL, 0);
  bg_f_io_buffer_alloc(io, len);
  bg_serialize_audio_frame_header(format, f, io->buffer, len);
  if(io->write_callback(io->data, io->buffer, len) < len)
    return 0;

  if(!bg_serialize_audio_frame(format,
                               f, io->write_callback,
                               io->data))
    return 0;
  if(!bg_f_chunk_write_footer(io, &ch))
    return 0;
  return 1;
  }

int bg_f_video_frame_read(bg_f_io_t * io, bg_f_chunk_t * ch,
                          const gavl_video_format_t * format,
                          gavl_video_frame_t * f, int big_endian,
                          gavl_dsp_context_t * ctx)
  {
  int len;
  len = bg_serialize_video_frame_header(format, f, NULL, 0);
  bg_f_io_buffer_alloc(io, len);
  if(io->read_callback(io->data, io->buffer, len) < len)
    return 0;
  bg_deserialize_video_frame_header(format, f, io->buffer, len);

  return bg_deserialize_video_frame(ctx,
                                    format, f, io->read_callback,
                                    io->data, big_endian);  
  
  }


int bg_f_video_frame_write(bg_f_io_t * io,
                           const gavl_video_format_t * format,
                           const gavl_video_frame_t * f)
  {
  int len;
  bg_f_chunk_t ch;
  
  if(!bg_f_chunk_write_header(io, &ch, CHUNK_TYPE_VIDEO_FRAME))
    return 0;
  
  len = bg_serialize_video_frame_header(format, f, NULL, 0);
  bg_f_io_buffer_alloc(io, len);
  bg_serialize_video_frame_header(format, f, io->buffer, len);
  if(io->write_callback(io->data, io->buffer, len) < len)
    return 0;

  if(!bg_serialize_video_frame(format,
                               f, io->write_callback,
                               io->data))
    return 0;
  if(!bg_f_chunk_write_footer(io, &ch))
    return 0;
  return 1;
  
  }
