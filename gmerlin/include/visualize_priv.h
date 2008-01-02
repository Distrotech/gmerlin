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

/* Communication between the visualizer and the slave */

#define BG_VIS_MSG_AUDIO_FORMAT  0
#define BG_VIS_MSG_AUDIO_DATA    1
#define BG_VIS_MSG_IMAGE_SIZE    2
#define BG_VIS_MSG_FPS           3
#define BG_VIS_MSG_QUIT          4
#define BG_VIS_MSG_VIS_PARAM     5
#define BG_VIS_MSG_OV_PARAM      6
#define BG_VIS_MSG_GAIN          7
#define BG_VIS_MSG_START         8

/* Let the slave tell his messages,
 * will be termimated with BG_VIS_SLAVE_MSG_END */
#define BG_VIS_MSG_TELL          9

/* Messages from the visualizer to the application */

#define BG_VIS_SLAVE_MSG_FPS     (BG_LOG_LEVEL_MAX+1)
#define BG_VIS_SLAVE_MSG_END     (BG_LOG_LEVEL_MAX+2)

/*
 * gmerlin_visualize_slave
 *   [-w "window_id"|-o "output_module"]
 *   -p "plugin_module"
 */
