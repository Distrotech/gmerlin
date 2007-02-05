/*****************************************************************
 
  e_lame.c
 
  Copyright (c) 2005-2006 by Burkhard Plaum - plaum@ipf.uni-stuttgart.de
 
  http://gmerlin.sourceforge.net
 
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
 
  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111, USA.
 
*****************************************************************/

#include <string.h>
#include <stdlib.h>
#include <config.h>
#include <errno.h>
#include <gmerlin/plugin.h>
#include <gmerlin/utils.h>
#include <gmerlin/log.h>
#define LOG_DOMAIN "e_lame"

#include <gmerlin_encoders.h>

#include <lame/lame.h>

/* Supported samplerates for MPEG-1/2/2.5 */

static int samplerates[] =
  {
    /* MPEG-2.5 */
    8000,
    11025,
    12000,
    /* MPEG-2 */
    16000,
    22050,
    24000,
    /* MPEG-1 */
    32000,
    44100,
    48000
  };

static int get_samplerate(int in_rate)
  {
  int i;
  int diff;
  int min_diff = 1000000;
  int min_i = -1;
  
  for(i = 0; i < sizeof(samplerates)/sizeof(samplerates[0]); i++)
    {
    if(samplerates[i] == in_rate)
      return in_rate;
    else
      {
      diff = abs(in_rate - samplerates[i]);
      if(diff < min_diff)
        {
        min_diff = diff;
        min_i = i;
        }
      }
    }
  if(min_i >= 0)
    {
    return samplerates[min_i];
    }
  else
    return 44100;
  }

/* Find the correct bitrate */

static int mpeg1_bitrates[] =
  {
    32,
    40,
    48,
    56,
    64,
    80,
    96,
    112,
    128,
    160,
    192,
    224,
    256,
    320
  };

static int mpeg2_bitrates[] =
  {
    8,
    16,
    24,
    32,
    40,
    48,
    56,
    64,
    80,
    96,
    112,
    128,
    144,
    160
  };

static int get_bitrate(int bitrate, int samplerate)
  {
  int i;
  int * bitrates;
  int diff;
  int min_diff = 1000000;
  int min_i = -1;

  if(samplerate >= 32000)
    bitrates = mpeg1_bitrates;
  else
    bitrates = mpeg2_bitrates;

  for(i = 0; i < sizeof(mpeg1_bitrates) / sizeof(mpeg1_bitrates[0]); i++)
    {
    if(bitrate == bitrates[i])
      return bitrate;
    diff = abs(bitrate - bitrates[i]);
    if(diff < min_diff)
      {
      min_diff = diff;
      min_i = i;
      }
    }
  if(min_i >= 0)
    return bitrates[min_i];
  return 128;
  }

typedef struct
  {
  char * error_msg;
  char * filename;
  
  lame_t lame;
  gavl_audio_format_t format;

  FILE * output;

  int do_id3v1;
  int do_id3v2;
  int id3v2_charset;
  
  uint8_t * output_buffer;
  int output_buffer_alloc;

  enum vbr_mode_e vbr_mode;

  /* Config stuff */
    
  int abr_min_bitrate;
  int abr_max_bitrate;
  int abr_bitrate;
  int cbr_bitrate;
  int vbr_quality;

  bgen_id3v1_t * id3v1;
  
  int64_t samples_read;
  
  } lame_priv_t;

static void * create_lame()
  {
  lame_priv_t * ret;
  ret = calloc(1, sizeof(*ret));

  ret->lame = lame_init();

  id3tag_init(ret->lame);

  ret->vbr_mode = vbr_off;
  
  return ret;
  }

static void destroy_lame(void * priv)
  {
  lame_priv_t * lame;
  lame = (lame_priv_t*)priv;
  
  free(lame);
  }

static const char * get_error_lame(void * priv)
  {
  lame_priv_t * lame;
  lame = (lame_priv_t*)priv;
  return lame->error_msg;
  }


static bg_parameter_info_t audio_parameters[] =
  {
    {
      name:        "lame_general",
      long_name:   "General",
      type:        BG_PARAMETER_SECTION
    },
    {
      name:        "bitrate_mode",
      long_name:   "Bitrate mode",
      type:        BG_PARAMETER_STRINGLIST,
      val_default: { val_str: "CBR" },
      multi_names: (char*[]){ "CBR",
                              "ABR",
                              "VBR",
                              (char*)0 },
      help_string: "CBR: Constant bitrate\nVBR: Variable bitrate\nABR: Average bitrate"
    },
    {
      name:        "stereo_mode",
      long_name:   "Stereo mode",
      type:        BG_PARAMETER_STRINGLIST,
      val_default: { val_str: "Auto" },
      multi_names: (char*[]){ "Stereo",
                              "Joint stereo",
                              "Auto",
                              (char*)0 },
      help_string: "Stereo: Completely independent channels\n\
Joint stereo: Improve quality (save bits) by using similarities of the channels\n\
Auto (recommended): Select one of the above depending on quality or bitrate setting"
    },
    {
      name:        "quality",
      long_name:   "Encoding speed",
      type:        BG_PARAMETER_SLIDER_INT,
      val_min:     { val_i: 0 },
      val_max:     { val_i: 9 },
      val_default: { val_i: 2 },
      help_string: "0: Slowest encoding, best quality\n\
9: Fastest encoding, worst quality" 
    },
    {
      name:        "lame_cbr_options",
      long_name:   "CBR options",
      type:        BG_PARAMETER_SECTION
    },
    {
      name:        "cbr_bitrate",
      long_name:   "Bitrate (kbps)",
      type:        BG_PARAMETER_INT,
      val_min:     { val_i: 8 },
      val_max:     { val_i: 320 },
      val_default: { val_i: 128 },
      help_string: "Bitrate in kbps. If your selection is no \
valid mp3 bitrate, we'll choose the closest value."
    },
    {
      name:        "lame_vbr_abr_options",
      long_name:   "VBR/ABR options",
      type:        BG_PARAMETER_SECTION
    },
    {
      name:        "vbr_quality",
      long_name:   "VBR Quality",
      type:        BG_PARAMETER_SLIDER_INT,
      val_min:     { val_i: 0 },
      val_max:     { val_i: 9 },
      val_default: { val_i: 4 },
      help_string: "VBR Quality level. 0: best, 9: worst"
    },
    {
      name:        "abr_bitrate",
      long_name:   "ABR overall bitrate (kbps)",
      type:        BG_PARAMETER_INT,
      val_min:     { val_i: 8 },
      val_max:     { val_i: 320 },
      val_default: { val_i: 128 },
      help_string: "Average bitrate for ABR mode"
    },
    {
      name:        "abr_min_bitrate",
      long_name:   "ABR min bitrate (kbps)",
      type:        BG_PARAMETER_INT,
      val_min:     { val_i: 0 },
      val_max:     { val_i: 320 },
      val_default: { val_i: 0 },
      help_string: "Minimum bitrate for ABR mode. 0 means let lame decide. \
If your selection is no valid mp3 bitrate, we'll choose the closest value."
    },
    {
      name:        "abr_max_bitrate",
      long_name:   "ABR max bitrate (kbps)",
      type:        BG_PARAMETER_INT,
      val_min:     { val_i: 0 },
      val_max:     { val_i: 320 },
      val_default: { val_i: 0 },
      help_string: "Maximum bitrate for ABR mode. 0 means let lame decide. \
If your selection is no valid mp3 bitrate, we'll choose the closest value."
    },
    { /* End of parameters */ }
  };

static bg_parameter_info_t * get_audio_parameters_lame(void * data)
  {
  return audio_parameters;
  }

static void set_audio_parameter_lame(void * data, int stream, char * name,
                                     bg_parameter_value_t * v)
  {
  lame_priv_t * lame;
  int i;
  lame = (lame_priv_t*)data;
  
  if(stream)
    return;
  else if(!name)
    {
    /* Finalize configuration and do some sanity checks */

    switch(lame->vbr_mode)
      {
      case vbr_abr:
        /* Average bitrate */
        if(lame_set_VBR_q(lame->lame, lame->vbr_quality))
          bg_log(BG_LOG_ERROR, LOG_DOMAIN,  "lame_set_VBR_q failed");

        if(lame_set_VBR_mean_bitrate_kbps(lame->lame, lame->abr_bitrate))
          bg_log(BG_LOG_ERROR, LOG_DOMAIN,
                 "lame_set_VBR_mean_bitrate_kbps failed");
        
        if(lame->abr_min_bitrate)
          {
          lame->abr_min_bitrate =
            get_bitrate(lame->abr_min_bitrate, lame->format.samplerate);
          if(lame->abr_min_bitrate > lame->abr_bitrate)
            {
            lame->abr_min_bitrate = get_bitrate(8, lame->format.samplerate);
            }
          if(lame_set_VBR_min_bitrate_kbps(lame->lame, lame->abr_min_bitrate))
            bg_log(BG_LOG_ERROR, LOG_DOMAIN,
                   "lame_set_VBR_min_bitrate_kbps failed");
          }
        if(lame->abr_max_bitrate)
          {
          lame->abr_max_bitrate =
            get_bitrate(lame->abr_max_bitrate, lame->format.samplerate);
          if(lame->abr_max_bitrate < lame->abr_bitrate)
            {
            lame->abr_max_bitrate = get_bitrate(320, lame->format.samplerate);
            }
          if(lame_set_VBR_max_bitrate_kbps(lame->lame, lame->abr_max_bitrate))
            bg_log(BG_LOG_ERROR, LOG_DOMAIN,
                   "lame_set_VBR_max_bitrate_kbps failed");
          }
        break;
      case vbr_default:
        if(lame_set_VBR_q(lame->lame, lame->vbr_quality))
          bg_log(BG_LOG_ERROR, LOG_DOMAIN,  "lame_set_VBR_q failed");
        break;
      case vbr_off:
        lame->cbr_bitrate =
            get_bitrate(lame->cbr_bitrate, lame->format.samplerate);
        if(lame_set_brate(lame->lame, lame->cbr_bitrate))
          bg_log(BG_LOG_ERROR, LOG_DOMAIN,  "lame_set_brate failed");
        break;
      default:
        break;
      }

    if(lame_init_params(lame->lame) < 0)
      bg_log(BG_LOG_ERROR, LOG_DOMAIN,  "lame_init_params failed");

    return;
    }

  if(!strcmp(name, "bitrate_mode"))
    {
    if(!strcmp(v->val_str, "ABR"))
      {
      lame->vbr_mode = vbr_abr;
      }
    else if(!strcmp(v->val_str, "VBR"))
      {
      lame->vbr_mode = vbr_default;
      }
    else
      {
      lame->vbr_mode = vbr_off;
      }
    if(lame_set_VBR(lame->lame, lame->vbr_mode))
      bg_log(BG_LOG_ERROR, LOG_DOMAIN,  "lame_set_VBR failed");
    
    if(lame_set_bWriteVbrTag(lame->lame, (lame->vbr_mode == vbr_off) ? 0 : 1))
      bg_log(BG_LOG_ERROR, LOG_DOMAIN,  "lame_set_bWriteVbrTag failed");

    //    lame_set_bWriteVbrTag(lame->lame, 0);
    }
  else if(!strcmp(name, "stereo_mode"))
    {
    if(lame->format.num_channels == 1)
      return;
    i = NOT_SET;
    if(!strcmp(v->val_str, "Stereo"))
      {
      i = STEREO;
      }
    else if(!strcmp(v->val_str, "Joint stereo"))
      {
      i = JOINT_STEREO;
      }
    if(i != NOT_SET)
      {
      if(lame_set_mode(lame->lame, JOINT_STEREO))
        bg_log(BG_LOG_ERROR, LOG_DOMAIN,  "lame_set_mode failed");
      }
    }
  else if(!strcmp(name, "quality"))
    {
    if(lame_set_quality(lame->lame, v->val_i))
      bg_log(BG_LOG_ERROR, LOG_DOMAIN,  "lame_set_quality failed");
    }
  
  else if(!strcmp(name, "cbr_bitrate"))
    {
    lame->cbr_bitrate = v->val_i;
    }
  else if(!strcmp(name, "vbr_quality"))
    {
    lame->vbr_quality = v->val_i;
    }
  else if(!strcmp(name, "abr_bitrate"))
    {
    lame->abr_bitrate = v->val_i;
    }
  else if(!strcmp(name, "abr_min_bitrate"))
    {
    lame->abr_min_bitrate = v->val_i;
    }
  else if(!strcmp(name, "abr_max_bitrate"))
    {
    lame->abr_max_bitrate = v->val_i;
    }
  }

/* Global parameters */

static bg_parameter_info_t parameters[] =
  {
    {
      name:        "do_id3v1",
      long_name:   "Write ID3V1.1 tag",
      type:        BG_PARAMETER_CHECKBUTTON,
      val_default: { val_i: 1 },
    },
    {
      name:        "do_id3v2",
      long_name:   "Write ID3V2 tag",
      type:        BG_PARAMETER_CHECKBUTTON,
      val_default: { val_i: 1 },
    },
    {
      name:        "id3v2_charset",
      long_name:   "ID3V2 Encoding",
      type:        BG_PARAMETER_STRINGLIST,
      val_default: { val_str: "3" },
      multi_names: (char*[]){ "0", "1",
                               "2", "3", (char*)0 },
      multi_labels: (char*[]){ "ISO-8859-1", "UTF-16 LE",
                               "UTF-16 BE", "UTF-8", (char*)0 },
    },
    { /* End of parameters */ }
  };

static bg_parameter_info_t * get_parameters_lame(void * data)
  {
  return parameters;
  }

static void set_parameter_lame(void * data, char * name, bg_parameter_value_t * v)
  {
  lame_priv_t * lame;
  lame = (lame_priv_t*)data;
  
  if(!name)
    return;
  else if(!strcmp(name, "do_id3v1"))
    lame->do_id3v1 = v->val_i;
  else if(!strcmp(name, "do_id3v2"))
    lame->do_id3v2 = v->val_i;
  else if(!strcmp(name, "id3v2_charset"))
    lame->id3v2_charset = atoi(v->val_str);
  }

static int open_lame(void * data, const char * filename,
                     bg_metadata_t * metadata,
                     bg_chapter_list_t * chapter_list)
  {
  int ret = 1;
  lame_priv_t * lame;
  bgen_id3v2_t * id3v2;

  lame = (lame_priv_t*)data;

  lame->output = fopen(filename, "wb+");
  if(!lame->output)
    {
    lame->error_msg = bg_sprintf("Cannot open %s: %s",
                                 filename, strerror(errno));
    bg_log(BG_LOG_ERROR, LOG_DOMAIN, lame->error_msg);
    return 0;
    }
  lame->filename = bg_strdup(lame->filename, filename);

  if(lame->do_id3v2)
    {
    id3v2 = bgen_id3v2_create(metadata);
    if(!bgen_id3v2_write(lame->output, id3v2, lame->id3v2_charset))
      ret = 0;
    bgen_id3v2_destroy(id3v2);
    }

  /* Create id3v1 tag. It will be appended to the file at the very end */

  if(lame->do_id3v1)
    {
    lame->id3v1 = bgen_id3v1_create(metadata);
    }
  
  return ret;
  }

static char * lame_extension = ".mp3";

static const char * get_extension_lame(void * data)
  {
  return lame_extension;
  }


static int add_audio_stream_lame(void * data, const char * language,
                                 gavl_audio_format_t * format)
  {
  lame_priv_t * lame;
  
  lame = (lame_priv_t*)data;
  
  /* Copy and adjust format */
    
  gavl_audio_format_copy(&(lame->format), format);

  lame->format.sample_format = GAVL_SAMPLE_FLOAT;
  lame->format.interleave_mode = GAVL_INTERLEAVE_NONE;
  lame->format.samplerate = get_samplerate(lame->format.samplerate);
  
  if(lame->format.num_channels > 2)
    {
    lame->format.num_channels = 2;
    lame->format.channel_locations[0] = GAVL_CHID_NONE;
    gavl_set_channel_setup(&(lame->format));
    }

  if(lame_set_in_samplerate(lame->lame, lame->format.samplerate))
    bg_log(BG_LOG_ERROR, LOG_DOMAIN,  "lame_set_in_samplerate failed");
  if(lame_set_num_channels(lame->lame,  lame->format.num_channels))
    bg_log(BG_LOG_ERROR, LOG_DOMAIN,  "lame_set_num_channels failed");

  if(lame_set_scale(lame->lame, 32767.0))
    bg_log(BG_LOG_ERROR, LOG_DOMAIN,  "lame_set_scale failed");
    
  
  //  lame_set_out_samplerate(lame->lame, lame->format.samplerate);
  
  return 0;
  }

static int write_audio_frame_lame(void * data, gavl_audio_frame_t * frame,
                                  int stream)
  {
  int ret = 1;
  int max_out_size, bytes_encoded;
  lame_priv_t * lame;
  
  lame = (lame_priv_t*)data;

  max_out_size = (5 * frame->valid_samples) / 4 + 7200;
  if(lame->output_buffer_alloc < max_out_size)
    {
    lame->output_buffer_alloc = max_out_size + 1024;
    lame->output_buffer = realloc(lame->output_buffer,
                                  lame->output_buffer_alloc);
    }

  bytes_encoded = lame_encode_buffer_float(lame->lame,
                                           frame->channels.f[0],
                                           (lame->format.num_channels > 1) ?
                                           frame->channels.f[1] :
                                           frame->channels.f[0],
                                           frame->valid_samples,
                                           lame->output_buffer,
                                           lame->output_buffer_alloc);
  if(fwrite(lame->output_buffer, 1, bytes_encoded, lame->output) < bytes_encoded)
    ret = 0;
  lame->samples_read += frame->valid_samples;
  return ret;
  }

static void get_audio_format_lame(void * data, int stream,
                                 gavl_audio_format_t * ret)
  {
  lame_priv_t * lame;
  lame = (lame_priv_t*)data;
  gavl_audio_format_copy(ret, &(lame->format));
  
  }

static int close_lame(void * data, int do_delete)
  {
  int ret = 1;
  lame_priv_t * lame;
  int bytes_encoded;
  lame = (lame_priv_t*)data;

  /* 1. Flush the buffer */

  if(lame->samples_read)
    {
    if(lame->output_buffer_alloc < 7200)
      {
      lame->output_buffer_alloc = 7200;
      lame->output_buffer = realloc(lame->output_buffer, lame->output_buffer_alloc);
      }
    bytes_encoded = lame_encode_flush(lame->lame, lame->output_buffer, 
                                      lame->output_buffer_alloc);

    if(fwrite(lame->output_buffer, 1, bytes_encoded, lame->output) < bytes_encoded)
      ret = 0;
    
    /* 2. Write xing tag */
    
    if(lame->vbr_mode != vbr_off)
      {
      lame_mp3_tags_fid(lame->lame, lame->output);
      }
    }
  
  /* 3. Write ID3V1 tag */

  if(lame->output)
    {
    if(ret && lame->id3v1)
      {
      fseek(lame->output, 0, SEEK_END);
      if(!bgen_id3v1_write(lame->output, lame->id3v1))
        ret = 0;
      bgen_id3v1_destroy(lame->id3v1);
      lame->id3v1 = (bgen_id3v1_t*)0;
      }
    
    /* 4. Close output file */
    
    fclose(lame->output);
    lame->output = (FILE*)0;
    }
  
  /* Clean up */
  if(lame->lame)
    {
    lame_close(lame->lame);
    lame->lame = (lame_t)0;
    }

  if(lame->filename)
    {
    /* Remove if necessary */
    if(do_delete)
      remove(lame->filename);
    free(lame->filename);
    lame->filename = (char*)0;
    }

  if(lame->output_buffer)
    {
    free(lame->output_buffer);
    lame->output_buffer = (uint8_t*)0;
    }
  return 1;
  }

bg_encoder_plugin_t the_plugin =
  {
    common:
    {
      name:            "e_lame",       /* Unique short name */
      long_name:       "Lame mp3 encoder",
      mimetypes:       NULL,
      extensions:      "mp3",
      type:            BG_PLUGIN_ENCODER_AUDIO,
      flags:           BG_PLUGIN_FILE,
      priority:        5,
      create:            create_lame,
      destroy:           destroy_lame,
      get_error:         get_error_lame,
      get_parameters:    get_parameters_lame,
      set_parameter:     set_parameter_lame,
    },
    max_audio_streams:   1,
    max_video_streams:   0,
    
    get_extension:       get_extension_lame,
    
    open:                open_lame,
    
    get_audio_parameters:    get_audio_parameters_lame,

    add_audio_stream:        add_audio_stream_lame,
    
    set_audio_parameter:     set_audio_parameter_lame,

    get_audio_format:        get_audio_format_lame,
    
    write_audio_frame:   write_audio_frame_lame,
    close:               close_lame
  };

/* Include this into all plugin modules exactly once
   to let the plugin loader obtain the API version */
BG_GET_PLUGIN_API_VERSION;
