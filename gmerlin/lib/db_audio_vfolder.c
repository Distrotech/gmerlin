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
  const char * name;
  bg_db_audio_category_t cats[5];
  }
vfolder_types[] =
  {
    .name = "By Artist",
    .cats = { BG_DB_AUDIO_CAT_GENRE,
              BG_DB_AUDIO_CAT_ALBUMARTIST_GROUP,
              BG_DB_AUDIO_CAT_ALBUMARTIST,
              BG_DB_AUDIO_CAT_ALBUM,
              BG_DB_AUDIO_CAT_SONG,
            }
  };
