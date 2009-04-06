/*****************************************************************
 * gmerlin-avdecoder - a general purpose multimedia decoding library
 *
 * Copyright (c) 2001 - 2008 Members of the Gmerlin project
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

#include <string.h>


#include <avdec_private.h>
#include <aac_frame.h>
#include <bitstream.h>

/* Some stuff taken from ffmpeg */

const int sample_rates[16] =
  {
    96000, 88200, 64000, 48000, 44100, 32000,
    24000, 22050, 16000, 12000, 11025, 8000, 7350
  };

const uint8_t aac_pred_sfb_max[] =
  {
    33, 33, 38, 40, 40, 40, 41, 41, 37, 37, 37, 34, 34
  };

#define MY_MIN(a, b) (a < b ? a : b)


typedef struct
  {
  int num_window_groups;
  int max_sfb;
  int common_window;
  int window_sequence;
  } aac_state_t;

#define ONLY_LONG_SEQUENCE   0
#define LONG_START_SEQUENCE  1
#define EIGHT_SHORT_SEQUENCE 2
#define LONG_STOP_SEQUENCE   3

static int ics_info(bgav_aac_header_t * h,
                    bgav_bitstream_t * b, aac_state_t * st)
  {
  int i;
  int dummy;
  int window_shape;
  
  st->num_window_groups = 1;
  
  if(!bgav_bitstream_get(b, &dummy, 1) || // ics_reserved_bit
     !bgav_bitstream_get(b, &st->window_sequence, 2) || // window_sequence
     !bgav_bitstream_get(b, &window_shape, 1)) // window_shape
    return 0;

  if(st->window_sequence == EIGHT_SHORT_SEQUENCE)
    {
    if(!bgav_bitstream_get(b, &st->max_sfb, 4))
      return 0;

    for(i = 0; i < 7; i++)
      {
      if(!bgav_bitstream_get(b, &dummy, 1)) // scale_factor_grouping
        return 0;
      if(!dummy)
        st->num_window_groups++;
      }
    }
  else
    {
    if(!bgav_bitstream_get(b, &st->max_sfb, 6) ||
       !bgav_bitstream_get(b, &dummy, 1)) // predictor_data_present
      return 0;
    if(dummy)
      {
      int sfb;
      if(!bgav_bitstream_get(b, &dummy, 1)) // predictor_reset
        {
        if(dummy)
          {
          if(!bgav_bitstream_get(b, &dummy, 5)) // predictor_reset_group_number
            return 0;
          }
        }

      for(sfb = 0;
          sfb < MY_MIN(st->max_sfb, aac_pred_sfb_max[h->samplerate_index]);
          sfb++)
        {
        if(!bgav_bitstream_get(b, &dummy, 1)) // prediction_used[sfb];
          return 0;
        }
      
      }
    }
  
  return 1;
  }

static int section_data(bgav_aac_header_t * h,
                        bgav_bitstream_t * b,
                        aac_state_t * st)
  {
  int g, k;

  
  
  for(g = 0; g < st->max_sfb; g++)
    {
    
    }
  }
     
static int individial_channel_stream(bgav_aac_header_t * h,
                                     bgav_bitstream_t * b,
                                     aac_state_t * st, int common_window)
  {
  int dummy;
  if(!bgav_bitstream_get(b, &dummy, 8)) // global_gain
    return 0;

  if(!common_window)
    {
    if(!ics_info(h, b, st))
      return 0;
    }
  
  return 0;
  }
     
static int single_channel_element(bgav_aac_header_t * h,
                                  bgav_bitstream_t * b, aac_state_t * st)
  {
  int dummy;
  
  if(!bgav_bitstream_get(b, &dummy, 4)) // element_instance_tag
    return 0;

  return individial_channel_stream(h, b, st, 0);
  
  }


static int channel_pair_element(bgav_aac_header_t * h,
                                bgav_bitstream_t * b, aac_state_t * st)
  {
  int dummy;
  if(!bgav_bitstream_get(b, &dummy, 4)) // element_instance_tag
    return 0;

  if(!bgav_bitstream_get(b, &st->common_window, 1)) // common_window
    return 0;

  if(st->common_window)
    {
    if(!ics_info(h, b, st))
      return 0;
    
    if(!bgav_bitstream_get(b, &dummy, 2)) // ms_mask_present
      return 0;

    if(dummy == 1)
      {
      int g;
      for(g = 0; g < st->num_window_groups * st->max_sfb; g++)
        {
        if(!bgav_bitstream_get(b, &dummy, 1)) // ms_used
          return 0;
        }
      }
    
    }

  if(!individial_channel_stream(h, b, st, st->common_window) ||
     !individial_channel_stream(h, b, st, st->common_window))
    return 0;
  
  return 1;
  }

/* Returns the number of bytes */
int bgav_aac_frame_parse(bgav_aac_header_t * h,
                         uint8_t * data, int len,
                         int * num_samples,
                         int * num_bytes)
  {
  aac_state_t st;
  bgav_bitstream_t b;
  int id_syn_ele;
  int done = 0;
  
  bgav_bitstream_init(&b, data, len);
  memset(&st, 0, sizeof(st));
  
  while(!done)
    {
    if(!bgav_bitstream_get(&b, &id_syn_ele, 3))
      return 0;

    switch(id_syn_ele)
      {
      case 0: // ID_SCE
        if(!single_channel_element(h, &b, &st))
          return 0;
        break;
      case 1: // ID_CPE
        if(!channel_pair_element(h, &b, &st))
          return 0;
        break;
      case 2: // ID_CCE
        break;
      case 3: // ID_LFE
        break;
      case 4: // ID_DSE
        break;
      case 5: // ID_PCE
        break;
      case 6: // ID_FIL
        break;
      case 7: // ID_END
        done = 1;
        break;
      }
    }
  return 1;
  }

int bgav_aac_header_parse(bgav_aac_header_t * h, uint8_t * data, int len)
  {
  bgav_bitstream_t b;
  bgav_bitstream_init(&b, data, len);

  if(bgav_bitstream_get(&b, &h->object_type, 5))
    return 0;

  if(h->object_type == 31)
    {
    if(bgav_bitstream_get(&b, &h->object_type, 6))
      return 0;
    h->object_type += 32;
    }

  if(bgav_bitstream_get(&b, &h->samplerate_index, 4))
    return 0;

  if(h->samplerate_index == 0x0f)
    {
    if(bgav_bitstream_get(&b, &h->samplerate, 24))
      return 0;
    }
  else
    h->samplerate = sample_rates[h->samplerate_index];
  return 1;
  }
