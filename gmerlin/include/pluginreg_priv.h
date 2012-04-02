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

/** \ingroup plugin_registry
 *  \brief Load plugin infos from an xml file
 *  \param filename The name of the xml file
 */

bg_plugin_info_t * bg_plugin_registry_load(const char * filename);

/** \ingroup plugin_registry
 *  \brief
 */

void bg_plugin_registry_save(bg_plugin_info_t * info);


void bg_plugin_info_destroy(bg_plugin_info_t * info);

bg_plugin_info_t * bg_edldec_get_info();
