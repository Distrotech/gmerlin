/*****************************************************************
 
  fileselect.h
 
  Copyright (c) 2003-2004 by Burkhard Plaum - plaum@ipf.uni-stuttgart.de
 
  http://gmerlin.sourceforge.net
 
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
 
  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111, USA.
 
*****************************************************************/

typedef struct bg_gtk_filesel_s bg_gtk_filesel_t;

/* Create fileselector with callback */

bg_gtk_filesel_t *
bg_gtk_filesel_create(const char * title,
                      void (*add_file)(char ** files, const char * plugin,
                                       void * data),
                      void (*close_notify)(bg_gtk_filesel_t *,
                                           void * data),
                      char ** plugins,
                      void * user_data,
                      GtkWidget * parent_window);

/* Create directory selector (for addig directories to the tree) */

bg_gtk_filesel_t *
bg_gtk_dirsel_create(const char * title,
                     void (*add_dir)(char * dir, int recursive,
                                     int subdirs_as_subalbums,
                                     const char * plugin,
                                     void * data),
                     void (*close_notify)(bg_gtk_filesel_t *,
                                          void * data),
                     char ** plugins,
                     void * user_data,
                     GtkWidget * parent_window);


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
                                 char ** default_dir,
                                 int ask_overwrite);
