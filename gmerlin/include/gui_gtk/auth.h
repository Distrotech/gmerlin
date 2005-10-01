/*****************************************************************
 
  auth.h
 
  Copyright (c) 2005 by Burkhard Plaum - plaum@ipf.uni-stuttgart.de
 
  http://gmerlin.sourceforge.net
 
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
 
  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111, USA.
 
*****************************************************************/

/*
 *  Simple authentication using a gtk dialog
 *  This function (along with NULL for the data)
 *  can be passed to bg_album_set_userpass_callback.
 */


int bg_gtk_get_userpass(const char * resource,
                        char ** user, char ** pass,
                        int * save_password,
                        void * data);
