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

#include <gmerlin/translation.h>

#include <bgfreetype.h>

#include <gmerlin/parameter.h>
#include <gmerlin/textrenderer.h>
#include <gmerlin/osd.h>
#include <gmerlin/utils.h>

struct bg_osd_s
  {
  bg_text_renderer_t * renderer;
  int enable;
  gavl_overlay_t * ovl;
  gavl_time_t duration;
  float font_size;
  gavl_timer_t * timer;
  int changed;
  };

bg_osd_t * bg_osd_create()
  {
  bg_parameter_value_t val;
  bg_osd_t * ret;
  ret = calloc(1, sizeof(*ret));
  ret->renderer = bg_text_renderer_create();

  /* We set special parameters for the textrenderer */

  val.val_str = bg_search_file_read("osd", "GmerlinOSD.pfb");
  bg_text_renderer_set_parameter(ret->renderer, "font_file", &val);
  free(val.val_str);

  val.val_f = 20.0;
  bg_text_renderer_set_parameter(ret->renderer, "font_size", &val);

  val.val_i = 20;
  bg_text_renderer_set_parameter(ret->renderer, "cache_size", &val);

  ret->timer = gavl_timer_create();
  gavl_timer_start(ret->timer);
  return ret;
  }

void bg_osd_destroy(bg_osd_t * osd)
  {
  bg_text_renderer_destroy(osd->renderer);
  gavl_timer_destroy(osd->timer);
  free(osd);
  }

static const bg_parameter_info_t parameters[] =
  {
    {
      .name =        "enable_osd",
      .long_name =   TRS("Enable OSD"),
      .type =        BG_PARAMETER_CHECKBUTTON,
      .val_default = { .val_i = 1 },
    },
    /* The following stuff is copied from textrenderer.c */
    {
      .name =        "font_size",
      .long_name =   TRS("Size"),
      .type =        BG_PARAMETER_FLOAT,
      .num_digits =  2,
      .val_min =     { .val_f = 12.0 },
      .val_max =     { .val_f = 100.0 },
      .val_default = { .val_f = 30.0 },
      .help_string = TRS("Specify fontsize for OSD. The value you enter, is for an image width of 640. For other \
widths, the value will be scaled"),
    },
    {
      .name =       "color",
      .long_name =  TRS("Foreground color"),
      .type =       BG_PARAMETER_COLOR_RGBA,
      .val_default = { .val_color = { 1.0, 1.0, 1.0, 1.0 } },
    },
#ifdef FT_STROKER_H
    {
      .name =       "border_color",
      .long_name =  TRS("Border color"),
      .type =       BG_PARAMETER_COLOR_RGB,
      .val_default = { .val_color = { 0.0, 0.0, 0.0, 1.0 } },
    },
    {
      .name =       "border_width",
      .long_name =  TRS("Border width"),
      .type =       BG_PARAMETER_FLOAT,
      .val_min =     { .val_f = 0.0 },
      .val_max =     { .val_f = 10.0 },
      .val_default = { .val_f = 2.0 },
      .num_digits =  2,
    },
#endif
    {
      .name =       "justify_h",
      .long_name =  TRS("Horizontal justify"),
      .type =       BG_PARAMETER_STRINGLIST,
      .val_default = { .val_str = "center" },
      .multi_names =  (char const *[]){ "center", "left", "right", NULL },
      .multi_labels = (char const *[]){ TRS("Center"), TRS("Left"), TRS("Right"), NULL  },
    },
    {
      .name =       "justify_v",
      .long_name =  TRS("Vertical justify"),
      .type =       BG_PARAMETER_STRINGLIST,
      .val_default = { .val_str = "center" },
      .multi_names =  (char const *[]){ "center", "top", "bottom", NULL  },
      .multi_labels = (char const *[]){ TRS("Center"), TRS("Top"), TRS("Bottom"), NULL },
    },
    {
      .name =        "border_left",
      .long_name =   TRS("Left border"),
      .type =        BG_PARAMETER_INT,
      .val_min =     { .val_i = 0     },
      .val_max =     { .val_i = 65535 },
      .val_default = { .val_i = 10    },
      .help_string = TRS("Distance from the left text border to the image border"),
    },
    {
      .name =        "border_right",
      .long_name =   TRS("Right border"),
      .type =        BG_PARAMETER_INT,
      .val_min =     { .val_i = 0     },
      .val_max =     { .val_i = 65535 },
      .val_default = { .val_i = 10    },
      .help_string = TRS("Distance from the right text border to the image border"),
    },
    {
      .name =        "border_top",
      .long_name =   TRS("Top border"),
      .type =        BG_PARAMETER_INT,
      .val_min =     { .val_i = 0     },
      .val_max =     { .val_i = 65535 },
      .val_default = { .val_i = 10    },
      .help_string = TRS("Distance from the top text border to the image border"),
    },
    {
      .name =        "border_bottom",
      .long_name =   TRS("Bottom border"),
      .type =        BG_PARAMETER_INT,
      .val_min =     { .val_i = 0     },
      .val_max =     { .val_i = 65535 },
      .val_default = { .val_i = 10    },
      .help_string = TRS("Distance from the bottom text border to the image border"),
    },
    {
      .name =        "duration",
      .long_name =   TRS("Duration (milliseconds)"),
      .type =        BG_PARAMETER_INT,
      .val_min =     { .val_i = 0 },
      .val_max =     { .val_i = 10000 },
      .val_default = { .val_i = 2000 },
    },
    { /* End of parameters */ }
  };

const bg_parameter_info_t * bg_osd_get_parameters(bg_osd_t * osd)
  {
  return parameters;
  }

void bg_osd_set_parameter(void * data, const char * name,
                          const bg_parameter_value_t * val)
  {
  bg_osd_t * osd;
  if(!name)
    return;
  osd = (bg_osd_t*)data;

  if(!strcmp(name, "enable_osd"))
    osd->enable = val->val_i;
  else if(!strcmp(name, "duration"))
    osd->duration = val->val_i * ((GAVL_TIME_SCALE) / 1000);
  else
    bg_text_renderer_set_parameter(osd->renderer, name, val);
  }

void bg_osd_set_overlay(bg_osd_t * osd, gavl_overlay_t * ovl)
  {
  osd->ovl = ovl;
  ovl->timestamp = -1;
  }

void bg_osd_init(bg_osd_t * osd, const gavl_video_format_t * format,
                 gavl_video_format_t * overlay_format)
  {
  bg_text_renderer_init(osd->renderer, format, overlay_format);
  }

int bg_osd_overlay_valid(bg_osd_t * osd)
  {
  gavl_time_t t;
  
  if(!osd->enable || (osd->ovl->timestamp < 0))
    return 0;

  t = gavl_timer_get(osd->timer);
  
  if(t < osd->ovl->timestamp)
    return 0;
  
  else if(t > osd->ovl->timestamp + osd->ovl->duration)
    {
    osd->ovl->timestamp = -1;
    return 0;
    }
  return 1;
  }

#define FLOAT_BAR_SIZE       18
#define FLOAT_BAR_SIZE_TOTAL 20

static void print_float(bg_osd_t * osd, float val, char c)
  {
  char _buf[FLOAT_BAR_SIZE_TOTAL+3];
  char * buf = _buf;
  int i, val_i;

  *buf = c; buf++;
  *buf = '\n'; buf++;
  
  *buf = '['; buf++;

  val_i = (int)(val * FLOAT_BAR_SIZE + 0.5);

  if(val_i > FLOAT_BAR_SIZE)
    val_i = FLOAT_BAR_SIZE;
  
  for(i = 0; i < val_i; i++)
    {
    *buf = '|'; buf++;
    }
  for(i = val_i; i < FLOAT_BAR_SIZE; i++)
    {
    *buf = '.'; buf++;
    }
  *buf = ']'; buf++;
  *buf = '\0';
  
  bg_text_renderer_render(osd->renderer, _buf, osd->ovl);

  osd->ovl->timestamp = gavl_timer_get(osd->timer);
  osd->ovl->duration = osd->duration;

  osd->changed = 1;
  }

int bg_osd_changed(bg_osd_t * osd)
  {
  if(osd->changed)
    {
    osd->changed = 0;
    return 1;
    }
  return 0;
  }

void bg_osd_set_volume_changed(bg_osd_t * osd, float val)
  {
  if(!osd->enable)
    return;
  print_float(osd, val, 'V');
  }

void bg_osd_set_brightness_changed(bg_osd_t * osd, float val)
  {
  if(!osd->enable)
    return;
  print_float(osd, val, 'B');

  }

void bg_osd_set_contrast_changed(bg_osd_t * osd, float val)
  {
  if(!osd->enable)
    return;
  print_float(osd, val, 'C');
  }

void bg_osd_set_saturation_changed(bg_osd_t * osd, float val)
  {
  if(!osd->enable)
    return;
  print_float(osd, val, 'S');
  }

