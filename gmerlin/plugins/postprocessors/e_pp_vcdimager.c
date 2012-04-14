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

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <config.h>
#include <gmerlin/translation.h>

#include <gmerlin/plugin.h>
#include <gmerlin/utils.h>
#include <gmerlin/log.h>
#include <gmerlin/subprocess.h>
#include "cdrdao_common.h"

#define LOG_DOMAIN "vcdimager"

/* Driver for vcdxgen and vcdxbuild */

typedef struct
  {
  char * vcd_version;

  char * bin_file;
  char * xml_file;
  char * cue_file;
  char * volume_label;

  bg_e_pp_callbacks_t * callbacks;

  bg_cdrdao_t * cdr;

  struct
    {
    char * name;
    int pp_only;
    } * files;
  int num_files;
  } vcdimager_t;

static void free_tracks(vcdimager_t * v)
  {
  int i;
  for(i = 0; i < v->num_files; i++)
    free(v->files[i].name);
  if(v->files)
    free(v->files);
  v->files = NULL;
  v->num_files = 0;
  }

static void * create_vcdimager()
  {
  vcdimager_t * ret;
  ret = calloc(1, sizeof(*ret));
  ret->cdr = bg_cdrdao_create();
  return ret;
  }

#define FREE(ptr) if(ptr) free(ptr);

static void destroy_vcdimager(void * priv)
  {
  vcdimager_t * vcdimager;
  vcdimager = priv;

  FREE(vcdimager->xml_file);
  FREE(vcdimager->bin_file);
  FREE(vcdimager->cue_file);
  FREE(vcdimager->volume_label);
  FREE(vcdimager->vcd_version);

  bg_cdrdao_destroy(vcdimager->cdr);

  free(vcdimager);
  }

#undef FREE



static const bg_parameter_info_t parameters[] =
  {
    {
      .name = "vcdimager",
      .long_name = TRS("VCD options"),
      .type = BG_PARAMETER_SECTION,
    },
    {
      .name = "vcd_version",
      .long_name = TRS("Format"),
      .type = BG_PARAMETER_STRINGLIST,
      .val_default = { .val_str = "vcd2" },
      .multi_names = (char const *[]){ "vcd11", "vcd2", "svcd", "hqsvcd", NULL },
      .multi_labels = (char const *[]){ TRS("VCD 1.1"), TRS("VCD 2.0"), TRS("SVCD"), TRS("HQSVCD"), NULL },
    },
    {
      .name = "volume_label",
      .long_name = TRS("ISO 9660 volume label"),
      .type = BG_PARAMETER_STRING,
      .val_default = { .val_str = "VIDEOCD" },
    },
    {
      .name = "xml_file",
      .long_name = TRS("Xml file"),
      .type = BG_PARAMETER_STRING,
      .val_default = { .val_str = "videocd.xml" },
    },
    {
      .name = "bin_file",
      .long_name = TRS("Bin file"),
      .type = BG_PARAMETER_STRING,
      .val_default = { .val_str = "videocd.bin" },
    },
    {
      .name = "cue_file",
      .long_name = TRS("Cue file"),
      .type = BG_PARAMETER_STRING,
      .val_default = { .val_str = "videocd.cue" },
    },
    CDRDAO_PARAMS,
    { /* End of parameters */ },
  };

static const bg_parameter_info_t * get_parameters_vcdimager(void * data)
  {
  return parameters;
  }

#define SET_STR(key) if(!strcmp(name, # key)) { vcd->key = bg_strdup(vcd->key, v->val_str); return; }

static void set_parameter_vcdimager(void * data, const char * name, const bg_parameter_value_t * v)
  {
  vcdimager_t * vcd = data;
  if(!name)
    return;
  SET_STR(bin_file);
  SET_STR(cue_file);
  SET_STR(xml_file);
  SET_STR(volume_label);
  SET_STR(vcd_version);
  
  bg_cdrdao_set_parameter(vcd->cdr, name, v);
  }

static void set_callbacks_vcdimager(void * data, bg_e_pp_callbacks_t * callbacks)
  {
  vcdimager_t * vcdimager;
  vcdimager = data;
  vcdimager->callbacks = callbacks;
  bg_cdrdao_set_callbacks(vcdimager->cdr, callbacks);
  
  }

static int init_vcdimager(void * data)
  {
  vcdimager_t * vcdimager;
  vcdimager = data;
#if 0
  if(!bg_search_file_exec("cdrdao", NULL) ||
     !bg_search_file_exec("vcdxgen", NULL) ||
     !bg_search_file_exec("vcdxbuild", NULL))
    return 0;
#endif
  free_tracks(vcdimager);
  
  return 1;
  }

static void add_track_vcdimager(void * data, const char * filename,
                                gavl_metadata_t * metadata, int pp_only)
  {
  vcdimager_t * vcdimager;
  vcdimager = data;
  vcdimager->files = realloc(vcdimager->files,
                             sizeof(*(vcdimager->files)) * (vcdimager->num_files+1));
  vcdimager->files[vcdimager->num_files].name = bg_strdup(NULL, filename);
  vcdimager->files[vcdimager->num_files].pp_only = pp_only;
  vcdimager->num_files++;
  }

static void parse_output_line(vcdimager_t * vcdimager, char * line)
  {
  int position, size;
  bg_log_level_t log_level = 0;
  char * start, *end, *id, * str;

  if(!strncmp(line, "<log ", 5))
    {
    if(!(start = strstr(line, "level=\"")))
      return;
    start += 7;
    if(!strncmp(start, "warning", 7))
      log_level = BG_LOG_WARNING;
    else if(!strncmp(start, "information", 11))
      log_level = BG_LOG_INFO;
    else if(!strncmp(start, "error", 5))
      log_level = BG_LOG_ERROR;
    
    if(!(start = strchr(start, '>')))
      return;
    start++;
    if(!(end = strstr(start, "</log>")))
      return;
    *end = '\0';
    bg_log(log_level, LOG_DOMAIN, "%s", start);
    }
  else if(!strncmp(line, "<progress ", 10))
    {
    if(!vcdimager->callbacks)
      return;
    
    if(!(start = strstr(line, "position=\"")))
      return;
    start += 10;
    position = atoi(start);
    if(!(start = strstr(line, "size=\"")))
      return;
    start += 6;
    size = atoi(start);
    if(!position && vcdimager->callbacks->action_callback)
      {
      if(!(start = strstr(line, "operation=\"")))
        return;
      start += 11;
      
      if(!strncmp(start, "scan\"", 5))
        {
        if(!(id = strstr(line, "id=\"")))
          return;
        id += 4;
        if(!(end = strchr(id, '"')))
          return;
        *end = '\0';
        str = bg_sprintf(TR("Scanning %s"), id);
        vcdimager->callbacks->action_callback(vcdimager->callbacks->data,
                                              str);
        free(str);
        }
      if(!strncmp(start, "write\"", 6))
        {
        str = bg_sprintf(TR("Writing image"));
        vcdimager->callbacks->action_callback(vcdimager->callbacks->data,
                                              str);
        free(str);
        }
      }
    if(vcdimager->callbacks->progress_callback)
      vcdimager->callbacks->progress_callback(vcdimager->callbacks->data,
                                              (float)position / (float)size);
    }
  }

static void run_vcdimager(void * data, const char * directory, int cleanup)
  {
  int err = 0;
  vcdimager_t * vcdimager;
  bg_subprocess_t * proc;
  char * str;
  char * commandline = NULL;
  int i;
  char * line = NULL;
  int line_alloc = 0;
  char * bin_file = NULL;
  char * cue_file = NULL;
  char * xml_file = NULL;
  vcdimager = data;
  
  /* Build vcdxgen commandline */

  //  bg_search_file_exec("vcdxgen", &commandline);

  xml_file = bg_sprintf("%s/%s", directory, vcdimager->xml_file);
    
  str = bg_sprintf("vcdxgen -o %s -t %s --iso-application-id=%s-%s",
                   xml_file, vcdimager->vcd_version, PACKAGE, VERSION);
  commandline = bg_strcat(commandline, str);
  free(str);

  if(vcdimager->volume_label)
    {
    str = bg_sprintf(" -l \"%s\"", vcdimager->volume_label);
    commandline = bg_strcat(commandline, str);
    free(str);
    }

  for(i = 0; i < vcdimager->num_files; i++)
    {
    str = bg_sprintf(" \"%s\"", vcdimager->files[i].name);
    commandline = bg_strcat(commandline, str);
    free(str);
    }
  
  proc = bg_subprocess_create(commandline, 0, 0, 1);
  free(commandline);
  commandline = NULL;
  
  while(bg_subprocess_read_line(proc->stderr_fd, &line, &line_alloc, -1))
    {
    /* If we read something from stderr, we know it's an error */
    if(line && (*line != '\0'))
      {
      bg_log(BG_LOG_ERROR, LOG_DOMAIN, "vcdxgen failed: %s", line);
      err = 1;
      }
    }
  bg_subprocess_close(proc);

  if(err)
    {
    if(cleanup)
      remove(xml_file);
    return;
    }
  /* Build vcdxbuild commandline */
  
  bg_search_file_exec("vcdxbuild", &commandline);
  
  bin_file = bg_sprintf("%s/%s", directory, vcdimager->bin_file);
  cue_file = bg_sprintf("%s/%s", directory, vcdimager->cue_file);
  
  str = bg_sprintf(" --gui -p -c %s -b %s %s/%s",
                   cue_file,
                   bin_file,
                   directory, vcdimager->xml_file);
  commandline = bg_strcat(commandline, str);
  free(str);

  proc = bg_subprocess_create(commandline, 0, 1, 0);
  free(commandline);
  while(bg_subprocess_read_line(proc->stdout_fd, &line, &line_alloc, -1))
    {
    parse_output_line(vcdimager, line);
    }
  bg_subprocess_close(proc); 

  /* If we reached this point, we can delete the mpg files as well as
     the xml file */

  if(cleanup)
    {
    for(i = 0; i < vcdimager->num_files; i++)
      {
      if(!vcdimager->files[i].pp_only)
        {
        bg_log(BG_LOG_INFO, LOG_DOMAIN, "Removing %s", vcdimager->files[i].name);
        remove(vcdimager->files[i].name);
        }
      }
    bg_log(BG_LOG_INFO, LOG_DOMAIN, "Removing %s", xml_file);
    remove(xml_file);
    }
  
  /* Run cdrdao */
  if(bg_cdrdao_run(vcdimager->cdr, cue_file) && cleanup)
    {
    bg_log(BG_LOG_INFO, LOG_DOMAIN, "Removing %s", bin_file);
    remove(bin_file);
    bg_log(BG_LOG_INFO, LOG_DOMAIN, "Removing %s", cue_file);
    remove(cue_file);
    }
  
  free(cue_file);
  free(bin_file);
  free(xml_file);
  
  }


static void stop_vcdimager(void * data)
  {
  vcdimager_t * vcdimager;
  vcdimager = data;
  bg_cdrdao_stop(vcdimager->cdr);
  }


const bg_encoder_pp_plugin_t the_plugin =
  {
    .common =
    {
      BG_LOCALE,
      .name =              "e_pp_vcdimager", /* Unique short name */
      .long_name =         TRS("VCD image generator/burner"),
      .description =       TRS("This is a frontend for generating (S)VCD images with the vcdimager tools (http://www.vcdimager.org). Burning with cdrdao (http://cdrdao.sourceforge.net) is also possible."),
      .type =              BG_PLUGIN_ENCODER_PP,
      .flags =             BG_PLUGIN_PP,
      .create =            create_vcdimager,
      .destroy =           destroy_vcdimager,
      .get_parameters =    get_parameters_vcdimager,
      .set_parameter =     set_parameter_vcdimager,
      .priority =          1,
    },
    .supported_extensions =        "mpg",
    .max_audio_streams =   1,
    .max_video_streams =   0,

    .set_callbacks =       set_callbacks_vcdimager,
    .init =                init_vcdimager,
    .add_track =           add_track_vcdimager,
    .run =                 run_vcdimager,
    .stop =                stop_vcdimager,
  };

/* Include this into all plugin modules exactly once
   to let the plugin loader obtain the API version */
BG_GET_PLUGIN_API_VERSION;
