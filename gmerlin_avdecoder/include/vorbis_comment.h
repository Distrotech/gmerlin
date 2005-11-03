/*****************************************************************
 
  vorbis_comment.h
 
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

typedef struct
  {
  char * vendor;
  int num_user_comments;

  char ** user_comments;
  } bgav_vorbis_comment_t;

int bgav_vorbis_comment_read(bgav_vorbis_comment_t * ret,
                             bgav_input_context_t * input);

void bgav_vorbis_comment_2_metadata(bgav_vorbis_comment_t * comment,
                                    bgav_metadata_t * m);

void bgav_vorbis_comment_free(bgav_vorbis_comment_t * ret);

void bgav_vorbis_comment_dump(bgav_vorbis_comment_t * ret);

const char *
bgav_vorbis_comment_get_field(bgav_vorbis_comment_t * vc, const char * key);
