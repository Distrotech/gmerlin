/*****************************************************************
 
  oa_oss.c
 
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

#include <sys/ioctl.h>
#include <sys/soundcard.h>
#include <unistd.h>
#include <stdlib.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <plugin.h>
#include <utils.h>
#include "oss_common.h"

#define MULTICHANNEL_NONE     0
#define MULTICHANNEL_DEVICES  1
#define MULTICHANNEL_CREATIVE 2

static bg_parameter_info_t parameters[] =
  {
    {
      name:        "multichannel_mode",
      long_name:   "Multichannel Mode",
      type:        BG_PARAMETER_STRINGLIST,
      val_default: { val_str: "None (Downmix)" },
      multi_names:    (char*[]){  "None (Downmix)",
                              "Multiple devices",
                              "Creative Multichannel",
                              (char*)0 },
    },
    {
      name:        "device",
      long_name:   "Device",
      type:        BG_PARAMETER_DEVICE,
      val_default: { val_str: "/dev/dsp" },
    },
    {
      name:        "use_rear_device",
      long_name:   "Use Rear Device",
      type:        BG_PARAMETER_CHECKBUTTON,
      val_default: { val_i: 0 },
    },
    {
      name:        "rear_device",
      long_name:   "Rear Device",
      type:        BG_PARAMETER_DEVICE,
      val_default: { val_str: "/dev/dsp1" },
    },
    {
      name:        "use_center_lfe_device",
      long_name:   "Use Center/LFE Device",
      type:        BG_PARAMETER_CHECKBUTTON,
      val_default: { val_i: 0 },
    },
    {
      name:        "center_lfe_device",
      long_name:   "Center/LFE Device",
      type:        BG_PARAMETER_DEVICE,
      val_default: { val_str: "/dev/dsp2" },
    },
    { /* End of parameters */ }
  };

typedef struct
  {
  int multichannel_mode;

  char * device_front;
  char * device_rear;
  char * device_center_lfe;

  int use_rear_device;
  int use_center_lfe_device;
    
  int fd_front;
  int fd_rear;
  int fd_center_lfe;
  
  int num_channels_front;
  int num_channels_rear;
  int num_channels_center_lfe;

  int bytes_per_sample;
  gavl_audio_format_t format;
  } oss_t;

static void * create_oss()
  {
  oss_t * ret = calloc(1, sizeof(*ret));
  ret->fd_front = -1;
  ret->fd_rear = -1;
  ret->fd_center_lfe = -1;
  return ret;
  }

static int open_devices(oss_t * priv, gavl_audio_format_t * format)
  {
  gavl_sample_format_t sample_format;
  gavl_sample_format_t test_format;
  int test_value;

  fprintf(stderr, "Open Devices\n");
  
  /* Open the devices */
  
  priv->fd_front = open(priv->device_front, O_WRONLY, 0);

  if(priv->fd_front == -1)
    {
    fprintf(stderr, "Cannot open %s\n", priv->device_front);
    goto fail;
    }

  if(priv->num_channels_rear)
    {
    priv->fd_rear = open(priv->device_rear, O_WRONLY, 0);
    if(priv->fd_rear == -1)
      {
      fprintf(stderr, "Cannot open %s\n", priv->device_rear);
      goto fail;
      }
    }
  if(priv->num_channels_center_lfe)
    {
    priv->fd_center_lfe = open(priv->device_center_lfe, O_WRONLY, 0);
    if(priv->fd_center_lfe == -1)
      {
      fprintf(stderr, "Cannot open %s\n", priv->device_center_lfe);
      goto fail;
      }
    }

  /* Set sample format */

  sample_format = bg_oss_set_sample_format(priv->fd_front,
                                           format->sample_format);

  if(sample_format == GAVL_SAMPLE_NONE)
    {
    fprintf(stderr, "Cannot set sampleformat for %s\n",
            priv->device_front);
    goto fail;
    }
  format->sample_format = sample_format;
  
  if(priv->num_channels_rear)
    {
    test_format = bg_oss_set_sample_format(priv->fd_rear,
                                           sample_format);
    if(test_format != sample_format)
      {
      fprintf(stderr, "Cannot set sampleformat for %s\n",
              priv->device_rear);
      goto fail;
      }
    }

  if(priv->num_channels_center_lfe)
    {
    test_format = bg_oss_set_sample_format(priv->fd_center_lfe,
                                           sample_format);
    if(test_format != sample_format)
      {
      fprintf(stderr, "Cannot set sampleformat for %s\n",
              priv->device_center_lfe);
      goto fail;
      }
    }

  /* Set numbers of channels */

  test_value =
    bg_oss_set_channels(priv->fd_front, priv->num_channels_front);
  if(test_value != priv->num_channels_front)
    {
    fprintf(stderr, "Device %s supports no %d-channel sound\n",
            priv->device_front,
            priv->num_channels_front);
    goto fail;
    }

  if(priv->num_channels_rear)
    {
    test_value =
      bg_oss_set_channels(priv->fd_rear, priv->num_channels_rear);
    if(test_value != priv->num_channels_rear)
      {
      fprintf(stderr, "Device %s supports no %d-channel sound\n",
              priv->device_rear,
              priv->num_channels_rear);
      goto fail;
      }
    }

  if(priv->num_channels_center_lfe)
    {
    test_value =
      bg_oss_set_channels(priv->fd_center_lfe, priv->num_channels_center_lfe);
    if(test_value != priv->num_channels_center_lfe)
      {
      fprintf(stderr, "Device %s supports no %d-channel sound\n",
              priv->device_center_lfe,
              priv->num_channels_center_lfe);
      goto fail;
      }
    }

  /* Set Samplerates */
    
  test_value =
    bg_oss_set_samplerate(priv->fd_front, format->samplerate);
  if(test_value != format->samplerate)
    {
    fprintf(stderr, "Samplerate %f KHz not supported by device %s\n",
            format->samplerate / 1000.0,
            priv->device_front);
    goto fail;
    }

  if(priv->num_channels_rear)
    {
    test_value =
      bg_oss_set_samplerate(priv->fd_rear, format->samplerate);
    if(test_value != format->samplerate)
      {
      fprintf(stderr, "Samplerate %f KHz not supported by device %s\n",
              format->samplerate / 1000.0,
              priv->device_rear);
      goto fail;
      }
    }

  if(priv->num_channels_center_lfe)
    {
    test_value =
      bg_oss_set_samplerate(priv->fd_center_lfe, format->samplerate);
    if(test_value != format->samplerate)
      {
      fprintf(stderr, "Samplerate %f KHz not supported by device %s\n",
              format->samplerate / 1000.0,
              priv->device_center_lfe);
      goto fail;
      }
    }

  return 1;
  fail:
  if(priv->fd_front > -1)
    {
    close(priv->fd_front);
    priv->fd_front = -1;
    }
  if(priv->fd_rear > -1)
    {
    close(priv->fd_rear);
    priv->fd_rear = -1;
    }
  if(priv->fd_center_lfe > -1)
    {
    close(priv->fd_center_lfe);
    priv->fd_center_lfe = -1;
    }
  return 0;
  }

static int open_oss(void * data, gavl_audio_format_t * format)
  {
  int front_channels = 0;
  int rear_channels = 0;
  int center_channel = 0;
  int lfe_channel = 0;
  int ret;
  oss_t * priv = (oss_t*)data;

  priv->fd_front      = -1;
  priv->fd_rear       = -1;
  priv->fd_center_lfe = -1;
  
  /* Check for multichannel */

  front_channels = gavl_front_channels(format);
  rear_channels = gavl_rear_channels(format);
  lfe_channel = gavl_lfe_channels(format);
  
  if(front_channels > 2)
    {
    front_channels = 2;
    center_channel = 1;
    }
  if(rear_channels > 2)
    {
    rear_channels = 2;
    }
  
  switch(priv->multichannel_mode)
    {
    /* No multichannel support -> downmix everything */
    case MULTICHANNEL_NONE:
      rear_channels = 0;
      center_channel = 0;
      lfe_channel = 0;
      priv->num_channels_front = front_channels;
      priv->num_channels_rear = 0;
      priv->num_channels_center_lfe = 0;
      format->interleave_mode = GAVL_INTERLEAVE_ALL;

      if(priv->num_channels_front == 1)
        format->channel_locations[0] = GAVL_CHID_FRONT_CENTER;
      else
        {
        format->channel_locations[0] = GAVL_CHID_FRONT_LEFT;
        format->channel_locations[1] = GAVL_CHID_FRONT_RIGHT;
        }
      break;
    /* Multiple devices */
    case MULTICHANNEL_DEVICES:
      /* If the input has lfe, but no center, we must upmix */
      if(lfe_channel)
        center_channel = 1;
      
      if(!priv->use_rear_device)
        rear_channels = 0;
      if(!priv->use_center_lfe_device)
        {
        center_channel = 0;
        lfe_channel = 0;
        }

      priv->num_channels_front = front_channels;
      priv->num_channels_rear = rear_channels;
      priv->num_channels_center_lfe = lfe_channel + center_channel;
      format->interleave_mode = GAVL_INTERLEAVE_2;
      
      break;
    /* All Channels to one device */
    case MULTICHANNEL_CREATIVE:
      /* We need 2 rear channels */
      //      fprintf(stderr, "Creative Multichannel!\n");
      
      if(center_channel || lfe_channel || rear_channels)
        rear_channels = 2;
      if(rear_channels)
        front_channels = 2;
      if(lfe_channel)
        center_channel = 1;
      priv->num_channels_front = front_channels + rear_channels +
        lfe_channel + center_channel;
      priv->num_channels_rear = 0;
      priv->num_channels_center_lfe = 0;
      format->interleave_mode = GAVL_INTERLEAVE_ALL;

      if(front_channels > 1)
        {
        format->channel_locations[0] = GAVL_CHID_FRONT_LEFT;
        format->channel_locations[1] = GAVL_CHID_FRONT_RIGHT;
        format->channel_locations[2] = GAVL_CHID_REAR_LEFT;
        format->channel_locations[3] = GAVL_CHID_REAR_RIGHT;
        format->channel_locations[4] = GAVL_CHID_FRONT_CENTER;
        format->channel_locations[5] = GAVL_CHID_LFE;
        }
      else
        format->channel_locations[0] = GAVL_CHID_FRONT_CENTER;
    }
  /* Reconstruct the speaker setup */

  format->num_channels = front_channels +
    rear_channels + center_channel + lfe_channel;
  
  ret = open_devices(priv, format);
  if(ret)
    {
    format->samples_per_frame = 1024;
    priv->bytes_per_sample =
      gavl_bytes_per_sample(format->sample_format);
    gavl_audio_format_copy(&(priv->format), format);
    }
  return ret;
  }

static void stop_oss(void * p)
  {
  oss_t * priv = (oss_t*)(p);

  if(priv->fd_front != -1)
    {
    close(priv->fd_front);
    priv->fd_front = -1;
    }
  if(priv->fd_rear != -1)
    {
    close(priv->fd_rear);
    priv->fd_rear = -1;
    }
  if(priv->fd_center_lfe != -1)
    {
    close(priv->fd_center_lfe);
    priv->fd_center_lfe = -1;
    }
  }

static void close_oss(void * p)
  {
  
  }

static int start_oss(void * data)
  {
  oss_t * priv = (oss_t*)data;

  if((priv->fd_front == -1) && (priv->fd_rear == -1)
     && (priv->fd_center_lfe == -1))
    return open_devices(priv, &(priv->format));
  else
    return 1;
  }

static void write_frame_oss(void * p, gavl_audio_frame_t * f)
  {
  oss_t * priv = (oss_t*)(p);

  //  fprintf(stderr, "Valid samples: %d\n", f->valid_samples);
  
  write(priv->fd_front, f->channels.s_8[0], f->valid_samples *
        priv->num_channels_front * priv->bytes_per_sample);

  if(priv->num_channels_rear)
    {
    write(priv->fd_rear, f->channels.s_8[2], f->valid_samples *
        priv->num_channels_rear * priv->bytes_per_sample);
    }

  if(priv->num_channels_center_lfe)
    {
    write(priv->fd_center_lfe, f->channels.s_8[4], f->valid_samples *
          priv->num_channels_center_lfe * priv->bytes_per_sample);
    }
  }

static void destroy_oss(void * p)
  {
  oss_t * priv = (oss_t*)(p);

  if(priv->device_front)
    free(priv->device_front);
  if(priv->device_rear)
    free(priv->device_rear);
  if(priv->device_center_lfe)
    free(priv->device_center_lfe);
  free(priv);
  }

static bg_parameter_info_t *
get_parameters_oss(void * priv)
  {
  return parameters;
  }

static int get_delay_oss(void * p)
  {
  int unplayed_bytes;
  oss_t * priv;
  priv = (oss_t*)(p);
  if(ioctl(priv->fd_front, SNDCTL_DSP_GETODELAY, &unplayed_bytes)== -1)
    {
    fprintf(stderr, "OSS Driver: SNDCTL_DSP_GETODELAY ioctl failed\n");
    return 0;
    }
  return unplayed_bytes/( priv->num_channels_front*priv->bytes_per_sample);
  }


/* Set parameter */

static void
set_parameter_oss(void * p, char * name, bg_parameter_value_t * val)
  {
  oss_t * priv = (oss_t*)(p);
  if(!name)
    return;
  //  fprintf(stderr, "Set parameter %s\n", name);
  if(!strcmp(name, "multichannel_mode"))
    {
   if(!strcmp(val->val_str, "None (Downmix)"))
      priv->multichannel_mode = MULTICHANNEL_NONE;
    else if(!strcmp(val->val_str, "Multiple devices"))
      priv->multichannel_mode = MULTICHANNEL_DEVICES;
    else if(!strcmp(val->val_str, "Creative Multichannel"))
      priv->multichannel_mode = MULTICHANNEL_CREATIVE;
    }
  else if(!strcmp(name, "device"))
    {
    priv->device_front = bg_strdup(priv->device_front, val->val_str);
    }
  else if(!strcmp(name, "use_rear_device"))
    {
    priv->use_rear_device = val->val_i;
    }
  else if(!strcmp(name, "rear_device"))
    {
    priv->device_rear = bg_strdup(priv->device_rear, val->val_str);
    }
  else if(!strcmp(name, "use_center_lfe_device"))
    {
    priv->use_center_lfe_device = val->val_i;
    }
  else if(!strcmp(name, "center_lfe_device"))
    {
    priv->device_center_lfe =
      bg_strdup(priv->device_center_lfe, val->val_str);
    }
  }

bg_oa_plugin_t the_plugin =
  {
    common:
    {
      name:          "oa_oss",
      long_name:     "OSS output driver",
      mimetypes:     (char*)0,
      extensions:    (char*)0,
      type:          BG_PLUGIN_OUTPUT_AUDIO,
      flags:         BG_PLUGIN_PLAYBACK,
      priority:      5,
      create:        create_oss,
      destroy:       destroy_oss,
      
      get_parameters: get_parameters_oss,
      set_parameter:  set_parameter_oss
    },

    open:          open_oss,
    start:         start_oss,
    write_frame:   write_frame_oss,
    stop:          stop_oss,
    close:         close_oss,
    get_delay:     get_delay_oss,
  };

/* Include this into all plugin modules exactly once
   to let the plugin loader obtain the API version */
BG_GET_PLUGIN_API_VERSION;
