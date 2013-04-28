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

static struct
  {
  const char * table;
  const char * name;
  bg_db_audio_category_t cats[BG_DB_VFOLDER_MAX_DEPTH];
  }
vfolder_types[] =
  {
    .table = "AUDIO_FILES",
    .name = "By Artist",
    .cats = { BG_DB_AUDIO_CAT_GENRE,
              BG_DB_AUDIO_CAT_ALBUMARTIST_GROUP,
              BG_DB_AUDIO_CAT_ALBUMARTIST,
              BG_DB_AUDIO_CAT_ALBUM,
              BG_DB_AUDIO_CAT_SONG,
            }
  };

void create_vfolders_audio_file(bg_db_t * db, bg_object_t * obj)
  {
  
  }

void create_vfolders_audio_album(bg_db_t * db, bg_object_t * obj)
  {
  
  }

void
bg_db_create_tables_vfolders(bg_db_t * db)
  {
  int i;
  char * str = bg_strdup("CREATE TABLE VFOLDERS ( ID PRIMARY KEY");
  char tmp_string[32];

  
  for(i = 0; i < BG_DB_VFOLDER_MAX_DEPTH; i++)
    {
    snprintf(tmp_string, 32, "CAT_%d", i+1);
    str = gavl_strcat(str, tmp_string);
    snprintf(tmp_string, 32, "VAL_%d", i+1);
    str = gavl_strcat(str, tmp_string);
    }

  str = gavl_strcat(str, ");");
  bg_sqlite_exec(db->db, str, NULL, NULL);
  free(str);
  }

void
bg_db_create_vfolders(bg_db_t * db, bg_object_t * obj)
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
    }
  
  }



/* External */
bg_object_t *
bg_db_audio_vfolder_browse_children(bg_db_t * db, int64_t id);

