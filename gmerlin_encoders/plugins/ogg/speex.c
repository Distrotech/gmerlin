/*****************************************************************
 
  speex.c
 
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

#include <string.h>
#include <stdlib.h>

#include <config.h>

#include <gmerlin/translation.h>
#include <gmerlin/plugin.h>
#include <gmerlin/utils.h>
#include <gmerlin/log.h>
#define LOG_DOMAIN "oggspeex"

#include <speex/speex.h>
#include <speex/speex_header.h>
#include <speex/speex_stereo.h>
#include <speex/speex_callbacks.h>

#include <ogg/ogg.h>
#include "ogg_common.h"

/* Newer speex version (1.1.x) don't have this */
#ifndef MAX_BYTES_PER_FRAME
#define MAX_BYTES_PER_FRAME 2000
#endif

/* Way too large but save */
#define BUFFER_SIZE (MAX_BYTES_PER_FRAME*10)

typedef struct
  {
  ogg_stream_state enc_os;
  
  long serialno;
  FILE * output;
  
  
  int64_t samples_read;
  
  gavl_audio_format_t * format;
  gavl_audio_frame_t * frame;

  int modeID;

  int bitrate;
  int abr_bitrate;
  int quality;
  int complexity;
  int vbr;
  int vad;
  int dtx;
  int nframes;

  void * enc;
  SpeexBits bits;
  int lookahead;

  int frames_encoded;

  uint8_t buffer[BUFFER_SIZE];
  
  } speex_t;

/* Comment building stuff (ripped from speexenc.c) */

#define readint(buf, base) (((buf[base+3]<<24)&0xff000000)| \
                           ((buf[base+2]<<16)&0xff0000)| \
                           ((buf[base+1]<<8)&0xff00)| \
                            (buf[base]&0xff))
#define writeint(buf, base, val) do{ buf[base+3]=((val)>>24)&0xff; \
                                     buf[base+2]=((val)>>16)&0xff; \
                                     buf[base+1]=((val)>>8)&0xff; \
                                     buf[base]=(val)&0xff; \
                                 }while(0)
static void comment_init(char **comments, int* length, char *vendor_string)
{
  int vendor_length=strlen(vendor_string);
  int user_comment_list_length=0;
  int len=4+vendor_length+4;
  char *p=(char*)malloc(len);
  if(p==NULL){
  }
  writeint(p, 0, vendor_length);
  memcpy(p+4, vendor_string, vendor_length);
  writeint(p, 4+vendor_length, user_comment_list_length);
  *length=len;
  *comments=p;
}
static void comment_add(char **comments, int* length, char *tag, char *val)
{
  char* p=*comments;
  int vendor_length=readint(p, 0);
  int user_comment_list_length=readint(p, 4+vendor_length);
  int tag_len=(tag?strlen(tag):0);
  int val_len=strlen(val);
  int len=(*length)+4+tag_len+val_len;

  p=(char*)realloc(p, len);
  if(p==NULL){
  }

  writeint(p, *length, tag_len+val_len);      /* length of comment */
  if(tag) memcpy(p+*length+4, tag, tag_len);  /* comment */
  memcpy(p+*length+4+tag_len, val, val_len);  /* comment */
  writeint(p, 4+vendor_length, user_comment_list_length+1);

  *comments=p;
  *length=len;
}
#undef readint
#undef writeint

static void * create_speex(FILE * output, long serialno)
  {
  speex_t * ret;
  ret = calloc(1, sizeof(*ret));
  ret->serialno = serialno;
  ret->output = output;
  ret->frame = gavl_audio_frame_create(NULL);
  return ret;
  }

static bg_parameter_info_t parameters[] =
  {
    {
      name:        "mode",
      long_name:   TRS("Speex mode"),
      type:        BG_PARAMETER_STRINGLIST,
      val_default: { val_str: "auto" },
      multi_names:  (char*[]){ "auto", "nb",         "wb",       "uwb",            (char*)0 },
      multi_labels: (char*[]){ TRS("Auto"), TRS("Narrowband"), TRS("Wideband"),
                               TRS("Ultra-wideband"), (char*)0 },
      help_string: TRS("Encoding mode. If you select Auto, the mode will be taken from the samplerate.")
    },
    {
      name:      "quality",
      long_name: TRS("Quality (10: best)"),
      type:      BG_PARAMETER_SLIDER_INT,
      val_min:     { val_i: 0 },
      val_max:     { val_i: 10 },
      val_default: { val_i: 3 },
    },
    {
      name:      "complexity",
      long_name: TRS("Encoding complexity"),
      type:      BG_PARAMETER_SLIDER_INT,
      val_min:     { val_i: 0 },
      val_max:     { val_i: 10 },
      val_default: { val_i: 3 },
    },
    {
      name:      "nframes",
      long_name: TRS("Frames per Ogg packet"),
      type:      BG_PARAMETER_SLIDER_INT,
      val_min:     { val_i: 1 },
      val_max:     { val_i: 10 },
      val_default: { val_i: 1 },
    },
    {
      name:        "bitrate",
      long_name:   TRS("Bitrate (kbps)"),
      type:        BG_PARAMETER_INT,
      val_min:     { val_i: 0 },
      val_max:     { val_i: 128 },
      val_default: { val_i: 8 },
      help_string: TRS("Bitrate (in kbps). Set to 0 for seleting the standard bitrates for the encoding mode."),
    },
    {
      name:        "vbr",
      long_name:   TRS("Variable bitrate"),
      type:        BG_PARAMETER_CHECKBUTTON,
    },
    {
      name:        "abr_bitrate",
      long_name:   TRS("Average bitrate (kbps)"),
      type:        BG_PARAMETER_INT,
      val_min:     { val_i: 0 },
      val_max:     { val_i: 128 },
      val_default: { val_i: 0 },
      help_string: TRS("Average bitrate (in kbps). Set to 0 for disabling ABR."),
    },
    {
      name:        "vad",
      long_name:   TRS("Use voice activity detection"),
      type:        BG_PARAMETER_CHECKBUTTON,
    },
    {
      name:        "dtx",
      long_name:   TRS("Enable file-based discontinuous transmission"),
      type:        BG_PARAMETER_CHECKBUTTON,
    },
    { /* End of parameters */ }
  };


static bg_parameter_info_t * get_parameters_speex()
  {
  return parameters;
  }

static void set_parameter_speex(void * data, const char * name,
                                 const bg_parameter_value_t * v)
  {
  speex_t * speex;
  speex = (speex_t*)data;
  
  if(!name)
    {
    return;
    }
  else if(!strcmp(name, "mode"))
    {
    if(!strcmp(v->val_str, "auto"))
      speex->modeID = -1;
    else if(!strcmp(v->val_str, "nb"))
      speex->modeID = SPEEX_MODEID_NB;
    else if(!strcmp(v->val_str, "wb"))
      speex->modeID = SPEEX_MODEID_WB;
    else if(!strcmp(v->val_str, "uwb"))
      speex->modeID = SPEEX_MODEID_UWB;
    }
  
  else if(!strcmp(name, "bitrate"))
    {
    speex->bitrate = v->val_i * 1000;
    }
  else if(!strcmp(name, "abr_bitrate"))
    {
    speex->abr_bitrate = v->val_i * 1000;
    }
  else if(!strcmp(name, "quality"))
    {
    speex->quality = v->val_i;
    }
  else if(!strcmp(name, "complexity"))
    {
    speex->complexity = v->val_i;
    }
  else if(!strcmp(name, "vbr"))
    {
    speex->vbr = v->val_i;
    }
  else if(!strcmp(name, "vad"))
    {
    speex->vad = v->val_i;
    }
  else if(!strcmp(name, "dtx"))
    {
    speex->dtx = v->val_i;
    }
  else if(!strcmp(name, "nframes"))
    {
    speex->nframes = v->val_i;
    }
  
  }

static void build_comment(char ** comments, int * comments_len, bg_metadata_t * metadata)
  {
  char * tmp_string;
  char * version;

  speex_lib_ctl(SPEEX_LIB_GET_VERSION_STRING, &version);
    
  tmp_string = bg_sprintf("Encoded with Speex %s", version);
  comment_init(comments, comments_len, tmp_string);
  free(tmp_string);
    
  if(metadata->artist)
    comment_add(comments, comments_len, "ARTIST=", metadata->artist);
  
  if(metadata->title)
    comment_add(comments, comments_len, "TITLE=", metadata->title);

  if(metadata->album)
    comment_add(comments, comments_len, "ALBUM=", metadata->album);
    
  if(metadata->genre)
    comment_add(comments, comments_len, "GENRE=", metadata->genre);

  if(metadata->date)
    comment_add(comments, comments_len, "DATE=", metadata->date);
  
  if(metadata->copyright)
    comment_add(comments, comments_len, "COPYRIGHT=", metadata->copyright);

  if(metadata->track)
    {
    tmp_string = bg_sprintf("%d", metadata->track);
    comment_add(comments, comments_len, "TRACKNUMBER=", tmp_string);
    free(tmp_string);
    }

  if(metadata->comment)
    comment_add(comments, comments_len, (char*)0, metadata->comment);
  }

static int init_speex(void * data, gavl_audio_format_t * format, bg_metadata_t * metadata)
  {
  float quality_f;
  char *comments = (char *)0;
  int comments_length = 0;
  SpeexMode *mode=NULL;
  SpeexHeader header;
  ogg_packet op;
  int dummy;
  
  speex_t * speex = (speex_t *)data;

  speex->format = format;

  /* Adjust the format */

  speex->format->interleave_mode = GAVL_INTERLEAVE_ALL;
  speex->format->sample_format = GAVL_SAMPLE_S16;
  if(speex->format->samplerate > 48000)
    speex->format->samplerate = 48000;
  if(speex->format->samplerate < 6000)
    speex->format->samplerate = 6000;

  if(speex->format->num_channels > 2)
    {
    speex->format->num_channels = 2;
    speex->format->channel_locations[0] = GAVL_CHID_NONE;
    gavl_set_channel_setup(speex->format);
    }
  
  /* Decide encoding mode */
  
  if(speex->modeID == -1)
    {
    if(speex->format->samplerate > 25000)
      speex->modeID = SPEEX_MODEID_UWB;
    else if(speex->format->samplerate > 12500)
      speex->modeID = SPEEX_MODEID_WB;
    else
      speex->modeID = SPEEX_MODEID_NB;
    }

  /* Setup header and mode */
    
  mode = speex_lib_get_mode(speex->modeID);

  
  speex_init_header(&header, speex->format->samplerate, 1, mode);
  header.frames_per_packet=speex->nframes;
  header.vbr=speex->vbr;
  header.nb_channels = speex->format->num_channels;

  /* Initialize encoder structs */
  
  ogg_stream_init(&speex->enc_os, speex->serialno);
  speex->enc = speex_encoder_init(mode);
  speex_bits_init(&speex->bits);
  
  /* Setup encoding parameters */

  speex_encoder_ctl(speex->enc, SPEEX_SET_COMPLEXITY, &speex->complexity);
  speex_encoder_ctl(speex->enc, SPEEX_SET_SAMPLING_RATE, &speex->format->samplerate);

  if(speex->vbr)
    {
    quality_f = (float)(speex->quality);
    speex_encoder_ctl(speex->enc, SPEEX_SET_VBR_QUALITY, &quality_f);
    }
  else
    speex_encoder_ctl(speex->enc, SPEEX_SET_QUALITY, &speex->quality);

  if(speex->bitrate)
    {
    speex_encoder_ctl(speex->enc, SPEEX_SET_BITRATE, &speex->bitrate);
    }
  if(speex->vbr)
    {
    speex_encoder_ctl(speex->enc, SPEEX_SET_VBR, &speex->vbr);
    }
  else if(speex->vad)
    {
    speex_encoder_ctl(speex->enc, SPEEX_SET_VAD, &speex->vad);
    }

  if(speex->dtx)
    {
    speex_encoder_ctl(speex->enc, SPEEX_SET_VAD, &speex->dtx);
    }
  if(speex->abr_bitrate)
    {
    speex_encoder_ctl(speex->enc, SPEEX_SET_ABR, &speex->abr_bitrate);
    }

  speex_encoder_ctl(speex->enc, SPEEX_GET_FRAME_SIZE, &speex->format->samples_per_frame);
  speex_encoder_ctl(speex->enc, SPEEX_GET_LOOKAHEAD,  &speex->lookahead);


  /* Allocate temporary frame */

  speex->frame = gavl_audio_frame_create(speex->format);
    
  /* Build comment (comments are UTF-8, good for us :-) */

  build_comment(&comments, &comments_length, metadata);

  /* Build header */
  op.packet = (unsigned char *)speex_header_to_packet(&header, &dummy);
  op.bytes = dummy;
  op.b_o_s = 1;
  op.e_o_s = 0;
  op.granulepos = 0;
  op.packetno = 0;
  
  /* And stream them out */
  ogg_stream_packetin(&speex->enc_os,&op);
  free(op.packet);
  if(!bg_ogg_flush_page(&speex->enc_os, speex->output, 1))
    bg_log(BG_LOG_WARNING, LOG_DOMAIN, "Got no Speex ID page");

  /* Build comment */
  op.packet = (unsigned char *)comments;
  op.bytes = comments_length;
  op.b_o_s = 0;
  op.e_o_s = 0;
  op.granulepos = 0;
  op.packetno = 1;
  ogg_stream_packetin(&speex->enc_os, &op);
  
  return 1;
  }

static int flush_header_pages_speex(void*data)
  {
  speex_t * speex;
  speex = (speex_t*)data;
  if(bg_ogg_flush(&speex->enc_os, speex->output, 1) <= 0)
    return 0;
  return 1;
  }


static int encode_frame(speex_t * speex, int eof)
  {
  int block_align;
  ogg_packet op;

  /* Mute last frame */
  
  if(eof)
    {
    /* Mute rest of last frame and encode it */
    if(speex->frame->valid_samples)
      {
      block_align = speex->format->num_channels *
        gavl_bytes_per_sample(speex->format->sample_format);
      
      memset(speex->frame->samples.u_8 + speex->frame->valid_samples * block_align,
             0,
             (speex->format->samples_per_frame - speex->frame->valid_samples) * block_align);

      if(speex->format->num_channels == 2)
        speex_encode_stereo_int(speex->frame->samples.s_16, speex->format->samples_per_frame,
                                &speex->bits);

      speex_encode_int(speex->enc, speex->frame->samples.s_16, &speex->bits);
      speex->frames_encoded++;
      }

    /* Insert zero frames to fill last packet */

    while(!speex->frames_encoded || (speex->frames_encoded % speex->nframes))
      {
      speex_bits_pack(&speex->bits, 15, 5);
      speex->frames_encoded++;
      }
    }
  
  if(speex->frames_encoded && !(speex->frames_encoded % speex->nframes))
    {
    /* Flush page */
    op.bytes  = speex_bits_write(&speex->bits, (char*)speex->buffer, BUFFER_SIZE);
    op.packet = speex->buffer;
    op.b_o_s  = 0;
    op.e_o_s  = eof;

    if(!eof)    
      op.granulepos = (speex->frames_encoded)*speex->format->samples_per_frame -
        speex->lookahead;
    else
      op.granulepos = speex->samples_read - speex->lookahead;
    
    op.packetno = 2 + (speex->frames_encoded / speex->nframes);
    ogg_stream_packetin(&speex->enc_os, &op);
    speex_bits_reset(&speex->bits);
    if(bg_ogg_flush(&speex->enc_os, speex->output, eof) < 0)
      return 0;
    }
  if(eof)
    return 1;

  if(speex->frame->valid_samples == speex->format->samples_per_frame)
    {
    if(speex->format->num_channels == 2)
      speex_encode_stereo_int(speex->frame->samples.s_16, speex->format->samples_per_frame,
                              &speex->bits);
    
    speex_encode_int(speex->enc, speex->frame->samples.s_16, &speex->bits);
    speex->frame->valid_samples = 0;
    speex->frames_encoded++;
    }
  return 1;
  }

static int write_audio_frame_speex(void * data, gavl_audio_frame_t * frame)
  {
  int result = 1;
  int samples_read = 0;
  int samples_copied;
  speex_t * speex;
  speex = (speex_t*)data;

  while(samples_read < frame->valid_samples)
    {
    samples_copied =
      gavl_audio_frame_copy(speex->format,
                            speex->frame,
                            frame,
                            speex->frame->valid_samples, /* dst_pos */
                            samples_read,                /* src_pos */
                            speex->format->samples_per_frame -
                            speex->frame->valid_samples, /* dst_size */
                            frame->valid_samples - samples_read /* src_size */ );
    speex->frame->valid_samples += samples_copied;
    samples_read += samples_copied;
    result = encode_frame(speex, 0);
    if(!result)
      break;
    }
  
  speex->samples_read += frame->valid_samples;
  return result;
  }

static int close_speex(void * data)
  {
  int ret = 1;
  speex_t * speex;
  speex = (speex_t*)data;

  if(!encode_frame(speex, 1))
    ret = 0;
     

  ogg_stream_clear(&speex->enc_os);
  gavl_audio_frame_destroy(speex->frame);
  speex_encoder_destroy(speex->enc);
  speex_bits_destroy(&speex->bits);
  
  free(speex);
  return ret;
  }


bg_ogg_codec_t bg_speex_codec =
  {
    name:      "speex",
    long_name: TRS("Speex encoder"),
    create: create_speex,

    get_parameters: get_parameters_speex,
    set_parameter:  set_parameter_speex,
    
    init_audio:     init_speex,
    
    //  int (*init_video)(void*, gavl_video_format_t * format);
  
    flush_header_pages: flush_header_pages_speex,
    
    encode_audio: write_audio_frame_speex,
    close: close_speex,
  };
