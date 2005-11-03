/*****************************************************************
 
  vorbis_comment.c
 
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

#include <string.h>
#include <stdlib.h>
#include <avdec_private.h>
#include <vorbis_comment.h>
#include <stdio.h>


/*
 *  Parsing code written straightforward after
 *  http://www.xiph.org/ogg/vorbis/doc/v-comment.html
 */

void bgav_vorbis_comment_dump(bgav_vorbis_comment_t * ret)
  {
  int i;
  fprintf(stderr, "Vorbis comment:\n");
  fprintf(stderr, "  Vendor string: %s\n", ret->vendor);
  
  for(i = 0; i < ret->num_user_comments; i++)
    {
    fprintf(stderr, "  %s\n", ret->user_comments[i]);
    }
  }

int bgav_vorbis_comment_read(bgav_vorbis_comment_t * ret,
                             bgav_input_context_t * input)
  {
  int i;
  uint32_t len;
  uint32_t num;
  
  // [vendor_length] = read an unsigned integer of 32 bits
  if(!bgav_input_read_32_le(input, &len))
    return 0;

  // [vendor_string] = read a UTF-8 vector as [vendor_length] octets
  ret->vendor = malloc(len+1);
  if(bgav_input_read_data(input, (uint8_t*)(ret->vendor), len) < len)
    return 0;
  ret->vendor[len] = '\0';

  // [user_comment_list_length] = read an unsigned integer of 32 bits
  if(!bgav_input_read_32_le(input, &num))
    return 0;

  ret->num_user_comments = num;
  ret->user_comments = calloc(ret->num_user_comments,
                              sizeof(*(ret->user_comments)));
  // iterate [user_comment_list_length] times
  for(i = 0; i < ret->num_user_comments; i++)
    {
    // [length] = read an unsigned integer of 32 bits
    if(!bgav_input_read_32_le(input, &len))
      return 0;

    
    ret->user_comments[i] = malloc(len + 1);
    if(bgav_input_read_data(input, (uint8_t*)(ret->user_comments[i]), len) < len)
      return 0;
    ret->user_comments[i][len] = '\0';
    }
  return 1;
  }

 
static char * _artist_key = "ARTIST";
static char * _author_key = "AUTHOR";
static char * _album_key = "ALBUM";
static char * _title_key = "TITLE";
// static char * _version_key = "VERSION=";
static char * _track_number_key = "TRACKNUMBER";
// static char * _organization_key = "ORGANIZATION=";
static char * _genre_key = "GENRE";
// static char * _description_key = "DESCRIPTION=";
static char * _date_key = "DATE";
// static char * _location_key = "LOCATION=";
static char * _copyright_key = "COPYRIGHT";

const char *
bgav_vorbis_comment_get_field(bgav_vorbis_comment_t * vc, const char * key)
  {
  int i;
  int key_len = strlen(key);
  
  for(i = 0; i < vc->num_user_comments; i++)
    {
    if(!strncasecmp(vc->user_comments[i], key, key_len) &&
        vc->user_comments[i][key_len] == '=')
      return vc->user_comments[i] + key_len + 1;
    }
  return (const char*)0;
  }

void bgav_vorbis_comment_2_metadata(bgav_vorbis_comment_t * comment,
                                    bgav_metadata_t * m)
  {
  const char * field;
  int j;

  if((field = bgav_vorbis_comment_get_field(comment, _artist_key)))
    m->artist = bgav_strndup(field, NULL);

  if((field = bgav_vorbis_comment_get_field(comment, _author_key)))
    m->author = bgav_strndup(field, NULL);

  if((field = bgav_vorbis_comment_get_field(comment, _album_key)))
    m->album = bgav_strndup(field, NULL);

  if((field = bgav_vorbis_comment_get_field(comment, _title_key)))
    m->title = bgav_strndup(field, NULL);

  if((field = bgav_vorbis_comment_get_field(comment, _genre_key)))
    m->genre = bgav_strndup(field, NULL);

  if((field = bgav_vorbis_comment_get_field(comment, _date_key)))
    m->date = bgav_strndup(field, NULL);

  if((field = bgav_vorbis_comment_get_field(comment, _copyright_key)))
    m->copyright = bgav_strndup(field, NULL);

  if((field = bgav_vorbis_comment_get_field(comment, _track_number_key)))
    m->track = atoi(field);
  
  for(j = 0; j < comment->num_user_comments; j++)
    {
    if(!strchr(comment->user_comments[j], '='))
      {
      m->comment = bgav_strndup(comment->user_comments[j], NULL);
      break;
      }
    }
  }

#define MY_FREE(ptr) if(ptr) free(ptr);

void bgav_vorbis_comment_free(bgav_vorbis_comment_t * ret)
  {
  int i;
  MY_FREE(ret->vendor);

  for(i = 0; i < ret->num_user_comments; i++)
    {
    MY_FREE(ret->user_comments[i]);
    }
  MY_FREE(ret->user_comments);
  }

