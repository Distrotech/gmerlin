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

/*
 *  These get a bunch of media files (a directory or an album file)
 *  and make them available like a recording device
 */

bg_plugin_info_t * bg_audiofiledevice_info(bg_plugin_registry_t * reg);

const bg_plugin_common_t * bg_audiofiledevice_get();

void * bg_audiofiledevice_create(bg_plugin_registry_t * reg);

#define bg_audiofiledevice_name        "i_audiofile"

