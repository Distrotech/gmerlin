/*****************************************************************
 
  id3.h
 
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


typedef struct
  {
  char * title;   /* Song title	 30 characters */
  char * artist;  /* Artist	 30 characters */
  char * album;   /* Album Album 30 characters */
  char * year;    /* Year         4 characters  */
  char * comment; /* Comment     30/28 characters */

  uint8_t genre;
  uint8_t track;
  
  } bgav_id3v1_tag_t;

bgav_id3v1_tag_t * bgav_id3v1_read(bgav_input_context_t * input);

int bgav_id3v1_probe(bgav_input_context_t * input);

void bgav_id3v1_destroy(bgav_id3v1_tag_t *);

void bgav_id3v1_2_metadata(bgav_id3v1_tag_t*, bgav_metadata_t * m);

const char * bgav_id3v1_get_genre(int id);

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

typedef struct
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
  
  } bgav_id3v2_tag_t;

int bgav_id3v2_probe(bgav_input_context_t * input);

void bgav_id3v2_dump(bgav_id3v2_tag_t * t);

bgav_id3v2_tag_t * bgav_id3v2_read(bgav_input_context_t * input);

void bgav_id3v2_destroy(bgav_id3v2_tag_t*);
void bgav_id3v2_2_metadata(bgav_id3v2_tag_t*, bgav_metadata_t*m);

bgav_id3v2_frame_t * bgav_id3v2_find_frame(bgav_id3v2_tag_t*t,
                                           uint32_t * fourccs);


