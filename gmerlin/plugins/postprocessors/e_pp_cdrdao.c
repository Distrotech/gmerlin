/*****************************************************************
 
  e_pp_cdrdao.c
 
  Copyright (c) 2006 by Burkhard Plaum - plaum@ipf.uni-stuttgart.de
 
  http://gmerlin.sourceforge.net
 
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
 
  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111, USA.
 
*****************************************************************/

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <config.h>
#include <plugin.h>
#include <utils.h>
#include "cdrdao_common.h"

typedef struct
  {
  int pre_gap;
  int use_cdtext;
  char * toc_file;
  int try_album;

  bg_cdrdao_t * cdr;
  char * error_msg;
  bg_e_pp_callbacks_t * callbacks;

  struct
    {
    bg_metadata_t metadata;
    char * filename;
    } * tracks;
  int num_tracks;
  } cdrdao_t;

static void free_tracks(cdrdao_t * c)
  {
  int i;
  if(c->num_tracks)
    {
    for(i = 0; i < c->num_tracks; i++)
      {
      bg_metadata_free(&(c->tracks[i].metadata));
      if(c->tracks[i].filename)
        free(c->tracks[i].filename);
      }
    free(c->tracks);
    c->tracks = NULL;
    }
  c->num_tracks = 0;
  }

static void * create_cdrdao()
  {
  cdrdao_t * ret;
  ret = calloc(1, sizeof(*ret));
  ret->cdr = bg_cdrdao_create();
  return ret;
  }

static void destroy_cdrdao(void * priv)
  {
  cdrdao_t * cdrdao;
  cdrdao = (cdrdao_t*)priv;
  bg_cdrdao_destroy(cdrdao->cdr);
  if(cdrdao->toc_file) free(cdrdao->toc_file);
  free(cdrdao);
  }

static const char * get_error_cdrdao(void * priv)
  {
  cdrdao_t * cdrdao;
  cdrdao = (cdrdao_t*)priv;
  return cdrdao->error_msg;
  }

static bg_parameter_info_t parameters[] =
  {
    {
      name: "cda",
      long_name: "Audio CD options",
      type: BG_PARAMETER_SECTION,
    },
    {
      name: "toc_file",
      long_name: "TOC file",
      type: BG_PARAMETER_STRING,
      val_default: { val_str: "audiocd.toc" },
    },
    {
      name: "pre_gap",
      long_name: "Gap between tracks",
      type: BG_PARAMETER_INT,
      val_default: { val_i: 150 },
      help_string: "Pre gap of each track in CD frames (1/75 seconds). Default is 150 (2 sec)."
    },
    {
      name: "use_cdtext",
      long_name: "Write CD-Text",
      type: BG_PARAMETER_CHECKBUTTON,
    },
    {
      name: "try_album",
      long_name: "Try album",
      type: BG_PARAMETER_CHECKBUTTON,
      help_string: "When encoding CDText, we assume a compilation by default (e.g. different\
 artists for each track). By enabling this option, we'll write CDText for a regular album if:\n\
1. The album string for each track is present and equal\n\
2. The artist string for each track is present or equal",\
    },
    CDRDAO_PARAMS,
    { /* End of parameters */ },
  };

static bg_parameter_info_t * get_parameters_cdrdao(void * data)
  {
  return parameters;
  }

static void set_parameter_cdrdao(void * data, char * name, bg_parameter_value_t * v)
  {
  cdrdao_t * cdrdao;
  cdrdao = (cdrdao_t*)data;
  if(!name)
    return;
  if(!strcmp(name, "use_cdtext"))
    cdrdao->use_cdtext = v->val_i;
  else if(!strcmp(name, "pre_gap"))
    cdrdao->pre_gap = v->val_i;
  else if(!strcmp(name, "try_album"))
    cdrdao->try_album = v->val_i;
  else if(!strcmp(name, "toc_file"))
    cdrdao->toc_file = bg_strdup(cdrdao->toc_file, v->val_str);
  }

static void set_callbacks_cdrdao(void * data, bg_e_pp_callbacks_t * callbacks)
  {
  cdrdao_t * cdrdao;
  cdrdao = (cdrdao_t*)data;
  cdrdao->callbacks = callbacks;
  }

static int init_cdrdao(void * data)
  {
  cdrdao_t * cdrdao;
  cdrdao = (cdrdao_t*)data;
  free_tracks(cdrdao);
  return 1;
  }

void add_track_cdrdao(void * data, const char * filename,
                      bg_metadata_t * metadata)
  {
  cdrdao_t * cdrdao;
  cdrdao = (cdrdao_t*)data;

  cdrdao->tracks = realloc(cdrdao->tracks,
                           (cdrdao->num_tracks+1)*sizeof(*cdrdao->tracks));

  memset(cdrdao->tracks + cdrdao->num_tracks, 0, sizeof(*cdrdao->tracks));

  bg_metadata_copy(&(cdrdao->tracks[cdrdao->num_tracks].metadata),
                   metadata);
  cdrdao->tracks[cdrdao->num_tracks].filename = bg_strdup((char*)0, filename);
  cdrdao->num_tracks++;
  }

static void execute_cdrdao(void * data, const char * directory, int cleanup)
  {
  
  }

bg_encoder_pp_plugin_t the_plugin =
  {
    common:
    {
      name:              "e_pp_cdrdao", /* Unique short name */
      long_name:         "Audio CD generator/burner",
      mimetypes:         NULL,
      type:              BG_PLUGIN_ENCODER_PP,
      create:            create_cdrdao,
      destroy:           destroy_cdrdao,
      get_error:         get_error_cdrdao,
      get_parameters:    get_parameters_cdrdao,
      set_parameter:     set_parameter_cdrdao,
      priority:          1,
    },
    max_audio_streams:   1,
    max_video_streams:   0,

    set_callbacks:       set_callbacks_cdrdao,
    init:                init_cdrdao,
    add_track:           add_track_cdrdao,
    execute:             execute_cdrdao,
    
  };

/* Include this into all plugin modules exactly once
   to let the plugin loader obtain the API version */
BG_GET_PLUGIN_API_VERSION;
