/*****************************************************************
 
  oss_common.c
 
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

#include <stdio.h>
#include <sys/ioctl.h>
#include <sys/soundcard.h>

#include <gavl/gavl.h>
#include <utils.h>

#include "oss_common.h"

gavl_sample_format_t
bg_oss_set_sample_format(int fd, gavl_sample_format_t format)
  {
  int i_tmp;
  gavl_sample_format_t ret;
      
  /* Check out sample format */
  
  switch(format)
    {
    case GAVL_SAMPLE_U8:
      i_tmp = AFMT_U8;
      break;
    case GAVL_SAMPLE_S8:
      i_tmp = AFMT_S8;
      break;
    case GAVL_SAMPLE_U16BE:
      i_tmp = AFMT_U16_BE;
      break;
    case GAVL_SAMPLE_U16LE:
      i_tmp = AFMT_U16_LE;
      break;
    case GAVL_SAMPLE_S16BE:
      i_tmp = AFMT_S16_BE;
      break;
    case GAVL_SAMPLE_S16LE:
      i_tmp = AFMT_S16_LE;
      break;
    default:
      i_tmp = AFMT_S16_NE;
      break;
    }

  //  fprintf(stderr, "Setting sample format %s\n",
  //          gavl_sample_format_to_string(format));
  
  if(ioctl(fd, SNDCTL_DSP_SETFMT, &i_tmp) == -1)
    {
    return GAVL_SAMPLE_NONE;
    }
  
  switch(i_tmp)
    {
    case AFMT_U8:
      ret = GAVL_SAMPLE_U8;
      break;
    case AFMT_S8:
      ret = GAVL_SAMPLE_S8;
      break;
    case AFMT_U16_BE:
      ret = GAVL_SAMPLE_U16BE;
      break;
    case AFMT_U16_LE:
      ret = GAVL_SAMPLE_U16LE;
      break;
    case AFMT_S16_BE:
      ret = GAVL_SAMPLE_S16BE;
      break;
    case AFMT_S16_LE:
      ret = GAVL_SAMPLE_S16LE;
      break;
    default:
      ret = GAVL_SAMPLE_NONE;
    }
  return ret;
  }

int bg_oss_set_channels(int fd, int num_channels)
  {
  //  fprintf(stderr, "Setting channels: %d\n", num_channels);
  
  if(ioctl(fd, SNDCTL_DSP_CHANNELS, &num_channels) == -1)
    return 0;
  return num_channels;
  }

int bg_oss_set_samplerate(int fd, int samplerate)
  {
  //  fprintf(stderr, "Setting samplerate %d\n", samplerate);
  
  if(ioctl(fd, SNDCTL_DSP_SPEED, &samplerate) == -1)
    return 0;
  return samplerate;
  }
