/*****************************************************************
 
  id3v2.c
 
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

#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include <avdec_private.h>

#define ENCODING_LATIN1   0x00
#define ENCODING_UTF16_LE 0x01
#define ENCODING_UTF16_BE 0x02
#define ENCODING_UTF8     0x03

static void dump_frame(bgav_id3v2_frame_t * frame)
  {
  int i;
  fprintf(stderr, "Header:\n");
  fprintf(stderr, "  Fourcc: ");
  bgav_dump_fourcc(frame->header.fourcc);
  fprintf(stderr, "\n");
  
  fprintf(stderr, "  Size:   %d\n", frame->header.size);

  fprintf(stderr, "  Flags:  ");

  if(frame->header.flags & ID3V2_FRAME_TAG_ALTER_PRESERVATION)
    fprintf(stderr, "ALTER_PRESERVATION ");
  if(frame->header.flags & ID3V2_FRAME_READ_ONLY)
    fprintf(stderr, "READ_ONLY ");
  if(frame->header.flags & ID3V2_FRAME_GROUPING)
    fprintf(stderr, "GOUPING ");
  if(frame->header.flags & ID3V2_FRAME_COMPRESSION)
    fprintf(stderr, "COMPRESSION ");
  if(frame->header.flags & ID3V2_FRAME_ENCRYPTION)
    fprintf(stderr, "ENCRYPTION ");
  if(frame->header.flags & ID3V2_FRAME_UNSYNCHRONIZED)
    fprintf(stderr, "UNSYNCHRONIZED ");
  if(frame->header.flags & ID3V2_FRAME_DATA_LENGTH)
    fprintf(stderr, "DATA_LENGTH");

  fprintf(stderr, "\n");

  if(frame->data)
    {
    fprintf(stderr, "Raw data:\n");
    bgav_hexdump(frame->data, frame->header.size, 16);
    }
  else if(frame->strings)
    {
    fprintf(stderr, "Strings:\n");
    i = 0;
    while(frame->strings[i])
      {
      fprintf(stderr, "%02x: %s\n", i, frame->strings[i]);
      i++;
      }
    }
  }

void bgav_id3v2_dump(bgav_id3v2_tag_t * t)
  {
  int i;
  fprintf(stderr, "============= ID3V2 tag =============\n");
  
  /* Dump header */

  fprintf(stderr, "Header:\n");
  fprintf(stderr, "  Major version: %d\n", t->header.major_version);
  fprintf(stderr, "  Minor version: %d\n", t->header.minor_version);
  fprintf(stderr, "  Flags:         ");
  if(t->header.flags & ID3V2_TAG_UNSYNCHRONIZED)
    {
    fprintf(stderr, "UNSYNCHRONIZED ");
    }
  if(t->header.flags & ID3V2_TAG_EXTENDED_HEADER)
    {
    fprintf(stderr, " EXTENDED_HEADER");
    }
  if(t->header.flags & ID3V2_TAG_EXPERIMENTAL)
    {
    fprintf(stderr, " EXPERIMENTAL");
    }
  if(t->header.flags & ID3V2_TAG_EXPERIMENTAL)
    {
    fprintf(stderr, " FOOTER_PRESENT");
    }
  fprintf(stderr, "\n");
  fprintf(stderr, "  Size: %d\n", t->header.size);

  for(i = 0; i < t->num_frames; i++)
    {
    fprintf(stderr, "========== Frame %d ==========\n", i+1);
    dump_frame(&(t->frames[i]));
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

static char ** read_string_list(uint8_t * data, int data_size)
  {
  int bytes_per_char;
  int i;
  uint8_t encoding;
  char * pos;
  char * end_pos;
  int num_strings;
  char ** ret;
  bgav_charset_converter_t * cnv;
  cnv = (bgav_charset_converter_t*)0;
  encoding = *data;

  if(data_size == 1)
    return (char**)0;
  
  switch(encoding)
    {
    case ENCODING_LATIN1:
      bytes_per_char = 1;
      cnv = bgav_charset_converter_create("LATIN1", "UTF-8");
      break;
    case ENCODING_UTF16_LE:
      bytes_per_char = 2;
      cnv = bgav_charset_converter_create("UTF16LE", "UTF-8");
      break;
    case ENCODING_UTF16_BE:
      bytes_per_char = 2;
      cnv = bgav_charset_converter_create("UTF16BE", "UTF-8");
      break;
    case ENCODING_UTF8:
      bytes_per_char = 1;
      break;
    default:
      fprintf(stderr, "Warning, unknown encoding %02x\n", encoding);
      return (char **)0;
    }
  
  pos = ((char*)data) + 1;
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
      ret[i] = bgav_convert_string(cnv, 
                                   pos, end_pos - pos,
                                   NULL);
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
                      char * probe_data,
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
        if(!bgav_input_read_32_be(input, &(ret->header.size)) ||
           !bgav_input_read_16_be(input, &(ret->header.flags)))
           
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
    
    if(!read_32_syncsave(input, &(ret->header.size)) ||
       !bgav_input_read_16_be(input, &(ret->header.flags)))
      return 0;
    }
  
  data = malloc(ret->header.size);
  if(bgav_input_read_data(input, data, ret->header.size) <
     ret->header.size)
    return 0;

  //  bgav_hexdump(data, ret->header.size, 16);
  
  if(((ret->header.fourcc & 0xFF000000) ==
     BGAV_MK_FOURCC('T', 0x00, 0x00, 0x00)) &&
     (ret->header.fourcc != BGAV_MK_FOURCC('T', 'X', 'X', 'X')))
    {
    ret->strings = read_string_list(data, ret->header.size);
    free(data);
    }
  else /* Copy raw data */
    {
    ret->data = data;
    
    }
  return 1;
  }

#define FRAMES_TO_ALLOC 16

bgav_id3v2_tag_t * bgav_id3v2_read(bgav_input_context_t * input)
  {
  uint8_t probe_data[3];
  int64_t tag_start_pos;
  int64_t start_pos;
  int frames_alloc = 0;
  
  bgav_id3v2_tag_t * ret = (bgav_id3v2_tag_t *)0;
  
  //  fprintf(stderr, "Reading ID3V2 tag...\n");
    
  if(bgav_input_read_data(input, probe_data, 3) < 3)
    goto fail;
  
  if((probe_data[0] != 'I') ||
     (probe_data[1] != 'D') ||
     (probe_data[2] != '3'))
    goto fail;
  
  ret = calloc(1, sizeof(*ret));
  
  /* Read header */
  
  if(!bgav_input_read_data(input, &(ret->header.major_version), 1) ||
     !bgav_input_read_data(input, &(ret->header.minor_version), 1) ||
     !bgav_input_read_data(input, &(ret->header.flags), 1) ||
     !read_32_syncsave(input, &(ret->header.size)))
    goto fail;

  tag_start_pos = input->position;
  
  /* Check for extended header */

  if(ret->header.flags & ID3V2_TAG_EXTENDED_HEADER)
    {
    start_pos = input->position;

    if(!read_32_syncsave(input, &(ret->extended_header.size)))
      goto fail;
    bgav_input_skip(input, ret->extended_header.size-4);
    }
  
  /* Read frames */

  while(input->position < tag_start_pos + ret->header.size)
    {
    if(input->position >= tag_start_pos + ret->header.size - 4)
      break;
    
    if(bgav_input_read_data(input, probe_data, 3) < 3)
      goto fail;

    if(!probe_data[0] && !probe_data[1] && !probe_data[2])
      break;

    else if((probe_data[0] == '3') && 
            (probe_data[1] == 'D') && 
            (probe_data[2] == 'I'))
      {
      bgav_input_skip(input, 7);
      break;
      }
    if(frames_alloc < ret->num_frames + 1)
      {
      frames_alloc += FRAMES_TO_ALLOC;
      ret->frames = realloc(ret->frames, frames_alloc * sizeof(*ret->frames));
      memset(ret->frames + ret->num_frames, 0,
             FRAMES_TO_ALLOC * sizeof(*ret->frames));
      }
    
    if(!read_frame(input,
                   &(ret->frames[ret->num_frames]),
                   probe_data,
                   ret->header.major_version))
      goto fail;
    ret->num_frames++;
    }
    
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
    //    fprintf(stderr, "Skipping %lld padding bytes\n",
    //            tag_start_pos + ret->header.size - input->position);
    bgav_input_skip(input, tag_start_pos + ret->header.size - input->position);
    //    fprintf(stderr, "Pos: %lld\n", input->position);
    }
  //  bgav_id3v2_dump(ret);
  return ret;

  fail:
  if(ret)
    bgav_id3v2_destroy(ret);
  return (bgav_id3v2_tag_t *)0;
  }

bgav_id3v2_frame_t * bgav_id3v2_find_frame(bgav_id3v2_tag_t*t,
                                           uint32_t * fourcc)
  {
  int i, j;
  
  //  fprintf(stderr, "Find frame ");
  //  bgav_dump_fourcc(fourcc);
  //  fprintf(stderr, "\n");
  
  for(i = 0; i < t->num_frames; i++)
    {
    j = 0;

    while(fourcc[j])
      {
      
      if(t->frames[i].header.fourcc == fourcc[j])
        {
        //      fprintf(stderr, "Found\n");
        return &(t->frames[i]);
        }
      j++;
      }
    }
  //  fprintf(stderr, "Not Found\n");
  return (bgav_id3v2_frame_t*)0;
  }

static uint32_t title_tags[] =
  {
    BGAV_MK_FOURCC('T','I','T','2'),
    BGAV_MK_FOURCC('T','T','2', 0x00),
    0x0
  };

static uint32_t album_tags[] =
  {
    BGAV_MK_FOURCC('T','A','L','B'),
    BGAV_MK_FOURCC('T','A','L', 0x00),
    0x0
  };


static uint32_t copyright_tags[] =
  {
    BGAV_MK_FOURCC('T','C','O','P'),
    BGAV_MK_FOURCC('T','C','R', 0x00),
    0x0
  };

static uint32_t artist_tags[] =
  {
    BGAV_MK_FOURCC('T','P','E','1'),
    BGAV_MK_FOURCC('T','P','1',0x00),
    0x00,
  };

static uint32_t date_tags[] =
  {
    BGAV_MK_FOURCC('T','Y','E',0x00),
    BGAV_MK_FOURCC('T','Y','E','R'),
    0x00,
  };

static uint32_t track_tags[] =
  {
    BGAV_MK_FOURCC('T', 'R', 'C', 'K'),
    BGAV_MK_FOURCC('T', 'R', 'K', 0x00),
    0x00,
  };

static uint32_t genre_tags[] =
  {
    BGAV_MK_FOURCC('T', 'C', 'O', 'N'),
    BGAV_MK_FOURCC('T', 'C', 'O', 0x00),
    0x00,
  };

static uint32_t comment_tags[] =
  {
    BGAV_MK_FOURCC('C', 'O', 'M', 'M'),
    BGAV_MK_FOURCC('C', 'O', 'M', 0x00),
    0x00,
  };

static char * get_comment(bgav_id3v2_frame_t* frame)
  {
  char * ret;
  uint8_t encoding;
  uint8_t * pos;
  int bytes_per_char;
  bgav_charset_converter_t * cnv =
    (bgav_charset_converter_t*)0;
  
  if(!frame->data)
    return (char*)0;
  
  encoding = *frame->data;
    
  switch(encoding)
    {
    case ENCODING_LATIN1:
      bytes_per_char = 1;
      cnv = bgav_charset_converter_create("LATIN1", "UTF-8");
      break;
    case ENCODING_UTF16_LE:
      bytes_per_char = 2;
      cnv = bgav_charset_converter_create("UTF16LE", "UTF-8");
      break;
    case ENCODING_UTF16_BE:
      bytes_per_char = 2;
      cnv = bgav_charset_converter_create("UTF16BE", "UTF-8");
      break;
    case ENCODING_UTF8:
      bytes_per_char = 1;
      break;
    default:
      fprintf(stderr, "Warning, unknown encoding %02x\n", encoding);
      return (char *)0;
    }
  
  pos = frame->data + 4; /* Skip encoding and language */

  /* Skip short description */
  
  while(!is_null(pos, bytes_per_char))
    pos += bytes_per_char;

  pos += bytes_per_char;
  
  if(cnv)
    ret = bgav_convert_string(cnv, 
                              pos, frame->header.size - (int)(pos - frame->data),
                              NULL);
  else
    ret = bgav_strndup(pos, NULL);

  if(cnv)
    bgav_charset_converter_destroy(cnv);
  return ret;
  }

void bgav_id3v2_2_metadata(bgav_id3v2_tag_t * t, bgav_metadata_t*m)
  {
  int i_tmp;
  bgav_id3v2_frame_t * frame;
  
  /* Title */

  frame = bgav_id3v2_find_frame(t, title_tags);
  if(frame && frame->strings)
    m->title = bgav_strndup(frame->strings[0], NULL);

  /* Album */
    
  frame = bgav_id3v2_find_frame(t, album_tags);
  if(frame && frame->strings)
    m->album = bgav_strndup(frame->strings[0], NULL);

  /* Copyright */
    
  frame = bgav_id3v2_find_frame(t, copyright_tags);
  if(frame && frame->strings)
    m->copyright = bgav_strndup(frame->strings[0], NULL);

  /* Artist */

  frame = bgav_id3v2_find_frame(t, artist_tags);
  if(frame && frame->strings)
    m->artist = bgav_strndup(frame->strings[0], NULL);

  /* Date */
  
  frame = bgav_id3v2_find_frame(t, date_tags);
  if(frame && frame->strings)
    m->date = bgav_strndup(frame->strings[0], NULL);

  /* Track */

  frame = bgav_id3v2_find_frame(t, track_tags);
  if(frame && frame->strings)
    m->track = atoi(frame->strings[0]);

  /* Genre */
  
  frame = bgav_id3v2_find_frame(t, genre_tags);
  if(frame && frame->strings)
    {
    if((frame->strings[0][0] == '(') && isdigit(frame->strings[0][1]))
      {
      i_tmp = atoi(&(frame->strings[0][1]));
      m->genre = bgav_strndup(bgav_id3v1_get_genre(i_tmp), NULL);
      }
    else
      {
      m->genre = bgav_strndup(frame->strings[0], NULL);
      }
    }

  /* Comment */
  
  frame = bgav_id3v2_find_frame(t, comment_tags);

  if(frame)
    m->comment = get_comment(frame);
  
  }

void bgav_id3v2_destroy(bgav_id3v2_tag_t * t)
  {
  int i, j;
  for(i = 0; i < t->num_frames; i++)
    {
    if(t->frames[i].strings)
      {
      j = 0;

      while(t->frames[i].strings[j])
        {
        free(t->frames[i].strings[j]);
        j++;
        }
      free(t->frames[i].strings);
      }
    else if(t->frames[i].data)
      free(t->frames[i].data);
    }
  free(t->frames);
  free(t);
  }

