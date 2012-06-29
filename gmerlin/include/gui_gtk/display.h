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


/* Display for Time */

typedef enum
  {
    BG_GTK_DISPLAY_SIZE_HUGE,   /* 240 x 96, 1/1 */
    BG_GTK_DISPLAY_SIZE_LARGE,  /* 120 x 48, 1/2 */
    BG_GTK_DISPLAY_SIZE_NORMAL, /*  80 x 32  1/3 */
    BG_GTK_DISPLAY_SIZE_SMALL,  /*  40 x 16  1/6 */
  } BG_GTK_DISPLAY_SIZE;

#define BG_GTK_DISPLAY_MODE_HMS        (1<<0) // 000:00:00
#define BG_GTK_DISPLAY_MODE_HMSMS      (1<<1) // 000:00:00.000
#define BG_GTK_DISPLAY_MODE_TIMECODE   (1<<2) //  00:00:00:00
#define BG_GTK_DISPLAY_MODE_FRAMECOUNT (1<<3) //  00000000

typedef struct bg_gtk_time_display_s bg_gtk_time_display_t;

bg_gtk_time_display_t *
bg_gtk_time_display_create(BG_GTK_DISPLAY_SIZE size,
                           int border_width, int mode_mask);

GtkWidget * bg_gtk_time_display_get_widget(bg_gtk_time_display_t *);

void bg_gtk_time_display_update(bg_gtk_time_display_t * d,
                                gavl_time_t time, int mode);

void bg_gtk_time_display_set_colors(bg_gtk_time_display_t * d,
                                    float * foreground,
                                    float * background);

void bg_gtk_time_display_destroy(bg_gtk_time_display_t * d);
