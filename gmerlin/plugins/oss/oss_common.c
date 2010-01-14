/*****************************************************************
 * gmerlin - a general purpose multimedia framework and applications
 *
 * Copyright (c) 2001 - 2010 Members of the Gmerlin project
 * gmerlin-general@lists.sourceforge.net
 * http://gmerlin.sourceforge.net
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 * *****************************************************************/

#include <stdio.h>
#include <sys/ioctl.h>
#include <sys/soundcard.h>

#include <gavl/gavl.h>
#include <gmerlin/utils.h>

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
    case GAVL_SAMPLE_U16:

#ifdef WORDS_BIGENDIAN
      i_tmp = AFMT_U16_BE;
#else
      i_tmp = AFMT_U16_LE;
#endif
      break;
    case GAVL_SAMPLE_S16:
      i_tmp = AFMT_S16_NE;
      break;
    default:
      i_tmp = AFMT_S16_NE;
      break;
    }

  
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
#ifdef WORDS_BIGENDIAN
    case AFMT_U16_BE:
      ret = GAVL_SAMPLE_U16;
      break;
    case AFMT_S16_BE:
      ret = GAVL_SAMPLE_S16;
      break;
#else
    case AFMT_U16_LE:
      ret = GAVL_SAMPLE_U16;
      break;
    case AFMT_S16_LE:
      ret = GAVL_SAMPLE_S16;
      break;
#endif
    default:
      ret = GAVL_SAMPLE_NONE;
    }
  return ret;
  }

int bg_oss_set_channels(int fd, int num_channels)
  {
  
  if(ioctl(fd, SNDCTL_DSP_CHANNELS, &num_channels) == -1)
    return 0;
  return num_channels;
  }

int bg_oss_set_samplerate(int fd, int samplerate)
  {
  
  if(ioctl(fd, SNDCTL_DSP_SPEED, &samplerate) == -1)
    return 0;
  return samplerate;
  }
