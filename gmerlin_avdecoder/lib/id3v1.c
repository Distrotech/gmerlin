/*****************************************************************
 
  id3v1.c
 
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

#include <stdlib.h>
#include <string.h>
#include <avdec_private.h>
#include <id3.h>



char * get_string(char * ptr, int max_size)
  {
  int i;
  char * end = ptr;
  for(i = 0; i < max_size; i++)
    {
    if(*end == '\0')
      break;
    end++;
    }
  return bgav_strndup(ptr, end);
  }

bgav_id3v1_tag_t * bgav_id3v1_read(bgav_input_context_t * input)
  {
  char buffer[128];
  char * pos;
  
  bgav_id3v1_tag_t * ret;

  if(bgav_input_read_data(input, buffer, 128) < 128)
    return (bgav_id3v1_tag_t *)0;

  ret = calloc(1, sizeof(*ret));
  
  pos = buffer + 3;
  ret->title = get_string(pos, 30);

  pos = buffer + 33;
  ret->artist = get_string(pos, 30);

  pos = buffer + 63;
  ret->album = get_string(pos, 30);

  pos = buffer + 93;
  ret->year = get_string(pos, 4);

  pos = buffer + 97;
  ret->comment = get_string(pos, 30);
  if(strlen(ret->comment) <= 28)
    ret->track = buffer[126];
  ret->genre = buffer[127];
  return ret;
  }

int bgav_id3v1_probe(bgav_input_context_t * input)
  {
  uint8_t probe_data[3];
  if(bgav_input_get_data(input, probe_data, 3) < 3)
    return 0;
  if((probe_data[0] == 'T') &&
     (probe_data[1] == 'A') &&
     (probe_data[2] == 'G'))
    return 1;
  return 0;
  }

#define FREE(s) if(s)free(s);

void bgav_id3v1_destroy(bgav_id3v1_tag_t * t)
  {
  FREE(t->title);
  FREE(t->artist);  /* Artist	 30 characters */
  FREE(t->album);   /* Album Album 30 characters */
  FREE(t->year);    /* Year         4 characters  */
  FREE(t->comment); /* Comment     30/28 characters */
  free(t);
  }

#define GENRE_MAX 0x94

const char *id3_genres[GENRE_MAX] =
  {
    "Blues", "Classic Rock", "Country", "Dance",
    "Disco", "Funk", "Grunge", "Hip-Hop",
    "Jazz", "Metal", "New Age", "Oldies",
    "Other", "Pop", "R&B", "Rap", "Reggae",
    "Rock", "Techno", "Industrial", "Alternative",
    "Ska", "Death Metal", "Pranks", "Soundtrack",
    "Euro-Techno", "Ambient", "Trip-Hop", "Vocal",
    "Jazz+Funk", "Fusion", "Trance", "Classical",
    "Instrumental", "Acid", "House", "Game",
    "Sound Clip", "Gospel", "Noise", "Alt",
    "Bass", "Soul", "Punk", "Space",
    "Meditative", "Instrumental Pop",
    "Instrumental Rock", "Ethnic", "Gothic",
    "Darkwave", "Techno-Industrial", "Electronic",
    "Pop-Folk", "Eurodance", "Dream",
    "Southern Rock", "Comedy", "Cult",
    "Gangsta Rap", "Top 40", "Christian Rap",
    "Pop/Funk", "Jungle", "Native American",
    "Cabaret", "New Wave", "Psychedelic", "Rave",
    "Showtunes", "Trailer", "Lo-Fi", "Tribal",
    "Acid Punk", "Acid Jazz", "Polka", "Retro",
    "Musical", "Rock & Roll", "Hard Rock", "Folk",
    "Folk/Rock", "National Folk", "Swing",
    "Fast-Fusion", "Bebob", "Latin", "Revival",
    "Celtic", "Bluegrass", "Avantgarde",
    "Gothic Rock", "Progressive Rock",
    "Psychedelic Rock", "Symphonic Rock", "Slow Rock",
    "Big Band", "Chorus", "Easy Listening",
    "Acoustic", "Humour", "Speech", "Chanson",
    "Opera", "Chamber Music", "Sonata", "Symphony",
    "Booty Bass", "Primus", "Porn Groove",
    "Satire", "Slow Jam", "Club", "Tango",
    "Samba", "Folklore", "Ballad", "Power Ballad",
    "Rhythmic Soul", "Freestyle", "Duet",
    "Punk Rock", "Drum Solo", "A Cappella",
    "Euro-House", "Dance Hall", "Goa",
    "Drum & Bass", "Club-House", "Hardcore",
    "Terror", "Indie", "BritPop", "Negerpunk",
    "Polsk Punk", "Beat", "Christian Gangsta Rap",
    "Heavy Metal", "Black Metal", "Crossover",
    "Contemporary Christian", "Christian Rock",
    "Merengue", "Salsa", "Thrash Metal",
    "Anime", "JPop", "Synthpop"
  };

#define CS(src, dst) \
if(t->src) m->dst = bgav_strndup(t->src, NULL);

#if 0
  
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

#endif



void bgav_id3v1_2_metadata(bgav_id3v1_tag_t * t, bgav_metadata_t * m)
  {
  CS(title, title);
  CS(artist, artist);
  CS(album, album);
  CS(year, date);
  CS(comment, comment);

  if(t->genre < GENRE_MAX)
    m->genre = bgav_strndup(id3_genres[t->genre], NULL);
  m->track = t->track;
  }
