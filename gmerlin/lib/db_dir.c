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
#include <stdlib.h>
#include <string.h>


void bg_db_dir_init(bg_db_dir_t * dir)
  {
  memset(dir, 0, sizeof(*dir));
  dir->id = -1;
  }

void bg_db_dir_free(bg_db_dir_t * dir)
  {
  if(dir->path)
    free(dir->path);
  }

int bg_db_dir_query(bg_db_t * db, bg_db_dir_t * dir)
  {
  
  }

void bg_db_dir_add(bg_db_t * db, bg_db_dir_t * dir)
  {
  
  }
