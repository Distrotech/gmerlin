/*****************************************************************
 * gavl - a general purpose audio/video processing library
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

#include <stdio.h>
#include <string.h>
#include <math.h>
#include <gavl/gavl.h>

static const struct
  {
  gavl_sample_format_t format;
  char * name;
  }
sample_format_names[] =
  {
    { GAVL_SAMPLE_NONE,   "Not specified" },
    { GAVL_SAMPLE_U8,     "Unsigned 8 bit"},
    { GAVL_SAMPLE_S8,     "Signed 8 bit"},
    { GAVL_SAMPLE_U16,    "Unsigned 16 bit"},
    { GAVL_SAMPLE_S16,    "Signed 16 bit"},
    { GAVL_SAMPLE_S32,    "Signed 32 bit"},
    { GAVL_SAMPLE_FLOAT,  "Floating point"},
    { GAVL_SAMPLE_DOUBLE, "Double precision"},
  };

const char * gavl_sample_format_to_string(gavl_sample_format_t format)
  {
  int i;
  for(i = 0; i < sizeof(sample_format_names)/sizeof(sample_format_names[0]); i++)
    {
    if(format == sample_format_names[i].format)
      return sample_format_names[i].name;
    }
  return (char*)0;
  }

static const struct
  {
  gavl_interleave_mode_t mode;
  char * name;
  }
interleave_mode_names[] =
  {
    { GAVL_INTERLEAVE_NONE, "Not interleaved" },
    { GAVL_INTERLEAVE_2,    "Interleaved channel pairs" },
    { GAVL_INTERLEAVE_ALL,  "All channels interleaved" },
  };


const char * gavl_interleave_mode_to_string(gavl_interleave_mode_t mode)
  {
  int i;
  for(i = 0;
      i < sizeof(interleave_mode_names)/sizeof(interleave_mode_names[0]);
      i++)
    {
    if(mode == interleave_mode_names[i].mode)
      return interleave_mode_names[i].name;
    }
  return (char*)0;
  }

static const struct
  {
  gavl_channel_id_t id;
  char * name;
  }
channel_id_names[] =
  {
#if 0
    GAVL_CHID_NONE         = 0,   /*!< Undefined                                 */
    GAVL_CHID_FRONT_CENTER,       /*!< For mono                                  */
    GAVL_CHID_FRONT_LEFT,         /*!< Front left                                */
    GAVL_CHID_FRONT_RIGHT,        /*!< Front right                               */
    GAVL_CHID_FRONT_CENTER_LEFT,  /*!< Left of Center                            */
    GAVL_CHID_FRONT_CENTER_RIGHT, /*!< Right of Center                           */
    GAVL_CHID_REAR_LEFT,          /*!< Rear left                                 */
    GAVL_CHID_REAR_RIGHT,         /*!< Rear right                                */
    GAVL_CHID_REAR_CENTER,        /*!< Rear Center                               */
    GAVL_CHID_SIDE_LEFT,          /*!< Side left                                 */
    GAVL_CHID_SIDE_RIGHT,         /*!< Side right                                */
    GAVL_CHID_LFE,                /*!< Subwoofer                                 */
    GAVL_CHID_AUX,                /*!< Additional channel (can be more than one) */
#endif
    { GAVL_CHID_NONE,                "Unknown channel" },
    { GAVL_CHID_FRONT_CENTER,        "Front C" },
    { GAVL_CHID_FRONT_LEFT,          "Front L" },
    { GAVL_CHID_FRONT_RIGHT,         "Front R" },
    { GAVL_CHID_FRONT_CENTER_LEFT,   "Front CL" },
    { GAVL_CHID_FRONT_CENTER_RIGHT,  "Front CR" },
    { GAVL_CHID_REAR_CENTER,         "Rear C" },
    { GAVL_CHID_REAR_LEFT,           "Rear L" },
    { GAVL_CHID_REAR_RIGHT,          "Rear R" },
    { GAVL_CHID_SIDE_LEFT,           "Side L" },
    { GAVL_CHID_SIDE_RIGHT,          "Side R" },
    { GAVL_CHID_LFE,                 "LFE" },
    { GAVL_CHID_AUX,                 "AUX" },
  };

const char * gavl_channel_id_to_string(gavl_channel_id_t id)
  {
  int i;
  for(i = 0;
      i < sizeof(channel_id_names)/sizeof(channel_id_names[0]);
      i++)
    {
    //    fprintf(stderr, "ID: %d\n", id);
    if(id == channel_id_names[i].id)
      return channel_id_names[i].name;
    }
  return (char*)0;
  }

void gavl_audio_format_dump(const gavl_audio_format_t * f)
  {
  int i;
  fprintf(stderr, "  Channels:          %d\n", f->num_channels);

  fprintf(stderr, "  Channel order:     ");
  for(i = 0; i < f->num_channels; i++)
    {
    fprintf(stderr, "%s", gavl_channel_id_to_string(f->channel_locations[i]));
    if(i < f->num_channels - 1)
      fprintf(stderr, ", ");
    }
  fprintf(stderr, "\n");

  fprintf(stderr, "  Samplerate:        %d\n", f->samplerate);
  fprintf(stderr, "  Samples per frame: %d\n", f->samples_per_frame);
  fprintf(stderr, "  Interleave Mode:   %s\n",
          gavl_interleave_mode_to_string(f->interleave_mode));
  fprintf(stderr, "  Sample format:     %s\n",
          gavl_sample_format_to_string(f->sample_format));
  
  if(gavl_front_channels(f) == 3)
    {
    if(f->center_level > 0.0)
      fprintf(stderr, "  Center level:      %0.1f dB\n", 20 * log10(f->center_level));
    else
      fprintf(stderr, "  Center level:      Zero\n");
    }
  if(gavl_rear_channels(f))
    {
    if(f->rear_level > 0.0)
      fprintf(stderr, "  Rear level:        %0.1f dB\n", 20 * log10(f->rear_level));
    else
      fprintf(stderr, "  Rear level:        Zero\n");
    }

  

  }

void gavl_set_channel_setup(gavl_audio_format_t * dst)
  {
  int i;
  if(dst->channel_locations[0] == GAVL_CHID_NONE)
    {
    switch(dst->num_channels)
      {
      case 1:
        dst->channel_locations[0] = GAVL_CHID_FRONT_CENTER;
        break;
      case 2: /* 2 Front channels (Stereo or Dual channels) */
        dst->channel_locations[0] = GAVL_CHID_FRONT_LEFT;
        dst->channel_locations[1] = GAVL_CHID_FRONT_RIGHT;
        break;
      case 3:
        dst->channel_locations[0] = GAVL_CHID_FRONT_LEFT;
        dst->channel_locations[1] = GAVL_CHID_FRONT_RIGHT;
        dst->channel_locations[2] = GAVL_CHID_FRONT_CENTER;
        break;
      case 4:
        dst->channel_locations[0] = GAVL_CHID_FRONT_LEFT;
        dst->channel_locations[1] = GAVL_CHID_FRONT_RIGHT;
        dst->channel_locations[2] = GAVL_CHID_REAR_LEFT;
        dst->channel_locations[3] = GAVL_CHID_REAR_RIGHT;
        break;
      case 5:
        dst->channel_locations[0] = GAVL_CHID_FRONT_LEFT;
        dst->channel_locations[1] = GAVL_CHID_FRONT_RIGHT;
        dst->channel_locations[2] = GAVL_CHID_REAR_LEFT;
        dst->channel_locations[3] = GAVL_CHID_REAR_RIGHT;
        dst->channel_locations[4] = GAVL_CHID_FRONT_CENTER;
        break;
      case 6:
        dst->channel_locations[0] = GAVL_CHID_FRONT_LEFT;
        dst->channel_locations[1] = GAVL_CHID_FRONT_RIGHT;
        dst->channel_locations[2] = GAVL_CHID_REAR_LEFT;
        dst->channel_locations[3] = GAVL_CHID_REAR_RIGHT;
        dst->channel_locations[4] = GAVL_CHID_FRONT_CENTER;
        dst->channel_locations[5] = GAVL_CHID_LFE;
        break;
      default:
        for(i = 0; i < dst->num_channels; i++)
          dst->channel_locations[i] = GAVL_CHID_AUX;
        break;
      }
    }
  }

void gavl_audio_format_copy(gavl_audio_format_t * dst,
                            const gavl_audio_format_t * src)
  {
  memcpy(dst, src, sizeof(*dst));
  }

int gavl_channel_index(const gavl_audio_format_t * f, gavl_channel_id_t id)
  {
  int i;
  for(i = 0; i < f->num_channels; i++)
    {
    if(f->channel_locations[i] == id)
      return i;
    }
  //  fprintf(stderr, "Channel %s not present!!! Format was\n",
  //          gavl_channel_id_to_string(id));
  //  gavl_audio_format_dump(f);
  return -1;
  }

int gavl_front_channels(const gavl_audio_format_t * f)
  {
  int i;
  int result = 0;
  for(i = 0; i < f->num_channels; i++)
    {
    switch(f->channel_locations[i])
      {
      case GAVL_CHID_FRONT_CENTER:
      case GAVL_CHID_FRONT_LEFT:
      case GAVL_CHID_FRONT_RIGHT:
      case GAVL_CHID_FRONT_CENTER_LEFT:
      case GAVL_CHID_FRONT_CENTER_RIGHT:
        result++;
        break;
      case GAVL_CHID_NONE:
      case GAVL_CHID_REAR_LEFT:
      case GAVL_CHID_REAR_RIGHT:
      case GAVL_CHID_REAR_CENTER:
      case GAVL_CHID_SIDE_LEFT:
      case GAVL_CHID_SIDE_RIGHT:
      case GAVL_CHID_LFE:
      case GAVL_CHID_AUX:
        break;
      }
    }
  return result;
  }

int gavl_rear_channels(const gavl_audio_format_t * f)
  {
  int i;
  int result = 0;
  for(i = 0; i < f->num_channels; i++)
    {
    switch(f->channel_locations[i])
      {
      case GAVL_CHID_REAR_LEFT:
      case GAVL_CHID_REAR_RIGHT:
      case GAVL_CHID_REAR_CENTER:
        result++;
        break;
      case GAVL_CHID_FRONT_CENTER:
      case GAVL_CHID_FRONT_LEFT:
      case GAVL_CHID_FRONT_RIGHT:
      case GAVL_CHID_FRONT_CENTER_LEFT:
      case GAVL_CHID_FRONT_CENTER_RIGHT:
      case GAVL_CHID_NONE:
      case GAVL_CHID_SIDE_LEFT:
      case GAVL_CHID_SIDE_RIGHT:
      case GAVL_CHID_LFE:
      case GAVL_CHID_AUX:
        break;
      }
    }
  return result;
  }

int gavl_side_channels(const gavl_audio_format_t * f)
  {
  int i;
  int result = 0;
  for(i = 0; i < f->num_channels; i++)
    {
    switch(f->channel_locations[i])
      {
      case GAVL_CHID_SIDE_LEFT:
      case GAVL_CHID_SIDE_RIGHT:
        result++;
        break;
      case GAVL_CHID_REAR_LEFT:
      case GAVL_CHID_REAR_RIGHT:
      case GAVL_CHID_REAR_CENTER:
      case GAVL_CHID_FRONT_CENTER:
      case GAVL_CHID_FRONT_LEFT:
      case GAVL_CHID_FRONT_RIGHT:
      case GAVL_CHID_FRONT_CENTER_LEFT:
      case GAVL_CHID_FRONT_CENTER_RIGHT:
      case GAVL_CHID_NONE:
      case GAVL_CHID_LFE:
      case GAVL_CHID_AUX:
        break;
      }
    }
  return result;
  }

int gavl_lfe_channels(const gavl_audio_format_t * f)
  {
  int i;
  int result = 0;
  for(i = 0; i < f->num_channels; i++)
    {
    switch(f->channel_locations[i])
      {
      case GAVL_CHID_LFE:
        result++;
        break;
      case GAVL_CHID_SIDE_LEFT:
      case GAVL_CHID_SIDE_RIGHT:
      case GAVL_CHID_REAR_LEFT:
      case GAVL_CHID_REAR_RIGHT:
      case GAVL_CHID_REAR_CENTER:
      case GAVL_CHID_FRONT_CENTER:
      case GAVL_CHID_FRONT_LEFT:
      case GAVL_CHID_FRONT_RIGHT:
      case GAVL_CHID_FRONT_CENTER_LEFT:
      case GAVL_CHID_FRONT_CENTER_RIGHT:
      case GAVL_CHID_NONE:
      case GAVL_CHID_AUX:
        break;
      }
    }
  return result;
  }

int gavl_aux_channels(const gavl_audio_format_t * f)
  {
  int i;
  int result = 0;
  for(i = 0; i < f->num_channels; i++)
    {
    switch(f->channel_locations[i])
      {
      case GAVL_CHID_AUX:
        result++;
        break;
      case GAVL_CHID_SIDE_LEFT:
      case GAVL_CHID_SIDE_RIGHT:
      case GAVL_CHID_REAR_LEFT:
      case GAVL_CHID_REAR_RIGHT:
      case GAVL_CHID_REAR_CENTER:
      case GAVL_CHID_FRONT_CENTER:
      case GAVL_CHID_FRONT_LEFT:
      case GAVL_CHID_FRONT_RIGHT:
      case GAVL_CHID_FRONT_CENTER_LEFT:
      case GAVL_CHID_FRONT_CENTER_RIGHT:
      case GAVL_CHID_NONE:
      case GAVL_CHID_LFE:
        break;
      }
    }
  return result;
  }



int gavl_bytes_per_sample(gavl_sample_format_t format)
  {
  switch(format)
    {
    case     GAVL_SAMPLE_U8:
    case     GAVL_SAMPLE_S8:
      return 1;
      break;
    case     GAVL_SAMPLE_U16:
    case     GAVL_SAMPLE_S16:
      return 2;
      break;
    case     GAVL_SAMPLE_S32:
      return 4;
      break;
    case     GAVL_SAMPLE_FLOAT:
      return sizeof(float);
      break;
    case     GAVL_SAMPLE_DOUBLE:
      return sizeof(double);
      break;
    case     GAVL_SAMPLE_NONE:
      return 0;
    }
  return 0;
  }


int gavl_audio_formats_equal(const gavl_audio_format_t * format_1,
                              const gavl_audio_format_t * format_2)
  {
  return !memcmp(format_1, format_2, sizeof(*format_1));
  }
