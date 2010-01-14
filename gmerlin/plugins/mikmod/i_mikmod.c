/*****************************************************************
 * gmerlin - a general purpose multimedia framework and applications
 *
 * Copyright (c) 2001 - 2010 Members of the Gmerlin project
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

#include <string.h>
#include <unistd.h>
#include <signal.h>

#include <config.h>
#include <gmerlin/translation.h>
#include <gmerlin/plugin.h>
#include <gmerlin/utils.h>

#include <gmerlin/subprocess.h>
#include <gmerlin/log.h>

#define LOG_DOMAIN "i_mikmod"

// the last number ist mono(1)/stereo(2)
#define MONO8      81
#define MONO16     161
#define STEREO8    82
#define STEREO16   162

typedef struct
  {
  bg_subprocess_t * proc;  
  bg_track_info_t track_info;
  int frequency;
  int output;
  int hidden_patterns;
  int force_volume;
  int use_surround;
  int use_interpolate;

  int block_align;
  int eof;
  } i_mikmod_t;

#ifdef dump
static void dump_mikmod(void * data, const char * path)
  {
  FILE * out = stderr;
  i_mikmod_t * mikmod;
  mikmod = (i_mikmod_t*)data;
   
  fprintf(out, "  DUMP_MIKMOD:\n");
  fprintf(out, "    Path:            %s\n", path);
  fprintf(out, "    Frequency:       %d\n", mikmod->frequency);

  if(mikmod->output == MONO8)
    fprintf(out, "    Output:          8m\n");
  else if(mikmod->output == MONO16)
    fprintf(out, "    Output:          8s\n");
  else if(mikmod->output == STEREO8)
    fprintf(out, "    Output:          16m\n");
  else if(mikmod->output == STEREO16)
    fprintf(out, "    Output:          16s\n");
  else
    fprintf(out, "    Output:          buggy mikmod.c\n");   

  fprintf(out, "    Use surround:    %d\n", mikmod->use_surround);
  fprintf(out, "    Hidden patterns: %d\n", mikmod->hidden_patterns);
  fprintf(out, "    Force volume:    %d\n", mikmod->force_volume);
  fprintf(out, "    Use interpolate: %d\n", mikmod->use_interpolate);
  }
#endif

static void * create_mikmod()
  {
  i_mikmod_t * ret = calloc(1, sizeof(*ret));
  return ret;
  }

  
/* Read one audio frame (returns FALSE on EOF) */
static int read_audio_samples_mikmod(void * data, gavl_audio_frame_t * f, int stream,
                                     int num_samples)
  {
  int result;
  i_mikmod_t * e = (i_mikmod_t*)data;

  result = bg_subprocess_read_data(e->proc->stdout_fd,
                                   f->samples.u_8, num_samples * e->block_align);

  if(result < 0)
    return 0;

  if(result < num_samples * e->block_align)
    e->eof = 1;
  
  f->valid_samples = result / e->block_align;
  return f->valid_samples;
  }

// arg = path of mod
static int open_mikmod(void * data, const char * arg)
  {
  int result;
  char *command;
  i_mikmod_t * mik = (i_mikmod_t*)data;
  gavl_audio_frame_t * test_frame;
  
  // if no mikmod installed 
  if(!bg_search_file_exec("mikmod", (char**)0))
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Cannot find mikmod executable");
    return 0;
    }

  /* Create track infos */
  mik->track_info.duration = GAVL_TIME_UNDEFINED;
  mik->track_info.num_audio_streams = 1;
  mik->track_info.audio_streams = calloc(1, sizeof(*(mik->track_info.audio_streams)));
  
  mik->track_info.audio_streams[0].format.samplerate = mik->frequency;

  if(mik->output == MONO8 || mik->output == MONO16)
    mik->track_info.audio_streams[0].format.num_channels = 1;
  else if(mik->output == STEREO8 || mik->output == STEREO16)
    mik->track_info.audio_streams[0].format.num_channels = 2;

  if(mik->output == MONO8 || mik->output == STEREO8)
    mik->track_info.audio_streams[0].format.sample_format = GAVL_SAMPLE_U8;
  else if(mik->output == MONO16 || mik->output == STEREO16)
    mik->track_info.audio_streams[0].format.sample_format = GAVL_SAMPLE_S16;

  mik->track_info.audio_streams[0].format.interleave_mode = GAVL_INTERLEAVE_ALL;
  mik->track_info.audio_streams[0].description = bg_strdup(NULL, "mikmod audio");
  mik->track_info.audio_streams[0].format.samples_per_frame = 1024;
  
  gavl_set_channel_setup(&(mik->track_info.audio_streams[0].format));

  command = bg_sprintf("mikmod -q --playmode 0 --noloops --exitafter -f %d -d stdout", mik->frequency);
    
  if(mik->output == MONO8)
    {
    command = bg_strcat(command, " -o 8m");
    mik->block_align = 1;
    }
  else if(mik->output == MONO16)
    {
    command = bg_strcat(command, " -o 16m");
    mik->block_align = 2;
    }
  else if(mik->output == STEREO8)
    {
    command = bg_strcat(command, " -o 8s");
    mik->block_align = 2;
    }
  else if(mik->output == STEREO16)
    {
    command = bg_strcat(command, " -o 16s");
    mik->block_align = 4;
    }
  if(mik->use_surround)
    command = bg_strcat(command, " -s");
  if(mik->use_interpolate)
    command = bg_strcat(command, " -i");
  if(mik->force_volume)
    command = bg_strcat(command, " -fa");
  if(mik->hidden_patterns)
    command = bg_strcat(command, " -c");

  command = bg_strcat(command, " ");
  command = bg_strcat(command, arg);

  /* Test file compatibility */
  mik->proc = bg_subprocess_create(command, 0, 1, 0);
  test_frame = gavl_audio_frame_create(&mik->track_info.audio_streams[0].format);

  result = read_audio_samples_mikmod(data, test_frame, 0, 1);

  if(result)
    {
    bg_subprocess_kill(mik->proc, SIGKILL);
    bg_subprocess_close(mik->proc);
    mik->proc = bg_subprocess_create(command, 0, 1, 0);
    }
  else
    {
    bg_subprocess_close(mik->proc);
    mik->proc = (bg_subprocess_t*)0;
    bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Unrecognized fileformat");
    }

  gavl_audio_frame_destroy(test_frame);
  
  free(command);
#ifdef dump
  dump_mikmod(data, (const char*)arg);
#endif
  return result;
  }

static int get_num_tracks_mikmod(void * data)
  {
  return 1;
  }

static bg_track_info_t * get_track_info_mikmod(void * data, int track)
  {
  i_mikmod_t * e = (i_mikmod_t*)data;
  return &(e->track_info);
  }


static void close_mikmod(void * data)
  {
  i_mikmod_t * e = (i_mikmod_t*)data;
  if(e->proc)
    {
    if(!e->eof)
      bg_subprocess_kill(e->proc, SIGKILL);
    bg_subprocess_close(e->proc);
    e->proc = (bg_subprocess_t*)0;
    }
  bg_track_info_free(&(e->track_info));
  }

static void destroy_mikmod(void * data)
  {
  i_mikmod_t * e = (i_mikmod_t*)data;
  close_mikmod(data);
  free(e);
  }

/* Configuration */

static const bg_parameter_info_t parameters[] = 
  {
    {
      .name =        "output",
      .long_name =   TRS("Output format"),
      .type =        BG_PARAMETER_STRINGLIST,
      .multi_names = (char const *[]){ "mono8", "stereo8", "mono16", "stereo16", (char*)0 },
      .multi_labels =  (char const *[]){ TRS("Mono 8bit"), TRS("Stereo 8bit"), TRS("Mono 16bit"), TRS("Stereo 16bit"), (char*)0 },
      
      .val_default = { .val_str = "stereo16" },
    },
      {
      .name =        "mixing_frequency",
      .long_name =   TRS("Samplerate"),
      .type =         BG_PARAMETER_INT,
      .val_min =     { .val_i = 4000 },
      .val_max =     { .val_i = 60000 },
      .val_default = { .val_i = 44100 }      
      // .help_string = "Mixing frequency for the Track"
    },
    {
      .name = "look_for_hidden_patterns_in_module",
      .long_name = TRS("Look for hidden patterns in module"),
      .opt =  "hidden",
      .type = BG_PARAMETER_CHECKBUTTON,
    },
    {
      .name = "use_surround_mixing",
      .long_name = TRS("Use surround mixing"),
      .opt =  "sur",
      .type = BG_PARAMETER_CHECKBUTTON,
    },
    {
      .name = "force_volume_fade_at_the_end_of_module",
      .long_name = TRS("Force volume fade at the end of module"),
      .opt =  "fade",
      .type = BG_PARAMETER_CHECKBUTTON,
    },
    {
      .name = "use_interpolate_mixing",
      .long_name = TRS("Use interpolate mixing"),
      .opt =  "interpol",
      .type = BG_PARAMETER_CHECKBUTTON,
    },
    { /* End of parameters */ }
  };

static const bg_parameter_info_t * get_parameters_mikmod(void * data)
  {
  return parameters;
  }

static void set_parameter_mikmod(void * data, const char * name,
                                 const bg_parameter_value_t * val)
  {
  i_mikmod_t * mikmod;
  mikmod = (i_mikmod_t*)data;
  if(!name)
    return;
  else if(!strcmp(name, "output"))
    {
    if(!strcmp(val->val_str, "mono8"))
      mikmod->output = MONO8;
    else if(!strcmp(val->val_str, "mono16"))
      mikmod->output = MONO16;
    else if(!strcmp(val->val_str, "stereo8"))
      mikmod->output = STEREO8;
    else if(!strcmp(val->val_str, "stereo16"))
      mikmod->output = STEREO16;
    }
  else if(!strcmp(name, "mixing_frequency"))
    mikmod->frequency = val->val_i;
  else if(!strcmp(name, "look_for_hidden_patterns_in_module"))
    mikmod->hidden_patterns = val->val_i;
  else if(!strcmp(name, "force_volume_fade_at_the_end_of_module"))
    mikmod->force_volume = val->val_i;
  else if(!strcmp(name, "use_interpolate_mixing"))
    mikmod->use_interpolate = val->val_i;
  else if(!strcmp(name, "use_surround_mixing"))
    mikmod->use_surround = val->val_i;
  }

static char const * const extensions = "it xm mod mtm  s3m stm ult far med dsm amf imf 669";

static const char * get_extensions(void * priv)
  {
  return extensions;
  }

const bg_input_plugin_t the_plugin =
  {
    .common =
    {
      BG_LOCALE,
      .name =            "i_mikmod",       /* Unique short name */
      .long_name =       TRS("mikmod input plugin"),
      .description =     TRS("Simple wrapper, which calls the mikmod program"),
      .type =            BG_PLUGIN_INPUT,
      .flags =           BG_PLUGIN_FILE,
      .priority =        1,
      .create =          create_mikmod,
      .destroy =         destroy_mikmod,
      .get_parameters =  get_parameters_mikmod,
      .set_parameter =   set_parameter_mikmod,
    },
    .get_extensions =    get_extensions,
    .open =              open_mikmod,
    .get_num_tracks =    get_num_tracks_mikmod,
    .get_track_info =    get_track_info_mikmod,
    
    .read_audio          = read_audio_samples_mikmod,
    .close =              close_mikmod
    
  };

/* Include this into all plugin modules exactly once
   to let the plugin loader obtain the API version */
BG_GET_PLUGIN_API_VERSION;
