/*****************************************************************
 
  demux_mpegaudio.c
 
  Copyright (c) 2003-2004 by Burkhard Plaum - plaum@ipf.uni-stuttgart.de
 
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
#include <ctype.h>
#include <avdec_private.h>
// #include <id3.h>
#include <xing.h>
#include <utils.h>

#define MAX_FRAME_BYTES 2881

/*
 *  This demuxer handles mpegaudio (mp3) along with id3tags,
 *  AlbumWrap files seeking (incl VBR) and so on
 */

/* MPEG Audio header parsing code */

static int mpeg_bitrates[5][16] = {
  /* MPEG-1 */
  { 0,  32000,  64000,  96000, 128000, 160000, 192000, 224000,    // I
       256000, 288000, 320000, 352000, 384000, 416000, 448000, 0},
  { 0,  32000,  48000,  56000,  64000,  80000,  96000, 112000,    // II
       128000, 160000, 192000, 224000, 256000, 320000, 384000, 0 },
  { 0,  32000,  40000,  48000,  56000,  64000,  80000,  96000,    // III
       112000, 128000, 160000, 192000, 224000, 256000, 320000, 0 },
   
  /* MPEG-2 LSF */
  { 0,  32000,  48000,  56000,  64000,  80000,  96000, 112000,    // I
       128000, 144000, 160000, 176000, 192000, 224000, 256000, 0 },
  { 0,   8000,  16000,  24000,  32000,  40000,  48000,  56000,
        64000,  80000,  96000, 112000, 128000, 144000, 160000, 0 } // II & III
};

static int mpeg_samplerates[3][3] = {
  { 44100, 48000, 32000 }, // MPEG1
  { 22050, 24000, 16000 }, // MPEG2
  { 11025, 12000, 8000 }   // MPEG2.5
};

#define MPEG_ID_MASK        0x00180000
#define MPEG_MPEG1          0x00180000
#define MPEG_MPEG2          0x00100000
#define MPEG_MPEG2_5        0x00000000
                                                                                
#define MPEG_LAYER_MASK     0x00060000
#define MPEG_LAYER_III      0x00020000
#define MPEG_LAYER_II       0x00040000
#define MPEG_LAYER_I        0x00060000
#define MPEG_PROTECTION     0x00010000
#define MPEG_BITRATE_MASK   0x0000F000
#define MPEG_FREQUENCY_MASK 0x00000C00
#define MPEG_PAD_MASK       0x00000200
#define MPEG_PRIVATE_MASK   0x00000100
#define MPEG_MODE_MASK      0x000000C0
#define MPEG_MODE_EXT_MASK  0x00000030
#define MPEG_COPYRIGHT_MASK 0x00000008
#define MPEG_HOME_MASK      0x00000004
#define MPEG_EMPHASIS_MASK  0x00000003
#define LAYER_I_SAMPLES       384
#define LAYER_II_III_SAMPLES 1152

/* Header detection stolen from the mpg123 plugin of xmms */

static int header_check(uint32_t head)
{
        if ((head & 0xffe00000) != 0xffe00000)
                return 0;
        if (!((head >> 17) & 3))
                return 0;
        if (((head >> 12) & 0xf) == 0xf)
                return 0;
        if (!((head >> 12) & 0xf))
                return 0;
        if (((head >> 10) & 0x3) == 0x3)
                return 0;
        if (((head >> 19) & 1) == 1 &&
            ((head >> 17) & 3) == 3 &&
            ((head >> 16) & 1) == 1)
                return 0;
        if ((head & 0xffff0000) == 0xfffe0000)
                return 0;
        //        fprintf(stderr, "Head check ok %08x\n", head);
        return 1;
}


#if 0
#define IS_MPEG_AUDIO_HEADER(h) (((h&0xFFE00000)==0xFFE00000)&&\
                                ((h&0x0000F000)!=0x0000F000)&&\
                                ((h&0x00060000)!=0)&&\
                                ((h&0x00180000)!=0x00080000)&&\
                                ((h&0x00000C00)!=0x00000C00))
#endif

typedef enum
  {
    MPEG_VERSION_NONE = 0,
    MPEG_VERSION_1 = 1,
    MPEG_VERSION_2 = 2,
    MPEG_VERSION_2_5
  } mpeg_version_t;

#define CHANNEL_STEREO   0
#define CHANNEL_JSTEREO  1
#define CHANNEL_DUAL     2
#define CHANNEL_MONO     3

typedef struct
  {
  mpeg_version_t version;
  int layer;
  int bitrate;    /* -1: VBR */
  int samplerate;
  int frame_bytes;
  int channel_mode;
  int mode;
  int samples_per_frame;
  } mpeg_header;


static int header_equal(mpeg_header * h1, mpeg_header * h2)
  {
  return ((h1->version == h2->version) &&
          (h1->layer == h2->layer) &&
          (h1->samplerate == h2->samplerate));
  }

#if 0
static void dump_header(mpeg_header * h)
  {
  fprintf(stderr, "Header:\n");
  fprintf(stderr, "  Version:     %s\n",
          (h->version == MPEG_VERSION_1 ? "1" :
           (h->version == MPEG_VERSION_2 ? "2" : "2.5")));
  fprintf(stderr, "  Layer:       %d\n", h->layer);
  fprintf(stderr, "  Bitrate:     %d\n", h->bitrate);
  fprintf(stderr, "  Samplerate:  %d\n", h->samplerate);
  fprintf(stderr, "  Frame bytes: %d\n", h->frame_bytes);

  switch(h->channel_mode)
    {
    case CHANNEL_STEREO:
      fprintf(stderr, "  Channel mode: Stereo\n");
      break;
    case CHANNEL_JSTEREO:
      fprintf(stderr, "  Channel mode: Joint Stereo\n");
      break;
    case CHANNEL_DUAL:
      fprintf(stderr, "  Channel mode: Dual\n");
      break;
    case CHANNEL_MONO:
      fprintf(stderr, "  Channel mode: Mono\n");
      break;
    }
  
  }
#endif

static int decode_header(mpeg_header * h, uint8_t * ptr)
  {
  uint32_t header;
  int index;
  /* For calculation of the byte length of a frame */
  int pad;
  int slots_per_frame;
  h->frame_bytes = 0;
  header =
    ptr[3] | (ptr[2] << 8) | (ptr[1] << 16) | (ptr[0] << 24);
  if(!header_check(header))
    return 0;
  //  fprintf(stderr, "*** Header: 0x%08x\n", header);

  index = (header & MPEG_MODE_MASK) >> 6;
  switch(index)
    {
    case 0:
      h->channel_mode = CHANNEL_STEREO;
      break;
    case 1:
      h->channel_mode = CHANNEL_JSTEREO;
      break;
    case 2:
      h->channel_mode = CHANNEL_DUAL;
      break;
    case 3:
      h->channel_mode = CHANNEL_MONO;
      break;
    }
  /* Get Version */
  switch(header & MPEG_ID_MASK)
    {
    case MPEG_MPEG1:
      h->version = MPEG_VERSION_1;
        break;
    case MPEG_MPEG2:
      h->version = MPEG_VERSION_2;
      break;
    case MPEG_MPEG2_5:
      h->version = MPEG_VERSION_2_5;
      break;
    default:
      return 0;
    }
  /* Get Layer */
  switch(header & MPEG_LAYER_MASK)
    {
    case MPEG_LAYER_I:
      h->layer = 1;
      break;
    case MPEG_LAYER_II:
      h->layer = 2;
      break;
    case MPEG_LAYER_III:
      h->layer = 3;
      break;
    }
  index = (header & MPEG_BITRATE_MASK) >> 12;
  switch(h->version)
    {
    case MPEG_VERSION_1:
      //         cerr << "MPEG-1 audio layer " << h->layer
      //              << " bitrate index: " << bitrate_index << "\n";
      switch(h->layer)
        {
        case 1:
          h->bitrate = mpeg_bitrates[0][index];
          break;
        case 2:
          h->bitrate = mpeg_bitrates[1][index];
          break;
        case 3:
          h->bitrate = mpeg_bitrates[2][index];
          break;
        }
      break;
    case MPEG_VERSION_2:
    case MPEG_VERSION_2_5:
      //         cerr << "MPEG-2 audio layer " << h->layer
      //              << " bitrate index: " << bitrate_index << "\n";
      switch(h->layer)
        {
        case 1:
          h->bitrate = mpeg_bitrates[3][index];
          break;
        case 2:
        case 3:
          h->bitrate = mpeg_bitrates[4][index];
          break;
        }
      break;
    default: // This won't happen, but keeps gcc quiet
      return 0;
    }
  index = (header & MPEG_FREQUENCY_MASK) >> 10;
  switch(h->version)
    {
    case MPEG_VERSION_1:
      h->samplerate = mpeg_samplerates[0][index];
      break;
    case MPEG_VERSION_2:
      h->samplerate = mpeg_samplerates[1][index];
      break;
    case MPEG_VERSION_2_5:
      h->samplerate = mpeg_samplerates[2][index];
      break;
    default: // This won't happen, but keeps gcc quiet
      return 0;
    }
  pad = (header & MPEG_PAD_MASK) ? 1 : 0;
  if(h->layer == 1)
    {
    h->frame_bytes = ((12 * h->bitrate / h->samplerate) + pad) * 4;
    }
  else
    {
    slots_per_frame = ((h->layer == 3) &&
      ((h->version == MPEG_VERSION_2) ||
       (h->version == MPEG_VERSION_2_5))) ? 72 : 144;
    h->frame_bytes = (slots_per_frame * h->bitrate) / h->samplerate + pad;
    }
  // h->mode = (ptr[3] >> 6) & 3;

  h->samples_per_frame =
    (h->layer == 1) ? LAYER_I_SAMPLES : LAYER_II_III_SAMPLES;

  if(h->version != MPEG_VERSION_1)
    h->samples_per_frame /= 2;
  
  //  dump_header(h);
  return 1;
  }

/* ALBW decoder */

typedef struct
  {
  int num_tracks;

  struct
    {
    //    int64_t position;
    //    int64_t size;
    int64_t start_pos;
    int64_t end_pos;
    char * filename;
    } * tracks;
  } bgav_albw_t;

#if 0
static void bgav_albw_dump(bgav_albw_t * a)
  {
  int i;
  fprintf(stderr, "Tracks: %d\n", a->num_tracks);

  for(i = 0; i < a->num_tracks; i++)
    fprintf(stderr, "Start: %lld, End: %lld, File: %s\n",
            a->tracks[i].start_pos,
            a->tracks[i].end_pos,
            a->tracks[i].filename);
  }
#endif
           
static int bgav_albw_probe(bgav_input_context_t * input)
  {
  uint8_t probe_data[4];

  if(bgav_input_get_data(input, probe_data, 4) < 4)
    return 0;
  if(!isdigit(probe_data[0]) ||
     (!isdigit(probe_data[1]) && probe_data[1] != ' ') ||
     (!isdigit(probe_data[2]) && probe_data[2] != ' ') ||
     (!isdigit(probe_data[3]) && probe_data[3] != ' '))
    return 0;
  return 1;
  }

#if 0
#define BUF_SIZE 1024

static void bgav_albw_unwrap(bgav_input_context_t * input, bgav_albw_t * a)
  {
  int i;
  char buffer[BUF_SIZE];
  int bytes_read;
  int bytes_to_read;

  FILE * output;
  for(i = 0; i < a->num_tracks; i++)
    {
    output = fopen(a->tracks[i].filename, "wb");
    //    fprintf(stderr, "%d: %s\n", i+1, a->tracks[i].filename);
    bytes_read = 1;
    while(bytes_read)
      {
      if(a->tracks[i].end_pos < input->position + BUF_SIZE)
        bytes_to_read =
          a->tracks[i].end_pos - input->position;
      else
        bytes_to_read = BUF_SIZE;
      if(!bytes_to_read)
        return;
      bytes_read = bgav_input_read_data(input, buffer, bytes_to_read);
      if(bytes_read < bytes_to_read)
        fprintf(stderr, "Unexpected EOF, exiting\n");
      fwrite(buffer, 1,  bytes_read, output);
      }
    fclose(output);
    }
  }
#endif

static void bgav_albw_destroy(bgav_albw_t * a)
  {
  int i;
  for(i = 0; i < a->num_tracks; i++)
    {
    if(a->tracks[i].filename)
      free(a->tracks[i].filename);
    }
  free(a->tracks);
  free(a);
  }

static bgav_albw_t * bgav_albw_read(bgav_input_context_t * input)
  {
  int i;
  double position;
  double size;
  char * rest;
  char * pos;
  char buffer[512];
  int64_t diff;

  bgav_albw_t * ret = (bgav_albw_t *)0;
  
  if(bgav_input_read_data(input, buffer, 12) < 12)
    goto fail;

  ret = calloc(1, sizeof(*ret));

  ret->num_tracks = atoi(buffer);

  ret->tracks = calloc(ret->num_tracks, sizeof(*(ret->tracks)));

  for(i = 0; i < ret->num_tracks; i++)
    {
    if(bgav_input_read_data(input, buffer, 501) < 501)
      goto fail;
    pos = buffer;

    size = strtod(pos, &rest);
    pos = rest;
    if(strncmp(pos, "[][]", 4))
      goto fail;
    
    pos += 4;
    position = strtod(pos, &rest);
    pos = rest;

    size     *= 10000.0;
    position *= 10000.0;
    
    ret->tracks[i].start_pos = (int64_t)(position + 0.5);
    ret->tracks[i].end_pos = ret->tracks[i].start_pos + (int64_t)(size + 0.5);

    if(strncmp(pos, "[][]", 4))
      goto fail;
    pos += 4;
    rest = &(buffer[500]);
    while(isspace(*rest))
      rest--;
    rest++;
    ret->tracks[i].filename = bgav_strndup(pos, rest);
    }

  diff = input->position - ret->tracks[0].start_pos;
  
  if(diff > 0)
    {
    //    fprintf(stderr, "WARNING: first file starts at %lld, pos: %lld (diff: %lld)\n",
    //            ret->tracks[0].start_pos, input->position,
    //            diff);
    for(i = 0; i < ret->num_tracks; i++)
      {
      ret->tracks[i].start_pos += diff;
      }
    }
  
  return ret;
  
  fail:
  if(ret)
    bgav_albw_destroy(ret);
  return (bgav_albw_t*)0;
  }

/* This is the actual demuxer */

typedef struct
  {
  int64_t data_start;
  int64_t data_end;

  bgav_albw_t * albw;
  
  /* Global metadata */
  bgav_metadata_t metadata;
  
  bgav_xing_header_t xing;
  int have_xing;
  mpeg_header header;

  int64_t frames;
  } mpegaudio_priv_t;

static void select_track_mpegaudio(bgav_demuxer_context_t * ctx,
                                   int track);

#define MAX_BYTES 2885 /* Maximum size of an mpeg audio frame + 4 bytes for next header */

static int probe_mpegaudio(bgav_input_context_t * input)
  {
  mpeg_header h1, h2;
  uint8_t probe_data[MAX_BYTES];
  
  /* Check for audio header */

  if(input->id3v2 && bgav_albw_probe(input))
    return 1;
  
  if(bgav_input_get_data(input, probe_data, 4) < 4)
    return 0;

  if(!decode_header(&h1, probe_data))
    {
    return 0;
    }

  /* Now, we look where the next header might come
     and decode from that point */

  if(h1.frame_bytes > 2881) /* Prevent possible security hole */
    return 0;

  if(bgav_input_get_data(input, probe_data, h1.frame_bytes + 4) < h1.frame_bytes + 4)
    return 0;

  if(!decode_header(&h2, &(probe_data[h1.frame_bytes])))
    return 0;

  if(!header_equal(&h1, &h2))
    return 0;

  return 1;
  }

static int resync(bgav_demuxer_context_t * ctx)
  {
  uint8_t buffer[4];
  mpegaudio_priv_t * priv;
  int skipped_bytes = 0;
  
  priv = (mpegaudio_priv_t*)(ctx->priv);

  while(1)
    {
    if(bgav_input_get_data(ctx->input, buffer, 4) < 4)
      return 0;
    if(decode_header(&(priv->header), buffer))
      break;
    else
      {
      bgav_input_skip(ctx->input, 1);
      skipped_bytes++;
      }
    }
  if(skipped_bytes)
    fprintf(stderr, "Skipped %d bytes in mpeg audio stream\n", skipped_bytes);
  return 1;
  }

static gavl_time_t get_duration(bgav_demuxer_context_t * ctx,
                                int64_t start_offset,
                                int64_t end_offset)
  {
  gavl_time_t ret = GAVL_TIME_UNDEFINED;
  uint8_t frame[MAX_FRAME_BYTES]; /* Max possible mpeg audio frame size */
  mpegaudio_priv_t * priv;

  //  memset(&(priv->xing), 0, sizeof(xing));

  if(!(ctx->input->input->seek_byte))
    return GAVL_TIME_UNDEFINED;
  
  priv = (mpegaudio_priv_t*)(ctx->priv);
  
  bgav_input_seek(ctx->input, start_offset, SEEK_SET);
  if(!resync(ctx))
    return 0;
  
  if(bgav_input_get_data(ctx->input, frame,
                         priv->header.frame_bytes) < priv->header.frame_bytes)
    return 0;
  
  if(bgav_xing_header_read(&(priv->xing), frame))
    {
    //    bgav_xing_header_dump(&(priv->xing));
    ret = gavl_samples_to_time(priv->header.samplerate,
                               (int64_t)(priv->xing.frames) *
                               priv->header.samples_per_frame);
    }
  else if(end_offset > start_offset)
    {
    ret = (GAVL_TIME_SCALE * (end_offset - start_offset) * 8) /
      (priv->header.bitrate);
    }
  return ret;
  }

static int set_stream(bgav_demuxer_context_t * ctx)
     
  {
  char * bitrate_string;
  const char * version_string;
  bgav_stream_t * s;
  uint8_t frame[MAX_FRAME_BYTES]; /* Max possible mpeg audio frame size */
  mpegaudio_priv_t * priv;
  
  priv = (mpegaudio_priv_t*)(ctx->priv);
  if(!resync(ctx))
    return 0;
  
  /* Check for a VBR header */
  
  if(bgav_input_get_data(ctx->input, frame,
                         priv->header.frame_bytes) < priv->header.frame_bytes)
    return 0;
  
  if(bgav_xing_header_read(&(priv->xing), frame))
    {
    //    fprintf(stderr, "*** VBR Header found %d\n", priv->xing.frames);
    priv->have_xing = 1;
    bgav_input_skip(ctx->input, priv->header.frame_bytes);
    priv->data_start += priv->header.frame_bytes;
    }
  else
    {
    //    fprintf(stderr, "*** NO VBR Header found\n");
    //    bgav_hexdump(frame, priv->header.frame_bytes, 16);
    priv->have_xing = 0;
    }

  s = ctx->tt->current_track->audio_streams;

  /* Get audio format */
  
  if(priv->header.channel_mode == CHANNEL_MONO)
    {
    s->data.audio.format.num_channels = 1;
    s->data.audio.format.channel_setup = GAVL_CHANNEL_MONO;
    }
  else
    {
    s->data.audio.format.num_channels = 2;
    s->data.audio.format.channel_setup = GAVL_CHANNEL_STEREO;
    }
  s->data.audio.format.samplerate = priv->header.samplerate;
  s->data.audio.format.samples_per_frame = priv->header.samples_per_frame;

  if(priv->have_xing)
    {
    s->container_bitrate = BGAV_BITRATE_VBR;
    s->codec_bitrate     = BGAV_BITRATE_VBR;
    }
  else
    {
    s->container_bitrate = priv->header.bitrate;
    s->codec_bitrate     = priv->header.bitrate;
    }
  
  if(s->description)
    free(s->description);
    
  switch(priv->header.version)
    {
    case MPEG_VERSION_1:
      version_string = "1";
      break;
    case MPEG_VERSION_2:
      version_string = "2";
      break;
    case MPEG_VERSION_2_5:
      version_string = "2.5";
      break;
    default:
      version_string = "Not specified";
      break;
    }
    
  if(priv->have_xing)
    bitrate_string = bgav_strndup("Variable", NULL);
  else
    bitrate_string =
      bgav_sprintf("%d kb/s",
                   s->container_bitrate/1000);

  ctx->stream_description =
    bgav_sprintf("MPEG-%s layer %d, bitrate: %s",
                 version_string, 
                 priv->header.layer, bitrate_string);
  free(bitrate_string);

  /* Get stream duration */

  ctx->tt->current_track->duration = ctx->tt->current_track->duration;
  return 1;
  }

static void get_metadata_albw(bgav_input_context_t* input,
                              int64_t * start_position,
                              int64_t * end_position,
                              bgav_metadata_t * metadata)
  {
  bgav_metadata_t metadata_v1;
  bgav_metadata_t metadata_v2;

  bgav_id3v1_tag_t * id3v1 = NULL;
  bgav_id3v2_tag_t * id3v2 = NULL;

  memset(&(metadata_v1), 0, sizeof(metadata_v1));
  memset(&(metadata_v2), 0, sizeof(metadata_v2));

  bgav_input_seek(input, *start_position, SEEK_SET);
  
  if(bgav_id3v2_probe(input))
    {
    id3v2 = bgav_id3v2_read(input);
    if(id3v2)
      {
      *start_position += bgav_id3v2_total_bytes(id3v2);
      bgav_id3v2_2_metadata(id3v2, &(metadata_v2));
      }
    }
  
  bgav_input_seek(input, *end_position - 128, SEEK_SET);

  if(bgav_id3v1_probe(input))
    {
    id3v1 = bgav_id3v1_read(input);
    if(id3v1)
      {
      *end_position -= 128;
      bgav_id3v1_2_metadata(id3v1, &(metadata_v1));
      }
    }
  bgav_metadata_merge(metadata, &metadata_v2, &metadata_v1);
  bgav_metadata_free(&metadata_v1);
  bgav_metadata_free(&metadata_v2);

  if(id3v1)
    bgav_id3v1_destroy(id3v1);
  if(id3v2)
    bgav_id3v2_destroy(id3v2);
  
  }

static bgav_track_table_t * albw_2_track(bgav_demuxer_context_t* ctx,
                                         bgav_albw_t * albw,
                                         bgav_metadata_t * global_metadata)
  {
  int i;
  const char * end_pos;
  bgav_track_table_t * ret;
  bgav_stream_t * s;
  bgav_metadata_t track_metadata;

  memset(&(track_metadata), 0, sizeof(track_metadata));
    
  if(!ctx->input->input->seek_byte)
    {
    return (bgav_track_table_t *)0;
    }
  
  ret = bgav_track_table_create(albw->num_tracks);
  
  for(i = 0; i < albw->num_tracks; i++)
    {
    s = bgav_track_add_audio_stream(&(ret->tracks[i]));
    s->fourcc = BGAV_MK_FOURCC('.', 'm', 'p', '3');
    end_pos = strrchr(albw->tracks[i].filename, '.');
    ret->tracks[i].name = bgav_strndup(albw->tracks[i].filename, end_pos);

    get_metadata_albw(ctx->input,
                      &(albw->tracks[i].start_pos),
                      &(albw->tracks[i].end_pos),
                      &track_metadata);
    bgav_metadata_merge(&(ret->tracks[i].metadata),
                        &track_metadata, global_metadata);
    bgav_metadata_free(&track_metadata);
    
    ret->tracks[i].duration = get_duration(ctx,
                                           albw->tracks[i].start_pos,
                                           albw->tracks[i].end_pos);
      
    
//    fprintf(stderr, "*** ret[i].duration: %f\n", gavl_time_to_seconds(ret->tracks[i].duration));
    }
  
  return ret;
  }

static int open_mpegaudio(bgav_demuxer_context_t * ctx,
                          bgav_redirector_context_t ** redir)
  {
  bgav_metadata_t metadata_v1;
  bgav_metadata_t metadata_v2;

  bgav_id3v1_tag_t * id3v1 = NULL;
  bgav_stream_t * s;
  
  mpegaudio_priv_t * priv;
  int64_t oldpos;
    
  memset(&(metadata_v1), 0, sizeof(metadata_v1));
  memset(&(metadata_v2), 0, sizeof(metadata_v2));
  
  priv = calloc(1, sizeof(*priv));
  ctx->priv = priv;    
  
  if(ctx->input->id3v2)
    {
    //    fprintf(stderr, "Found ID3V2!\n");
    
    bgav_id3v2_2_metadata(ctx->input->id3v2, &(metadata_v2));
    
    /* Check for ALBW, but only on a seekable source! */
    if(ctx->input->input->seek_byte &&
       bgav_albw_probe(ctx->input))
      {
      priv->albw = bgav_albw_read(ctx->input);
      //      bgav_albw_dump(priv->albw);
      }
    }
  
  if(ctx->input->input->seek_byte && !priv->albw)
    {
    oldpos = ctx->input->position;
    bgav_input_seek(ctx->input, -128, SEEK_END);

    if(bgav_id3v1_probe(ctx->input))
      {
      id3v1 = bgav_id3v1_read(ctx->input);
      if(id3v1)
        bgav_id3v1_2_metadata(id3v1, &(metadata_v1));
      }
    bgav_input_seek(ctx->input, oldpos, SEEK_SET);
    }
  bgav_metadata_merge(&(priv->metadata), &(metadata_v2), &(metadata_v1));
  
  if(priv->albw)
    {
    ctx->tt = albw_2_track(ctx, priv->albw, &(priv->metadata));
    }
  else /* We know the start and end offsets right now */
    {
    ctx->tt = bgav_track_table_create(1);

    s = bgav_track_add_audio_stream(ctx->tt->tracks);
    s->fourcc = BGAV_MK_FOURCC('.', 'm', 'p', '3');
    
    if(ctx->input->input->seek_byte)
      {
      priv->data_start = (ctx->input->id3v2) ?
        bgav_id3v2_total_bytes(ctx->input->id3v2) : 0;
      priv->data_end   = (id3v1) ? ctx->input->total_bytes - 128 :
        ctx->input->total_bytes;
      }
    ctx->tt->tracks[0].duration = get_duration(ctx,
                                               priv->data_start,
                                               priv->data_end);
    bgav_metadata_merge(&(ctx->tt->tracks[0].metadata),
                        &metadata_v2, &metadata_v1);
    }

  if(id3v1)
    bgav_id3v1_destroy(id3v1);
  bgav_metadata_free(&metadata_v1);
  bgav_metadata_free(&metadata_v2);
  
  if(ctx->input->input->seek_byte)
    ctx->can_seek = 1;

  if(!ctx->tt->tracks[0].name && ctx->input->metadata.title)
    {
    // fprintf(stderr, "demux_mpegaudio: Setting track name from metadata title\n");
    ctx->tt->tracks[0].name = bgav_strndup(ctx->input->metadata.title,
                                           NULL);
    }
  return 1;
  }

static int next_packet_mpegaudio(bgav_demuxer_context_t * ctx)
  {
  bgav_packet_t * p;
  bgav_stream_t * s;
  mpegaudio_priv_t * priv;
  priv = (mpegaudio_priv_t*)(ctx->priv);

  
  if(priv->data_end && (priv->data_end - ctx->input->position < 4))
    {
//    fprintf(stderr, "Stream finished %lld %lld\n",
//            priv->data_end, ctx->input->position);
    return 0;
    }
  if(!resync(ctx))
    {
//    fprintf(stderr, "Lost sync\n");
    return 0;
    }
  s = ctx->tt->current_track->audio_streams;
  p = bgav_packet_buffer_get_packet_write(s->packet_buffer, s);
  bgav_packet_alloc(p, priv->header.frame_bytes);

  if(bgav_input_read_data(ctx->input, p->data, priv->header.frame_bytes) <
     priv->header.frame_bytes)
    {
//    fprintf(stderr, "EOF\n");
    return 0;
    }
  p->data_size = priv->header.frame_bytes;
  
  p->keyframe  = 1;
  p->timestamp_scaled = priv->frames * (int64_t)priv->header.samples_per_frame;

  bgav_packet_done_write(p);

  //  fprintf(stderr, "Frame: %lld\n", priv->frames);

  priv->frames++;
  return 1;
  }

static void seek_mpegaudio(bgav_demuxer_context_t * ctx, gavl_time_t time)
  {
  int64_t pos;
  mpegaudio_priv_t * priv;
  bgav_stream_t * s;
  
  priv = (mpegaudio_priv_t*)(ctx->priv);
  s = ctx->tt->current_track->audio_streams;
  
  if(priv->have_xing) /* VBR */
    {
    pos =
      bgav_xing_get_seek_position(&(priv->xing),
                                  100.0 * (float)time / (float)(ctx->tt->current_track->duration));
    //    fprintf(stderr, "VBR Seek position: %lld\n", pos);
    //    bgav_xing_header_dump(&(priv->xing));
    }
  else /* CBR */
    {
    pos = ((priv->data_end - priv->data_start) * time) / ctx->tt->current_track->duration;
    }

  s->time_scaled =
    gavl_time_to_samples(ctx->tt->current_track->audio_streams[0].data.audio.format.samplerate, time);
  
  pos += priv->data_start;
  //  fprintf(stderr, "Seeking: %lld %lld %lld %f %f %f\n", pos, priv->data_start, priv->data_end,
  //          (float)time / (float)(ctx->duration), gavl_time_to_seconds(time),
  //          gavl_time_to_seconds(ctx->duration));
  bgav_input_seek(ctx->input, pos, SEEK_SET);
  }

static void close_mpegaudio(bgav_demuxer_context_t * ctx)
  {
  mpegaudio_priv_t * priv;
  priv = (mpegaudio_priv_t*)(ctx->priv);
  
  bgav_metadata_free(&(priv->metadata));
  
  if(priv->albw)
    bgav_albw_destroy(priv->albw);
  free(priv);
  }

static void select_track_mpegaudio(bgav_demuxer_context_t * ctx,
                                   int track)
  {
  mpegaudio_priv_t * priv;

  priv = (mpegaudio_priv_t *)(ctx->priv);

  if(priv->albw)
    {
    priv->data_start = priv->albw->tracks[track].start_pos;
    priv->data_end   = priv->albw->tracks[track].end_pos;
    }
  //  return;
  if(ctx->input->input->seek_byte)
    bgav_input_seek(ctx->input, priv->data_start, SEEK_SET);
  set_stream(ctx);
  }

bgav_demuxer_t bgav_demuxer_mpegaudio =
  {
    probe:        probe_mpegaudio,
    open:         open_mpegaudio,
    next_packet:  next_packet_mpegaudio,
    seek:         seek_mpegaudio,
    close:        close_mpegaudio,
    select_track: select_track_mpegaudio
  };
