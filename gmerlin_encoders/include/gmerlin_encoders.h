/*****************************************************************

  gmerlin_encoders.h

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

#include <stdio.h>
#include <gmerlin/plugin.h>

/* ID3 V1.1 and V2.4 support */

typedef struct bgen_id3v1_s bgen_id3v1_t;
typedef struct bgen_id3v2_s bgen_id3v2_t;

bgen_id3v1_t * bgen_id3v1_create(const bg_metadata_t*);
bgen_id3v2_t * bgen_id3v2_create(const bg_metadata_t*);

int bgen_id3v1_write(FILE * output, const bgen_id3v1_t *);
int bgen_id3v2_write(FILE * output, const bgen_id3v2_t *);

void bgen_id3v1_destroy(bgen_id3v1_t *);
void bgen_id3v2_destroy(bgen_id3v2_t *);
