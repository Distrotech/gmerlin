/*****************************************************************
 * gmerlin - a general purpose multimedia framework and applications
 *
 * Copyright (c) 2001 - 2008 Members of the Gmerlin project
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

#include <gmerlin/plugin.h>
#include <gmerlin/pluginfuncs.h>

int bg_encoder_cb_create_output_file(bg_encoder_callbacks_t * cb,
                                     const char * filename)
  {
  if(cb && cb->create_output_file)
    return cb->create_output_file(cb->data, filename);
  else
    return 1;
  }

int bg_encoder_cb_create_temp_file(bg_encoder_callbacks_t * cb,
                                   const char * filename)
  {
  if(cb && cb->create_temp_file)
    return cb->create_temp_file(cb->data, filename);
  else
    return 1;
  
  }

int bg_iw_cb_create_output_file(bg_iw_callbacks_t * cb,
                                const char * filename)
  {
  if(cb && cb->create_output_file)
    return cb->create_output_file(cb->data, filename);
  else
    return 1;
  }
