/*****************************************************************
 
  oss_common.h
 
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


gavl_sample_format_t
bg_oss_set_sample_format(int fd, gavl_sample_format_t format);

int bg_oss_set_channels(int fd, int num_channels);

int bg_oss_set_samplerate(int fd, int samplerate);
