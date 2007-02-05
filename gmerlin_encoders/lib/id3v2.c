/*****************************************************************
 
  id3v2.c
 
  Copyright (c) 2005 by Burkhard Plaum - plaum@ipf.uni-stuttgart.de
 
  http://gmerlin.sourceforge.net
 
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
 
  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111, USA.
 
*****************************************************************/

#include <stdlib.h>
#include <string.h>
#include <gmerlin/utils.h>
#include <gmerlin/charset.h>

#include <gmerlin_encoders.h>

/* Simple ID3 writer.
   We do the following:
   
   - Only ID3V2.4 tags are written
   - All metadata are converted to single line strings
   - We use UTF-8 encoding for the tags
*/

/* FOURCC stuff */

#define MK_FOURCC(a, b, c, d) ((a<<24)|(b<<16)|(c<<8)|d)

static int write_fourcc(FILE * output, uint32_t fourcc)
  {
  uint8_t data[4];

  data[0] = (fourcc >> 24) & 0xff;
  data[1] = (fourcc >> 16) & 0xff;
  data[2] = (fourcc >> 8) & 0xff;
  data[3] = (fourcc) & 0xff;

  if(fwrite(data, 1, 4, output) < 4)
    return 0;
  return 1;
  }


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
  uint32_t fourcc;
  char * str;
  } id3v2_frame_t;

/* Flags for ID3V2 Tag header */

#define ID3V2_TAG_UNSYNCHRONIZED  (1<<7)
#define ID3V2_TAG_EXTENDED_HEADER (1<<6)
#define ID3V2_TAG_EXPERIMENTAL    (1<<5)
#define ID3V2_TAG_FOOTER_PRESENT  (1<<4)

struct bgen_id3v2_s
  {
  struct
    {
    uint8_t major_version;
    uint8_t minor_version;
    uint8_t flags;
    uint32_t size;
    } header;
  
  int num_frames;
  id3v2_frame_t * frames;

  };

static void add_frame(bgen_id3v2_t * tag, uint32_t fourcc, char * string)
  {
  tag->frames = realloc(tag->frames,
                        (tag->num_frames+1)*sizeof(*(tag->frames)));
  tag->frames[tag->num_frames].fourcc = fourcc;
  tag->frames[tag->num_frames].str = bg_strdup(NULL, string);
  tag->num_frames++;
  }

/*
  char * artist;
  char * title;
  char * album;
      
  int track;
  char * date;
  char * genre;
  char * comment;

  char * author;
  char * copyright;
*/

#define TEXT_FRAME(str, fcc) if(m->str) { add_frame(ret, fcc, m->str); }

#define INT_FRAME(i, fcc) if(m->i) \
    { \
    tmp_string = bg_sprintf("%d", m->i);\
    add_frame(ret, fcc, tmp_string);\
    free(tmp_string); \
    }

bgen_id3v2_t * bgen_id3v2_create(const bg_metadata_t * m)
  {
  int year;
  char * tmp_string;
  
  bgen_id3v2_t * ret;
  ret = calloc(1, sizeof(*ret));

  ret->header.major_version = 4;
  ret->header.minor_version = 4;
  ret->header.flags = 0;

  TEXT_FRAME(artist,    MK_FOURCC('T', 'P', 'E', '1'));
  TEXT_FRAME(title,     MK_FOURCC('T', 'I', 'T', '2'));
  TEXT_FRAME(album,     MK_FOURCC('T', 'A', 'L', 'B'));
  INT_FRAME(track,      MK_FOURCC('T', 'R', 'C', 'K'));
  TEXT_FRAME(genre,     MK_FOURCC('T', 'C', 'O', 'N'));
  TEXT_FRAME(author,    MK_FOURCC('T', 'C', 'O', 'M'));
  TEXT_FRAME(copyright, MK_FOURCC('T', 'C', 'O', 'P'));

  year = bg_metadata_get_year(m);
  if(year)
    {
    tmp_string = bg_sprintf("%d", year);\
    add_frame(ret, MK_FOURCC('T', 'Y', 'E', 'R'), tmp_string);\
    free(tmp_string); \
    }

  TEXT_FRAME(comment,   MK_FOURCC('C', 'O', 'M', 'M'));
  return ret;
  }

static int write_32_syncsave(FILE * output, uint32_t num)
  {
  uint8_t data[4];
  data[0] = (num >> 21) & 0x7f;
  data[1] = (num >> 14) & 0x7f;
  data[2] = (num >> 7) & 0x7f;
  data[3] = num & 0x7f;
  if(fwrite(data, 1, 4, output) < 4)
    return 0;
  return 1;
  }

static int write_frame(FILE * output, id3v2_frame_t * frame,
                       int encoding)
  {
  uint8_t flags[2] = { 0x00, 0x00 };
  uint8_t comm_header[3] = { 'X', 'X', 'X' };
  uint8_t bom[2] = { 0xff, 0xfe };
  uint8_t terminator[2] = { 0x00, 0x00 };
  int is_comment = 0;
  // uint8_t encoding = 0x03; /* We do everything in UTF-8 */
  int len;
  uint32_t size_pos, end_pos, size;
  
  char * str;

  bg_charset_converter_t * cnv;
  
  /* Write 10 bytes header */

  if(!write_fourcc(output, frame->fourcc))
    return 0;
  
  size_pos = ftell(output);

  if(!write_32_syncsave(output, 0))
    return 0;
  
  /* Frame flags are zero */

  if(fwrite(flags, 1, 2, output) < 2)
    return 0;
  
  /* Encoding */
  if(fwrite(&encoding, 1, 1, output) < 1)
    return 0;
  
  /* For COMM frames, we need to set the language as well */

  if(frame->fourcc == MK_FOURCC('C','O','M','M'))  
    {
    is_comment = 1;
    if(fwrite(comm_header, 1, 3, output) < 3)
      return 0;
    }

  switch(encoding)
    {
    case ID3_ENCODING_LATIN1:
      if(is_comment)
        {
        if(fwrite(terminator, 1, 1, output) < 1)
          return 0;
        }
      cnv = bg_charset_converter_create("UTF-8", "ISO-8859-1");
      str = bg_convert_string(cnv, frame->str, -1, (int*)0);
      len = strlen(str)+1;
      if(fwrite(str, 1, len, output) < len)
        return 0;
      bg_charset_converter_destroy(cnv);
      free(str);
      break;
    case ID3_ENCODING_UTF16_BOM:
      /* Short Comment */
      if(is_comment)
        {
        if(fwrite(bom, 1, 2, output) < 2)
          return 0;
        if(fwrite(terminator, 1, 2, output) < 2)
          return 0;
        }
      /* Long Comment */
      if(fwrite(bom, 1, 2, output) < 2)
        return 0;
      cnv = bg_charset_converter_create("UTF-8", "UTF-16LE");
      str = bg_convert_string(cnv, frame->str, -1, &len);
      if(fwrite(str, 1, len, output) < len)
        return 0;
      if(fwrite(terminator, 1, 2, output) < 2)
        return 0;
      bg_charset_converter_destroy(cnv);
      free(str);
      break;
    case ID3_ENCODING_UTF16_BE:
      if(is_comment)
        {
        /* Short Comment */
        if(fwrite(terminator, 1, 2, output) < 2)
          return 0;
        }
      /* Long Comment */
      cnv = bg_charset_converter_create("UTF-8", "UTF-16BE");
      str = bg_convert_string(cnv, frame->str, -1, &len);
      if(fwrite(str, 1, len, output) < len)
        return 0;
      if(fwrite(terminator, 1, 2, output) < 2)
        return 0;
      bg_charset_converter_destroy(cnv);
      free(str);
      break;
    case ID3_ENCODING_UTF8:
      if(is_comment)
        {
        if(fwrite(terminator, 1, 1, output) < 1)
          return 0;
        }
      /* Then, write the string including terminating 0x00 character */
      len = strlen(frame->str)+1;
      
      if(fwrite(frame->str, 1, len, output) < len)
        return 0;
      break;
    }
  

  end_pos = ftell(output);
  size = end_pos - size_pos - 6;
  
  fseek(output, size_pos, SEEK_SET);
  if(!write_32_syncsave(output, size))
    return 0;
  fseek(output, end_pos, SEEK_SET);
  return 1;
  }

int bgen_id3v2_write(FILE * output, const bgen_id3v2_t * tag,
                     int encoding)
  {
  int i;
  uint32_t size_pos, size, end_pos;
  
  
  uint8_t header[6] = { 'I', 'D', '3', 0x04, 0x00, 0x00 };
  
  /* Return if the tag has zero frames */

  if(!tag->num_frames)
    return 1;

  /* Write header */

  if(fwrite(header, 1, 6, output) < 6)
    return 0;
  
  /* Write dummy size (will be filled in later) */

  size_pos = ftell(output);
  write_32_syncsave(output, 0);

  /* Write all frames */

  for(i = 0; i < tag->num_frames; i++)
    {
    write_frame(output, &tag->frames[i], encoding);
    }

  end_pos = ftell(output);
  size = end_pos - size_pos - 4;

  fseek(output, size_pos, SEEK_SET);
  write_32_syncsave(output, size);
  fseek(output, end_pos, SEEK_SET);
  return 1;
  }

void bgen_id3v2_destroy(bgen_id3v2_t * tag)
  {
  int i;

  if(tag->frames)
    {
    for(i = 0; i < tag->num_frames; i++)
      free(tag->frames[i].str);
    free(tag->frames);
    }
  free(tag);
  }

