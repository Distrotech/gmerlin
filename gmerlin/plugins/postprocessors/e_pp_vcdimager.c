/*****************************************************************
 
  e_pp_vcdimager.c
 
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
#include <log.h>
#include <subprocess.h>
#include "cdrdao_common.h"

#define LOG_DOMAIN "vcdimager"

/* Driver for vcdxgen and vcdxbuild */

typedef struct
  {
  char * vcd_version;
  char * error_msg;

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
  vcdimager = (vcdimager_t*)priv;

  FREE(vcdimager->xml_file);
  FREE(vcdimager->bin_file);
  FREE(vcdimager->cue_file);
  FREE(vcdimager->volume_label);
  FREE(vcdimager->error_msg);
  FREE(vcdimager->vcd_version);

  bg_cdrdao_destroy(vcdimager->cdr);

  free(vcdimager);
  }

#undef FREE

static const char * get_error_vcdimager(void * priv)
  {
  vcdimager_t * vcdimager;
  vcdimager = (vcdimager_t*)priv;
  return vcdimager->error_msg;
  }


static bg_parameter_info_t parameters[] =
  {
    {
      name: "vcdimager",
      long_name: "VCD options",
      type: BG_PARAMETER_SECTION,
    },
    {
      name: "vcd_version",
      long_name: "Format",
      type: BG_PARAMETER_STRINGLIST,
      val_default: { val_str: "vcd2" },
      multi_names: (char*[]){ "vcd11", "vcd2", "svcd", "hqsvcd", (char*)0 },
      multi_labels: (char*[]){ "VCD 1.1", "VCD 2.0", "SVCD", "HQSVCD", (char*)0 },
    },
    {
      name: "volume_label",
      long_name: "ISO 9660 volume label",
      type: BG_PARAMETER_STRING,
      val_default: { val_str: "VIDEOCD" },
    },
    {
      name: "xml_file",
      long_name: "Xml file",
      type: BG_PARAMETER_STRING,
      val_default: { val_str: "videocd.xml" },
    },
    {
      name: "bin_file",
      long_name: "Bin file",
      type: BG_PARAMETER_STRING,
      val_default: { val_str: "videocd.bin" },
    },
    {
      name: "cue_file",
      long_name: "Cue file",
      type: BG_PARAMETER_STRING,
      val_default: { val_str: "videocd.cue" },
    },
    CDRDAO_PARAMS,
    { /* End of parameters */ },
  };

static bg_parameter_info_t * get_parameters_vcdimager(void * data)
  {
  return parameters;
  }

#define SET_STR(key) if(!strcmp(name, # key)) { vcd->key = bg_strdup(vcd->key, v->val_str); return; }

static void set_parameter_vcdimager(void * data, char * name, bg_parameter_value_t * v)
  {
  vcdimager_t * vcd = (vcdimager_t*)data;
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
  vcdimager = (vcdimager_t*)data;
  vcdimager->callbacks = callbacks;
  bg_cdrdao_set_callbacks(vcdimager->cdr, callbacks);
  
  }

static int init_vcdimager(void * data)
  {
  vcdimager_t * vcdimager;
  vcdimager = (vcdimager_t*)data;
#if 0
  if(!bg_search_file_exec("cdrdao", (char**)0) ||
     !bg_search_file_exec("vcdxgen", (char**)0) ||
     !bg_search_file_exec("vcdxbuild", (char**)0))
    return 0;
#endif
  free_tracks(vcdimager);
  
  return 1;
  }

static void add_track_vcdimager(void * data, const char * filename,
                                bg_metadata_t * metadata, int pp_only)
  {
  vcdimager_t * vcdimager;
  vcdimager = (vcdimager_t*)data;
  vcdimager->files = realloc(vcdimager->files,
                             sizeof(*(vcdimager->files)) * (vcdimager->num_files+1));
  vcdimager->files[vcdimager->num_files].name = bg_strdup((char*)0, filename);
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
    bg_log(log_level, LOG_DOMAIN, start);
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
        str = bg_sprintf("Scanning %s", id);
        vcdimager->callbacks->action_callback(vcdimager->callbacks->data,
                                              str);
        free(str);
        }
      if(!strncmp(start, "write\"", 6))
        {
        str = bg_sprintf("Writing image");
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
  char * commandline = (char*)0;
  int i;
  char * line = (char*)0;
  int line_alloc = 0;
  char * bin_file = (char*)0;
  char * cue_file = (char*)0;
  char * xml_file = (char*)0;
  vcdimager = (vcdimager_t*)data;
  
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
  commandline = (char*)0;
  
  while(bg_subprocess_read_line(proc->stderr, &line, &line_alloc, -1))
    {
    /* If we read something from stderr, we know it's an error */
    if(line && (*line != '\0'))
      {
      bg_log(BG_LOG_ERROR, LOG_DOMAIN, "vcdxgen failed: %s\n", line);
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
  while(bg_subprocess_read_line(proc->stdout, &line, &line_alloc, -1))
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
        bg_log(BG_LOG_INFO, LOG_DOMAIN, "Removing %s", vcdimager->files[i]);
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
  vcdimager = (vcdimager_t*)data;
  bg_cdrdao_stop(vcdimager->cdr);
  }


bg_encoder_pp_plugin_t the_plugin =
  {
    common:
    {
      name:              "e_pp_vcdimager", /* Unique short name */
      long_name:         "VCD image generator/burner",
      mimetypes:         NULL,
      extensions:        "mpg",
      type:              BG_PLUGIN_ENCODER_PP,
      flags:             0,
      create:            create_vcdimager,
      destroy:           destroy_vcdimager,
      get_error:         get_error_vcdimager,
      get_parameters:    get_parameters_vcdimager,
      set_parameter:     set_parameter_vcdimager,
      priority:          1,
    },
    max_audio_streams:   1,
    max_video_streams:   0,

    set_callbacks:       set_callbacks_vcdimager,
    init:                init_vcdimager,
    add_track:           add_track_vcdimager,
    run:                 run_vcdimager,
    stop:                stop_vcdimager,
  };

/* Include this into all plugin modules exactly once
   to let the plugin loader obtain the API version */
BG_GET_PLUGIN_API_VERSION;
