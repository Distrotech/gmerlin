/*****************************************************************
 
  pwc.h
 
  Copyright (c) 2003-2006 by Burkhard Plaum - plaum@ipf.uni-stuttgart.de
 
  http://gmerlin.sourceforge.net
 
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
 
  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111, USA.
 
*****************************************************************/

int bg_pwc_probe(int fd);

void * bg_pwc_get_parameters(int fd,
                                    bg_parameter_info_t ** parameters);

void bg_pwc_destroy(void*);

void bg_pwc_set_parameter(int fd, void * priv, char * name,
                                 bg_parameter_value_t * val);
