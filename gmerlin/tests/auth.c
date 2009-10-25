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

#include <stdio.h>
#include <gtk/gtk.h>
#include <gui_gtk/auth.h>
#include <gui_gtk/gtkutils.h>

int main(int argc, char ** argv)
  {
  char * user = (char*)0;
  char * pass = (char*)0;
  int save_auth = 0;
  bg_gtk_init(&argc, &argv, NULL, NULL, NULL);

  if(bg_gtk_get_userpass("Area 51",
                         &user, &pass,
                         &save_auth,
                         NULL))
    {
    fprintf(stderr, "User: %s\n", user);
    fprintf(stderr, "Pass: %s\n", pass);
    fprintf(stderr, "Save: %d\n", save_auth);
    }
  else
    fprintf(stderr, "Cancel\n");
   
  
  return 0;
  }
