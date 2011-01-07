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


typedef struct bg_gtk_filesel_s bg_gtk_filesel_t;

/* Create fileselector with callback */

bg_gtk_filesel_t *
bg_gtk_filesel_create(const char * title,
                      void (*add_file)(char ** files, const char * plugin,
                                       void * data),
                      void (*close_notify)(bg_gtk_filesel_t *,
                                           void * data),
                      void * user_data,
                      GtkWidget * parent_window,
                      bg_plugin_registry_t * plugin_reg, int type_mask,
                      int flag_mask);

/* Create directory selector (for addig directories to the tree) */

bg_gtk_filesel_t *
bg_gtk_dirsel_create(const char * title,
                     void (*add_dir)(char * dir, int recursive,
                                     int subdirs_as_subalbums,
                                     int watch,
                                     const char * plugin,
                                     void * data),
                     void (*close_notify)(bg_gtk_filesel_t *,
                                          void * data),
                     void * user_data,
                     GtkWidget * parent_window,
                     bg_plugin_registry_t * plugin_reg, int type_mask, int flag_mask);


/* Destroy fileselector */

void bg_gtk_filesel_destroy(bg_gtk_filesel_t * filesel);

/* Show the window */

/* A non modal window will destroy itself when it's closed */

void bg_gtk_filesel_run(bg_gtk_filesel_t * filesel, int modal);

/* Get the current working directory */

const char * bg_gtk_filesel_get_directory(bg_gtk_filesel_t * filesel);
void bg_gtk_filesel_set_directory(bg_gtk_filesel_t * filesel,
                                  const char * dir);

/*
 *  Create a temporary fileselector and ask
 *  for a file to save something
 *
 *  Return value should be freed with free();
 */

char * bg_gtk_get_filename_write(const char * title,
                                 char ** directory,
                                 int ask_overwrite, GtkWidget * parent);

char * bg_gtk_get_filename_read(const char * title,
                                char ** directory, GtkWidget * parent);
