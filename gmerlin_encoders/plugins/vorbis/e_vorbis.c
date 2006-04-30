/*****************************************************************
 
  e_vorbis.c
 
  Copyright (c) 2005 by Burkhard Plaum - plaum@ipf.uni-stuttgart.de
 
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
#include <gmerlin_encoders.h>

#include <gmerlin/plugin.h>
#include <gmerlin/utils.h>

#include <vorbis/vorbisenc.h>

/* This plugin tries to resemble the behaviour and user options
   found in the oggenc program. The "Advanced Options" however,
   are left out for now */

typedef struct
  {
  char * error_msg;

  /* Ogg vorbis stuff */
    
  ogg_stream_state enc_os;
  ogg_page enc_og;
  ogg_packet enc_op;
  vorbis_info enc_vi;
  vorbis_comment enc_vc;
  vorbis_dsp_state enc_vd;
  vorbis_block enc_vb;

  /* Misc stuff */

  FILE * output;
  gavl_audio_format_t format;
  bg_metadata_t metadata;
  gavl_audio_frame_t * frame;
  char * filename;
  
  /* Options */

  int managed;
  int min_bitrate;
  int nominal_bitrate;
  int max_bitrate;

  float quality;     /* Float from 0 to 1 (low->high) */
  int quality_set;
      
  } vorbis_t;

static void * create_vorbis()
  {
  vorbis_t * ret;
  ret = calloc(1, sizeof(*ret));

  ret->frame = gavl_audio_frame_create(NULL);
    
  return ret;
  }

static void destroy_vorbis(void * priv)
  {
  vorbis_t * vorbis;
  vorbis = (vorbis_t*)priv;

  ogg_stream_clear(&vorbis->enc_os);
  vorbis_block_clear(&vorbis->enc_vb);
  vorbis_dsp_clear(&vorbis->enc_vd);
  vorbis_comment_clear(&vorbis->enc_vc);
  vorbis_info_clear(&vorbis->enc_vi);

  gavl_audio_frame_destroy(vorbis->frame);
  
  free(vorbis);
  }

static const char * get_error_vorbis(void * priv)
  {
  vorbis_t * vorbis;
  vorbis = (vorbis_t*)priv;
  return vorbis->error_msg;
  }

#if 0 /* Common parameters (nothing here yet) */

static bg_parameter_info_t parameters[] =
  {
    { /* End of parameters */ }
  };

static bg_parameter_info_t * get_parameters_vorbis(void * data)
  {
  return parameters;
  }

static void set_parameter_vorbis(void * data, char * name, bg_parameter_value_t * v)
  {
  vorbis_t * vorbis;
  vorbis = (vorbis_t*)data;
  
  if(!name)
    return;
  }
#endif

static bg_parameter_info_t audio_parameters[] =
  {
    {
      name:        "bitrate_mode",
      long_name:   "Bitrate mode",
      type:        BG_PARAMETER_STRINGLIST,
      val_default: { val_str: "VBR" },
      multi_names: (char*[]){ "VBR", "Managed", (char*)0 },
      help_string: "Bitrate mode:\n\
VBR: You specify a quality and (optionally) a minimum and maximum bitrate\n\
Managed: You specify a nominal bitrate and (optionally) a minimum and maximum bitrate\n\
VBR is recommended, managed bitrate might result in a worse quality"
    },
    {
      name:        "nominal_bitrate",
      long_name:   "Nominal bitrate (kbps)",
      type:        BG_PARAMETER_INT,
      val_min:     { val_i: 0 },
      val_max:     { val_i: 1000 },
      val_default: { val_i: 128 },
      help_string: "Nominal bitrate (in kbps) for managed mode",
    },
    {
      name:      "quality",
      long_name: "VBR Quality (10: best)",
      type:      BG_PARAMETER_SLIDER_FLOAT,
      val_min:     { val_f: 0.0 },
      val_max:     { val_f: 10.0 },
      val_default: { val_f: 3.0 },
      num_digits:  1,
      help_string: "Quality for VBR mode\n\
10: best (largest output file)\n\
0:  worst (smallest output file",
    },
    {
      name:        "min_bitrate",
      long_name:   "Minimum bitrate (kbps)",
      type:        BG_PARAMETER_INT,
      val_min:     { val_i: 0 },
      val_max:     { val_i: 1000 },
      val_default: { val_i: 0 },
      help_string: "Optional minimum bitrate (in kbps)\n\
0 = unspecified",
    },
    {
      name:        "max_bitrate",
      long_name:   "Maximum bitrate (kbps)",
      type:        BG_PARAMETER_INT,
      val_min:     { val_i: 0 },
      val_max:     { val_i: 1000 },
      val_default: { val_i: 0 },
      help_string: "Optional maximum bitrate (in kbps)\n\
0 = unspecified",
    },
    { /* End of parameters */ }
  };


static bg_parameter_info_t * get_audio_parameters_vorbis(void * data)
  {
  return audio_parameters;
  }

static void build_comment(vorbis_comment * vc, bg_metadata_t * metadata)
  {
  char * tmp_string;
  
  vorbis_comment_init(vc);
  
  if(metadata->artist)
    vorbis_comment_add_tag(vc, "ARTIST", metadata->artist);
  
  if(metadata->title)
    vorbis_comment_add_tag(vc, "TITLE", metadata->title);

  if(metadata->album)
    vorbis_comment_add_tag(vc, "ALBUM", metadata->album);
    
  if(metadata->genre)
    vorbis_comment_add_tag(vc, "GENRE", metadata->genre);

  if(metadata->date)
    vorbis_comment_add_tag(vc, "DATE", metadata->date);
  
  if(metadata->copyright)
    vorbis_comment_add_tag(vc, "COPYRIGHT", metadata->copyright);

  if(metadata->track)
    {
    tmp_string = bg_sprintf("%d", metadata->track);
    vorbis_comment_add_tag(vc, "TRACKNUMBER", tmp_string);
    free(tmp_string);
    }

  if(metadata->comment)
    vorbis_comment_add(vc, metadata->comment);
  }

static void init_encode(vorbis_t * vorbis)
  {
  ogg_packet header_main;
  ogg_packet header_comments;
  ogg_packet header_codebooks;
    
  unsigned int serialno;
  struct ovectl_ratemanage_arg ai;
  
  vorbis_info_init(&vorbis->enc_vi);

  /* VBR Initialization */
  if(!vorbis->managed) 
    {
    vorbis_encode_setup_vbr(&vorbis->enc_vi, vorbis->format.num_channels,
                            vorbis->format.samplerate, vorbis->quality);
    
    /* If we have additional bitrate rescrictions, set them here */
    if((vorbis->min_bitrate > 0) || (vorbis->max_bitrate > 0))
      {
      vorbis_encode_ctl(&vorbis->enc_vi, OV_ECTL_RATEMANAGE_GET, &ai);
      
      ai.bitrate_hard_min=vorbis->min_bitrate;
      ai.bitrate_hard_max=vorbis->max_bitrate;
      ai.management_active=1;
      
      vorbis_encode_ctl(&vorbis->enc_vi, OV_ECTL_RATEMANAGE_SET, &ai);
      }
    }
  else
    {
    vorbis_encode_setup_managed(&vorbis->enc_vi, vorbis->format.num_channels,
                                vorbis->format.samplerate,
                                vorbis->max_bitrate>0 ? vorbis->max_bitrate : -1,
                                vorbis->nominal_bitrate,
                                vorbis->min_bitrate>0 ? vorbis->min_bitrate : -1);

    }

  /* Turn off management entirely (if it was turned on). */
  if(!vorbis->max_bitrate && !vorbis->min_bitrate && !vorbis->managed)
    {
    vorbis_encode_ctl(&vorbis->enc_vi, OV_ECTL_RATEMANAGE_SET, NULL);
    }

  vorbis_encode_setup_init(&vorbis->enc_vi);

  vorbis_analysis_init(&vorbis->enc_vd,&vorbis->enc_vi);
  vorbis_block_init(&vorbis->enc_vd,&vorbis->enc_vb);

  serialno = rand();
  ogg_stream_init(&vorbis->enc_os, serialno);

  /* Build comment (comments are UTF-8, good for us :-) */

  build_comment(&vorbis->enc_vc, &vorbis->metadata);
  
  /* Build the packets */
  vorbis_analysis_headerout(&vorbis->enc_vd,&vorbis->enc_vc,
                            &header_main,&header_comments,&header_codebooks);
  
  /* And stream them out */
  ogg_stream_packetin(&vorbis->enc_os,&header_main);
  ogg_stream_packetin(&vorbis->enc_os,&header_comments);
  ogg_stream_packetin(&vorbis->enc_os,&header_codebooks);

  while(ogg_stream_pageout(&vorbis->enc_os,&vorbis->enc_og))
    {
    fwrite(vorbis->enc_og.header,1,vorbis->enc_og.header_len,vorbis->output);
    fwrite(vorbis->enc_og.body,1,vorbis->enc_og.body_len,vorbis->output);
    }
  }

static void set_audio_parameter_vorbis(void * data, int stream, char * name,
                                       bg_parameter_value_t * v)
  {
  vorbis_t * vorbis;
  vorbis = (vorbis_t*)data;
  
  if(stream)
    return;
    
  if(!name)
    {
    init_encode(vorbis);
    return;
    }
  else if(!strcmp(name, "nominal_bitrate"))
    {
    vorbis->nominal_bitrate = v->val_i * 1000;
    if(vorbis->nominal_bitrate < 0)
      vorbis->nominal_bitrate = -1;
    }
  else if(!strcmp(name, "min_bitrate"))
    {
    vorbis->min_bitrate = v->val_i * 1000;
    if(vorbis->min_bitrate < 0)
      vorbis->min_bitrate = -1;
    }
  else if(!strcmp(name, "max_bitrate"))
    {
    vorbis->max_bitrate = v->val_i * 1000;
    if(vorbis->max_bitrate < 0)
      vorbis->max_bitrate = -1;
    }
  else if(!strcmp(name, "quality"))
    {
    vorbis->quality = v->val_f * 0.1;
    }
  else if(!strcmp(name, "bitrate_mode"))
    {
    if(!strcmp(v->val_str, "VBR"))
      vorbis->managed = 0;
    else if(!strcmp(v->val_str, "Managed"))
      vorbis->managed = 1;
    }
  
  }

static int open_vorbis(void * data, const char * filename,
                       bg_metadata_t * metadata)
  {
  vorbis_t * vorbis;
  vorbis = (vorbis_t*)data;
  bg_metadata_copy(&(vorbis->metadata), metadata);

  vorbis->filename = bg_strdup(vorbis->filename, filename);

  //  fprintf(stderr, "Open Vorbis %s...", vorbis->filename);
  vorbis->output = fopen(vorbis->filename, "wb");
  //  fprintf(stderr, "Done %p\n", vorbis->output);

  if(!vorbis->output)
    return 0;
    
  return 1;
  }

static char * vorbis_extension = ".ogg";

static const char * get_extension_vorbis(void * data)
  {
  return vorbis_extension;
  }

static int add_audio_stream_vorbis(void * data, gavl_audio_format_t * format)
  {
  vorbis_t * vorbis;
  vorbis = (vorbis_t*)data;
  gavl_audio_format_copy(&(vorbis->format), format);

  /* Adjust the format */

  vorbis->format.interleave_mode = GAVL_INTERLEAVE_NONE;
  vorbis->format.sample_format = GAVL_SAMPLE_FLOAT;
    
  return 0;
  }

static void flush_data(vorbis_t * vorbis)
  {
  /* While we can get enough data from the library to analyse, one
     block at a time... */

  while(vorbis_analysis_blockout(&vorbis->enc_vd,&vorbis->enc_vb)==1)
    {
    
    /* Do the main analysis, creating a packet */
    vorbis_analysis(&(vorbis->enc_vb), NULL);
    vorbis_bitrate_addblock(&vorbis->enc_vb);
    
    while(vorbis_bitrate_flushpacket(&vorbis->enc_vd, &vorbis->enc_op))
      {
      /* Add packet to bitstream */
      ogg_stream_packetin(&vorbis->enc_os,&vorbis->enc_op);
      
      /* If we've gone over a page boundary, we can do actual output,
         so do so (for however many pages are available) */
      
      while(ogg_stream_pageout(&vorbis->enc_os,&vorbis->enc_og))
        {
        fwrite(vorbis->enc_og.header,1,vorbis->enc_og.header_len,vorbis->output);
        fwrite(vorbis->enc_og.body,1,vorbis->enc_og.body_len,vorbis->output);
        }
      }
    }
  }

static void write_audio_frame_vorbis(void * data, gavl_audio_frame_t * frame,
                                  int stream)
  {
  int i;
  vorbis_t * vorbis;
  float **buffer;
     
  vorbis = (vorbis_t*)data;

  buffer = vorbis_analysis_buffer(&(vorbis->enc_vd), frame->valid_samples);

  for(i = 0; i < vorbis->format.num_channels; i++)
    {
    vorbis->frame->channels.f[i] = buffer[i];
    }
  gavl_audio_frame_copy(&(vorbis->format),
                        vorbis->frame,
                        frame,
                        0, 0, frame->valid_samples, frame->valid_samples);
  vorbis_analysis_wrote(&(vorbis->enc_vd), frame->valid_samples);
  flush_data(vorbis);
  }

static void get_audio_format_vorbis(void * data, int stream,
                                 gavl_audio_format_t * ret)
  {
  vorbis_t * vorbis;
  vorbis = (vorbis_t*)data;
  gavl_audio_format_copy(ret, &(vorbis->format));
  
  }


static void close_vorbis(void * data, int do_delete)
  {
  vorbis_t * vorbis;
  vorbis = (vorbis_t*)data;
  
  vorbis_analysis_wrote(&(vorbis->enc_vd), 0);
  flush_data(vorbis);

  fclose(vorbis->output);

  bg_metadata_free(&vorbis->metadata);
  
  if(do_delete)
    remove(vorbis->filename);

  if(vorbis->filename)
    free(vorbis->filename);
  }

bg_encoder_plugin_t the_plugin =
  {
    common:
    {
      name:            "e_vorbis",       /* Unique short name */
      long_name:       "Vorbis encoder",
      mimetypes:       NULL,
      extensions:      "ogg",
      type:            BG_PLUGIN_ENCODER_AUDIO,
      flags:           BG_PLUGIN_FILE,
      priority:        5,
      create:            create_vorbis,
      destroy:           destroy_vorbis,
      get_error:         get_error_vorbis,
#if 0
      get_parameters:    get_parameters_vorbis,
      set_parameter:     set_parameter_vorbis,
#endif
    },
    max_audio_streams:   1,
    max_video_streams:   0,
    
    get_extension:       get_extension_vorbis,
    
    open:                open_vorbis,
    
    get_audio_parameters:    get_audio_parameters_vorbis,

    add_audio_stream:        add_audio_stream_vorbis,
    
    set_audio_parameter:     set_audio_parameter_vorbis,

    get_audio_format:        get_audio_format_vorbis,
    
    write_audio_frame:   write_audio_frame_vorbis,
    close:               close_vorbis
  };

/* Include this into all plugin modules exactly once
   to let the plugin loader obtain the API version */
BG_GET_PLUGIN_API_VERSION;
