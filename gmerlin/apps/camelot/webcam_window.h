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

typedef struct gmerlin_webcam_window_s gmerlin_webcam_window_t;

#define WINDOW_NAME  "Camelot"
#define WINDOW_CLASS "camelot"
#define WINDOW_ICON "camelot_icon.png"

gmerlin_webcam_window_t *
gmerlin_webcam_window_create(gmerlin_webcam_t * w,
                             bg_plugin_registry_t * reg,
                             bg_cfg_registry_t * cfg_reg);
void
gmerlin_webcam_window_destroy(gmerlin_webcam_window_t * w);

void
gmerlin_webcam_window_show(gmerlin_webcam_window_t * w);


const bg_parameter_info_t *
gmerlin_webcam_window_get_parameters(gmerlin_webcam_window_t * w);

int
gmerlin_webcam_window_get_parameter(void * priv, const char * name,
                                    bg_parameter_value_t * val);

void
gmerlin_webcam_window_set_parameter(void * priv, const char * name,
                                    const bg_parameter_value_t * val);


