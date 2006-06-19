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
#include <log.h>
#include "cdrdao_common.h"

#define LOG_DOMAIN "e_pp_cdrdao"

typedef struct
  {
  int pregap;
  int use_cdtext;
  char * toc_file;

  bg_cdrdao_t * cdr;
  char * error_msg;
  bg_e_pp_callbacks_t * callbacks;

  struct
    {
    bg_metadata_t metadata;
    char * filename;
    uint32_t duration; /* In samples */
    int pp_only;
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
  //  fprintf(stderr, "create_cdrdao %p\n", ret);
  return ret;
  }

static void destroy_cdrdao(void * priv)
  {
  cdrdao_t * cdrdao;
  cdrdao = (cdrdao_t*)priv;
  bg_cdrdao_destroy(cdrdao->cdr);
  if(cdrdao->toc_file) free(cdrdao->toc_file);
  //  fprintf(stderr, "destroy_cdrdao %p\n", cdrdao);
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
    cdrdao->pregap = v->val_i;
  else if(!strcmp(name, "toc_file"))
    cdrdao->toc_file = bg_strdup(cdrdao->toc_file, v->val_str);
  else
    bg_cdrdao_set_parameter(cdrdao->cdr, name, v);
  }

static void set_callbacks_cdrdao(void * data, bg_e_pp_callbacks_t * callbacks)
  {
  cdrdao_t * cdrdao;
  cdrdao = (cdrdao_t*)data;
  cdrdao->callbacks = callbacks;
  bg_cdrdao_set_callbacks(cdrdao->cdr, callbacks);
  }

static int init_cdrdao(void * data)
  {
  cdrdao_t * cdrdao;
  cdrdao = (cdrdao_t*)data;
  //  fprintf(stderr, "init_cdrdao\n");
  free_tracks(cdrdao);
  /* Check for cdrdao */
  if(!bg_search_file_exec("cdrdao", (char**)0))
    return 0;
  return 1;
  }

/*
 *  Micro wav parser: Gets the duration of the wave file.
 *  We need this to write it into the toc file for the case that
 *  pregaps are used
 */

typedef struct
  {
  char fourcc[4];
  uint32_t length;
  } chunk_t;

static int read_chunk(FILE * input, chunk_t * ret)
  {
  uint8_t buf[4];
  if(fread(ret->fourcc, 1, 4, input) < 4)
    return 0;
  if(fread(buf, 1, 4, input) < 4)
    return 0;
  ret->length =
    buf[0] |
    ((uint32_t)(buf[1]) << 8) |
    ((uint32_t)(buf[2]) << 16) |
    ((uint32_t)(buf[3]) << 24);
  return 1;
  }

static int32_t get_wav_length(const char * filename)
  {
  int32_t ret;
  chunk_t ch;
  FILE * wav;
  uint8_t buf[12];
  wav = fopen(filename, "r");
  if(!wav)
    return -1;
  ret = -1;

  if(fread(buf, 1, 12, wav) < 12)
    goto fail;
  if((buf[0] != 'R') ||
     (buf[1] != 'I') ||
     (buf[2] != 'F') ||
     (buf[3] != 'F') ||
     (buf[8] != 'W') ||
     (buf[9] != 'A') ||
     (buf[10] != 'V') ||
     (buf[11] != 'E'))
    goto fail;
    
  ret = -1;
  while(read_chunk(wav, &ch))
    {
    if((ch.fourcc[0] == 'd') &&
       (ch.fourcc[1] == 'a') &&
       (ch.fourcc[2] == 't') &&
       (ch.fourcc[3] == 'a'))
      {
      ret = ch.length / 4;
      break;
      }
    fseek(wav, ch.length, SEEK_CUR);
    }
  
  fail:
  fclose(wav);
  return ret;
  }

static void add_track_cdrdao(void * data, const char * filename,
                             bg_metadata_t * metadata, int pp_only)
  {
  int32_t duration = 0;
  cdrdao_t * cdrdao;
  cdrdao = (cdrdao_t*)data;

  if(cdrdao->pregap > 0)
    {
    duration = get_wav_length(filename);
    if(duration <= 0)
      {
      fprintf(stderr, "Cannot get duration of file %s\n", filename);
      return;
      }
    }
  
  cdrdao->tracks = realloc(cdrdao->tracks,
                           (cdrdao->num_tracks+1)*sizeof(*cdrdao->tracks));

  memset(cdrdao->tracks + cdrdao->num_tracks, 0, sizeof(*cdrdao->tracks));

  bg_metadata_copy(&(cdrdao->tracks[cdrdao->num_tracks].metadata),
                   metadata);
  cdrdao->tracks[cdrdao->num_tracks].filename = bg_strdup((char*)0, filename);
  cdrdao->tracks[cdrdao->num_tracks].pp_only = pp_only;
  
  if(cdrdao->pregap > 0)
    cdrdao->tracks[cdrdao->num_tracks].duration = duration;
  
  cdrdao->num_tracks++;
  fprintf(stderr, "add_track_cdrdao %d %s\n", cdrdao->num_tracks, filename);
  }

static void run_cdrdao(void * data, const char * directory, int cleanup)
  {
  FILE * outfile;
  int i;
  int do_album = 0;
  cdrdao_t * cdrdao;
  char * filename;
  int do_author;
  int do_comment = 1;
  int do_cdtext;
  int same_artist;
  
  int pregap_mm;
  int pregap_ss;
  int pregap_ff;
  int pregap;
  
  cdrdao = (cdrdao_t*)data;

  if(!cdrdao->num_tracks)
    {
    bg_log(BG_LOG_WARNING, LOG_DOMAIN, "Skipping cdrdao run (no tracks)");
    return;
    }
  
  /* Check, if we can write cdtext */

  do_cdtext = cdrdao->use_cdtext;
  do_author = 1;
  if(do_cdtext)
    {
    for(i = 0; i < cdrdao->num_tracks; i++)
      {
      if(!cdrdao->tracks[i].metadata.artist ||
         !cdrdao->tracks[i].metadata.title)
        {
        do_cdtext = 0;
        break;
        }
      if(!cdrdao->tracks[i].metadata.author)
        do_author = 0;
      if(!cdrdao->tracks[i].metadata.comment)
        do_comment = 0;
      }
    }

  /* Check, whether to write CDText for an album */

  same_artist = 1;
  if(do_cdtext)
    {
    if(cdrdao->use_cdtext)
      {
      do_album = 1;
      
      if(!cdrdao->tracks[0].metadata.album)
        do_album = 0;
      else
        {
        for(i = 1; i < cdrdao->num_tracks; i++)
          {
          if(strcmp(cdrdao->tracks[i].metadata.artist, cdrdao->tracks[0].metadata.artist))
            {
            same_artist = 0;
            }
          if(!cdrdao->tracks[i].metadata.album ||
             strcmp(cdrdao->tracks[i].metadata.album,  cdrdao->tracks[0].metadata.album))
            {
            do_album = 0;
            }
          }
        }
      }
    }
  
  /* Calculate pregap */
  pregap = cdrdao->pregap;
  pregap_ff = pregap % 75;
  pregap /= 75;
  pregap_ss = pregap % 60;
  pregap /= 60;
  pregap_mm = pregap;
    
  /* Open output file */
  filename = bg_sprintf("%s/%s", directory, cdrdao->toc_file);
  outfile = fopen(filename, "w");
  if(!outfile)
    return;
  /* Write header */
  fprintf(outfile, "CD_DA\n");
  if(do_cdtext)
    {
    // global CD-TEXT data
    fprintf(outfile, "CD_TEXT {\n");
    // Mapping from language number (0..7) used in 'LANGUAGE' statements 
    // to language code.
    fprintf(outfile, "  LANGUAGE_MAP {\n");
    fprintf(outfile, "    0 : EN\n");  // 9 is the code for ENGLISH,
    fprintf(outfile, "  }\n");         // I don't know any other language code, yet

    // Language number should always start with 0
    fprintf(outfile, "  LANGUAGE 0 {\n");
        // Required fields - at least all CD-TEXT CDs I've seen so far have them.
    fprintf(outfile, "    TITLE \"%s\"\n", (do_album ? cdrdao->tracks[0].metadata.album : "Audio CD"));
    fprintf(outfile, "    PERFORMER \"%s\"\n", (same_artist ? cdrdao->tracks[0].metadata.artist :
                                                "Various"));
    fprintf(outfile, "    DISC_ID \"XY12345\"\n");
    fprintf(outfile, "    UPC_EAN \"\"\n"); // usually empty
    
    // Further possible items, all of them are optional
    fprintf(outfile, "    ARRANGER \"\"\n");
    fprintf(outfile, "    SONGWRITER \"\"\n");
    fprintf(outfile, "    MESSAGE \"\"\n");
    fprintf(outfile, "    GENRE \"\"\n"); // I'm not sure if this should be really ascii data
    fprintf(outfile, "  }\n");
    fprintf(outfile, "}\n");
    }
  /* Loop over tracks */

  for(i = 0; i < cdrdao->num_tracks; i++)
    {
    fprintf(outfile, "TRACK AUDIO\n");
    if(do_cdtext)
      {
      fprintf(outfile, "CD_TEXT {\n");
      fprintf(outfile, "  LANGUAGE 0 {\n");
      fprintf(outfile, "    TITLE \"%s\"\n", cdrdao->tracks[i].metadata.title);
      fprintf(outfile, "    PERFORMER \"%s\"\n", cdrdao->tracks[i].metadata.artist);
      fprintf(outfile, "    ISRC \"US-XX1-98-01234\"\n");
      fprintf(outfile, "    ARRANGER \"\"\n");
      fprintf(outfile, "    SONGWRITER \"%s\"\n", (do_author ? cdrdao->tracks[i].metadata.author : ""));
      fprintf(outfile, "    MESSAGE \"%s\"\n", (do_comment ? cdrdao->tracks[i].metadata.comment : ""));
      fprintf(outfile, "  }\n");
      fprintf(outfile, "}\n");
      
      if(i && (cdrdao->pregap > 0))
        {
        fprintf(outfile, "PREGAP %d:%d:%d\n", pregap_mm, 
                pregap_ss, pregap_ff);
        fprintf(outfile, "FILE \"%s\" 0 %d\n\n", cdrdao->tracks[i].filename,
                cdrdao->tracks[i].duration);
        }
      else
        {
        fprintf(outfile, "FILE \"%s\" 0\n\n", cdrdao->tracks[i].filename);
        }
      }
    }
  fclose(outfile);

  /* Run cdrdao */

  if(bg_cdrdao_run(cdrdao->cdr, filename) && cleanup)
    {
    for(i = 0; i < cdrdao->num_tracks; i++)
      {
      if(!cdrdao->tracks[i].pp_only)
        {
        bg_log(BG_LOG_INFO, LOG_DOMAIN, "Removing %s", cdrdao->tracks[i].filename);
        remove(cdrdao->tracks[i].filename);
        }
      }
    bg_log(BG_LOG_INFO, LOG_DOMAIN, "Removing %s", filename);
    remove(filename);
    }
  free(filename);
  }

static void stop_cdrdao(void * data)
  {
  cdrdao_t * cdrdao;
  cdrdao = (cdrdao_t*)data;
  bg_cdrdao_stop(cdrdao->cdr);
  }

bg_encoder_pp_plugin_t the_plugin =
  {
    common:
    {
      name:              "e_pp_cdrdao", /* Unique short name */
      long_name:         "Audio CD generator/burner",
      extensions:        "wav",
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
    run:                 run_cdrdao,
    stop:                stop_cdrdao,
    
  };

/* Include this into all plugin modules exactly once
   to let the plugin loader obtain the API version */
BG_GET_PLUGIN_API_VERSION;
