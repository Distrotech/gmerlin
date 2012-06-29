/*****************************************************************
 * gmerlin-avdecoder - a general purpose multimedia decoding library
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
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include <avdec_private.h>


#define ID3V2_FRAME_TAG_ALTER_PRESERVATION  (1<<14)
#define ID3V2_FRAME_FILE_ALTER_PRESERVATION (1<<13)
#define ID3V2_FRAME_READ_ONLY               (1<<12)
#define ID3V2_FRAME_GROUPING                (1<<6)
#define ID3V2_FRAME_COMPRESSION             (1<<3)
#define ID3V2_FRAME_ENCRYPTION              (1<<2)
#define ID3V2_FRAME_UNSYNCHRONIZED          (1<<1)
#define ID3V2_FRAME_DATA_LENGTH             (1<<0)

typedef struct
  {
  struct
    {
    uint32_t fourcc;
    uint32_t size;
    uint16_t flags;
    
    } header;

  /* Either data or strings is non-null */

  uint8_t * data;  /* Raw data from the file */
  char ** strings; /* NULL terminated array  */
  
  } bgav_id3v2_frame_t;

/* Flags for ID3V2 Tag header */

#define ID3V2_TAG_UNSYNCHRONIZED  (1<<7)
#define ID3V2_TAG_EXTENDED_HEADER (1<<6)
#define ID3V2_TAG_EXPERIMENTAL    (1<<5)
#define ID3V2_TAG_FOOTER_PRESENT  (1<<4)

struct bgav_id3v2_tag_s
  {
  struct
    {
    uint8_t major_version;
    uint8_t minor_version;
    uint8_t flags;
    uint32_t size;
    } header;

  struct
    {
    uint32_t size;
    uint32_t num_flags;
    uint8_t * flags;
    } extended_header;
  
  int total_bytes;
 
  int num_frames;
  bgav_id3v2_frame_t * frames;
  const bgav_options_t * opt;
  };

int bgav_id3v2_total_bytes(bgav_id3v2_tag_t* tag)
  {
  return tag->total_bytes;
  }



static bgav_id3v2_frame_t * bgav_id3v2_find_frame(bgav_id3v2_tag_t*t,
                                                  const uint32_t * fourccs);


#define ENCODING_LATIN1    0x00
#define ENCODING_UTF16_BOM 0x01
#define ENCODING_UTF16_BE  0x02
#define ENCODING_UTF8      0x03

static void dump_frame(bgav_id3v2_frame_t * frame)
  {
  int i;
  bgav_dprintf( "Header:\n");
  bgav_dprintf( "  Fourcc: ");
  bgav_dump_fourcc(frame->header.fourcc);
  bgav_dprintf( "\n");
  
  bgav_dprintf( "  Size:   %d\n", frame->header.size);

  bgav_dprintf( "  Flags:  ");

  if(frame->header.flags & ID3V2_FRAME_TAG_ALTER_PRESERVATION)
    bgav_dprintf( "ALTER_PRESERVATION ");
  if(frame->header.flags & ID3V2_FRAME_READ_ONLY)
    bgav_dprintf( "READ_ONLY ");
  if(frame->header.flags & ID3V2_FRAME_GROUPING)
    bgav_dprintf( "GOUPING ");
  if(frame->header.flags & ID3V2_FRAME_COMPRESSION)
    bgav_dprintf( "COMPRESSION ");
  if(frame->header.flags & ID3V2_FRAME_ENCRYPTION)
    bgav_dprintf( "ENCRYPTION ");
  if(frame->header.flags & ID3V2_FRAME_UNSYNCHRONIZED)
    bgav_dprintf( "UNSYNCHRONIZED ");
  if(frame->header.flags & ID3V2_FRAME_DATA_LENGTH)
    bgav_dprintf( "DATA_LENGTH");

  bgav_dprintf( "\n");

  if(frame->data)
    {
    bgav_dprintf( "Raw data:\n");
    bgav_hexdump(frame->data, frame->header.size, 16);
    }
  else if(frame->strings)
    {
    bgav_dprintf( "Strings:\n");
    i = 0;
    while(frame->strings[i])
      {
      bgav_dprintf( "%02x: %s\n", i, frame->strings[i]);
      i++;
      }
    }
  }

static void free_frame(bgav_id3v2_frame_t * frame)
  {
  int j;
  if(frame->strings)
    {
    j = 0;
    
    while(frame->strings[j])
      {
      free(frame->strings[j]);
      j++;
      }
    free(frame->strings);
    }
  else if(frame->data)
    free(frame->data);
  }

void bgav_id3v2_dump(bgav_id3v2_tag_t * t)
  {
  int i;
  bgav_dprintf( "============= ID3V2 tag =============\n");
  
  /* Dump header */

  bgav_dprintf( "Header:\n");
  bgav_dprintf( "  Major version: %d\n", t->header.major_version);
  bgav_dprintf( "  Minor version: %d\n", t->header.minor_version);
  bgav_dprintf( "  Flags:         ");
  if(t->header.flags & ID3V2_TAG_UNSYNCHRONIZED)
    bgav_dprintf( "UNSYNCHRONIZED ");
  if(t->header.flags & ID3V2_TAG_EXTENDED_HEADER)
    bgav_dprintf( " EXTENDED_HEADER");
  if(t->header.flags & ID3V2_TAG_EXPERIMENTAL)
    bgav_dprintf( " EXPERIMENTAL");
  if(t->header.flags & ID3V2_TAG_EXPERIMENTAL)
    bgav_dprintf( " FOOTER_PRESENT");
  bgav_dprintf( "\n");
  bgav_dprintf( "  Size: %d\n", t->header.size);
  
  for(i = 0; i < t->num_frames; i++)
    {
    bgav_dprintf( "========== Frame %d ==========\n", i+1);
    dump_frame(&t->frames[i]);
    }
  }

static int read_32_syncsave(bgav_input_context_t * input, uint32_t * ret)
  {
  uint8_t data[4];
  if(bgav_input_read_data(input, data, 4) < 4)
    return 0;

  *ret = (uint32_t)data[0] << 24;
  *ret >>= 1;
  *ret |= ((uint32_t)data[1] << 16);
  *ret >>= 1;
  *ret |= ((uint32_t)data[2] << 8);
  *ret >>= 1;
  *ret |= (uint32_t)data[3];
  return 1;
  }

int bgav_id3v2_probe(bgav_input_context_t * input)
  {
  uint8_t data[3];
  if(bgav_input_get_data(input, data, 3) < 3)
    return 0;

  if((data[0] == 'I') &&
     (data[1] == 'D') &&
     (data[2] == '3'))
    return 1;
  return 0;
  }

static int is_null(const char * ptr, int num_bytes)
  {
  int i;
  for(i = 0; i < num_bytes; i++)
    {
    if(ptr[i] != '\0')
      return 0;
    }
  return 1;
  }

static char ** read_string_list(const bgav_options_t * opt,
                                uint8_t * data, int data_size)
  {
  int bytes_per_char;
  int i;
  uint8_t encoding;
  char * pos;
  char * end_pos;
  int num_strings;
  char ** ret;
  bgav_charset_converter_t * cnv;
  cnv = NULL;
  encoding = *data;

  if(data_size == 1)
    return NULL;
  
  switch(encoding)
    {
    case ENCODING_LATIN1:
      bytes_per_char = 1;
      cnv = bgav_charset_converter_create(opt, "LATIN1", "UTF-8");
      pos = ((char*)data) + 1;
      break;
    case ENCODING_UTF16_BOM:
      bytes_per_char = 2;

      if((data[1] == 0xFF) && (data[2] == 0xFE))
        cnv = bgav_charset_converter_create(opt, "UTF16LE", "UTF-8");
      else if((data[2] == 0xFF) && (data[1] == 0xFE))
        cnv = bgav_charset_converter_create(opt, "UTF16BE", "UTF-8");
      pos = ((char*)data) + 3;
      break;
    case ENCODING_UTF16_BE:
      bytes_per_char = 2;
      cnv = bgav_charset_converter_create(opt, "UTF16BE", "UTF-8");
      pos = ((char*)data) + 1;
      break;
    case ENCODING_UTF8:
      bytes_per_char = 1;
      pos = ((char*)data) + 1;
      break;
    default:
      return NULL;
    }
  
  end_pos = pos;

  /* Count the strings */

  num_strings = 1;
  for(i = 0; i < data_size; i+= bytes_per_char)
    {
    if(is_null(pos, bytes_per_char))
      num_strings++;
    }

  ret = calloc(num_strings+1, sizeof(char*));

  for(i = 0; i < num_strings; i++)
    {
    end_pos = pos;
    
    while(!is_null(end_pos, bytes_per_char))
      {
      end_pos += bytes_per_char;
      if(end_pos - (char * )data >= data_size)
        break;
      }
    if(cnv)
      {
      ret[i] = bgav_convert_string(cnv,
                                   pos, end_pos - pos,
                                   NULL);
      }
    else
      ret[i] = bgav_strndup(pos, end_pos);
    pos = end_pos;
    pos += bytes_per_char;
    }
  if(cnv)
    bgav_charset_converter_destroy(cnv);
  return ret;
  }

static int read_frame(bgav_input_context_t * input,
                      bgav_id3v2_frame_t * ret,
                      uint8_t * probe_data,
                      int major_version)
  {
  uint8_t buf[4];
  uint8_t * data;
  
  if(major_version < 4)
    {
    switch(major_version)
      {
      case 2:
        /* 3 bytes for ID and size */
        ret->header.fourcc =
          (((uint32_t)probe_data[0] << 24) | 
           ((uint32_t)probe_data[1] << 16) |
           ((uint32_t)probe_data[2] << 8));
        if(bgav_input_read_data(input, buf, 3) < 3)
          return 0;
        ret->header.size =
          (((uint32_t)buf[0] << 16) | 
           ((uint32_t)buf[1] << 8) |
           ((uint32_t)buf[2]));
        break;
      case 3:
        /* 4 bytes for ID and size */
        if(bgav_input_read_data(input, buf, 1) < 1)
          return 0;
        ret->header.fourcc = (((uint32_t)probe_data[0] << 24) | 
                              ((uint32_t)probe_data[1] << 16) |
                              ((uint32_t)probe_data[2] << 8) |
                              ((uint32_t)buf[0]));
        if(!bgav_input_read_32_be(input, &ret->header.size) ||
           !bgav_input_read_16_be(input, &ret->header.flags))
           
          return 0;
        break;       
      }
    }
  else
    {
    if(bgav_input_read_data(input, buf, 1) < 1)
      return 0;
    ret->header.fourcc = (((uint32_t)probe_data[0] << 24) | 
                          ((uint32_t)probe_data[1] << 16) |
                          ((uint32_t)probe_data[2] << 8) |
                          ((uint32_t)buf[0]));
    
    if(!read_32_syncsave(input, &ret->header.size) ||
       !bgav_input_read_16_be(input, &ret->header.flags))
      return 0;
    }
  if(ret->header.size > input->total_bytes - input->position)
    return 0;
  
  data = calloc(ret->header.size+2, 1);
  if(bgav_input_read_data(input, data, ret->header.size) <
     ret->header.size)
    return 0;
  
  if(((ret->header.fourcc & 0xFF000000) ==
     BGAV_MK_FOURCC('T', 0x00, 0x00, 0x00)) &&
     (ret->header.fourcc != BGAV_MK_FOURCC('T', 'X', 'X', 'X')))
    {
    ret->strings = read_string_list(input->opt, data, ret->header.size);
    free(data);
    }
  else /* Copy raw data */
    {
    ret->data = data;
    
    }
  //  dump_frame(ret);  
  return 1;
  }

#define FRAMES_TO_ALLOC 16

bgav_id3v2_tag_t * bgav_id3v2_read(bgav_input_context_t * input)
  {
  uint8_t probe_data[3];
  int64_t tag_start_pos;
  int64_t start_pos;
  int frames_alloc = 0;
  bgav_input_context_t * input_mem;
  uint8_t * data;
  int data_size;
      
  bgav_id3v2_tag_t * ret = NULL;
  
  if(bgav_input_read_data(input, probe_data, 3) < 3)
    goto fail;
  
  if((probe_data[0] != 'I') ||
     (probe_data[1] != 'D') ||
     (probe_data[2] != '3'))
    goto fail;
  
  ret = calloc(1, sizeof(*ret));
  ret->opt = input->opt;
  /* Read header */
  
  if(!bgav_input_read_data(input, &ret->header.major_version, 1) ||
     !bgav_input_read_data(input, &ret->header.minor_version, 1) ||
     !bgav_input_read_data(input, &ret->header.flags, 1) ||
     !read_32_syncsave(input, &ret->header.size))
    goto fail;

  tag_start_pos = input->position;
  
  /* Check for extended header */

  if(ret->header.flags & ID3V2_TAG_EXTENDED_HEADER)
    {
    start_pos = input->position;

    if(!read_32_syncsave(input, &ret->extended_header.size))
      goto fail;
    bgav_input_skip(input, ret->extended_header.size-4);
    }
  
  /* Read frames */

  data_size = tag_start_pos + ret->header.size - input->position;
  data = malloc(data_size);
  if(bgav_input_read_data(input, data, data_size) < data_size)
    goto fail;

  input_mem = bgav_input_open_memory(data, data_size, input->opt);
    
  while(input_mem->position < data_size)
    {
    if(input_mem->position >= data_size - 4)
      break;
    
    if(bgav_input_read_data(input_mem, probe_data, 3) < 3)
      goto fail;

    if(!probe_data[0] && !probe_data[1] && !probe_data[2])
      break;

    else if((probe_data[0] == '3') && 
            (probe_data[1] == 'D') && 
            (probe_data[2] == 'I'))
      {
      bgav_input_skip(input_mem, 7);
      break;
      }
    if(frames_alloc < ret->num_frames + 1)
      {
      frames_alloc += FRAMES_TO_ALLOC;
      ret->frames = realloc(ret->frames, frames_alloc * sizeof(*ret->frames));
      memset(ret->frames + ret->num_frames, 0,
             FRAMES_TO_ALLOC * sizeof(*ret->frames));
      }
    
    if(!read_frame(input_mem,
                   &ret->frames[ret->num_frames],
                   probe_data,
                   ret->header.major_version))
      {
      free_frame(&ret->frames[ret->num_frames]);
      break;
      }
    ret->num_frames++;
    }
  bgav_input_close(input_mem);
  bgav_input_destroy(input_mem);
  free(data);
  
  ret->total_bytes = ret->header.size + 10;
  /* Read footer */
  
  if(ret->header.flags & ID3V2_TAG_FOOTER_PRESENT)
    {
    bgav_input_skip(input, 10);
    ret->total_bytes += 10;
    }

  /* Skip padding */
  
  else if(tag_start_pos + ret->header.size > input->position)
    {
    bgav_input_skip(input, tag_start_pos + ret->header.size - input->position);
    }
  //  bgav_id3v2_dump(ret);
  return ret;

  fail:
  if(ret)
    bgav_id3v2_destroy(ret);
  return NULL;
  }

static bgav_id3v2_frame_t * bgav_id3v2_find_frame(bgav_id3v2_tag_t*t,
                                                  const uint32_t * fourcc)
  {
  int i, j;
  
  for(i = 0; i < t->num_frames; i++)
    {
    j = 0;

    while(fourcc[j])
      {
      
      if(t->frames[i].header.fourcc == fourcc[j])
        {
        return &t->frames[i];
        }
      j++;
      }
    }
  return NULL;
  }

static const uint32_t title_tags[] =
  {
    BGAV_MK_FOURCC('T','I','T','2'),
    BGAV_MK_FOURCC('T','T','2', 0x00),
    0x0
  };

static const uint32_t album_tags[] =
  {
    BGAV_MK_FOURCC('T','A','L','B'),
    BGAV_MK_FOURCC('T','A','L', 0x00),
    0x0
  };


static const uint32_t copyright_tags[] =
  {
    BGAV_MK_FOURCC('T','C','O','P'),
    BGAV_MK_FOURCC('T','C','R', 0x00),
    0x0
  };

static const uint32_t artist_tags[] =
  {
    BGAV_MK_FOURCC('T','P','E','1'),
    BGAV_MK_FOURCC('T','P','1',0x00),
    0x00,
  };

static const uint32_t albumartist_tags[] =
  {
    BGAV_MK_FOURCC('T','P','E','2'),
    BGAV_MK_FOURCC('T','P','2',0x00),
    0x00,
  };

static const uint32_t year_tags[] =
  {
    BGAV_MK_FOURCC('T','Y','E',0x00),
    BGAV_MK_FOURCC('T','Y','E','R'),
    0x00,
  };

static const uint32_t track_tags[] =
  {
    BGAV_MK_FOURCC('T', 'R', 'C', 'K'),
    BGAV_MK_FOURCC('T', 'R', 'K', 0x00),
    0x00,
  };

static const uint32_t genre_tags[] =
  {
    BGAV_MK_FOURCC('T', 'C', 'O', 'N'),
    BGAV_MK_FOURCC('T', 'C', 'O', 0x00),
    0x00,
  };

/* Author == composer */

static const uint32_t author_tags[] =
  {
    BGAV_MK_FOURCC('T', 'C', 'O', 'M'),
    BGAV_MK_FOURCC('T', 'C', 'M', 0x00),
    0x00,
  };


static const uint32_t comment_tags[] =
  {
    BGAV_MK_FOURCC('C', 'O', 'M', 'M'),
    BGAV_MK_FOURCC('C', 'O', 'M', 0x00),
    0x00,
  };

static char * get_comment(const bgav_options_t * opt,
                          bgav_id3v2_frame_t* frame)
  {
  char * ret;
  uint8_t encoding;
  uint8_t * pos;
  int bytes_per_char;
  bgav_charset_converter_t * cnv = NULL;
  
  if(!frame->data)
    return NULL;
  
  encoding = *frame->data;
    
  switch(encoding)
    {
    case ENCODING_LATIN1:
      bytes_per_char = 1;
      cnv = bgav_charset_converter_create(opt, "LATIN1", "UTF-8");
      pos = frame->data + 4;
      break;
    case ENCODING_UTF16_BOM:
      bytes_per_char = 2;

      if((frame->data[4] == 0xFF) && (frame->data[5] == 0xFE))
        cnv = bgav_charset_converter_create(opt, "UTF16LE", "UTF-8");
      else if((frame->data[5] == 0xFF) && (frame->data[4] == 0xFE))
        cnv = bgav_charset_converter_create(opt, "UTF16BE", "UTF-8");
      pos = frame->data + 6;
      break;
    case ENCODING_UTF16_BE:
      bytes_per_char = 2;
      cnv = bgav_charset_converter_create(opt, "UTF16BE", "UTF-8");
      pos = frame->data + 4;
      break;
    case ENCODING_UTF8:
      bytes_per_char = 1;
      pos = frame->data + 4;
      break;
    default:
      return NULL;
    }
  
  pos = frame->data + 4; /* Skip encoding and language */

  /* Skip short description */
  
  while(!is_null((char*)pos, bytes_per_char))
    pos += bytes_per_char;

  pos += bytes_per_char;
  
  if(cnv)
    ret = bgav_convert_string(cnv, 
                              (char*)pos, frame->header.size - (int)(pos - frame->data),
                              NULL);
  else
    ret = bgav_strdup((char*)pos);

  if(cnv)
    bgav_charset_converter_destroy(cnv);
  return ret;
  }

void bgav_id3v2_2_metadata(bgav_id3v2_tag_t * t, gavl_metadata_t*m)
  {
  int i_tmp;
  bgav_id3v2_frame_t * frame;
  
  /* Title */

  frame = bgav_id3v2_find_frame(t, title_tags);
  if(frame && frame->strings)
    gavl_metadata_set(m, GAVL_META_TITLE, frame->strings[0]);

  /* Album */
    
  frame = bgav_id3v2_find_frame(t, album_tags);
  if(frame && frame->strings)
    gavl_metadata_set(m, GAVL_META_ALBUM, frame->strings[0]);

  /* Copyright */
    
  frame = bgav_id3v2_find_frame(t, copyright_tags);
  if(frame && frame->strings)
    gavl_metadata_set(m, GAVL_META_COPYRIGHT, frame->strings[0]);

  /* Artist */

  frame = bgav_id3v2_find_frame(t, artist_tags);
  if(frame && frame->strings)
    gavl_metadata_set(m, GAVL_META_ARTIST, frame->strings[0]);

  /* Albumartist */

  frame = bgav_id3v2_find_frame(t, albumartist_tags);
  if(frame && frame->strings)
    gavl_metadata_set(m, GAVL_META_ALBUMARTIST, frame->strings[0]);
  
  /* Author */

  frame = bgav_id3v2_find_frame(t, author_tags);
  if(frame && frame->strings)
    gavl_metadata_set(m, GAVL_META_AUTHOR, frame->strings[0]);
  
  /* Year */
  
  frame = bgav_id3v2_find_frame(t, year_tags);
  if(frame && frame->strings)
    gavl_metadata_set(m, GAVL_META_YEAR, frame->strings[0]);

  /* Track */

  frame = bgav_id3v2_find_frame(t, track_tags);
  if(frame && frame->strings)
    gavl_metadata_set_int(m, GAVL_META_TRACKNUMBER, atoi(frame->strings[0]));
  
  /* Genre */
  
  frame = bgav_id3v2_find_frame(t, genre_tags);
  if(frame && frame->strings)
    {
    if((frame->strings[0][0] == '(') && isdigit(frame->strings[0][1]))
      {
      i_tmp = atoi(&frame->strings[0][1]);
      gavl_metadata_set(m, GAVL_META_GENRE, bgav_id3v1_get_genre(i_tmp));
      }
    else
      {
      gavl_metadata_set(m, GAVL_META_GENRE, frame->strings[0]);
      }
    }

  /* Comment */
  
  frame = bgav_id3v2_find_frame(t, comment_tags);

  if(frame)
    gavl_metadata_set_nocpy(m, GAVL_META_COMMENT, get_comment(t->opt, frame));
  
  }

void bgav_id3v2_destroy(bgav_id3v2_tag_t * t)
  {
  int i;
  for(i = 0; i < t->num_frames; i++)
    {
    free_frame(&t->frames[i]);
    }
  free(t->frames);
  free(t);
  }

