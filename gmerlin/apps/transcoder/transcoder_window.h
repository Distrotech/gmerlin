/*****************************************************************
 
  transcoder_window.h
 
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

typedef struct transcoder_window_s transcoder_window_t;

transcoder_window_t * transcoder_window_create();

void transcoder_window_destroy(transcoder_window_t*);
void transcoder_window_run(transcoder_window_t *);
transcoder_window_t * transcoder_window_create();

void transcoder_window_destroy(transcoder_window_t*);
void transcoder_window_run(transcoder_window_t *);

