/*****************************************************************
 
  subovl_dvd.c
 
  Copyright (c) 2006 by Burkhard Plaum - plaum@ipf.uni-stuttgart.de
 
  http://gmerlin.sourceforge.net
 
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
 
  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111, USA.
 
*****************************************************************/

#include <avdec_private.h>

static int init_dvdsub(bgav_stream_t * s)
  {
  return 0;
  }

/* Query if a subtitle is available */
static int has_subtitle_dvdsub(bgav_stream_t * s)
  {
  return 0;
  }

/*
 *  Decodes one frame. If frame is NULL;
 *  the frame is skipped
 */

static int decode_dvdsub(bgav_stream_t * s, gavl_overlay_t * ovl)
  {
  return 0;
  }

static void close_dvdsub(bgav_stream_t * s)
  {

  }

static void resync_dvdsub(bgav_stream_t * s)
  {
  
  }

static bgav_subtitle_overlay_decoder_t decoder =
  {
    fourccs: (uint32_t[]){ BGAV_MK_FOURCC('D', 'V', 'D', 'S'), 0 },
    name:    "DVD subtitle decoder",
    init:         init_dvdsub,
    has_subtitle: has_subtitle_dvdsub,
    decode:       decode_dvdsub,
    close:        close_dvdsub,
    resync:       resync_dvdsub,
  };

void bgav_init_subtitle_overlay_decoders_dvd()
  {
  bgav_subtitle_overlay_decoder_register(&decoder);
  }
