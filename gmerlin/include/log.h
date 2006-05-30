/*****************************************************************
 
  log.h
 
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

#ifndef __BG_LOG_H_
#define __BG_LOG_H_

/* Gmerlin log facilities */

#include "parameter.h"
#include "msgqueue.h"

typedef enum
  {
    BG_LOG_DEBUG,
    BG_LOG_WARNING,
    BG_LOG_ERROR,
    BG_LOG_INFO
  } bg_log_level_t;

void bg_log(bg_log_level_t level, const char * domain,
            const char * format, ...);

void bg_set_log_dest(bg_msg_queue_t *);

const char * bg_log_level_to_string(bg_log_level_t level);

#endif // __BG_LOG_H_
