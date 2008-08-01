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

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <config.h>
#include <translation.h>
#include <plugin.h>
#include <utils.h>
#include <log.h>
#include <bggavl.h>

#define LOG_DOMAIN "fv_tctweak"

#define MODE_OFF              0
#define MODE_INTERPOLATE      1
#define MODE_REMOVE_REDUNDANT 2
#define MODE_REMOVE_ALL       3
#define MODE_ADD              4
#define MODE_ADD_FIRST        5

typedef struct
  {
  bg_read_video_func_t read_func;
  void * read_data;
  int read_stream;
  
  gavl_video_format_t format;
  gavl_timecode_t last_timecode;
  int mode;

  int int_framerate;
  int drop;
  int hours, minutes, seconds, frames;
  gavl_timecode_t first_timecode;
  } tc_priv_t;

static void * create_tctweak()
  {
  tc_priv_t * ret;
  ret = calloc(1, sizeof(*ret));
  return ret;
  }

static void destroy_tctweak(void * priv)
  {
  tc_priv_t * vp;
  vp = (tc_priv_t *)priv;
  free(vp);
  }

static const bg_parameter_info_t parameters[] =
  {
    {
      .gettext_domain = PACKAGE,
      .gettext_directory = LOCALE_DIR,
      .name = "mode",
      .long_name = TRS("Mode"),
      .type = BG_PARAMETER_STRINGLIST,
      .multi_names = (const char*[]){ "off",
                                "interpolate",
                                "remove_redundant",
                                "remove_all",
                                "add",
                                "add_first",
                                (char*)0 },
      .multi_labels = (const char*[]){TRS("Do nothing"),
                                      TRS("Interpolate nonexisting"),
                                      TRS("Remove redundant"),
                                      TRS("Remove all"),
                                      TRS("Add new"),
                                      TRS("Add new (first only)"),
                                      (char*)0 },
      .val_default  = { .val_str = "off" },
    },
    {
      .name      = "int_framerate",
      .long_name = "Integer framerate",
      .type      = BG_PARAMETER_INT,
      .val_min     = { .val_i = 1 },
      .val_max     = { .val_i = 999 },
      .val_default = { .val_i = 25 },
      .help_string = TRS("Set the integer framerate used when adding new timecodes"),
    },
    {
      .name      = "drop",
      .long_name = "Drop frame",
      .type      = BG_PARAMETER_CHECKBUTTON,
      .help_string = TRS("Set the if drop frame is used when adding new timecodes"),
    },
    {
      .name      = "hours",
      .long_name = "Start hour",
      .type      = BG_PARAMETER_INT,
      .val_min     = { .val_i = 0 },
      .val_max     = { .val_i = 23 },
      .val_default = { .val_i = 0 },
      .help_string = TRS("Set the start hours used when adding new timecodes"),
    },
    {
      .name      = "minutes",
      .long_name = "Start minute",
      .type      = BG_PARAMETER_INT,
      .val_min     = { .val_i = 0 },
      .val_max     = { .val_i = 59 },
      .val_default = { .val_i = 0 },
      .help_string = TRS("Set the start minutes used when adding new timecodes"),
    },
    {
      .name      = "seconds",
      .long_name = "Start second",
      .type      = BG_PARAMETER_INT,
      .val_min     = { .val_i = 0 },
      .val_max     = { .val_i = 59 },
      .val_default = { .val_i = 0 },
      .help_string = TRS("Set the start seconds used when adding new timecodes"),
    },
    {
      .name      = "frames",
      .long_name = "Start frames",
      .type      = BG_PARAMETER_INT,
      .val_min     = { .val_i = 0 },
      .val_max     = { .val_i = 999 },
      .val_default = { .val_i = 0 },
      .help_string = TRS("Set the start frames used when adding new timecodes"),
    },
    { /* End of parameters */ },
  };

static const bg_parameter_info_t * get_parameters_tctweak(void * priv)
  {
  return parameters;
  }

static void
set_parameter_tctweak(void * priv, const char * name,
                      const bg_parameter_value_t * val)
  {
  tc_priv_t * vp;
  vp = (tc_priv_t *)priv;

  if(!name)
    return;
  if(!strcmp(name, "mode"))
    {
    if(!strcmp(val->val_str, "off"))
      vp->mode = MODE_OFF;
    else if(!strcmp(val->val_str, "interpolate"))
      vp->mode = MODE_INTERPOLATE;
    else if(!strcmp(val->val_str, "remove_redundant"))
      vp->mode = MODE_REMOVE_REDUNDANT;
    else if(!strcmp(val->val_str, "remove_all"))
      vp->mode = MODE_REMOVE_ALL;
    else if(!strcmp(val->val_str, "add"))
      vp->mode = MODE_ADD;
    else if(!strcmp(val->val_str, "add_first"))
      vp->mode = MODE_ADD_FIRST;
    }
  else if(!strcmp(name, "int_framerate"))
    vp->int_framerate = val->val_i;
  else if(!strcmp(name, "drop"))
    vp->drop = val->val_i;
  else if(!strcmp(name, "hours"))
    vp->hours = val->val_i;
  else if(!strcmp(name, "minutes"))
    vp->minutes = val->val_i;
  else if(!strcmp(name, "seconds"))
    vp->seconds = val->val_i;
  else if(!strcmp(name, "frames"))
    vp->frames = val->val_i;
  
  }

static void connect_input_port_tctweak(void * priv,
                                    bg_read_video_func_t func,
                                    void * data, int stream, int port)
  {
  tc_priv_t * vp;
  vp = (tc_priv_t *)priv;

  if(!port)
    {
    vp->read_func = func;
    vp->read_data = data;
    vp->read_stream = stream;
    }
  
  }

static void set_input_format_tctweak(void * priv,
                                gavl_video_format_t * format, int port)
  {
  tc_priv_t * vp;
  vp = (tc_priv_t *)priv;
  
  if(port)
    return;
  
  gavl_video_format_copy(&vp->format, format);
  
  vp->last_timecode = GAVL_TIMECODE_UNDEFINED;
  switch(vp->mode)
    {
    case MODE_OFF:
    case MODE_INTERPOLATE:
    case MODE_REMOVE_REDUNDANT:
      break;
    case MODE_REMOVE_ALL:
      vp->format.timecode_format.int_framerate = 0;
      vp->format.timecode_format.flags         = 0;
      break;
    case MODE_ADD:
    case MODE_ADD_FIRST:
      vp->format.timecode_format.int_framerate = vp->int_framerate;
      vp->format.timecode_format.flags = 0;
      if(vp->drop)
        vp->format.timecode_format.flags |= GAVL_TIMECODE_DROP_FRAME;
      gavl_timecode_from_hmsf(&vp->first_timecode,
                              vp->hours,
                              vp->minutes,
                              vp->seconds,
                              vp->frames);
      break;
    }
  }

static void get_output_format_tctweak(void * priv,
                                 gavl_video_format_t * format)
  {
  tc_priv_t * vp;
  vp = (tc_priv_t *)priv;
  
  gavl_video_format_copy(format, &vp->format);
  }

static gavl_timecode_t do_increment(tc_priv_t * vp, gavl_timecode_t tc)
  {
  int64_t frames;
  frames = gavl_timecode_to_framecount(&vp->format.timecode_format,
                                       tc);
  frames++;
  return gavl_timecode_from_framecount(&vp->format.timecode_format,
                                       frames);
  }

static int read_video_tctweak(void * priv, gavl_video_frame_t * frame,
                         int stream)
  {
  tc_priv_t * vp;
  gavl_timecode_t tc;
  vp = (tc_priv_t *)priv;

  if(!vp->read_func(vp->read_data, frame, vp->read_stream))
    return 0;
  
  if(!vp->format.timecode_format.int_framerate)
    return 1;
  
  switch(vp->mode)
    {
    case MODE_OFF:
      break;
    case MODE_INTERPOLATE:
      if(frame->timecode == GAVL_TIMECODE_UNDEFINED)
        {
        if(vp->last_timecode)
          frame->timecode = do_increment(vp, vp->last_timecode);
        }
      vp->last_timecode = frame->timecode;
      break;
    case MODE_REMOVE_REDUNDANT:
      if(frame->timecode != GAVL_TIMECODE_UNDEFINED)
        {
        if(vp->last_timecode == GAVL_TIMECODE_UNDEFINED)
          {
          vp->last_timecode = frame->timecode;
          return 1;
          }
        tc = do_increment(vp, vp->last_timecode);
        if(tc == frame->timecode)
          {
          frame->timecode = GAVL_TIMECODE_UNDEFINED;
          }
        else
          {
          fprintf(stderr, "Non continous timecode: ");
          gavl_timecode_dump(&vp->format.timecode_format, tc);
          fprintf(stderr, " != ");
          gavl_timecode_dump(&vp->format.timecode_format, frame->timecode);
          fprintf(stderr, "\n");
          }
        vp->last_timecode = tc;
        }
      else if(vp->last_timecode != GAVL_TIMECODE_UNDEFINED)
        vp->last_timecode = do_increment(vp, vp->last_timecode);
      break;
    case MODE_REMOVE_ALL:
      frame->timecode = GAVL_TIMECODE_UNDEFINED;
      break;
    case MODE_ADD:
    case MODE_ADD_FIRST:
      if(vp->last_timecode == GAVL_TIMECODE_UNDEFINED)
        {
        frame->timecode = vp->first_timecode;
        vp->last_timecode = frame->timecode;
        }
      else if(vp->mode == MODE_ADD)
        {
        frame->timecode = do_increment(vp, vp->last_timecode);
        vp->last_timecode = frame->timecode;
        }
      else
        frame->timecode = GAVL_TIMECODE_UNDEFINED;
      break;
    }
  return 1;
  }

const bg_fv_plugin_t the_plugin = 
  {
    .common =
    {
      BG_LOCALE,
      .name =      "fv_tctweak",
      .long_name = TRS("Tweak timecodes"),
      .description = TRS("Add/remove/interpolate timecodes"),
      .type =     BG_PLUGIN_FILTER_VIDEO,
      .flags =    BG_PLUGIN_FILTER_1,
      .create =   create_tctweak,
      .destroy =   destroy_tctweak,
      .get_parameters =   get_parameters_tctweak,
      .set_parameter =    set_parameter_tctweak,
      .priority =         1,
    },
    
    .connect_input_port = connect_input_port_tctweak,
    
    .set_input_format = set_input_format_tctweak,
    .get_output_format = get_output_format_tctweak,

    .read_video = read_video_tctweak,
    
  };

/* Include this into all plugin modules exactly once
   to let the plugin loader obtain the API version */
BG_GET_PLUGIN_API_VERSION;
