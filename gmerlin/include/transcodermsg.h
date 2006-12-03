/*****************************************************************
 
  transcodermsg.h

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

/* Messages of the transcoder */

/*
 *  0: Number of streams
 */

#define BG_TRANSCODER_MSG_NUM_AUDIO_STREAMS 1

/*
 *  0: stream index
 *  1: input format
 *  2: output format
 */

#define BG_TRANSCODER_MSG_AUDIO_FORMAT      2

/* Output file (0: Filename) */

#define BG_TRANSCODER_MSG_AUDIO_FILE        3


/*
 *  0: Number of streams
 */

#define BG_TRANSCODER_MSG_NUM_VIDEO_STREAMS 4

/*
 *  0: stream index
 *  1: input format
 *  2: output format
 */

#define BG_TRANSCODER_MSG_VIDEO_FORMAT      5

/* Output file (0: Filename) */

#define BG_TRANSCODER_MSG_VIDEO_FILE        6

#define BG_TRANSCODER_MSG_FILE              7

/*
 *  arg 1: float percentage_done
 *  arg 2: gavl_time_t remaining_time
 */

#define BG_TRANSCODER_MSG_PROGRESS          8

#define BG_TRANSCODER_MSG_FINISHED          9

/*
 *  arg 1: What started? (transcoding etc.)
 */

#define BG_TRANSCODER_MSG_START            10

/*
 *  arg 1: Metadata
 */

#define BG_TRANSCODER_MSG_METADATA         11

/*
 *  arg 1: Error message
 */

#define BG_TRANSCODER_MSG_ERROR            12
