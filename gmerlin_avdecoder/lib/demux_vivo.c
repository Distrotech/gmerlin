/*****************************************************************
 
  demux_vivo.c
 
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

#include <avdec_private.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#define AUDIO_STREAM_ID 0
#define VIDEO_STREAM_ID 1


/*
 *  VIVO demuxer
 *  Written from MPlayer, cleaned up a lot and made
 *  reentrant
 */

/* VIVO audio standards from vivog723.acm:

    G.723:
        FormatTag = 0x111
        Channels = 1 - mono
        SamplesPerSec = 8000 - 8khz
        AvgBytesPerSec = 800
        BlockAlign (bytes per block) = 24
        BitsPerSample = 8

    Siren:
        FormatTag = 0x112
        Channels = 1 - mono
        SamplesPerSec = 16000 - 16khz
        AvgBytesPerSec = 2000
        BlockAlign (bytes per block) = 40
        BitsPerSample = 8
*/

typedef struct
  {
  /* Main stuff */

  int version;
  float fps;
  uint32_t duration;
  uint32_t length;
  uint32_t rate;
  uint32_t vid_rate;
  uint32_t playtime1;
  uint32_t playtime2;
  int buffer;
  uint32_t preroll;
  
  char * title;
  char * author;
  char * copyright;
  char * producer;

  /* These seem to be present in VIVO 2 only */
    
  int width;
  int height;
  int display_width;
  int display_height;

  /* Additional headers */
  
  int have_record_2;
    
  struct
    {
    int time_unit_num;
    int time_unit_den;
    } record_2;

  /* Record type 3 and 4 seem to be identical */

  int have_record_3_4;
  
  struct
    {
    int type; /* 3 or 4 */
    uint32_t length;
    uint32_t initial_frame_length;
    uint32_t nominal_bitrate;
    uint32_t sampling_frequency;
    uint32_t gain_factor;
    } record_3_4;
      
  } vivo_header_t;

static int read_length(bgav_input_context_t * input)
  {
  uint8_t c;
  int len;
  
  if(!bgav_input_read_data(input, &c, 1))
    return -1;

  len = c;
  while(c & 0x80)
    {
    len = 0x80 * (len - 0x80);

    if(!bgav_input_read_data(input, &c, 1))
      return -1;
    len += c;
    }
  return len;
  }

static int check_key(uint8_t * buffer, const uint8_t * key, uint8_t ** pos)
  {
  char * pos1;
  
  if(!strncmp(buffer, key, strlen(key)))
    {
    pos1 = strchr(buffer, ':');
    if(pos1)
      {
      pos1++;
      while(isspace(*pos1) && (*pos1 != '\0'))
        pos1++;

      if(*pos1 == '\0')
        return 0;
      *pos = pos1;
      return 1;
      }
    }
  return 0;
  }

static void vivo_header_dump(vivo_header_t * h)
  {
  fprintf(stderr, "Main VIVO header\n");
  fprintf(stderr, "  Version:       %d\n", h->version);
  fprintf(stderr, "  FPS:           %f\n", h->fps);
  fprintf(stderr, "  Duration:      %d\n", h->duration);
  fprintf(stderr, "  Length:        %d\n", h->length);
  fprintf(stderr, "  Rate:          %d\n", h->rate);
  fprintf(stderr, "  VidRate:       %d\n", h->vid_rate);
  fprintf(stderr, "  Playtime1:     %d\n", h->playtime1);
  fprintf(stderr, "  Playtime2:     %d\n", h->playtime2);
  fprintf(stderr, "  Buffer:        %d\n", h->buffer);
  fprintf(stderr, "  Preroll:       %d\n", h->preroll);
  fprintf(stderr, "  Title:         %s\n", h->title);
  fprintf(stderr, "  Author:        %s\n", h->author);
  fprintf(stderr, "  Copyright:     %s\n", h->copyright);
  fprintf(stderr, "  Producer:      %s\n", h->producer);
  fprintf(stderr, "  Width:         %d\n", h->width);
  fprintf(stderr, "  Height:        %d\n", h->height);
  fprintf(stderr, "  DisplayWidth:  %d\n", h->display_width);
  fprintf(stderr, "  DisplayHeight: %d\n", h->display_height);

  if(h->have_record_2)
    {
    fprintf(stderr, "RecordType 2\n");
    fprintf(stderr, "  TimeUnitNumerator:   %d\n", h->record_2.time_unit_num);
    fprintf(stderr, "  TimeUnitDenominator: %d\n", h->record_2.time_unit_den);
    }

  if(h->have_record_3_4)
    {
    fprintf(stderr, "RecordType %d\n", h->record_3_4.type);
    fprintf(stderr, "  Length:             %d\n", h->record_3_4.length);
    fprintf(stderr, "  InitialFrameLength: %d\n", h->record_3_4.initial_frame_length);
    fprintf(stderr, "  SamplingFrequency:  %d\n", h->record_3_4.sampling_frequency);
    fprintf(stderr, "  GainFactor:         %d\n", h->record_3_4.gain_factor);
    }
  
  }

static int vivo_header_read(vivo_header_t * ret, bgav_input_context_t * input)
  {
  uint8_t c;
  int len;
  int64_t header_start;
  char * buffer = (uint8_t*)0;
  uint8_t * pos = (uint8_t*)0;
  int result = 0;
  int record_type;
    
  int buffer_alloc = 0;
  
  /* First, read the main stuff */

  if(!bgav_input_read_data(input, &c, 1))
    goto fail;

  if((c & 0xf0) != 0x00)
    goto fail;

  len = read_length(input);
  if(len < 0)
    goto fail;

  header_start = input->position;
  
  while(input->position < header_start + len)
    {
    if(!bgav_input_read_line(input, &buffer, &buffer_alloc, 0))
      goto fail;

    if(check_key(buffer, "Version", &pos))
      {
      while(!isdigit(*pos) && (*pos != '\0'))
        pos++;
      if(*pos == '\0')
        goto fail;
      ret->version = atoi(pos);
      }
    else if(check_key(buffer, "FPS", &pos))
      ret->fps = atof(pos);
    else if(check_key(buffer, "Duration", &pos))
      ret->duration = strtoul(pos, NULL, 10);
    else if(check_key(buffer, "Rate", &pos))
      ret->rate = strtoul(pos, NULL, 10);
    else if(check_key(buffer, "VidRate", &pos))
      ret->vid_rate = strtoul(pos, NULL, 10);
    else if(check_key(buffer, "Playtime1", &pos))
      ret->playtime1 = strtoul(pos, NULL, 10);
    else if(check_key(buffer, "Playtime2", &pos))
      ret->playtime2 = strtoul(pos, NULL, 10);
    else if(check_key(buffer, "Buffer", &pos))
      ret->buffer = strtol(pos, NULL, 10);
    else if(check_key(buffer, "Preroll", &pos))
      ret->preroll = strtoul(pos, NULL, 10);
    else if(check_key(buffer, "Title", &pos))
      ret->title = bgav_strndup(pos, NULL);
    else if(check_key(buffer, "Author", &pos))
      ret->author = bgav_strndup(pos, NULL);
    else if(check_key(buffer, "Copyright", &pos))
      ret->copyright = bgav_strndup(pos, NULL);
    else if(check_key(buffer, "Producer", &pos))
      ret->producer = bgav_strndup(pos, NULL);
    else if(check_key(buffer, "Width", &pos))
      ret->width = strtoul(pos, NULL, 10);
    else if(check_key(buffer, "Height", &pos))
      ret->height = strtoul(pos, NULL, 10);
    else if(check_key(buffer, "DisplayWidth", &pos))
      ret->display_width = strtoul(pos, NULL, 10);
    else if(check_key(buffer, "DisplayHeight", &pos))
      ret->display_height = strtoul(pos, NULL, 10);
    }

  /* How, check if we have additional headers */

  while(1)
    {
    if(!bgav_input_get_data(input, &c, 1))
      goto fail;
    
    if((c & 0xf0) != 0x00)
      break; /* Reached non header data */

    bgav_input_skip(input, 1);
        
    len = read_length(input);
    if(len < 0)
      goto fail;
    
    header_start = input->position;

    /* Get the record type */

    record_type = -1;
    while(input->position < header_start + len)
      {
      if(!bgav_input_read_line(input, &buffer, &buffer_alloc, 0))
        goto fail;

      /* Skip empty lines */
      if(*buffer == '\0')
        continue;
      
      if(!check_key(buffer, "RecordType", &pos))
        {
        fprintf(stderr, "Unknown extended header: %s\n", buffer);
        break;
        }
      record_type = atoi(pos);
      break;
      }

    if(record_type == -1)
      goto fail;
        
    switch(record_type)
      {
      case 2:
        while(input->position < header_start + len)
          {
          if(!bgav_input_read_line(input, &buffer, &buffer_alloc, 0))
            goto fail;

          if(check_key(buffer, "TimestampType", &pos))
            {
            if(strcmp(pos, "relative"))
              fprintf(stderr, "Warning, unknown timestamp type: %s\n",
                      pos);
            }
          else if(check_key(buffer, "TimeUnitNumerator", &pos))
            {
            ret->record_2.time_unit_num = strtoul(pos, NULL, 10);
            }
          else if(check_key(buffer, "TimeUnitDenominator", &pos))
            {
            ret->record_2.time_unit_den = strtoul(pos, NULL, 10);
            }
          }
        ret->have_record_2 = 1;
        break;
      case 3:
      case 4:
        while(input->position < header_start + len)
          {
          if(!bgav_input_read_line(input, &buffer, &buffer_alloc, 0))
            goto fail;

          if(check_key(buffer, "Length", &pos))
            ret->record_3_4.length = strtoul(pos, NULL, 10);

          else if(check_key(buffer, "InitialFrameLength", &pos))
            ret->record_3_4.initial_frame_length = strtoul(pos, NULL, 10);

          else if(check_key(buffer, "NominalBitrate", &pos))
            ret->record_3_4.nominal_bitrate = strtoul(pos, NULL, 10);

          else if(check_key(buffer, "SamplingFrequency", &pos))
            ret->record_3_4.sampling_frequency = strtoul(pos, NULL, 10);

          else if(check_key(buffer, "GainFactor", &pos))
            ret->record_3_4.gain_factor = strtoul(pos, NULL, 10);
          
          }
        ret->record_3_4.type = record_type;
        ret->have_record_3_4 = 1;
        break;
      }
    }

  result = 1;

  vivo_header_dump(ret);
  
  fail:
  if(buffer)
    free(buffer);
  return result;
  }

#define MY_FREE(p) if(p) free(p);

static void vivo_header_free(vivo_header_t * h)
  {
  MY_FREE(h->title);
  MY_FREE(h->author);
  MY_FREE(h->copyright);
  MY_FREE(h->producer);
  }

#undef MY_FREE

typedef struct
  {
  vivo_header_t header;
  
  } vivo_priv_t;

static int probe_vivo(bgav_input_context_t * input)
  {
  /* We look, if we have the string "Version:Vivo/" in the first 32 bytes */
    
  int i;
  uint8_t probe_data[32];

  if(bgav_input_get_data(input, probe_data, 32) < 32)
    return 0;

  for(i = 0; i < 32-13; i++)
    {
    if(!strncmp(&(probe_data[i]), "Version:Vivo/", 13))
      return 1;
    }
  return 0;
  }

  
static int open_vivo(bgav_demuxer_context_t * ctx,
                    bgav_redirector_context_t ** redir)
  {
  
  vivo_priv_t * priv;
  bgav_stream_t * audio_stream = (bgav_stream_t*)0;
  bgav_stream_t * video_stream = (bgav_stream_t*)0;
  
  priv = calloc(1, sizeof(*priv));
  ctx->priv = priv;
  
  /* Read header */

  if(!vivo_header_read(&(priv->header), ctx->input))
    goto fail;

  /* Create track */

  ctx->tt = bgav_track_table_create(1);
  
  /* Set up audio stream */
  
  audio_stream = bgav_track_add_audio_stream(ctx->tt->current_track);
  audio_stream->stream_id = AUDIO_STREAM_ID;

  if(priv->header.version == 1)
    {
    audio_stream->fourcc = BGAV_WAVID_2_FOURCC(0x0111);
    audio_stream->data.audio.format.samplerate = 8000;
    }
  else if(priv->header.version == 2)
    {
    audio_stream->fourcc = BGAV_WAVID_2_FOURCC(0x0112);
    audio_stream->data.audio.format.samplerate = 16000;
    }
  audio_stream->data.audio.format.num_channels = 1;
    
  /* Set up video stream */
  
  video_stream = bgav_track_add_video_stream(ctx->tt->current_track);
  //  video_stream->fourcc = BGAV_MK_FOURCC('v', 'i', 'v', '1');
  video_stream->stream_id = VIDEO_STREAM_ID;

  /* Set up metadata */

  ctx->tt->current_track->metadata.title     = bgav_strndup(priv->header.title, NULL);
  ctx->tt->current_track->metadata.author    = bgav_strndup(priv->header.author, NULL);
  ctx->tt->current_track->metadata.copyright = bgav_strndup(priv->header.copyright, NULL);
  ctx->tt->current_track->metadata.comment   = bgav_sprintf("Made with %s",
                                                            priv->header.producer);
  
  return 1;
  
  fail:
  return 0;
  }

static int next_packet_vivo(bgav_demuxer_context_t * ctx)
  {
  bgav_packet_t * p;
  bgav_stream_t * s;
  vivo_priv_t * priv;
  priv = (vivo_priv_t *)(ctx->priv);
  
  return 1;
  }

static void close_vivo(bgav_demuxer_context_t * ctx)
  {
  vivo_priv_t * priv;
  priv = (vivo_priv_t *)(ctx->priv);
  
  vivo_header_free(&(priv->header));
  free(priv);
  }

bgav_demuxer_t bgav_demuxer_vivo =
  {
    probe:       probe_vivo,
    open:        open_vivo,
    next_packet: next_packet_vivo,
    close:       close_vivo
  };

