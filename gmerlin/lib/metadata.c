/*****************************************************************
 
  metadata.c
 
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

#include <parameter.h>
#include <streaminfo.h>
#include <utils.h>

#define MY_FREE(ptr) \
  if(ptr) \
    { \
    free(ptr); \
    ptr = NULL; \
    }

void bg_metadata_free(bg_metadata_t * m)
  {
  MY_FREE(m->artist);
  MY_FREE(m->title);
  MY_FREE(m->album);
  MY_FREE(m->genre);
  MY_FREE(m->comment);
  MY_FREE(m->author);
  MY_FREE(m->copyright);
  }

#define MY_STRCPY(s) \
  dst->s = bg_strdup(dst->s, src->s);

#define MY_INTCOPY(i) \
  dst->i = src->i;

void bg_metadata_copy(bg_metadata_t * dst, const bg_metadata_t * src)
  {
  MY_STRCPY(artist);
  MY_STRCPY(title);
  MY_STRCPY(album);

  MY_STRCPY(date);
  MY_STRCPY(genre);
  MY_STRCPY(comment);

  MY_STRCPY(author);
  MY_STRCPY(copyright);
  MY_INTCOPY(track);
  }

#undef MY_STRCPY
#undef MY_INTCPY

#if 0

typedef struct
  {
  /* Strings here MUST be in UTF-8 */
  char * artist;
  char * title;
  char * album;
      
  int track;
  char * date;
  char * genre;
  char * comment;

  char * author;
  char * copyright;
  } bg_metadata_t;
#endif

static bg_parameter_info_t parameters[] =
  {
    {
      name:      "artist",
      long_name: "Artist",
      type:      BG_PARAMETER_STRING,
    },
    {
      name:      "title",
      long_name: "Title",
      type:      BG_PARAMETER_STRING,
    },
    {
      name:      "album",
      long_name: "Album",
      type:      BG_PARAMETER_STRING,
    },
    {
      name:      "track",
      long_name: "Track",
      type:      BG_PARAMETER_INT,
    },
    {
      name:      "genre",
      long_name: "Genre",
      type:      BG_PARAMETER_STRING,
    },
    {
      name:      "comment",
      long_name: "Comment",
      type:      BG_PARAMETER_STRING,
    },
    {
      name:      "author",
      long_name: "Author",
      type:      BG_PARAMETER_STRING,
    },
    {
      name:      "copyright",
      long_name: "Copyright",
      type:      BG_PARAMETER_STRING,
    },
    { /* End of parameters */ }
  };

#define SP_STR(s) if(!strcmp(ret[i].name, # s)) \
    ret[i].val_default.val_str = bg_strdup(ret[i].val_default.val_str, m->s)

#define SP_INT(s) if(!strcmp(ret[i].name, # s)) \
    ret[i].val_default.val_i = m->s;

bg_parameter_info_t * bg_metadata_get_parameters(bg_metadata_t * m)
  {
  int i;
  
  bg_parameter_info_t * ret;
  ret = bg_parameter_info_copy_array(parameters);

  if(!m)
    return ret;
  
  i = 0;
  while(ret[i].name)
    {
    SP_STR(artist);
    SP_STR(title);
    SP_STR(album);
      
    SP_INT(track);
    SP_STR(date);
    SP_STR(genre);
    SP_STR(comment);

    SP_STR(author);
    SP_STR(copyright);

    i++;
    }
  return ret;
  }

#undef SP_STR
#undef SP_INT

#define SP_STR(s) if(!strcmp(name, # s))                               \
    { \
    m->s = bg_strdup(m->s, val->val_str); \
    return; \
    }
    
#define SP_INT(s) if(!strcmp(name, # s)) \
    { \
    m->s = val->val_i; \
    }

void bg_metadata_set_parameter(void * data, char * name,
                               bg_parameter_value_t * val)
  {
  bg_metadata_t * m = (bg_metadata_t*)data;
  
  if(!name)
    return;

  SP_STR(artist);
  SP_STR(title);
  SP_STR(album);
  
  SP_INT(track);
  SP_STR(date);
  SP_STR(genre);
  SP_STR(comment);
  
  SP_STR(author);
  SP_STR(copyright);
  }

#undef SP_STR
#undef SP_INT
