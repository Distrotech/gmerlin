/*****************************************************************
 
  apetag.c
 
  Copyright (c) 2005-2006 by Burkhard Plaum - plaum@ipf.uni-stuttgart.de
 
  http://gmerlin.sourceforge.net
 
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
 
  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111, USA.
 
*****************************************************************/
#include <avdec_private.h>

#include <stdlib.h>
#include <string.h>
#include <stdio.h>



struct bgav_ape_tag_s
  {
  uint32_t num_items;

  struct
    {
    char * key;
    char * str;
    } * items;

  };

int bgav_ape_tag_probe(bgav_input_context_t * input, int * tag_size)
  {
  uint32_t size, flags;
  
  uint8_t probe_data[32];
  uint8_t * ptr;
  
  if(bgav_input_read_data(input, probe_data, 32) < 32)
    return 0;

  if(strncmp((char*)probe_data, "APETAGEX", 8))
    return 0;

  /* Now, compute the size */ 

  ptr = &(probe_data[12]);
  
  size = BGAV_PTR_2_32LE(ptr);

  ptr = &(probe_data[20]);
  
  flags = BGAV_PTR_2_32LE(ptr);

  *tag_size = size;

  if(flags & 0x80000000)
    *tag_size += 32;
  
  return 1;
  }

bgav_ape_tag_t * bgav_ape_tag_read(bgav_input_context_t * input, int tag_size)
  {
  
  int i;
  bgav_ape_tag_t *ret;
  uint32_t flags, item_value_size;
  uint8_t * buffer, *ptr;

  ret = calloc(1, sizeof(*ret));
  
  buffer = malloc(tag_size);
  if(bgav_input_read_data(input, buffer, tag_size) < tag_size)
    return (bgav_ape_tag_t *)0;

  /* Go to start of footer */
  ptr = buffer + (tag_size - 16);
  
  ret->num_items = BGAV_PTR_2_32LE(ptr);ptr += 4;
  ret->items = calloc(ret->num_items, sizeof(*ret->items));

  flags = BGAV_PTR_2_32LE(ptr);

  ptr = buffer;
  if(flags & 0x80000000)
    ptr += 32;
  
  /* Goto start of the items */
  
  for(i = 0; i < ret->num_items; i++)
    {
    item_value_size = BGAV_PTR_2_32LE(ptr); ptr+=4;
    flags  = BGAV_PTR_2_32LE(ptr);          ptr+=4;

    ret->items[i].key = bgav_strndup((char*)ptr, (char*)0);
    ptr += strlen(ret->items[i].key) + 1;

    if((flags & 0x00000006) == 0) /* UTF-8 Data */
      {
      ret->items[i].str = bgav_strndup((char*)ptr, (char*)(ptr + item_value_size));
      ptr += item_value_size;
      }
    }
  bgav_ape_tag_dump(ret);
  return ret;
  }

void bgav_ape_tag_dump(bgav_ape_tag_t * tag)
  {
  int i;
  fprintf(stderr, "APE Tag %d items\n", tag->num_items);

  for(i = 0; i < tag->num_items; i++)
    {
    fprintf(stderr, "  Item %d\n", i+1);
    fprintf(stderr, "    Key: %s\n", tag->items[i].key);

    if(tag->items[i].str)
      fprintf(stderr, "    String: %s\n", tag->items[i].str);
    else
      fprintf(stderr, "    No usable value\n");
    }
  }

#define STRVAL(k, v) \
  if(!strcasecmp(tag->items[i].key, k) && tag->items[i].str)  \
    m->v = bgav_strndup(tag->items[i].str, (char*)0)

#define INTVAL(k, v) \
  if(!strcasecmp(tag->items[i].key, k) && tag->items[i].str)  \
    m->v = atoi(tag->items[i].str)

void bgav_ape_tag_2_metadata(bgav_ape_tag_t * tag, bgav_metadata_t * m)
  {
  int i;
  for(i = 0; i < tag->num_items; i++)
    {
    STRVAL("Genre",     genre);
    STRVAL("Year",      date);
    STRVAL("Artist",    artist);
    STRVAL("Album",     album);
    STRVAL("Title",     title);
    STRVAL("Comment",   comment);
    STRVAL("Composer",  author);
    STRVAL("Copyright", copyright);

    INTVAL("Track", track);
    }
  }

#undef STRVAL
#undef INTVAL

#define FREE(p) if(p) free(p);

void bgav_ape_tag_destroy(bgav_ape_tag_t * tag)
  {
  int i;
  for(i = 0; i < tag->num_items; i++)
    {
    FREE(tag->items[i].key);
    FREE(tag->items[i].str);
    }
  FREE(tag->items);
  free(tag);
  }

#undef FREE
