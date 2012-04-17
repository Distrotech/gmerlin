/*****************************************************************
 * gmerlin - a general purpose multimedia framework and applications
 *
 * Copyright (c) 2001 - 2011 Members of the Gmerlin project
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <gmerlin/utils.h>

#include <cdio/cdio.h>
#include <cdio/cdtext.h>

#include <gavl/metatags.h>


#include "cdaudio.h"

#define GET_FIELD(dst, key) \
  field = cdtext_get_const(key, cdtext);

#define GET_FIELD_DEFAULT(dst,key)                                      \
  field = cdtext_get_const(key, cdtext);                                \
  if(field)                                                             \
    {                                                                   \
    info[idx->tracks[i].index].metadata.dst = bg_strdup(info[idx->tracks[i].index].metadata.dst, field); \
    if(!info[idx->tracks[i].index].metadata.dst)                        \
      info[idx->tracks[i].index].metadata.dst = bg_strdup(info[idx->tracks[i].index].metadata.dst, dst); \
    }

int bg_cdaudio_get_metadata_cdtext(CdIo_t * cdio,
                                   bg_track_info_t * info,
                                   bg_cdaudio_index_t* idx)
  {
  int i;
  const char * field;
    
  /* Global information */

  const char * title  = NULL;
  const char * artist  = NULL;
  const char * author  = NULL;
  const char * album   = NULL;
  const char * genre   = NULL;
  const char * comment = NULL;
  const cdtext_t * cdtext;

  /* Get information for the whole disc */
  
  cdtext = cdio_get_cdtext (cdio, 0);

  if(!cdtext)
    return 0;
  
  artist  = cdtext_get_const(CDTEXT_PERFORMER, cdtext);
  author  = cdtext_get_const(CDTEXT_COMPOSER, cdtext); /* Composer overwrites songwriter */

  if(!author)
    author  = cdtext_get_const(CDTEXT_SONGWRITER, cdtext);
  
  album  = cdtext_get_const(CDTEXT_TITLE, cdtext);
  genre  = cdtext_get_const(CDTEXT_GENRE, cdtext);
  comment  = cdtext_get_const(CDTEXT_MESSAGE, cdtext);
  
  for(i = 0; i < idx->num_tracks; i++)
    {
    if(idx->tracks[i].is_audio)
      {
      cdtext = cdio_get_cdtext (cdio, i+1);
      if(!cdtext)
        return 0;
      
      GET_FIELD(title, CDTEXT_TITLE);
      
      if(!title)
        return 0;

      gavl_metadata_set(&info[idx->tracks[i].index].metadata,
                        GAVL_META_TITLE, title);

      if((field = cdtext_get_const(CDTEXT_PERFORMER, cdtext)))
        gavl_metadata_set(&info[idx->tracks[i].index].metadata,
                          GAVL_META_ARTIST, field);
      else
        gavl_metadata_set(&info[idx->tracks[i].index].metadata,
                          GAVL_META_ARTIST, artist);


      if((field = cdtext_get_const(CDTEXT_COMPOSER, cdtext)))
        gavl_metadata_set(&info[idx->tracks[i].index].metadata,
                          GAVL_META_AUTHOR, field);
      else if((field = cdtext_get_const(CDTEXT_SONGWRITER, cdtext)))
        gavl_metadata_set(&info[idx->tracks[i].index].metadata,
                          GAVL_META_AUTHOR, field);
      else if(author)
        gavl_metadata_set(&info[idx->tracks[i].index].metadata,
                          GAVL_META_AUTHOR, author);


      if((field = cdtext_get_const(CDTEXT_GENRE, cdtext)))
        gavl_metadata_set(&info[idx->tracks[i].index].metadata,
                          GAVL_META_GENRE, field);
      else
        gavl_metadata_set(&info[idx->tracks[i].index].metadata,
                          GAVL_META_GENRE, genre);

      if((field = cdtext_get_const(CDTEXT_MESSAGE, cdtext)))
        gavl_metadata_set(&info[idx->tracks[i].index].metadata,
                          GAVL_META_COMMENT, field);
      else
        gavl_metadata_set(&info[idx->tracks[i].index].metadata,
                          GAVL_META_COMMENT, comment);


      gavl_metadata_set(&info[idx->tracks[i].index].metadata,
                        GAVL_META_ALBUM, album);
      gavl_metadata_set(&info[idx->tracks[i].index].metadata,
                        GAVL_META_ALBUMARTIST, artist);
      }
    }
  return 1;
  
  }

#undef GET_FIELD
#undef GET_FIELD_DEFAULT
