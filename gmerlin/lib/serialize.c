/*****************************************************************
 * gmerlin - a general purpose multimedia framework and applications
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

#include <gavl/gavl.h>
#include <gmerlin/serialize.h>

#include <stdlib.h>
#include <string.h>


#define SERIALIZE_VERSION 0

/* Get/Set routines for structures */

static inline const uint8_t * get_8(const uint8_t * data, uint32_t * val)
  {
  *val = *data;
  data++;
  return data;
  }

static inline const uint8_t * get_16(const uint8_t * data, uint32_t * val)
  {
  *val = ((data[0] << 8) | data[1]);
  data+=2;
  return data;
  }

static inline const uint8_t * get_32(const uint8_t * data, uint32_t * val)
  {
  *val = ((data[0] << 24) | (data[1] << 16) | (data[2] << 8) | data[3]);
  data+=4;
  return data;
  }

static inline const uint8_t * get_64(const uint8_t * data, uint64_t * val)
  {
  *val  = data[0]; *val <<= 8;
  *val |= data[1]; *val <<= 8;
  *val |= data[2]; *val <<= 8;
  *val |= data[3]; *val <<= 8;
  *val |= data[4]; *val <<= 8;
  *val |= data[5]; *val <<= 8;
  *val |= data[6]; *val <<= 8;
  *val |= data[7];
  data+=8;
  return data;
  }

static inline const uint8_t * get_str(const uint8_t * data, char ** val)
  {
  uint32_t len;
  const uint8_t * ret;

  ret = get_32(data, &len);

  if(len)
    {
    *val = malloc(len+1);
    memcpy(*val, ret, len);
    (*val)[len] = '\0';
    }
  return ret + len;
  }

static inline uint8_t * set_8(uint8_t * data, uint8_t val)
  {
  *data = val;
  data++;
  return data;
  }

static inline uint8_t * set_16(uint8_t * data, uint16_t val)
  {
  data[0] = (val & 0xff00) >> 8;
  data[1] = (val & 0xff);
  data+=2;
  return data;
  }

static inline uint8_t * set_32(uint8_t * data, uint32_t val)
  {
  data[0] = (val & 0xff000000) >> 24;
  data[1] = (val & 0xff0000) >> 16;
  data[2] = (val & 0xff00) >> 8;
  data[3] = (val & 0xff);
  data+=4;
  return data;
  }

static inline uint8_t * set_64(uint8_t * data, uint64_t val)
  {
  data[0] = (val & 0xff00000000000000LL) >> 56;
  data[1] = (val & 0x00ff000000000000LL) >> 48;
  data[2] = (val & 0x0000ff0000000000LL) >> 40;
  data[3] = (val & 0x000000ff00000000LL) >> 32;
  data[4] = (val & 0x00000000ff000000LL) >> 24;
  data[5] = (val & 0x0000000000ff0000LL) >> 16;
  data[6] = (val & 0x000000000000ff00LL) >> 8;
  data[7] = (val & 0x00000000000000ffLL);
  data+=8;
  return data;
  }

static int str_len(const char * str)
  {
  int ret = 4;
  if(str)
    ret += strlen(str);
  return ret;
  }

static inline uint8_t * set_str(uint8_t * data, const char * val)
  {
  uint32_t len;
  if(val)
    len = strlen(val);
  else
    len = 0;
  
  data = set_32(data, len);
  if(len)
    memcpy(data, val, len);
  return data + len;
  }


int
bg_serialize_audio_format(const gavl_audio_format_t * format,
                          uint8_t * pos, int len)
  {
  int i;
  int len_needed = 24 + 8 * format->num_channels;
  
  if(len_needed > len)
    return len_needed;

  pos = set_32(pos, format->samples_per_frame);
  pos = set_32(pos, format->samplerate);
  pos = set_32(pos, format->num_channels);
  pos = set_8(pos, format->sample_format);
  pos = set_8(pos, format->interleave_mode);

  pos = set_32(pos, (int32_t)(format->center_level*1.0e6));
  pos = set_32(pos, (int32_t)(format->rear_level*1.0e6));
  
  for(i = 0; i < format->num_channels; i++)
    pos = set_8(pos, format->channel_locations[i]);
  
  return len_needed;
  }

int
bg_deserialize_audio_format(gavl_audio_format_t * format,
                            const uint8_t * pos, int len, int * big_endian)
  {
  int i;
  uint32_t tmp;

  pos = get_32(pos, &tmp);  format->samples_per_frame = tmp;
  pos = get_32(pos, &tmp);  format->samplerate = tmp;
  pos = get_32(pos, &tmp);  format->num_channels = tmp;
  pos = get_8(pos,  &tmp);  format->sample_format = tmp;
  pos = get_8(pos,  &tmp);  format->interleave_mode = tmp;
  pos = get_32(pos, &tmp);  format->center_level = (float)(tmp)*1.0e-6;
  pos = get_32(pos, &tmp);  format->rear_level = (float)(tmp)*1.0e-6;
  
  for(i = 0; i < format->num_channels; i++)
    {
    pos = get_8(pos, &tmp);
    format->channel_locations[i] = tmp;
    }
  return 1;
  }

int
bg_serialize_video_format(const gavl_video_format_t * format,
                          uint8_t * pos, int len)
  {
  int len_needed = 47;
  if(len < len_needed)
    return len_needed;
  pos = set_32(pos, format->frame_width);
  pos = set_32(pos, format->frame_height);
  pos = set_32(pos, format->image_width);
  pos = set_32(pos, format->image_height);
  pos = set_32(pos, format->pixel_width);
  pos = set_32(pos, format->pixel_height);
  pos = set_32(pos, format->pixelformat);
  pos = set_32(pos, format->timescale);
  pos = set_32(pos, format->frame_duration);
  pos = set_8(pos, format->framerate_mode);
  pos = set_8(pos, format->interlace_mode);
  pos = set_8(pos, format->chroma_placement);
  pos = set_32(pos, format->timecode_format.int_framerate);
  pos = set_32(pos, format->timecode_format.flags);
  
  return len_needed;
  }


int
bg_deserialize_video_format(gavl_video_format_t * format,
                            const uint8_t * pos, int len, int * big_endian)
  {
  uint32_t tmp;
  pos = get_32(pos, &(tmp)); format->frame_width                   = tmp;
  pos = get_32(pos, &(tmp)); format->frame_height                  = tmp;
  pos = get_32(pos, &(tmp)); format->image_width                   = tmp;
  pos = get_32(pos, &(tmp)); format->image_height                  = tmp;
  pos = get_32(pos, &(tmp)); format->pixel_width                   = tmp;
  pos = get_32(pos, &(tmp)); format->pixel_height                  = tmp;
  pos = get_32(pos, &(tmp)); format->pixelformat                   = tmp;
  pos = get_32(pos, &(tmp)); format->timescale                     = tmp;
  pos = get_32(pos, &(tmp)); format->frame_duration                = tmp;
  pos = get_8(pos,  &(tmp)); format->framerate_mode                = tmp;
  pos = get_8(pos,  &(tmp)); format->interlace_mode                = tmp;
  pos = get_8(pos,  &(tmp)); format->chroma_placement              = tmp;
  pos = get_32(pos, &(tmp)); format->timecode_format.int_framerate = tmp;
  pos = get_32(pos, &(tmp)); format->timecode_format.flags         = tmp;
  return 1;
  }

/* Frames */

int
bg_serialize_audio_frame_header(const gavl_audio_format_t * format,
                                const gavl_audio_frame_t * frame,
                                uint8_t * pos, int len)
  {
  int len_needed =
    8 + /* timestamp */
    4; /* Duration */
  if(len_needed > len)
    return len_needed;
  
  pos = set_64(pos, frame->timestamp);
  pos = set_32(pos, frame->valid_samples);
  return len_needed;
  }

int
bg_deserialize_audio_frame_header(const gavl_audio_format_t * format,
                                  gavl_audio_frame_t * frame,
                                  const uint8_t * pos, int len)
  {
  pos = get_64(pos, (uint64_t*)(&frame->timestamp));
  pos = get_32(pos, (uint32_t*)(&frame->valid_samples));
  return 1;
  }


int
bg_serialize_video_frame_header(const gavl_video_format_t * format,
                                const gavl_video_frame_t * frame,
                                uint8_t * pos, int len)
  {
  int len_needed =
    8 + /* timestamp */
    8; /* Duration  */

  if(format->timecode_format.int_framerate)
    len_needed += 8; /*  Timecode  */

  if(format->interlace_mode == GAVL_INTERLACE_MIXED)
    len_needed += 1; /*  Interlace mode  */
  
  if(len_needed > len)
    return len_needed;
  
  pos = set_64(pos, frame->timestamp);
  pos = set_64(pos, frame->duration);
  
  if(format->timecode_format.int_framerate)
    pos = set_64(pos, frame->timecode);

  if(format->interlace_mode == GAVL_INTERLACE_MIXED)
    pos = set_8(pos, frame->interlace_mode);
  
  return len_needed;
  }

int
bg_deserialize_video_frame_header(const gavl_video_format_t * format,
                                  gavl_video_frame_t * frame,
                                  const uint8_t * pos, int len)
  {
  uint32_t tmp;
  pos = get_64(pos, (uint64_t*)(&frame->timestamp));
  pos = get_64(pos, (uint64_t*)(&frame->duration));
  if(format->timecode_format.int_framerate)
    pos = get_64(pos, (uint64_t*)(&frame->timecode));
  if(format->interlace_mode == GAVL_INTERLACE_MIXED)
    {
    pos = get_8(pos, &tmp);
    frame->interlace_mode = tmp;
    }
  return 1;
  }


int
bg_serialize_audio_frame(const gavl_audio_format_t * format,
                         const gavl_audio_frame_t * frame,
                         bg_serialize_write_callback_t func, void * data)
  {

  }

int
bg_serialize_video_frame(const gavl_video_format_t * format,
                         const gavl_video_frame_t * frame,
                         bg_serialize_write_callback_t func, void * data)
  {

  }

/* */




int
bg_deserialize_audio_frame(const gavl_audio_format_t * format,
                           const gavl_audio_frame_t * frame,
                           bg_serialize_read_callback_t func, void * data, int * big_endian)
  {

  }

int
bg_deserialize_video_frame(const gavl_video_format_t * format,
                           const gavl_video_frame_t * frame,
                           bg_serialize_read_callback_t func, void * data, int * big_endian)
  {
  
  }

