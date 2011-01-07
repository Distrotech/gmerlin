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

#include <stdio.h>
#include <gtk/gtk.h>
#include <gui_gtk/question.h>
#include <gui_gtk/gtkutils.h>

int main(int argc, char ** argv)
  {
  int ans;
  bg_gtk_init(&argc, &argv, NULL, NULL, NULL);
 
  ans = bg_gtk_question("Switch coffemachine to backwards\rOr not?", NULL);

  fprintf(stderr, "Got answer: %s\n", (ans)? "yes" : "no");
  
  return 0;
  }
