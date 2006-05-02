/*****************************************************************

  peakdetector.c

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

#include <stdlib.h>


#include <gavl.h>

struct gavl_peak_detector_s
  {
  int64_t min_i;
  int64_t max_i;
  double min_d;
  double max_d;
  
  gavl_audio_format_t format;
  void (*update_channel)(gavl_peak_detector_t*,void*,int);
  void (*update)(gavl_peak_detector_t*, gavl_audio_frame_t*);
  };

static void update_none(gavl_peak_detector_t*pd, gavl_audio_frame_t*f)
  {
  int i;
  for(i = 0; i < pd->format.num_channels; i++)
    {
    pd->update_channel(pd, f->channels.s_8[i], f->valid_samples);
    }
  }

static void update_all(gavl_peak_detector_t*pd, gavl_audio_frame_t*f)
  {
  pd->update_channel(pd, f->samples.s_8,
                     f->valid_samples * pd->format.num_channels);
  }

static void update_2(gavl_peak_detector_t*pd, gavl_audio_frame_t*f)
  {
  int i;
  for(i = 0; i < pd->format.num_channels/2; i++)
    {
    pd->update_channel(pd, f->channels.s_8[i*2], f->valid_samples*2);
    }
  if(pd->format.num_channels % 2)
    pd->update_channel(pd, f->channels.s_8[pd->format.num_channels-1],
                       f->valid_samples);
  }

static void update_channel_u8(gavl_peak_detector_t * pd, void * _samples,
                              int num)
  {
  int i;
  uint8_t * samples = (uint8_t *)_samples;
  for(i = 0; i < num; i++)
    {
    if(samples[i] > pd->max_i) pd->max_i = samples[i];
    if(samples[i] < pd->min_i) pd->min_i = samples[i];
    }
  pd->min_d = (double)((int)pd->min_i-0x80) / 128.0;
  pd->max_d = (double)((int)pd->max_i-0x80) / 127.0;
  }

static void update_channel_s8(gavl_peak_detector_t * pd, void * _samples,
                              int num)
  {
  int i;
  int8_t * samples = (int8_t *)_samples;
  for(i = 0; i < num; i++)
    {
    if(samples[i] > pd->max_i) pd->max_i = samples[i];
    if(samples[i] < pd->min_i) pd->min_i = samples[i];
    }
  pd->min_d = (double)((int)pd->min_i) / 128.0;
  pd->max_d = (double)((int)pd->max_i) / 127.0;
  }

static void update_channel_u16(gavl_peak_detector_t * pd, void * _samples,
                               int num)
  {
  int i;
  uint16_t * samples = (uint16_t *)_samples;
  for(i = 0; i < num; i++)
    {
    if(samples[i] > pd->max_i) pd->max_i = samples[i];
    if(samples[i] < pd->min_i) pd->min_i = samples[i];
    }
  pd->min_d = (double)((int)pd->min_i-0x8000) / 32768.0;
  pd->max_d = (double)((int)pd->max_i-0x8000) / 32767.0;
  }

static void update_channel_s16(gavl_peak_detector_t * pd, void * _samples,
                               int num)
  {
  int i;
  int16_t * samples = (uint16_t *)_samples;
  for(i = 0; i < num; i++)
    {
    if(samples[i] > pd->max_i) pd->max_i = samples[i];
    if(samples[i] < pd->min_i) pd->min_i = samples[i];
    }
  pd->min_d = (double)((int)pd->min_i) / 32768.0;
  pd->max_d = (double)((int)pd->max_i) / 32767.0;
  }

static void update_channel_s32(gavl_peak_detector_t * pd, void * _samples,
                               int num)
  {
  int i;
  int32_t * samples = (uint32_t *)_samples;
  for(i = 0; i < num; i++)
    {
    if(samples[i] > pd->max_i) pd->max_i = samples[i];
    if(samples[i] < pd->min_i) pd->min_i = samples[i];
    }
  pd->min_d = (double)((int)pd->min_i) / 2147483648.0;
  pd->max_d = (double)((int)pd->max_i) / 2147483647.0;
  }

static void update_channel_float(gavl_peak_detector_t * pd, void * _samples,
                               int num)
  {
  int i;
  float * samples = (float*)_samples;
  for(i = 0; i < num; i++)
    {
    if(samples[i] > pd->max_d) pd->max_d = samples[i];
    if(samples[i] < pd->min_d) pd->min_d = samples[i];
    }
  }

gavl_peak_detector_t * gavl_peak_detector_create()
  {
  gavl_peak_detector_t * ret;
  ret = calloc(1, sizeof(*ret));
  return ret;
  }


void gavl_peak_detector_destroy(gavl_peak_detector_t *pd)
  {
  free(pd);
  }


void gavl_peak_detector_set_format(gavl_peak_detector_t *pd,
                                    gavl_audio_format_t * format)
  {
  gavl_audio_format_copy(&pd->format, format);
  switch(pd->format.interleave_mode)
    {
    case GAVL_INTERLEAVE_NONE:
      pd->update = update_none;
      break;
    case GAVL_INTERLEAVE_ALL:
      pd->update = update_all;
      break;
    case GAVL_INTERLEAVE_2:
      pd->update = update_2;
      break;
    }
  switch(pd->format.sample_format)
    {
    case GAVL_SAMPLE_U8:
      pd->update_channel = update_channel_u8;
      break;
    case GAVL_SAMPLE_S8:
      pd->update_channel = update_channel_s8;
      break;
    case GAVL_SAMPLE_U16:
      pd->update_channel = update_channel_u16;
      break;
    case GAVL_SAMPLE_S16:
      pd->update_channel = update_channel_s16;
      break;
    case GAVL_SAMPLE_S32:
      pd->update_channel = update_channel_s32;
      break;
    case GAVL_SAMPLE_FLOAT:
      pd->update_channel = update_channel_float;
      break;
    case GAVL_SAMPLE_NONE:
      break;
    }
  gavl_peak_detector_reset(pd);
  }

void gavl_peak_detector_update(gavl_peak_detector_t *pd,
                               gavl_audio_frame_t * frame)
  {
  pd->update(pd, frame);
  }


void gavl_peak_detector_get_peak(gavl_peak_detector_t * pd,
                                 double * min, double * max)
  {
  *min = pd->min_d;
  *max = pd->max_d;
  }
  
void gavl_peak_detector_reset(gavl_peak_detector_t * pd)
  {
  switch(pd->format.sample_format)
    {
    case GAVL_SAMPLE_U8:
      pd->min_i = 0x80;
      pd->max_i = 0x80;
      break;
    case GAVL_SAMPLE_U16:
      pd->min_i = 0x8000;
      pd->max_i = 0x8000;
      break;
    case GAVL_SAMPLE_S8:
    case GAVL_SAMPLE_S16:
    case GAVL_SAMPLE_S32:
      pd->min_i = 0;
      pd->max_i = 0;
      break;
    case GAVL_SAMPLE_FLOAT:
    case GAVL_SAMPLE_NONE:
      break;
    }
  pd->min_d = 0.0;
  pd->max_d = 0.0;
  }
