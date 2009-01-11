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
#include <h264_header.h>

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
  const uint8_t * pos = in_buffer;
  while(*pos == 0x00)
    pos++;
  pos++; // 0x01
  header->ref_idc       = pos[0] >> 5;
  header->unit_type = pos[0] & 0x1f;
  return pos - in_buffer + 1;
  }

int bgav_h264_decode_nal_rbsp(const uint8_t * in_buffer, int len,
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

static void get_hrd_parameters(bgav_bitstream_t * b,
                               bgav_h264_vui_t * vui)
  {
  int dummy, i;
  int cpb_cnt_minus1 = get_golomb_ue(b);

  bgav_bitstream_get(b, &dummy, 4); // bit_rate_scale
  bgav_bitstream_get(b, &dummy, 4); // cpb_size_scale
  
  for(i = 0; i <= cpb_cnt_minus1; i++ )
    {
    get_golomb_ue(b); // bit_rate_value_minus1[ SchedSelIdx ]
    get_golomb_ue(b); // cpb_size_value_minus1[ SchedSelIdx ]
    bgav_bitstream_get(b, &dummy, 1); // cbr_flag[ SchedSelIdx ]
    }
  bgav_bitstream_get(b, &dummy, 5); // initial_cpb_removal_delay_length_minus1
  bgav_bitstream_get(b, &vui->cpb_removal_delay_length_minus1, 5); 
  bgav_bitstream_get(b, &vui->dpb_output_delay_length_minus1, 5); 
  bgav_bitstream_get(b, &dummy, 5); // time_offset_length
  }

static void vui_parse(bgav_bitstream_t * b, bgav_h264_vui_t * vui)
  {
  bgav_bitstream_get(b, &vui->aspect_ratio_info_present_flag, 1);
  if(vui->aspect_ratio_info_present_flag)
    {
    bgav_bitstream_get(b, &vui->aspect_ratio_idc, 8);
    if(vui->aspect_ratio_idc == 255) // Extended_SAR
      {
      bgav_bitstream_get(b, &vui->sar_width, 16);
      bgav_bitstream_get(b, &vui->sar_height, 16);
      }
    }

  bgav_bitstream_get(b, &vui->overscan_info_present_flag, 1);
  if(vui->overscan_info_present_flag)
    bgav_bitstream_get(b, &vui->overscan_appropriate_flag, 1);

  bgav_bitstream_get(b, &vui->video_signal_type_present_flag, 1);
  if(vui->video_signal_type_present_flag)
    {
    bgav_bitstream_get(b, &vui->video_format, 3);
    bgav_bitstream_get(b, &vui->video_full_range_flag, 1);
    bgav_bitstream_get(b, &vui->colour_description_present_flag, 1);
    if(vui->colour_description_present_flag)
      {
      bgav_bitstream_get(b, &vui->colour_primaries, 8);
      bgav_bitstream_get(b, &vui->transfer_characteristics, 8);
      bgav_bitstream_get(b, &vui->matrix_coefficients, 8);
      }
    }

  bgav_bitstream_get(b, &vui->chroma_loc_info_present_flag, 1);
  if(vui->chroma_loc_info_present_flag)
    {
    vui->chroma_sample_loc_type_top_field    = get_golomb_ue(b);
    vui->chroma_sample_loc_type_bottom_field = get_golomb_ue(b);
    }

  bgav_bitstream_get(b, &vui->timing_info_present_flag, 1);
  if(vui->timing_info_present_flag)
    {
    bgav_bitstream_get(b, &vui->num_units_in_tick, 32);
    bgav_bitstream_get(b, &vui->time_scale, 32);
    bgav_bitstream_get(b, &vui->fixed_frame_rate_flag, 1);
    }

  bgav_bitstream_get(b, &vui->nal_hrd_parameters_present_flag, 1);
  if(vui->nal_hrd_parameters_present_flag)
    get_hrd_parameters(b, vui);

  bgav_bitstream_get(b, &vui->vcl_hrd_parameters_present_flag, 1);
  if(vui->vcl_hrd_parameters_present_flag)
    get_hrd_parameters(b, vui);

  if(vui->nal_hrd_parameters_present_flag || vui->vcl_hrd_parameters_present_flag)
    bgav_bitstream_get(b, &vui->low_delay_hrd_flag, 1);

  bgav_bitstream_get(b, &vui->pic_struct_present_flag, 1);
  
  }

static void vui_dump(bgav_h264_vui_t * vui)
  {
  bgav_dprintf("    aspect_ratio_info_present_flag:    %d\n", vui->aspect_ratio_info_present_flag);
  if(vui->aspect_ratio_info_present_flag )
    {
    bgav_dprintf("    aspect_ratio_idc:                  %d\n", vui->aspect_ratio_idc );
    if( vui->aspect_ratio_idc == 255 )
      {
      bgav_dprintf("    sar_width:                         %d\n", vui->sar_width );
      bgav_dprintf("    sar_height:                        %d\n", vui->sar_height );
      }
    }
  bgav_dprintf("    overscan_info_present_flag:        %d\n", vui->overscan_info_present_flag );
  if( vui->overscan_info_present_flag )
    bgav_dprintf("    overscan_appropriate_flag:       %d\n", vui->overscan_appropriate_flag );

  bgav_dprintf("    video_signal_type_present_flag:    %d\n", vui->video_signal_type_present_flag );
  if( vui->video_signal_type_present_flag )
    {
    bgav_dprintf("    video_format:                      %d\n", vui->video_format );
    bgav_dprintf("    video_full_range_flag:             %d\n", vui->video_full_range_flag );
    bgav_dprintf("    colour_description_present_flag:   %d\n", vui->colour_description_present_flag );
    if( vui->colour_description_present_flag )
      {
      bgav_dprintf("    colour_primaries:                  %d\n", vui->colour_primaries );
      bgav_dprintf("    transfer_characteristics:          %d\n", vui->transfer_characteristics );
      bgav_dprintf("    matrix_coefficients:               %d\n", vui->matrix_coefficients );
      }
    }
  bgav_dprintf("    chroma_loc_info_present_flag:        %d\n", vui->chroma_loc_info_present_flag );
  if( vui->chroma_loc_info_present_flag )
    {
    bgav_dprintf("    chroma_sample_loc_type_top_field:    %d\n", vui->chroma_sample_loc_type_top_field  );
    bgav_dprintf("    chroma_sample_loc_type_bottom_field: %d\n", vui->chroma_sample_loc_type_bottom_field );
    }
  bgav_dprintf("    timing_info_present_flag:            %d\n", vui->timing_info_present_flag );
  if( vui->timing_info_present_flag )
    {
    bgav_dprintf("    num_units_in_tick:                 %d\n", vui->num_units_in_tick );
    bgav_dprintf("    time_scale:                        %d\n", vui->time_scale );
    bgav_dprintf("    fixed_frame_rate_flag:             %d\n", vui->fixed_frame_rate_flag );
    }
  bgav_dprintf("    nal_hrd_present_flag:                %d\n", vui->nal_hrd_parameters_present_flag );
  bgav_dprintf("    vcl_hrd_present_flag:                %d\n", vui->vcl_hrd_parameters_present_flag );

  if(vui->nal_hrd_parameters_present_flag || vui->vcl_hrd_parameters_present_flag)
    {
    bgav_dprintf("    low_delay_hrd_flag:                  %d\n", vui->low_delay_hrd_flag);
    }
  bgav_dprintf("    pic_struct_present_flag:             %d\n", vui->pic_struct_present_flag );
  }

static void skip_scaling_list(bgav_bitstream_t * b, int num)
  {
  int i;
  for(i = 0; i < num; i++)
    get_golomb_se(b);
  }

int bgav_h264_sps_parse(const bgav_options_t * opt,
                        bgav_h264_sps_t * sps,
                        const uint8_t * buffer, int len)
  {
  int i;
  bgav_bitstream_t b;
  int dummy;
  //  fprintf(stderr, "Parsing SPS %d bytes\n", len);
  // bgav_hexdump(buffer, len, 16);

  bgav_bitstream_init(&b, buffer, len);

  bgav_bitstream_get(&b, &sps->profile_idc, 8);
  bgav_bitstream_get(&b, &sps->constraint_set0_flag, 1);
  bgav_bitstream_get(&b, &sps->constraint_set1_flag, 1);
  bgav_bitstream_get(&b, &sps->constraint_set2_flag, 1);
  bgav_bitstream_get(&b, &sps->constraint_set3_flag, 1);
  
  bgav_bitstream_get(&b, &dummy, 4); /* reserved_zero_4bits */
  bgav_bitstream_get(&b, &sps->level_idc, 8); /* level_idc */

  sps->seq_parameter_set_id      = get_golomb_ue(&b);

  /* ffmpeg has just (sps->profile_idc >= 100) */
  if(sps->profile_idc == 100 ||
     sps->profile_idc == 110 ||
     sps->profile_idc == 122 ||
     sps->profile_idc == 244 ||
     sps->profile_idc == 44 ||
     sps->profile_idc == 83 ||
     sps->profile_idc == 86 ) 
    {
    sps->chroma_format_idc = get_golomb_ue(&b);
    if(sps->chroma_format_idc == 3)
      bgav_bitstream_get(&b, &sps->separate_colour_plane_flag, 1);
    sps->bit_depth_luma_minus8 = get_golomb_ue(&b);
    sps->bit_depth_chroma_minus8 = get_golomb_ue(&b);

    bgav_bitstream_get(&b, &sps->qpprime_y_zero_transform_bypass_flag, 1);
    bgav_bitstream_get(&b, &sps->seq_scaling_matrix_present_flag, 1);
    for(i = 0; i < ((sps->chroma_format_idc != 3 ) ? 8 : 12 ); i++)
      {
      bgav_bitstream_get(&b, &dummy, 1);
      if(dummy)
        {
        if(i < 6)
          skip_scaling_list(&b, 16);
        else
          skip_scaling_list(&b, 64);
        }
      }
    }
  
  sps->log2_max_frame_num_minus4 = get_golomb_ue(&b);
  sps->pic_order_cnt_type        = get_golomb_ue(&b);

  if(!sps->pic_order_cnt_type)
    sps->log2_max_pic_order_cnt_lsb_minus4 = get_golomb_ue(&b);
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

void bgav_h264_sps_dump(bgav_h264_sps_t * sps)
  {
  int i;
  bgav_dprintf("SPS:\n");
  bgav_dprintf("  profile_idc:                             %d\n", sps->profile_idc);
  bgav_dprintf("  constraint_set0_flag:                    %d\n", sps->constraint_set0_flag);
  bgav_dprintf("  constraint_set1_flag:                    %d\n", sps->constraint_set1_flag);
  bgav_dprintf("  constraint_set2_flag:                    %d\n", sps->constraint_set2_flag);
  bgav_dprintf("  level_idc:                               %d\n", sps->level_idc);
  bgav_dprintf("  seq_parameter_set_id:                    %d\n", sps->seq_parameter_set_id);

  if(sps->profile_idc == 100 ||
     sps->profile_idc == 110 ||
     sps->profile_idc == 122 ||
     sps->profile_idc == 244 ||
     sps->profile_idc == 44 ||
     sps->profile_idc == 83 ||
     sps->profile_idc == 86 ) 
    {
    bgav_dprintf("  chroma_format_idc:                       %d\n", sps->chroma_format_idc);
    if(sps->chroma_format_idc == 3)
      bgav_dprintf("  separate_colour_plane_flag:              %d\n", sps->separate_colour_plane_flag);

    bgav_dprintf("  bit_depth_luma_minus8:                   %d\n", sps->bit_depth_luma_minus8);
    bgav_dprintf("  bit_depth_chroma_minus8:                 %d\n", sps->bit_depth_chroma_minus8);
    bgav_dprintf("  qpprime_y_zero_transform_bypass_flag:    %d\n", sps->qpprime_y_zero_transform_bypass_flag);
    bgav_dprintf("  seq_scaling_matrix_present_flag:         %d\n", sps->seq_scaling_matrix_present_flag);
    }
  
  bgav_dprintf("  log2_max_frame_num_minus4:               %d\n", sps->log2_max_frame_num_minus4);
  bgav_dprintf("  pic_order_cnt_type:                      %d\n", sps->pic_order_cnt_type);

  if( sps->pic_order_cnt_type == 0 )
    bgav_dprintf("  log2_max_pic_order_cnt_lsb_minus4:       %d\n", sps->log2_max_pic_order_cnt_lsb_minus4);
  else if(sps->pic_order_cnt_type == 1)
    {
    bgav_dprintf("  delta_pic_order_always_zero_flag:      %d\n", sps->delta_pic_order_always_zero_flag);
    bgav_dprintf("  offset_for_non_ref_pic:                %d\n", sps->offset_for_non_ref_pic);
    bgav_dprintf("  offset_for_top_to_bottom_field:        %d\n", sps->offset_for_top_to_bottom_field);
    bgav_dprintf("  num_ref_frames_in_pic_order_cnt_cycle: %d\n", sps->num_ref_frames_in_pic_order_cnt_cycle);

    for(i = 0; i < sps->num_ref_frames_in_pic_order_cnt_cycle; i++)
      {
      bgav_dprintf("  offset_for_ref_frame[%d]:              %d\n", i, sps->offset_for_ref_frame[i]);
      }
    }

  bgav_dprintf("  num_ref_frames:                          %d\n", sps->num_ref_frames);
  bgav_dprintf("  gaps_in_frame_num_value_allowed_flag:    %d\n", sps->gaps_in_frame_num_value_allowed_flag);
  bgav_dprintf("  pic_width_in_mbs_minus1:                 %d\n", sps->pic_width_in_mbs_minus1);
  bgav_dprintf("  pic_height_in_map_units_minus1:          %d\n", sps->pic_height_in_map_units_minus1);
  bgav_dprintf("  frame_mbs_only_flag:                     %d\n", sps->frame_mbs_only_flag);
  
  if( !sps->frame_mbs_only_flag )
    bgav_dprintf("  mb_adaptive_frame_field_flag:            %d\n", sps->mb_adaptive_frame_field_flag);
  bgav_dprintf("  direct_8x8_inference_flag:               %d\n", sps->direct_8x8_inference_flag);
  bgav_dprintf("  frame_cropping_flag:                     %d\n", sps->frame_cropping_flag);
  if( sps->frame_cropping_flag )
    {
    bgav_dprintf("  frame_crop_left_offset:                  %d\n", sps->frame_crop_left_offset);
    bgav_dprintf("  frame_crop_right_offset:                 %d\n", sps->frame_crop_right_offset);
    bgav_dprintf("  frame_crop_top_offset:                   %d\n", sps->frame_crop_top_offset);
    bgav_dprintf("  frame_crop_bottom_offset:                %d\n", sps->frame_crop_bottom_offset);
    }
  bgav_dprintf("  vui_parameters_present_flag:             %d\n", sps->vui_parameters_present_flag);

  if(sps->vui_parameters_present_flag)
    vui_dump(&sps->vui);
  
  }

int bgav_h264_decode_sei_message_header(const uint8_t * data, int len,
                                        int * sei_type, int * sei_size)
  {
  const uint8_t * ptr = data;
  *sei_type = 0;
  *sei_size = 0;

  while(*ptr == 0xff)
    {
    *sei_type += 0xff;
    ptr++;
    }
  *sei_type += *ptr;
  ptr++;

  while(*ptr == 0xff)
    {
    *sei_size += 0xff;
    ptr++;
    }
  *sei_size += *ptr;
  ptr++;

  return ptr - data;
  
  }

int bgav_h264_decode_sei_pic_timing(const uint8_t * data, int len,
                                    bgav_h264_sps_t * sps,
                                    int * pic_struct)
  {
  int dummy;
  bgav_bitstream_t b;
  *pic_struct = -1;
  bgav_bitstream_init(&b, data, len);
  
  if(sps->vui.nal_hrd_parameters_present_flag ||
     sps->vui.vcl_hrd_parameters_present_flag)
    {
    bgav_bitstream_get(&b, &dummy, sps->vui.cpb_removal_delay_length_minus1+1);
    bgav_bitstream_get(&b, &dummy, sps->vui.dpb_output_delay_length_minus1+1);
    }
  if(sps->vui.pic_struct_present_flag)
    bgav_bitstream_get(&b, pic_struct, 4);
  return 1;
  }
