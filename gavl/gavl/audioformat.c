/*****************************************************************
 
  audioformat.c
 
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
#include <string.h>
#include <math.h>
#include <gavl/gavl.h>

static struct
  {
  gavl_sample_format_t format;
  char * name;
  }
sample_format_names[] =
  {
    { GAVL_SAMPLE_NONE,  "Not specified" },
    { GAVL_SAMPLE_U8,    "Unsigned 8 bit"},
    { GAVL_SAMPLE_S8,    "Signed 8 bit"},
    { GAVL_SAMPLE_U16,   "Unsigned 16 bit"},
    { GAVL_SAMPLE_S16,   "Signed 16 bit"},
    { GAVL_SAMPLE_S32,   "Signed 32 bit"},
    { GAVL_SAMPLE_FLOAT, "Floating point"},
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

static struct
  {
  gavl_channel_setup_t channel_setup;
  char * name;
  }
channel_setup_names[] =
  {
    { GAVL_CHANNEL_NONE,   "Not Specified" },
    { GAVL_CHANNEL_MONO,   "Mono" },
    { GAVL_CHANNEL_STEREO, "Stereo" },  /* 2 Front channels (Stereo or Dual channels) */
    { GAVL_CHANNEL_3F,    "3 Front" },
    { GAVL_CHANNEL_2F1R,  "2 Front, 1 Rear" },
    { GAVL_CHANNEL_3F1R,  "3 Front, 1 Rear" },
    { GAVL_CHANNEL_2F2R,  "2 Front, 2 Rear" },
    { GAVL_CHANNEL_3F2R,  "3 Front, 2 Rear" }
  };

const char * gavl_channel_setup_to_string(gavl_channel_setup_t channel_setup)
  {
  int i;
  for(i = 0; i < sizeof(channel_setup_names)/sizeof(channel_setup_names[0]); i++)
    {
    if(channel_setup == channel_setup_names[i].channel_setup)
      return channel_setup_names[i].name;
    }
  return (char*)0;
  }

static struct
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

static struct
  {
  gavl_channel_id_t id;
  char * name;
  }
channel_id_names[] =
  {
    { GAVL_CHID_NONE,  "Unknown channel" },
    { GAVL_CHID_FRONT, "Front" },
    { GAVL_CHID_FRONT_LEFT,  "Front L" },
    { GAVL_CHID_FRONT_RIGHT, "Front R" },
    { GAVL_CHID_FRONT_CENTER, "Front C" },
    { GAVL_CHID_REAR, "Rear" },
    { GAVL_CHID_REAR_LEFT, "Rear L" },
    { GAVL_CHID_REAR_RIGHT, "Rear R" },
    { GAVL_CHID_LFE, "LFE" },
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
  fprintf(stderr, "  Channels:          %d (%s", f->num_channels,
          gavl_channel_setup_to_string(f->channel_setup));
  
  if(f->lfe)
    fprintf(stderr, "+LFE)\n");
  else
    fprintf(stderr, ")\n");

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

int gavl_num_channels(gavl_channel_setup_t s)
  {
  switch(s)
    {
    case GAVL_CHANNEL_NONE:
      return 0;
      break;
    case GAVL_CHANNEL_MONO:
      return 1;
      break;
    case GAVL_CHANNEL_STEREO: /* 2 Front channels (Stereo or Dual channels) */
      return 2;
      break;
    case GAVL_CHANNEL_3F:
    case GAVL_CHANNEL_2F1R:
      return 3;
      break;
    case GAVL_CHANNEL_3F1R:
    case GAVL_CHANNEL_2F2R:
      return 4;
      break;
    case GAVL_CHANNEL_3F2R:
      return 5;
      break;
    }
  return 0;
  }


void gavl_set_channel_setup(gavl_audio_format_t * dst)
  {
  if(dst->channel_setup == GAVL_CHANNEL_NONE)
    {
    dst->lfe = 0;
    switch(dst->num_channels)
      {
      case 1:
        dst->channel_setup = GAVL_CHANNEL_MONO;
        break;
      case 2:
        dst->channel_setup = GAVL_CHANNEL_STEREO;
        break;
      case 3:
        dst->channel_setup = GAVL_CHANNEL_3F;
        break;
      case 4:
        dst->channel_setup = GAVL_CHANNEL_2F2R;
        break;
      case 5:
        dst->channel_setup = GAVL_CHANNEL_3F2R;
        break;
      case 6:
        dst->channel_setup = GAVL_CHANNEL_3F2R;
        dst->lfe = 1;
        break;
      }
    }

  if(dst->channel_locations[0] == GAVL_CHID_NONE)
    {
    switch(dst->channel_setup)
      {
      case GAVL_CHANNEL_NONE:
        break;
      case GAVL_CHANNEL_MONO:
        dst->channel_locations[0] = GAVL_CHID_FRONT;
        break;
      case GAVL_CHANNEL_STEREO: /* 2 Front channels (Stereo or Dual channels) */
        dst->channel_locations[0] = GAVL_CHID_FRONT_LEFT;
        dst->channel_locations[1] = GAVL_CHID_FRONT_RIGHT;
        break;
      case GAVL_CHANNEL_3F:
        dst->channel_locations[0] = GAVL_CHID_FRONT_LEFT;
        dst->channel_locations[1] = GAVL_CHID_FRONT_RIGHT;
        dst->channel_locations[2] = GAVL_CHID_FRONT_CENTER;
        break;
      case GAVL_CHANNEL_2F1R:
        dst->channel_locations[0] = GAVL_CHID_FRONT_LEFT;
        dst->channel_locations[1] = GAVL_CHID_FRONT_RIGHT;
        dst->channel_locations[2] = GAVL_CHID_REAR;
        break;
      case GAVL_CHANNEL_3F1R:
        dst->channel_locations[0] = GAVL_CHID_FRONT_LEFT;
        dst->channel_locations[1] = GAVL_CHID_FRONT_RIGHT;
        dst->channel_locations[2] = GAVL_CHID_REAR;
        dst->channel_locations[3] = GAVL_CHID_FRONT_CENTER;
        break;
      case GAVL_CHANNEL_2F2R:
        dst->channel_locations[0] = GAVL_CHID_FRONT_LEFT;
        dst->channel_locations[1] = GAVL_CHID_FRONT_RIGHT;
        dst->channel_locations[2] = GAVL_CHID_REAR_LEFT;
        dst->channel_locations[3] = GAVL_CHID_REAR_RIGHT;
        break;
      case GAVL_CHANNEL_3F2R:
        dst->channel_locations[0] = GAVL_CHID_FRONT_LEFT;
        dst->channel_locations[1] = GAVL_CHID_FRONT_RIGHT;
        dst->channel_locations[2] = GAVL_CHID_REAR_LEFT;
        dst->channel_locations[3] = GAVL_CHID_REAR_RIGHT;
        dst->channel_locations[4] = GAVL_CHID_FRONT_CENTER;
        break;
      }
    if(dst->lfe)
      dst->channel_locations[dst->num_channels-1] = GAVL_CHID_LFE;
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
  fprintf(stderr, "Channel %s not present!!! Format was\n",
          gavl_channel_id_to_string(id));
  gavl_audio_format_dump(f);
  return -1;
  }

int gavl_front_channels(const gavl_audio_format_t * f)
  {
  switch(f->channel_setup)
    {
    case GAVL_CHANNEL_NONE:
      return 0;
      break;
    case GAVL_CHANNEL_MONO:
      return 1;
      break;
    case GAVL_CHANNEL_STEREO: /* 2 Front channels (Stereo or Dual channels) */
    case GAVL_CHANNEL_2F1R:
    case GAVL_CHANNEL_2F2R:
      return 2;
      break;
    case GAVL_CHANNEL_3F:
    case GAVL_CHANNEL_3F1R:
    case GAVL_CHANNEL_3F2R:
      return 3;
      break;
    }
  return 0;
  }

int gavl_rear_channels(const gavl_audio_format_t * f)
  {
  switch(f->channel_setup)
    {
    case GAVL_CHANNEL_NONE:
    case GAVL_CHANNEL_MONO:
    case GAVL_CHANNEL_STEREO: /* 2 Front channels (Stereo or Dual channels) */
    case GAVL_CHANNEL_3F:
      return 0;
      break;
    case GAVL_CHANNEL_2F1R:
    case GAVL_CHANNEL_3F1R:
      return 1;
      break;
    case GAVL_CHANNEL_2F2R:
    case GAVL_CHANNEL_3F2R:
      return 2;
      break;
    }
  return 0;
  }

int gavl_lfe_channels(const gavl_audio_format_t * f)
  {
  return !!(f->lfe);
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
    case     GAVL_SAMPLE_NONE:
      return 0;
    }
  return 0;
  }


