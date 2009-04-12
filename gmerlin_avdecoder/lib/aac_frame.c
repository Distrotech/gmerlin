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

/* Most stuff taken from ffmpeg and faad2 */

static const uint8_t hcb_sf[][2];

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
  int num_windows;
  
  uint8_t sfb_cb[8][8*15];
  int num_sec[8];

  int sect_start[8][15*8];
  int sect_end[8][15*8];
  uint16_t sect_sfb_offset[8][15*8];
  
  int scale_factor_grouping;
  
  } aac_state_t;

#define ONLY_LONG_SEQUENCE   0
#define LONG_START_SEQUENCE  1
#define EIGHT_SHORT_SEQUENCE 2
#define LONG_STOP_SEQUENCE   3

#define ZERO_HCB       0
#define FIRST_PAIR_HCB 5
#define ESC_HCB        11
#define QUAD_LEN       4
#define PAIR_LEN       2
#define NOISE_HCB      13
#define INTENSITY_HCB2 14
#define INTENSITY_HCB  15

static void get_window_grouping_info(bgav_aac_header_t * h, aac_state_t * st);

static int ics_info(bgav_aac_header_t * h,
                    bgav_bitstream_t * b, aac_state_t * st)
  {
  int dummy;
  int window_shape;
  
  if(!bgav_bitstream_get(b, &dummy, 1) || // ics_reserved_bit
     !bgav_bitstream_get(b, &st->window_sequence, 2) || // window_sequence
     !bgav_bitstream_get(b, &window_shape, 1)) // window_shape
    return 0;

  if(st->window_sequence == EIGHT_SHORT_SEQUENCE)
    {
    st->scale_factor_grouping = 0;
    if(!bgav_bitstream_get(b, &st->max_sfb, 4) ||
       !bgav_bitstream_get(b, &st->scale_factor_grouping, 7)) // scale_factor_grouping
      return 0;
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

  /* Window grouping info */

  get_window_grouping_info(h, st);
  
  return 1;
  }

static int section_data(bgav_aac_header_t * h,
                        bgav_bitstream_t * b,
                        aac_state_t * st)
  {
  int g;
  uint8_t sect_esc_val, sect_bits;
  uint8_t sect_cb[8][15*8];
  int dummy;
  
  if(st->window_sequence == EIGHT_SHORT_SEQUENCE)
    sect_bits = 3;
  else
    sect_bits = 5;
  sect_esc_val = (1<<sect_bits) - 1;
  
  for(g = 0; g < st->num_window_groups; g++)
    {
    int k = 0;
    int i = 0;

    while(k < st->max_sfb)
      {
      uint8_t sfb;
      int sect_len_incr;
      uint16_t sect_len = 0;

      if(!bgav_bitstream_get(b, &dummy, 4))
        return 0;

      sect_cb[g][i] = dummy;

      while(1)
        {
        if(!bgav_bitstream_get(b, &sect_len_incr, sect_bits))
          return 0;
        if(sect_len_incr != sect_esc_val)
          break;
        else
          sect_len += sect_esc_val;
        }
      
      st->sect_start[g][i] = k;
      st->sect_end[g][i] = k + sect_len;
      
      for(sfb = k; sfb < k + sect_len; sfb++)
        st->sfb_cb[g][sfb] = sect_cb[g][i];
      k += sect_len;
      i++;
      }
    st->num_sec[g] = i;
    }
  return 1;
  }

static int skip_scale_factor(bgav_bitstream_t * b)
  {
  int dummy;
  int offset = 0;
  
  while(hcb_sf[offset][1])
    {
    if(!bgav_bitstream_get(b, &dummy, 1))
      return 0;
    
    offset += hcb_sf[offset][dummy];
    
    if(offset > 240)
      {
      return 0;
      }
    }
  return 1;
  }

static int scale_factor_data(bgav_aac_header_t * h,
                             bgav_bitstream_t * b,
                             aac_state_t * st)
  {
  int g, sfb;
  int noise_pcm_flag = 1;
  int dummy;
  for(g = 0; g < st->num_window_groups; g++)
    {
    for(sfb = 0; sfb < st->max_sfb; sfb++)
      {
      switch(st->sfb_cb[g][sfb])
        {
        case ZERO_HCB:
          break;
        case NOISE_HCB:
         if(noise_pcm_flag)
           {
           if(!bgav_bitstream_get(b, &dummy, 9))
             return 0;
           noise_pcm_flag = 0;
           }
         else if(!skip_scale_factor(b))
           return 0;
         break;
        default:
          if(!skip_scale_factor(b))
            return 0;
          break;
        }
      }
    }
  return 1;
  }

static int pulse_data(bgav_aac_header_t * h,
                      bgav_bitstream_t * b,
                      aac_state_t * st)
  {
  int i;
  int number_pulse, dummy;

  if(!bgav_bitstream_get(b, &number_pulse, 2) || 
     !bgav_bitstream_get(b, &dummy, 6)) // pulse_start_sfb
    return 0;

  for(i = 0; i < number_pulse+1; i++)
    {
    if(!bgav_bitstream_get(b, &dummy, 5) || // pulse_offset[i]
       !bgav_bitstream_get(b, &dummy, 4))   // pulse_amp[i]
      return 0;
    }
  return 1;
  }

static int tns_data(bgav_aac_header_t * h,
                    bgav_bitstream_t * b,
                    aac_state_t * st)
  {
  int w, filt, i, start_coef_bits = 3, coef_bits, dummy;
  int n_filt_bits = 2;
  int length_bits = 6;
  int order_bits = 5;

  int filt_val, order_val, coef_compress;
  
  if(st->window_sequence == EIGHT_SHORT_SEQUENCE)
    {
    n_filt_bits = 1;
    length_bits = 4;
    order_bits = 3;
    }
  
  for(w = 0; w < st->num_windows; w++)
    {
    if(!bgav_bitstream_get(b, &filt_val, n_filt_bits))
      return 0;

    if(filt_val)
      {
      if(!bgav_bitstream_get(b, &dummy, 1))
        return 0;
      if(dummy)
        start_coef_bits = 4;
      }

    for(filt = 0; filt < filt_val; filt++)
      {
      if(!bgav_bitstream_get(b, &dummy, length_bits))
        return 0;
      if(!bgav_bitstream_get(b, &order_val, order_bits))
        return 0;
      if(order_val)
        {
        if(!bgav_bitstream_get(b, &dummy, 1) || // direction[w][filt];
           !bgav_bitstream_get(b, &coef_compress, 1))
          return 0;
        
        coef_bits = start_coef_bits - coef_compress;
        for(i = 0; i < order_val; i++)
          {
          if(!bgav_bitstream_get(b, &dummy, coef_bits))
            return 0;
          }
        }
      }
    
    }
  return 1;
  }

static int gain_control_data(bgav_aac_header_t * h,
                             bgav_bitstream_t * b,
                             aac_state_t * st)
  {
  int max_band, adjust_num, bd, wd, ad, dummy;

  if(!bgav_bitstream_get(b, &max_band, 2))
    return 0;
  
  if(st->window_sequence == ONLY_LONG_SEQUENCE)
    {
    for (bd = 1; bd <= max_band; bd++)
      {
      for (wd = 0; wd < 1; wd++)
        {
        if(!bgav_bitstream_get(b, &adjust_num, 3))
          return 0;
        for(ad = 0; ad < adjust_num; ad++)
          {
          if(!bgav_bitstream_get(b, &dummy, 4) ||
             !bgav_bitstream_get(b, &dummy, 5))
            return 0;
          }
        }
      }
    }
  else if(st->window_sequence == LONG_START_SEQUENCE)
    {
    for(bd = 1; bd <= max_band; bd++)
      {
      for(wd = 0; wd < 2; wd++)
        {
        if(!bgav_bitstream_get(b, &adjust_num, 3))
          return 0;
        for(ad = 0; ad < adjust_num; ad++)
          {
          if(!bgav_bitstream_get(b, &dummy, 4))
            return 0;

          if(!wd)
            {
            if(!bgav_bitstream_get(b, &dummy, 4))
              return 0;
            }
          else
            {
            if(!bgav_bitstream_get(b, &dummy, 2))
              return 0;
            }
          }
        }
      }
    }
  else if(st->window_sequence == EIGHT_SHORT_SEQUENCE)
    {
    for (bd = 1; bd <= max_band; bd++)
      {
      for (wd = 0; wd < 8; wd++)
        {
        if(!bgav_bitstream_get(b, &adjust_num, 3))
          return 0;
        for(ad = 0; ad < adjust_num; ad++)
          {
          if(!bgav_bitstream_get(b, &dummy, 4) ||
             !bgav_bitstream_get(b, &dummy, 2))
            return 0;
          }
        }
      }
    }
  else if(st->window_sequence == LONG_STOP_SEQUENCE)
    {
    for(bd = 1; bd <= max_band; bd++)
      {
      for(wd = 0; wd < 2; wd++)
        {
        if(!bgav_bitstream_get(b, &adjust_num, 3))
          return 0;
        for(ad = 0; ad < adjust_num; ad++)
          {
          if(!bgav_bitstream_get(b, &dummy, 4))
            return 0;

          if(!wd)
            {
            if(!bgav_bitstream_get(b, &dummy, 4))
              return 0;
            }
          else
            {
            if(!bgav_bitstream_get(b, &dummy, 5))
              return 0;
            }
          }
        }
      }
    }
  return 1;
  }

static int spectral_data(bgav_aac_header_t * h,
                         bgav_bitstream_t * b,
                         aac_state_t * st)
  {
  int g, i;

  for(g = 0; g < st->num_window_groups; g++)
    {
    for(i = 0; i < st->num_sec[g]; i++)
      {
      
      }
    }
  
  return 0;
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

  if(!section_data(h, b, st))
    return 0;
  
  if(!scale_factor_data(h, b, st))
    return 0;

  if(!bgav_bitstream_get(b, &dummy, 1)) // pulse_data_present
    return 0;

  if(dummy)
    {
    if(!pulse_data(h, b, st))
      return 0;
    }

  if(!bgav_bitstream_get(b, &dummy, 1)) // tns_data_present
    return 0;

  if(dummy)
    {
    if(!tns_data(h, b, st))
      return 0;
    }

  if(!bgav_bitstream_get(b, &dummy, 1)) // gain_control_data_present
    return 0;

  if(dummy)
    {
    if(!gain_control_data(h, b, st))
      return 0;
    }
  
  if(!spectral_data(h, b, st))
    return 0;
  
  return 1;
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
        if(!single_channel_element(h, &b, &st))
          return 0;
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

/* Huffman tables */

/*
** FAAD2 - Freeware Advanced Audio (AAC) Decoder including SBR decoding
** Copyright (C) 2003-2005 M. Bakker, Nero AG, http://www.nero.com
**  
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
** 
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
** 
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software 
** Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
**
** Any non-GPL usage of this software or parts of this software is strictly
** forbidden.
**
** The "appropriate copyright message" mentioned in section 2c of the GPLv2
** must read: "Code from FAAD2 is copyright (c) Nero AG, www.nero.com"
**
** Commercial non-GPL licensing of this software is possible.
** For more info contact Nero AG through Mpeg4AAClicense@nero.com.
**
** $Id: aac_frame.c,v 1.3 2009-04-12 12:42:17 gmerlin Exp $
**/

/* Binary search huffman table HCB_SF */

static const uint8_t hcb_sf[][2] = {
    { /*  0 */  1, 2 },
    { /*  1 */  60, 0 },
    { /*  2 */  1, 2 },
    { /*  3 */  2, 3 },
    { /*  4 */  3, 4 },
    { /*  5 */  59, 0 },
    { /*  6 */  3, 4 },
    { /*  7 */  4, 5 },
    { /*  8 */  5, 6 },
    { /*  9 */  61, 0 },
    { /* 10 */  58, 0 },
    { /* 11 */  62, 0 },
    { /* 12 */  3, 4 },
    { /* 13 */  4, 5 },
    { /* 14 */  5, 6 },
    { /* 15 */  57, 0 },
    { /* 16 */  63, 0 },
    { /* 17 */  4, 5 },
    { /* 18 */  5, 6 },
    { /* 19 */  6, 7 },
    { /* 20 */  7, 8 },
    { /* 21 */  56, 0 },
    { /* 22 */  64, 0 },
    { /* 23 */  55, 0 },
    { /* 24 */  65, 0 },
    { /* 25 */  4, 5 },
    { /* 26 */  5, 6 },
    { /* 27 */  6, 7 },
    { /* 28 */  7, 8 },
    { /* 29 */  66, 0 },
    { /* 30 */  54, 0 },
    { /* 31 */  67, 0 },
    { /* 32 */  5, 6 },
    { /* 33 */  6, 7 },
    { /* 34 */  7, 8 },
    { /* 35 */  8, 9 },
    { /* 36 */  9, 10 },
    { /* 37 */  53, 0 },
    { /* 38 */  68, 0 },
    { /* 39 */  52, 0 },
    { /* 40 */  69, 0 },
    { /* 41 */  51, 0 },
    { /* 42 */  5, 6 },
    { /* 43 */  6, 7 },
    { /* 44 */  7, 8 },
    { /* 45 */  8, 9 },
    { /* 46 */  9, 10 },
    { /* 47 */  70, 0 },
    { /* 48 */  50, 0 },
    { /* 49 */  49, 0 },
    { /* 50 */  71, 0 },
    { /* 51 */  6, 7 },
    { /* 52 */  7, 8 },
    { /* 53 */  8, 9 },
    { /* 54 */  9, 10 },
    { /* 55 */  10, 11 },
    { /* 56 */  11, 12 },
    { /* 57 */  72, 0 },
    { /* 58 */  48, 0 },
    { /* 59 */  73, 0 },
    { /* 60 */  47, 0 },
    { /* 61 */  74, 0 },
    { /* 62 */  46, 0 },
    { /* 63 */  6, 7 },
    { /* 64 */  7, 8 },
    { /* 65 */  8, 9 },
    { /* 66 */  9, 10 },
    { /* 67 */  10, 11 },
    { /* 68 */  11, 12 },
    { /* 69 */  76, 0 },
    { /* 70 */  75, 0 },
    { /* 71 */  77, 0 },
    { /* 72 */  78, 0 },
    { /* 73 */  45, 0 },
    { /* 74 */  43, 0 },
    { /* 75 */  6, 7 },
    { /* 76 */  7, 8 },
    { /* 77 */  8, 9 },
    { /* 78 */  9, 10 },
    { /* 79 */  10, 11 },
    { /* 80 */  11, 12 },
    { /* 81 */  44, 0 },
    { /* 82 */  79, 0 },
    { /* 83 */  42, 0 },
    { /* 84 */  41, 0 },
    { /* 85 */  80, 0 },
    { /* 86 */  40, 0 },
    { /* 87 */  6, 7 },
    { /* 88 */  7, 8 },
    { /* 89 */  8, 9 },
    { /* 90 */  9, 10 },
    { /* 91 */  10, 11 },
    { /* 92 */  11, 12 },
    { /* 93 */  81, 0 },
    { /* 94 */  39, 0 },
    { /* 95 */  82, 0 },
    { /* 96 */  38, 0 },
    { /* 97 */  83, 0 },
    { /* 98 */  7, 8 },
    { /* 99 */  8, 9 },
    { /* 00 */  9, 10 },
    { /* 01 */  10, 11 },
    { /* 02 */  11, 12 },
    { /* 03 */  12, 13 },
    { /* 04 */  13, 14 },
    { /* 05 */  37, 0 },
    { /* 06 */  35, 0 },
    { /* 07 */  85, 0 },
    { /* 08 */  33, 0 },
    { /* 09 */  36, 0 },
    { /* 10 */  34, 0 },
    { /* 11 */  84, 0 },
    { /* 12 */  32, 0 },
    { /* 13 */  6, 7 },
    { /* 14 */  7, 8 },
    { /* 15 */  8, 9 },
    { /* 16 */  9, 10 },
    { /* 17 */  10, 11 },
    { /* 18 */  11, 12 },
    { /* 19 */  87, 0 },
    { /* 20 */  89, 0 },
    { /* 21 */  30, 0 },
    { /* 22 */  31, 0 },
    { /* 23 */  8, 9 },
    { /* 24 */  9, 10 },
    { /* 25 */  10, 11 },
    { /* 26 */  11, 12 },
    { /* 27 */  12, 13 },
    { /* 28 */  13, 14 },
    { /* 29 */  14, 15 },
    { /* 30 */  15, 16 },
    { /* 31 */  86, 0 },
    { /* 32 */  29, 0 },
    { /* 33 */  26, 0 },
    { /* 34 */  27, 0 },
    { /* 35 */  28, 0 },
    { /* 36 */  24, 0 },
    { /* 37 */  88, 0 },
    { /* 38 */  9, 10 },
    { /* 39 */  10, 11 },
    { /* 40 */  11, 12 },
    { /* 41 */  12, 13 },
    { /* 42 */  13, 14 },
    { /* 43 */  14, 15 },
    { /* 44 */  15, 16 },
    { /* 45 */  16, 17 },
    { /* 46 */  17, 18 },
    { /* 47 */  25, 0 },
    { /* 48 */  22, 0 },
    { /* 49 */  23, 0 },
    { /* 50 */  15, 16 },
    { /* 51 */  16, 17 },
    { /* 52 */  17, 18 },
    { /* 53 */  18, 19 },
    { /* 54 */  19, 20 },
    { /* 55 */  20, 21 },
    { /* 56 */  21, 22 },
    { /* 57 */  22, 23 },
    { /* 58 */  23, 24 },
    { /* 59 */  24, 25 },
    { /* 60 */  25, 26 },
    { /* 61 */  26, 27 },
    { /* 62 */  27, 28 },
    { /* 63 */  28, 29 },
    { /* 64 */  29, 30 },
    { /* 65 */  90, 0 },
    { /* 66 */  21, 0 },
    { /* 67 */  19, 0 },
    { /* 68 */   3, 0 },
    { /* 69 */   1, 0 },
    { /* 70 */   2, 0 },
    { /* 71 */   0, 0 },
    { /* 72 */  23, 24 },
    { /* 73 */  24, 25 },
    { /* 74 */  25, 26 },
    { /* 75 */  26, 27 },
    { /* 76 */  27, 28 },
    { /* 77 */  28, 29 },
    { /* 78 */  29, 30 },
    { /* 79 */  30, 31 },
    { /* 80 */  31, 32 },
    { /* 81 */  32, 33 },
    { /* 82 */  33, 34 },
    { /* 83 */  34, 35 },
    { /* 84 */  35, 36 },
    { /* 85 */  36, 37 },
    { /* 86 */  37, 38 },
    { /* 87 */  38, 39 },
    { /* 88 */  39, 40 },
    { /* 89 */  40, 41 },
    { /* 90 */  41, 42 },
    { /* 91 */  42, 43 },
    { /* 92 */  43, 44 },
    { /* 93 */  44, 45 },
    { /* 94 */  45, 46 },
    { /* 95 */   98, 0 },
    { /* 96 */   99, 0 },
    { /* 97 */  100, 0 },
    { /* 98 */  101, 0 },
    { /* 99 */  102, 0 },
    { /* 00 */  117, 0 },
    { /* 01 */   97, 0 },
    { /* 02 */   91, 0 },
    { /* 03 */   92, 0 },
    { /* 04 */   93, 0 },
    { /* 05 */   94, 0 },
    { /* 06 */   95, 0 },
    { /* 07 */   96, 0 },
    { /* 08 */  104, 0 },
    { /* 09 */  111, 0 },
    { /* 10 */  112, 0 },
    { /* 11 */  113, 0 },
    { /* 12 */  114, 0 },
    { /* 13 */  115, 0 },
    { /* 14 */  116, 0 },
    { /* 15 */  110, 0 },
    { /* 16 */  105, 0 },
    { /* 17 */  106, 0 },
    { /* 18 */  107, 0 },
    { /* 19 */  108, 0 },
    { /* 20 */  109, 0 },
    { /* 21 */  118, 0 },
    { /* 22 */    6, 0 },
    { /* 23 */    8, 0 },
    { /* 24 */    9, 0 },
    { /* 25 */   10, 0 },
    { /* 26 */    5, 0 },
    { /* 27 */  103, 0 },
    { /* 28 */  120, 0 },
    { /* 29 */  119, 0 },
    { /* 30 */    4, 0 },
    { /* 31 */    7, 0 },
    { /* 32 */   15, 0 },
    { /* 33 */   16, 0 },
    { /* 34 */   18, 0 },
    { /* 35 */   20, 0 },
    { /* 36 */   17, 0 },
    { /* 37 */   11, 0 },
    { /* 38 */   12, 0 },
    { /* 39 */   14, 0 },
    { /* 40 */   13, 0 }
};

/* Window grouping info */

static const uint8_t num_swb_1024_window[] =
  {
    41, 41, 47, 49, 49, 51, 47, 47, 43, 43, 43, 40
  };

static const uint16_t swb_offset_1024_96[] =
  {
    0, 4, 8, 12, 16, 20, 24, 28, 32, 36, 40, 44, 48, 52, 56,
    64, 72, 80, 88, 96, 108, 120, 132, 144, 156, 172, 188, 212, 240,
    276, 320, 384, 448, 512, 576, 640, 704, 768, 832, 896, 960, 1024
  };

static const uint16_t swb_offset_1024_64[] =
  {
    0, 4, 8, 12, 16, 20, 24, 28, 32, 36, 40, 44, 48, 52, 56,
    64, 72, 80, 88, 100, 112, 124, 140, 156, 172, 192, 216, 240, 268,
    304, 344, 384, 424, 464, 504, 544, 584, 624, 664, 704, 744, 784, 824,
    864, 904, 944, 984, 1024
  };

static const uint16_t swb_offset_1024_48[] =
  {
    0, 4, 8, 12, 16, 20, 24, 28, 32, 36, 40, 48, 56, 64, 72,
    80, 88, 96, 108, 120, 132, 144, 160, 176, 196, 216, 240, 264, 292,
    320, 352, 384, 416, 448, 480, 512, 544, 576, 608, 640, 672, 704, 736,
    768, 800, 832, 864, 896, 928, 1024
  };

static const uint16_t swb_offset_1024_32[] =
  {
    0, 4, 8, 12, 16, 20, 24, 28, 32, 36, 40, 48, 56, 64, 72,
    80, 88, 96, 108, 120, 132, 144, 160, 176, 196, 216, 240, 264, 292,
    320, 352, 384, 416, 448, 480, 512, 544, 576, 608, 640, 672, 704, 736,
    768, 800, 832, 864, 896, 928, 960, 992, 1024
  };

static const uint16_t swb_offset_1024_24[] =
  {
    0, 4, 8, 12, 16, 20, 24, 28, 32, 36, 40, 44, 52, 60, 68,
    76, 84, 92, 100, 108, 116, 124, 136, 148, 160, 172, 188, 204, 220,
    240, 260, 284, 308, 336, 364, 396, 432, 468, 508, 552, 600, 652, 704,
    768, 832, 896, 960, 1024
  };

static const uint16_t swb_offset_1024_16[] =
  {
    0, 8, 16, 24, 32, 40, 48, 56, 64, 72, 80, 88, 100, 112, 124,
    136, 148, 160, 172, 184, 196, 212, 228, 244, 260, 280, 300, 320, 344,
    368, 396, 424, 456, 492, 532, 572, 616, 664, 716, 772, 832, 896, 960, 1024
  };

static const uint16_t swb_offset_1024_8[] =
  {
    0, 12, 24, 36, 48, 60, 72, 84, 96, 108, 120, 132, 144, 156, 172,
    188, 204, 220, 236, 252, 268, 288, 308, 328, 348, 372, 396, 420, 448,
    476, 508, 544, 580, 620, 664, 712, 764, 820, 880, 944, 1024
  };


static const uint16_t *swb_offset_1024_window[] =
  {
    swb_offset_1024_96,      /* 96000 */
    swb_offset_1024_96,      /* 88200 */
    swb_offset_1024_64,      /* 64000 */
    swb_offset_1024_48,      /* 48000 */
    swb_offset_1024_48,      /* 44100 */
    swb_offset_1024_32,      /* 32000 */
    swb_offset_1024_24,      /* 24000 */
    swb_offset_1024_24,      /* 22050 */
    swb_offset_1024_16,      /* 16000 */
    swb_offset_1024_16,      /* 12000 */
    swb_offset_1024_16,      /* 11025 */
    swb_offset_1024_8        /* 8000  */
  };

static const uint8_t num_swb_128_window[] =
  {
    12, 12, 12, 14, 14, 14, 15, 15, 15, 15, 15, 15
  };

static const uint16_t swb_offset_128_96[] =
  {
    0, 4, 8, 12, 16, 20, 24, 32, 40, 48, 64, 92, 128
  };

static const uint16_t swb_offset_128_64[] =
  {
    0, 4, 8, 12, 16, 20, 24, 32, 40, 48, 64, 92, 128
  };

static const uint16_t swb_offset_128_48[] =
  {
    0, 4, 8, 12, 16, 20, 28, 36, 44, 56, 68, 80, 96, 112, 128
  };

static const uint16_t swb_offset_128_24[] =
  {
    0, 4, 8, 12, 16, 20, 24, 28, 36, 44, 52, 64, 76, 92, 108, 128
  };

static const uint16_t swb_offset_128_16[] =
  {
    0, 4, 8, 12, 16, 20, 24, 28, 32, 40, 48, 60, 72, 88, 108, 128
  };

static const uint16_t swb_offset_128_8[] =
  {
    0, 4, 8, 12, 16, 20, 24, 28, 36, 44, 52, 60, 72, 88, 108, 128
  };

static const  uint16_t *swb_offset_128_window[] =
  {
    swb_offset_128_96,       /* 96000 */
    swb_offset_128_96,       /* 88200 */
    swb_offset_128_64,       /* 64000 */
    swb_offset_128_48,       /* 48000 */
    swb_offset_128_48,       /* 44100 */
    swb_offset_128_48,       /* 32000 */
    swb_offset_128_24,       /* 24000 */
    swb_offset_128_24,       /* 22050 */
    swb_offset_128_16,       /* 16000 */
    swb_offset_128_16,       /* 12000 */
    swb_offset_128_16,       /* 11025 */
    swb_offset_128_8         /* 8000  */
  };

#define bit_set(A, B) ((A) & (1<<(B)))

static void get_window_grouping_info(bgav_aac_header_t * h, aac_state_t * st)
  {
  int i, g;
  int sf_index = h->samplerate_index;
  int num_swb;
  int window_group_length[8];
  
  switch(st->window_sequence)
    {
    case ONLY_LONG_SEQUENCE:
    case LONG_START_SEQUENCE:
    case LONG_STOP_SEQUENCE:
      st->num_windows = 1;
      st->num_window_groups = 1;
      
      num_swb = num_swb_1024_window[sf_index];
      for(i = 0; i < num_swb; i++)
        {
        st->sect_sfb_offset[0][i] = swb_offset_1024_window[sf_index][i];
        }
      st->sect_sfb_offset[0][num_swb] = 1024;
      
      break;
    case EIGHT_SHORT_SEQUENCE:
      st->num_windows = 8;
      st->num_window_groups = 1;
      num_swb = num_swb_128_window[sf_index];
      window_group_length[st->num_window_groups-1] = 1;

      for(i = 0; i < st->num_windows-1; i++)
        {
        if(bit_set(st->scale_factor_grouping, 6-i) == 0)
          {
          st->num_window_groups += 1;
          window_group_length[st->num_window_groups-1] = 1;
          }
        else
          {
          window_group_length[st->num_window_groups-1] += 1;
          }
        }
      
      /* preparation of sect_sfb_offset for short blocks */
      for(g = 0; g < st->num_window_groups; g++)
        {
        uint16_t width;
        uint8_t sect_sfb = 0;
        uint16_t offset = 0;
        
        for(i = 0; i < num_swb; i++)
          {
          if(i+1 == num_swb)
            {
            width = (1024/8) - swb_offset_128_window[sf_index][i];
            }
          else
            {
            width = swb_offset_128_window[sf_index][i+1] -
              swb_offset_128_window[sf_index][i];
            }
          width *= window_group_length[g];
          st->sect_sfb_offset[g][sect_sfb++] = offset;
          offset += width;
          }
        st->sect_sfb_offset[g][sect_sfb] = offset;
        }
      break;
    default:
      break;
    }
  
  }
