/*****************************************************************
 * gmerlin - a general purpose multimedia framework and applications
 *
 * Copyright (c) 2001 - 2011 Members of the Gmerlin project
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

#include <config.h>
#include <gmerlin/translation.h>

#include <gmerlin/plugin.h>
#include <gmerlin/utils.h>
#include <gmerlin/log.h>

#define LOG_DOMAIN "i_edl"

typedef struct
  {
  bg_edl_t * edl;
  } edl_t;

static void * create_edl()
  {
  edl_t * ret = calloc(1, sizeof(*ret));
  return ret;
  }

static void close_edl(void * data)
  {
  edl_t * e = (edl_t*)data;
  if(e->edl)
    {
    bg_edl_destroy(e->edl);
    e->edl = NULL;
    }
  }

static void destroy_edl(void * data)
  {
  edl_t * e = (edl_t*)data;
  close_edl(e);
  free(e);
  }

static int open_edl(void * data, const char * arg)
  {
  edl_t * e = (edl_t*)data;
  e->edl = bg_edl_load(arg);
  if(e->edl)
    {
    //    bg_edl_dump(e->edl);
    return 1;
    }
  return 0;
  }

static int get_num_tracks_edl(void * data)
  {
  return 0;
  }

static const bg_edl_t * get_edl_edl(void * data)
  {
  edl_t * e = (edl_t*)data;
  return e->edl;
  }


const bg_input_plugin_t the_plugin =
  {
    .common =
    {
      BG_LOCALE,
      .name =            "i_edl",       /* Unique short name */
      .long_name =       TRS("Parser for gmerlin EDLs"),
      .description =     TRS("This parses the XML file and exports an EDL, which can be played with the builtin EDL decoder."),
      .type =            BG_PLUGIN_INPUT,
      .flags =           BG_PLUGIN_FILE,
      .priority =        5,
      .create =          create_edl,
      .destroy =         destroy_edl,
      //      .get_parameters =  get_parameters_lqt,
      //      .set_parameter =   set_parameter_lqt,
    },

    //    .get_extensions =    get_extensions,
    .open =              open_edl,
    .get_num_tracks =    get_num_tracks_edl,
    .get_edl        =    get_edl_edl,
    
    .close =              close_edl,
    
  };

/* Include this into all plugin modules exactly once
   to let the plugin loader obtain the API version */
BG_GET_PLUGIN_API_VERSION;
