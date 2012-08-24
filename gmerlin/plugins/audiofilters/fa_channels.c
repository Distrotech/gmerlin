/*****************************************************************
 * gmerlin - a general purpose multimedia framework and applications
 *
 * Copyright (c) 2001 - 2012 Members of the Gmerlin project
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
#include <string.h>
#include <stdio.h>

#include <config.h>
#include <gmerlin/translation.h>
#include <gmerlin/plugin.h>
#include <gmerlin/utils.h>
#include <gmerlin/log.h>

#define LOG_DOMAIN "fa_channels"

typedef struct
  {
  int front_channels;
  int rear_channels;
  int lfe;
  
  int need_restart;

  gavl_audio_format_t format;

  bg_parameter_info_t * parameters;

  gavl_audio_source_t * in_src;
  
  } channels_priv_t;

static void * create_channels()
  {
  channels_priv_t * ret;
  ret = calloc(1, sizeof(*ret));
  return ret;
  }

static void destroy_channels(void * priv)
  {
  channels_priv_t * vp;
  vp = priv;
  free(vp);
  }

static const bg_parameter_info_t parameters[] =
  {
    {
    .name =        "front_channels",
    .long_name =   TRS("Front channels"),
    .type =        BG_PARAMETER_INT,
    .flags =       BG_PARAMETER_SYNC,
    .val_min =     { .val_i = 1 },
    .val_max =     { .val_i = 5 },
    .val_default = { .val_i = 2 },
    },
    {
    .name =        "rear_channels",
    .long_name =   TRS("Rear channels"),
    .type =        BG_PARAMETER_INT,
    .flags =       BG_PARAMETER_SYNC,
    .val_min =     { .val_i = 0 },
    .val_max =     { .val_i = 3 },
    .val_default = { .val_i = 0 },
    },
    {
    .name =        "lfe",
    .long_name =   TRS("LFE"),
    .type =        BG_PARAMETER_CHECKBUTTON,
    .flags =       BG_PARAMETER_SYNC,
    .val_default = { .val_i = 0 },
    },
    { /* End of parameters */ }
  };

static const bg_parameter_info_t * get_parameters_channels(void * priv)
  {
  return parameters;
  }

static void
set_parameter_channels(void * priv, const char * name,
                         const bg_parameter_value_t * val)
  {
  channels_priv_t * vp = priv;

  if(!name)
    return;

  else if(!strcmp(name, "front_channels"))
    {
    if(vp->front_channels != val->val_i)
      {
      vp->need_restart = 1;
      vp->front_channels = val->val_i;
      }
    }
  else if(!strcmp(name, "rear_channels"))
    {
    if(vp->rear_channels != val->val_i)
      {
      vp->need_restart = 1;
      vp->rear_channels = val->val_i;
      }
    }
  else if(!strcmp(name, "lfe"))
    {
    if(vp->lfe != val->val_i)
      {
      vp->need_restart = 1;
      vp->lfe = val->val_i;
      }
    }
  }

static int need_restart_channels(void * priv)
  {
  channels_priv_t * vp = priv;
  return vp->need_restart;
  }

static gavl_source_status_t read_func(void * priv,
                                      gavl_audio_frame_t ** frame)
  {
  channels_priv_t * vp = priv;
  return gavl_audio_source_read_frame(vp->in_src, frame);
  }

static void set_channel_setup(channels_priv_t * vp, gavl_audio_format_t * f)
  {
  f->num_channels = 0;

  switch(vp->front_channels)
    {
    case 1:
      f->channel_locations[f->num_channels++] = GAVL_CHID_FRONT_CENTER;
      break;
    case 2:
      f->channel_locations[f->num_channels++] = GAVL_CHID_FRONT_LEFT;
      f->channel_locations[f->num_channels++] = GAVL_CHID_FRONT_RIGHT;
      break;
    case 3:
      f->channel_locations[f->num_channels++] = GAVL_CHID_FRONT_LEFT;
      f->channel_locations[f->num_channels++] = GAVL_CHID_FRONT_CENTER;
      f->channel_locations[f->num_channels++] = GAVL_CHID_FRONT_RIGHT;
      break;
    case 4:
      f->channel_locations[f->num_channels++] = GAVL_CHID_FRONT_LEFT;
      f->channel_locations[f->num_channels++] = GAVL_CHID_FRONT_RIGHT;
      f->channel_locations[f->num_channels++] = GAVL_CHID_FRONT_CENTER_LEFT;
      f->channel_locations[f->num_channels++] = GAVL_CHID_FRONT_CENTER_LEFT;
      break;
    case 5:
      f->channel_locations[f->num_channels++] = GAVL_CHID_FRONT_LEFT;
      f->channel_locations[f->num_channels++] = GAVL_CHID_FRONT_CENTER;
      f->channel_locations[f->num_channels++] = GAVL_CHID_FRONT_RIGHT;
      f->channel_locations[f->num_channels++] = GAVL_CHID_FRONT_CENTER_LEFT;
      f->channel_locations[f->num_channels++] = GAVL_CHID_FRONT_CENTER_LEFT;
      break;
    }

  switch(vp->rear_channels)
    {
    case 1:
      f->channel_locations[f->num_channels++] = GAVL_CHID_REAR_CENTER;
      break;
    case 2:
      f->channel_locations[f->num_channels++] = GAVL_CHID_REAR_LEFT;
      f->channel_locations[f->num_channels++] = GAVL_CHID_REAR_RIGHT;
      break;
    case 3:
      f->channel_locations[f->num_channels++] = GAVL_CHID_REAR_LEFT;
      f->channel_locations[f->num_channels++] = GAVL_CHID_REAR_RIGHT;
      f->channel_locations[f->num_channels++] = GAVL_CHID_REAR_CENTER;
      break;
    }

  if(vp->lfe)
    f->channel_locations[f->num_channels++] = GAVL_CHID_LFE;
  }

static gavl_audio_source_t *
connect_channels(void * priv,
                     gavl_audio_source_t * src,
                     const gavl_audio_options_t * opt)
  {
  gavl_audio_format_t format;
  channels_priv_t * vp = priv;
  vp->in_src = src;
  
  gavl_audio_format_copy(&format,
                         gavl_audio_source_get_src_format(vp->in_src));

  set_channel_setup(vp, &format);
  gavl_audio_source_set_dst(vp->in_src, 0, &format);
  return gavl_audio_source_create(read_func, vp, 0, &format);
  }

const bg_fa_plugin_t the_plugin = 
  {
    .common =
    {
      BG_LOCALE,
      .name =      "fa_channels",
      .long_name = TRS("Force channel setup"),
      .description = TRS("This forces a channel setup as input for the next filter."),
      .type =     BG_PLUGIN_FILTER_AUDIO,
      .flags =    BG_PLUGIN_FILTER_1,
      .create =   create_channels,
      .destroy =   destroy_channels,
      .get_parameters =   get_parameters_channels,
      .set_parameter =    set_parameter_channels,
      .priority =         1,
    },
    .connect = connect_channels,
    .need_restart = need_restart_channels,
  };

/* Include this into all plugin modules exactly once
   to let the plugin loader obtain the API version */
BG_GET_PLUGIN_API_VERSION;
