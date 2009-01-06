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

#include <stdlib.h>

#include <avdec_private.h>
#include <mpv_header.h>

/* golomb parsing */

static int get_golomb_ue(bgav_bitstream_t * b)
  {
  int bits, num = 0;
  while(1)
    {
    bgav_bitstream_get(b, &bits, 1);
    if(bits)
      break;
    else
      num++;
    }
  /* The variable codeNum is then assigned as follows:
     codeNum = 2^leadingZeroBits - 1 + read_bits( leadingZeroBits ) */
  
  bgav_bitstream_get(b, &bits, num);
  return (1 << num) - 1 + bits;
  }

static int get_golomb_se(bgav_bitstream_t * b)
  {
  int ret = get_golomb_ue(b);
  if(ret & 1)
    return ret>>1;
  else
    return -(ret>>1);
  }


/* */

const uint8_t *
bgav_h264_find_nal_start(const uint8_t * buffer, int len)
  {
  const uint8_t * ptr;
  
  ptr = bgav_mpv_find_startcode(buffer, buffer + len);

  if(!ptr)
    return NULL;
  
  /* Get zero bytes before actual startcode */
  while((ptr > buffer) && (*(ptr-1) == 0x00))
    ptr--;
  
  return ptr;
  }

int bgav_h264_get_nal_size(const uint8_t * buffer, int len)
  {
  const uint8_t * end;

  if(len < 4)
    return -1;
  
  end = bgav_h264_find_nal_start(buffer + 4, len - 4);
  if(end)
    return (end - buffer);
  else
    return -1;
  }

int bgav_h264_decode_nal_header(const uint8_t * in_buffer, int len,
                                bgav_h264_nal_header_t * header)
  {
  if(len < 4)
    return 0;
  header->ref_idc       = in_buffer[3] >> 5;
  header->unit_type = in_buffer[3] & 0x1f;
  return 4;
  }

int bgav_h264_decode_nal_rpsp(const uint8_t * in_buffer, int len,
                              uint8_t * ret)
  {
  const uint8_t * src = in_buffer;
  const uint8_t * end = in_buffer + len;
  uint8_t * dst = ret;
  //  int i;

  while(src < end)
    {
    /* 1 : 2^22 */
    if(BGAV_UNLIKELY((src < end - 3) &&
                     (src[0] == 0x00) &&
                     (src[1] == 0x00) &&
                     (src[2] == 0x03)))
      {
      dst[0] = src[0];
      dst[1] = src[1];
      
      dst += 2;
      src += 3;
      }
    else
      {
      dst[0] = src[0];
      src++;
      dst++;
      }
    }
  return dst - ret;
  }

/* SPS stuff */

static void vui_parse(bgav_bitstream_t * b, bgav_h264_vui_t * vui)
  {
  bgav_bitstream_get(&b, &vui->aspect_ratio_info_present_flag, 1);
  if(vui->aspect_ratio_info_present_flag)
    {
    bgav_bitstream_get(&b, &vui->aspect_ratio_idc, 8);
    if(vui->aspect_ratio_idc == 255) // Extended_SAR
      {
      bgav_bitstream_get(&b, &vui->sar_width, 16);
      bgav_bitstream_get(&b, &vui->sar_height, 16);
      }
    }

  bgav_bitstream_get(&b, &vui->overscan_info_present_flag, 1);
  if(vui->overscan_info_present_flag)
    bgav_bitstream_get(&b, &vui->overscan_appropriate_flag, 1);

  bgav_bitstream_get(&b, &vui->video_signal_type_present_flag, 1);
  if(vui->video_signal_type_present_flag)
    {
    bgav_bitstream_get(&b, &vui->video_format, 3);
    bgav_bitstream_get(&b, &vui->video_full_range_flag, 1);
    bgav_bitstream_get(&b, &vui->colour_description_present_flag, 1);
    if(vui->colour_description_present_flag)
      {
      bgav_bitstream_get(&b, &vui->colour_primaries, 8);
      bgav_bitstream_get(&b, &vui->transfer_characteristics, 8);
      bgav_bitstream_get(&b, &vui->matrix_coefficients, 8);
      }
    }

  bgav_bitstream_get(&b, &vui->chroma_loc_info_present_flag, 1);
  if(vui->chroma_loc_info_present_flag)
    {
    vui->chroma_sample_loc_type_top_field    = get_golomb_ue(&b);
    vui->chroma_sample_loc_type_bottom_field = get_golomb_ue(&b);
    }

  bgav_bitstream_get(&b, &vui->timing_info_present_flag, 1);
  if(vui->timing_info_present_flag)
    {
    bgav_bitstream_get(&b, &vui->num_units_in_tick, 32);
    bgav_bitstream_get(&b, &vui->time_scale, 32);
    bgav_bitstream_get(&b, &vui->fixed_frame_rate_flag, 1);
    }
  }

int bgav_h264_sps_parse(const bgav_options_t * opt,
                        bgav_h264_sps_t * sps,
                        const uint8_t * buffer, int len)
  {
  int i;
  bgav_bitstream_t b;
  int dummy;
  bgav_bitstream_init(&b, buffer, len);

  bgav_bitstream_get(&b, &sps->profile_idc, 8);
  bgav_bitstream_get(&b, &sps->constraint_set0_flag, 1);
  bgav_bitstream_get(&b, &sps->constraint_set1_flag, 1);
  bgav_bitstream_get(&b, &sps->constraint_set2_flag, 1);
  
  bgav_bitstream_get(&b, &dummy, 5); /* reserved_zero_5bits */
  bgav_bitstream_get(&b, &sps->level_idc, 8); /* level_idc */

  sps->seq_parameter_set_id      = get_golomb_ue(&b);
  sps->log2_max_frame_num_minus4 = get_golomb_ue(&b);
  sps->pic_order_cnt_type        = get_golomb_ue(&b);

  if(!sps->pic_order_cnt_type)
    {
    sps->log2_max_pic_order_cnt_lsb_minus4 = get_golomb_ue(&b);
    }
  else if(sps->pic_order_cnt_type == 1)
    {
    bgav_bitstream_get(&b, &sps->delta_pic_order_always_zero_flag, 1);

    sps->offset_for_non_ref_pic = get_golomb_se(&b);  
    sps->offset_for_top_to_bottom_field = get_golomb_se(&b); 
    sps->num_ref_frames_in_pic_order_cnt_cycle = get_golomb_ue(&b);

    sps->offset_for_ref_frame =
      malloc(sizeof(*sps->offset_for_ref_frame) *
             sps->num_ref_frames_in_pic_order_cnt_cycle);
    for(i = 0; i < sps->num_ref_frames_in_pic_order_cnt_cycle; i++)
      {
      sps->offset_for_ref_frame[i] = get_golomb_se(&b);
      }
    }
  sps->num_ref_frames = get_golomb_ue(&b);
  bgav_bitstream_get(&b, &sps->gaps_in_frame_num_value_allowed_flag, 1);

  sps->pic_width_in_mbs_minus1 = get_golomb_ue(&b);
  sps->pic_height_in_map_units_minus1 = get_golomb_ue(&b);

  bgav_bitstream_get(&b, &sps->frame_mbs_only_flag, 1);

  if(!sps->frame_mbs_only_flag)
    bgav_bitstream_get(&b, &sps->mb_adaptive_frame_field_flag, 1);

  bgav_bitstream_get(&b, &sps->direct_8x8_inference_flag, 1);
  bgav_bitstream_get(&b, &sps->frame_cropping_flag, 1);
  if(sps->frame_cropping_flag)
    {
    sps->frame_crop_left_offset   = get_golomb_ue(&b);
    sps->frame_crop_right_offset  = get_golomb_ue(&b);
    sps->frame_crop_top_offset    = get_golomb_ue(&b);
    sps->frame_crop_bottom_offset = get_golomb_ue(&b);
    }
  bgav_bitstream_get(&b, &sps->vui_parameters_present_flag, 1);

  if(sps->vui_parameters_present_flag)
    vui_parse(&b, &sps->vui);
  
  return 1;
  }

void bgav_h264_sps_free(bgav_h264_sps_t * sps)
  {
  if(sps->offset_for_ref_frame)
    free(sps->offset_for_ref_frame);
  }
