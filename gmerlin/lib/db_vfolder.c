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

#include <config.h>
#include <unistd.h>

#include <mediadb_private.h>
#include <gmerlin/log.h>
#include <gmerlin/utils.h>
#include <string.h>

#define LOG_DOMAIN "db.vfolder"

static const struct
  {
  bg_db_object_type_t type;
  bg_db_category_t cats[BG_DB_VFOLDER_MAX_DEPTH];
  }
vfolder_types[] =
  {
    {
      .type = BG_DB_OBJECT_AUDIO_FILE,
      .cats = { BG_DB_CAT_GENRE,
                BG_DB_CAT_ALBUMARTIST_GROUP,
                BG_DB_CAT_ALBUMARTIST,
                BG_DB_CAT_AUDIOALBUM },
    },
    { /* End */ }
  };

static void create_vfolders_audio_file(bg_db_t * db, bg_db_object_t * obj)
  {
  int i = 0;
  int j;
  
  while(vfolder_types[i].type)
    {
    if(vfolder_types[i].type != BG_DB_OBJECT_AUDIO_FILE)
      {
      i++;
      continue;
      }

    j = 0;

    while(vfolder_types[i].cats[j])
      {
      switch(vfolder_types[i].cats[j])
        {
        case BG_DB_CAT_SONGTITLE:
          break;
        case BG_DB_CAT_YEAR:
          break;
        case BG_DB_CAT_ALBUMYEAR:
          break;
        case BG_DB_CAT_ARTIST:
          break;
        case BG_DB_CAT_ARTIST_GROUP:
          break;
        case BG_DB_CAT_ALBUMARTIST:
          break;
        case BG_DB_CAT_ALBUMARTIST_GROUP:
          break;
        case BG_DB_CAT_GENRE:
          break;
        case BG_DB_CAT_AUDIOALBUM:
          break;
        }
      j++; 
      }
    i++;
    }
  
  }

static void create_vfolders_audio_album(bg_db_t * db, bg_db_object_t * obj)
  {
  
  }

void
bg_db_create_tables_vfolders(bg_db_t * db)
  {
  int i;
  char * str = gavl_strdup("CREATE TABLE VFOLDERS ( ID INTEGER PRIMARY KEY,"
                           "TYPE INTEGER, "
                           "DEPTH INTEGER" );
  char tmp_string[32];
  
  for(i = 0; i < BG_DB_VFOLDER_MAX_DEPTH; i++)
    {
    snprintf(tmp_string, 32, ", CAT_%d INTEGER", i+1);
    str = gavl_strcat(str, tmp_string);
    snprintf(tmp_string, 32, ", VAL_%d INTEGER", i+1);
    str = gavl_strcat(str, tmp_string);
    }

  str = gavl_strcat(str, ");");
  bg_sqlite_exec(db->db, str, NULL, NULL);
  free(str);
  }

void
bg_db_create_vfolders(bg_db_t * db, bg_db_object_t * obj)
  {
  bg_db_object_type_t type;

  type = bg_db_object_get_type(obj);

  switch(type)
    {
    case BG_DB_OBJECT_AUDIO_FILE:
      create_vfolders_audio_file(db, obj);
      break;
    case BG_DB_OBJECT_AUDIO_ALBUM:
      create_vfolders_audio_album(db, obj);
      break;
    default:
      break;
    }
  
  }

