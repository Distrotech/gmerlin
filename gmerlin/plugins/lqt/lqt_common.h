/*****************************************************************
 
  lqt_common.h
 
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

#include <lqt.h>
#include <lqt_codecinfo.h>
#include <colormodels.h>
#include <parameter.h>

void bg_lqt_create_codec_info(bg_parameter_info_t * parameter_info,
                              int audio, int video, int encode, int decode);

int bg_lqt_set_parameter(const char * name, bg_parameter_value_t * val,
                         bg_parameter_info_t * info);

extern int * bg_lqt_supported_colormodels;
                     
void bg_lqt_set_audio_parameter(quicktime_t * file,
                                int stream,
                                char * name,
                                bg_parameter_value_t * val,
                                lqt_parameter_info_t * lqt_parameter_info);

void bg_lqt_set_video_parameter(quicktime_t * file,
                                int stream,
                                char * name,
                                bg_parameter_value_t * val,
                                lqt_parameter_info_t * lqt_parameter_info);
