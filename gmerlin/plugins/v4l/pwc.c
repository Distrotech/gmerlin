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

#include <sys/ioctl.h>
#include <sys/types.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <config.h>
#include <translation.h>
#include <plugin.h>
#include <utils.h>


#include <linux/videodev.h>
#include "pwc-ioctl.h"
#include "pwc.h"

typedef struct
  {
  struct pwc_leds led;
  } pwc_priv_t;

/* Special stuff for Phillips webcams */

int bg_pwc_probe(int fd)
  {
  struct pwc_probe p;
  struct video_capability c;
  
  memset(&p, 0, sizeof(p));
  memset(&c, 0, sizeof(c));

  if(ioctl(fd, VIDIOCPWCPROBE, &p) < 0)
    return 0;
  
  if(ioctl(fd, VIDIOCGCAP, &c) < 0)
    return 0;

  
  if(!strcmp(p.name, c.name))
    return 1;
  return 0;
  }

static const bg_parameter_info_t pwc_parameters[] =
  {
    {
      .name =        "pwc_general",
      .long_name =   TRS("PWC General"),
      .type =        BG_PARAMETER_SECTION,
    },
    {
      .name =        "pwc_framerate",
      .long_name =   TRS("Framerate"),
      .type =        BG_PARAMETER_INT,
      .val_min =     { .val_i = 4 },
      .val_max =     { .val_i = 30 },
      .val_default = { .val_i = 10 },
    },
    {
      .name =        "pwc_compression",
      .long_name =   TRS("Compression"),
      .type =        BG_PARAMETER_STRINGLIST,
      .flags =       BG_PARAMETER_SYNC,
      .val_default = { .val_str = "Medium" },
      .multi_labels = (char const *[]){ TRS("None"),
                              TRS("Low"),
                              TRS("Medium"),
                              TRS("High"),
                              (char*)0 },
      .multi_names =     (char const *[]){ "None",
                              "Low",
                              "Medium",
                              "High",
                              (char*)0 },
    },
    {
      .name =        "pwc_gain",
      .long_name =   TRS("Gain control (-1 = Auto)"),
      .type =        BG_PARAMETER_SLIDER_INT,
      .flags =       BG_PARAMETER_SYNC,
      .val_default = { .val_i = -1 },
      .val_min =     { .val_i = -1 },
      .val_max =     { .val_i = 65535 },
    },
    {
      .name =        "pwc_shutterspeed",
      .long_name =   TRS("Shutter speed (-1 = Auto)"),
      .type =        BG_PARAMETER_SLIDER_INT,
      .flags =       BG_PARAMETER_SYNC,
      .val_default = { .val_i = -1 },
      .val_min =     { .val_i = -1 },
      .val_max =     { .val_i = 65535 },
    },
    {
      .name =        "pwc_sharpness",
      .long_name =   TRS("Sharpness (-1 = Auto)"),
      .type =        BG_PARAMETER_SLIDER_INT,
      .flags =       BG_PARAMETER_SYNC,
      .val_default = { .val_i = -1 },
      .val_min =     { .val_i = -1 },
      .val_max =     { .val_i = 65535 },
    },
    {
      .name =        "pwc_backlight",
      .long_name =   TRS("Backlight compensation"),
      .type =        BG_PARAMETER_CHECKBUTTON,
      .flags =       BG_PARAMETER_SYNC,
      .val_default = { .val_i = 1 },
    },
    {
      .name =        "pwc_flicker",
      .long_name =   TRS("Flicker compensation"),
      .type =        BG_PARAMETER_CHECKBUTTON,
      .flags =       BG_PARAMETER_SYNC,
      .val_default = { .val_i = 0 },
    },
    {
      .name =        "pwc_whitebalance_section",
      .long_name =   TRS("PWC Whitebalance"),
      .type =        BG_PARAMETER_SECTION,
    },
    {
      .name =        "pwc_whitebalance",
      .long_name =   TRS("White balance"),
      .type =        BG_PARAMETER_STRINGLIST,
      .flags =       BG_PARAMETER_SYNC,
      .val_default = { .val_str = "Auto" },
      .multi_names =     (char const *[]){ "Indoor",
                                  "Outdoor",
                                  "Fluorescent lighting",
                                  "Manual",
                                  "Auto",
                                  (char*)0 },
      .multi_labels =     (char const *[]){ TRS("Indoor"),
                                   TRS("Outdoor"),
                                   TRS("Fluorescent lighting"),
                                   TRS("Manual"),
                                   TRS("Auto"),
                                   (char*)0 },
    },
    {
      .name =        "pwc_manual_red",
      .long_name =   TRS("Manual red gain"),
      .type =        BG_PARAMETER_SLIDER_INT,
      .flags =       BG_PARAMETER_SYNC,
      .val_min =     { .val_i = 0 },
      .val_max =     { .val_i = 65535 },
      .val_default = { .val_i = 32000 },
    },
    {
      .name =        "pwc_manual_blue",
      .long_name =   TRS("Manual blue gain"),
      .type =        BG_PARAMETER_SLIDER_INT,
      .flags =       BG_PARAMETER_SYNC,
      .val_min =     { .val_i = 0 },
      .val_max =     { .val_i = 65535 },
      .val_default = { .val_i = 32000 },
    },
    {
      .name =        "pwc_control_speed",
      .long_name =   TRS("Auto speed"),
      .type =        BG_PARAMETER_SLIDER_INT,
      .flags =       BG_PARAMETER_SYNC,
      .val_min =     { .val_i = 0 },
      .val_max =     { .val_i = 65535 },
      .val_default = { .val_i = 32000 },
    },
    {
      .name =        "pwc_control_delay",
      .long_name =   TRS("Auto delay"),
      .type =        BG_PARAMETER_SLIDER_INT,
      .flags =       BG_PARAMETER_SYNC,
      .val_min =     { .val_i = 0 },
      .val_max =     { .val_i = 65535 },
      .val_default = { .val_i = 32000 },
    },
    {
      .name =        "pwc_led_section",
      .long_name =   TRS("PWC LED"),
      .type =        BG_PARAMETER_SECTION,
    },
    {
      .name =        "pwc_led_on",
      .long_name =   TRS("LED on time (secs)"),
      .type =        BG_PARAMETER_FLOAT,
      .val_min =     { .val_f = 0.0 },
      .val_max =     { .val_f = 255.0 },
      .val_default = { .val_f = 1.0 },
      .num_digits =  1,
    },
    {
      .name =        "pwc_led_off",
      .long_name =   TRS("LED off time (secs)"),
      .type =        BG_PARAMETER_FLOAT,
      .val_min =     { .val_f = 0.0 },
      .val_max =     { .val_f = 255.0 },
      .val_default = { .val_f = 0.0 },
      .num_digits =  1,
    },
    { /* End of parameters */ }
    
  };

/* Reallocate parameter array */

void * bg_pwc_get_parameters(int fd, bg_parameter_info_t ** parameters)
  {
  int i;
  bg_parameter_info_t * p;
  int num_generic_parameters;
  int num_pwc_parameters;
  pwc_priv_t * ret;

  
  p = *parameters;
  
  num_generic_parameters = 0;
  while(p[num_generic_parameters].name)
    {
    /* Exchange the "whiteness" parameter by Gamma */

    if(!strcmp(p[num_generic_parameters].name, "whiteness"))
      {
      p[num_generic_parameters].long_name =
        bg_strdup(p[num_generic_parameters].long_name,
                  "Gamma");
      }
    num_generic_parameters++;
    }

  num_pwc_parameters = 0;
  while(pwc_parameters[num_pwc_parameters].name)
    num_pwc_parameters++;

  p = realloc(p, (num_generic_parameters+num_pwc_parameters+1)*sizeof(*p));

  memset(p + num_generic_parameters, 0, (num_pwc_parameters+1)*sizeof(*p));

  for(i = 0; i < num_pwc_parameters; i++)
    {
    bg_parameter_info_copy(p + (num_generic_parameters + i),
                           &(pwc_parameters[i]));
    }
  *parameters = p;

  ret = calloc(1, sizeof(*ret));
  return ret;
  }

void bg_pwc_destroy(void*p)
  {
  free(p);
  }


/* Handle parameters beginning with "pwc_" */

void bg_pwc_set_parameter(int fd, void * priv,
                          const char * name, const bg_parameter_value_t * val)
  {
  pwc_priv_t * p;
  int compression;
  struct video_window win;
  struct pwc_whitebalance whitebalance;
  struct pwc_wb_speed wb_speed;

  p = (pwc_priv_t*)priv;
    
  if(!name)
    {
    ioctl(fd, VIDIOCPWCSLED, &(p->led));
    return;
    }
  else if(!strcmp(name, "pwc_framerate"))
    {
    if(ioctl(fd, VIDIOCGWIN, &win) < 0)
      return;
    
    win.flags &= ~PWC_FPS_FRMASK;
    win.flags |= (val->val_i << PWC_FPS_SHIFT);
    ioctl(fd, VIDIOCSWIN, &win);
    }
  else if(!strcmp(name, "pwc_compression"))
    {
    compression = 0;
    if(!strcmp(val->val_str, "None"))
      compression = 0;
    else if(!strcmp(val->val_str, "Low"))
      compression = 1;
    else if(!strcmp(val->val_str, "Medium"))
      compression = 2;
    else if(!strcmp(val->val_str, "High"))
      compression = 3;
    ioctl(fd, VIDIOCPWCSCQUAL, &compression);
    }
  else if(!strcmp(name, "pwc_gain"))
    {
    ioctl(fd, VIDIOCPWCSAGC, &(val->val_i));
    }
  else if(!strcmp(name, "pwc_shutterspeed"))
    {
    ioctl(fd, VIDIOCPWCSSHUTTER, &(val->val_i));
    }
  else if(!strcmp(name, "pwc_sharpness"))
    {
    ioctl(fd, VIDIOCPWCSCONTOUR, &(val->val_i));
    }
  else if(!strcmp(name, "pwc_backlight"))
    {
    ioctl(fd, VIDIOCPWCSBACKLIGHT, &(val->val_i));
    }
  else if(!strcmp(name, "pwc_flicker"))
    {
    ioctl(fd, VIDIOCPWCSFLICKER, &(val->val_i));
    }
  else if(!strcmp(name, "pwc_whitebalance"))
    {
    if(ioctl(fd, VIDIOCPWCGAWB, &whitebalance) < 0)
      return;
    if(!strcmp(val->val_str, "Indoor"))
      whitebalance.mode = PWC_WB_INDOOR;
    else if(!strcmp(val->val_str, "Outdoor"))
      whitebalance.mode = PWC_WB_OUTDOOR;
    else if(!strcmp(val->val_str, "Fluorescent lighting"))
      whitebalance.mode = PWC_WB_FL;
    else if(!strcmp(val->val_str, "Manual"))
      whitebalance.mode = PWC_WB_MANUAL;
    else if(!strcmp(val->val_str, "Auto"))
      whitebalance.mode = PWC_WB_AUTO;
    ioctl(fd, VIDIOCPWCSAWB, &whitebalance);
    }
  else if(!strcmp(name, "pwc_manual_red"))
    {
    if(ioctl(fd, VIDIOCPWCGAWB, &whitebalance) < 0)
      return;
    whitebalance.manual_red = val->val_i;
    ioctl(fd, VIDIOCPWCSAWB, &whitebalance);
    }
  else if(!strcmp(name, "pwc_manual_blue"))
    {
    if(ioctl(fd, VIDIOCPWCGAWB, &whitebalance) < 0)
      return;
    whitebalance.manual_blue = val->val_i;
    ioctl(fd, VIDIOCPWCSAWB, &whitebalance);
    }
  else if(!strcmp(name, "pwc_control_speed"))
    {
    if(ioctl(fd, VIDIOCPWCGAWBSPEED, &wb_speed) < 0)
      return;
    wb_speed.control_speed = val->val_i;
    ioctl(fd, VIDIOCPWCSAWBSPEED, &wb_speed);
    }
  else if(!strcmp(name, "pwc_control_delay"))
    {
    if(ioctl(fd, VIDIOCPWCGAWBSPEED, &wb_speed) < 0)
      return;
    wb_speed.control_delay = val->val_i;
    ioctl(fd, VIDIOCPWCSAWBSPEED, &wb_speed);
    }
  else if(!strcmp(name, "pwc_led_on"))
    {
    p->led.led_on = (int)(val->val_f * 1000.0 + 0.5);
    }
  else if(!strcmp(name, "pwc_led_off"))
    {
    p->led.led_off = (int)(val->val_f * 1000.0 + 0.5);
    }
  
  }
