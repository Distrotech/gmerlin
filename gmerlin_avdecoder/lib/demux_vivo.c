/*****************************************************************
 
  demux_vivo.c
 
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

#include <avdec_private.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#define AUDIO_STREAM_ID 0
#define VIDEO_STREAM_ID 1


/*
 *  VIVO demuxer
 *  Written with MPlayer source as reference.
 *  cleaned up a lot and made reentrant
 *  (actually rewritten from scratch)
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

static int check_key(char * buffer, const char * key, char ** pos)
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
  bgav_dprintf( "Main VIVO header\n");
  bgav_dprintf( "  Version:       %d\n", h->version);
  bgav_dprintf( "  FPS:           %f\n", h->fps);
  bgav_dprintf( "  Duration:      %d\n", h->duration);
  bgav_dprintf( "  Length:        %d\n", h->length);
  bgav_dprintf( "  Rate:          %d\n", h->rate);
  bgav_dprintf( "  VidRate:       %d\n", h->vid_rate);
  bgav_dprintf( "  Playtime1:     %d\n", h->playtime1);
  bgav_dprintf( "  Playtime2:     %d\n", h->playtime2);
  bgav_dprintf( "  Buffer:        %d\n", h->buffer);
  bgav_dprintf( "  Preroll:       %d\n", h->preroll);
  bgav_dprintf( "  Title:         %s\n", h->title);
  bgav_dprintf( "  Author:        %s\n", h->author);
  bgav_dprintf( "  Copyright:     %s\n", h->copyright);
  bgav_dprintf( "  Producer:      %s\n", h->producer);
  bgav_dprintf( "  Width:         %d\n", h->width);
  bgav_dprintf( "  Height:        %d\n", h->height);
  bgav_dprintf( "  DisplayWidth:  %d\n", h->display_width);
  bgav_dprintf( "  DisplayHeight: %d\n", h->display_height);

  if(h->have_record_2)
    {
    bgav_dprintf( "RecordType 2\n");
    bgav_dprintf( "  TimeUnitNumerator:   %d\n", h->record_2.time_unit_num);
    bgav_dprintf( "  TimeUnitDenominator: %d\n", h->record_2.time_unit_den);
    }

  if(h->have_record_3_4)
    {
    bgav_dprintf( "RecordType %d\n", h->record_3_4.type);
    bgav_dprintf( "  Length:             %d\n", h->record_3_4.length);
    bgav_dprintf( "  InitialFrameLength: %d\n", h->record_3_4.initial_frame_length);
    bgav_dprintf( "  SamplingFrequency:  %d\n", h->record_3_4.sampling_frequency);
    bgav_dprintf( "  GainFactor:         %d\n", h->record_3_4.gain_factor);
    }
  
  }

static int vivo_header_read(vivo_header_t * ret, bgav_input_context_t * input)
  {
  uint8_t c;
  int len;
  int64_t header_start;
  char * buffer = (char*)0;
  char * pos = (char*)0;
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
    if(!bgav_input_read_line(input, &buffer, &buffer_alloc, 0, (int*)0))
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
      ret->title = bgav_strdup(pos);
    else if(check_key(buffer, "Author", &pos))
      ret->author = bgav_strdup(pos);
    else if(check_key(buffer, "Copyright", &pos))
      ret->copyright = bgav_strdup(pos);
    else if(check_key(buffer, "Producer", &pos))
      ret->producer = bgav_strdup(pos);
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
      if(!bgav_input_read_line(input, &buffer, &buffer_alloc, 0, (int*)0))
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
          if(!bgav_input_read_line(input, &buffer, &buffer_alloc, 0, (int*)0))
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
          if(!bgav_input_read_line(input, &buffer, &buffer_alloc, 0, (int*)0))
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
  uint32_t audio_pos;
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
    if(!strncmp((char*)(&(probe_data[i])), "Version:Vivo/", 13))
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
  
  audio_stream = bgav_track_add_audio_stream(ctx->tt->current_track, ctx->opt);
  audio_stream->stream_id = AUDIO_STREAM_ID;

  if(priv->header.version == 1)
    {
    /* G.723 */
    audio_stream->fourcc = BGAV_WAVID_2_FOURCC(0x0111);
    audio_stream->data.audio.format.samplerate = 8000;
    audio_stream->container_bitrate = 800 * 8;
    audio_stream->data.audio.block_align = 24;
    audio_stream->data.audio.bits_per_sample = 8;
    }
  else if(priv->header.version == 2)
    {
    /* Siren */
    audio_stream->fourcc = BGAV_WAVID_2_FOURCC(0x0112);
    audio_stream->data.audio.format.samplerate = 16000;
    audio_stream->container_bitrate = 2000 * 8;
    audio_stream->data.audio.block_align = 40;
    audio_stream->data.audio.bits_per_sample = 16;
    }
  audio_stream->data.audio.format.num_channels = 1;
  audio_stream->codec_bitrate = audio_stream->container_bitrate;
  /* Set up video stream */
  
  video_stream = bgav_track_add_video_stream(ctx->tt->current_track, ctx->opt);
  video_stream->vfr_timestamps = 1;

  if(priv->header.version == 1)
    {
    video_stream->fourcc = BGAV_MK_FOURCC('v', 'i', 'v', '1');
    }
  else if(priv->header.version == 2)
    {
    video_stream->fourcc = BGAV_MK_FOURCC('v', 'i', 'v', 'o');
    video_stream->data.video.format.image_width = priv->header.width;
    video_stream->data.video.format.frame_width = priv->header.width;

    video_stream->data.video.format.image_height = priv->header.height;
    video_stream->data.video.format.frame_height = priv->header.height;
    }
  video_stream->data.video.format.pixel_width = 1;
  video_stream->data.video.format.pixel_height = 1;
  video_stream->data.video.format.framerate_mode = GAVL_FRAMERATE_VARIABLE;
  
  video_stream->stream_id = VIDEO_STREAM_ID;
  
  //  video_stream->data.video.format.timescale = (int)(priv->header.fps * 1000.0);
  
  video_stream->data.video.format.timescale = 1000;
  video_stream->timescale = 1000;
  
  video_stream->data.video.format.frame_duration = 1000.0;
  video_stream->data.video.depth = 24;
  video_stream->data.video.image_size = video_stream->data.video.format.image_width *
    video_stream->data.video.format.image_height * 3;
  /* Set up metadata */

  ctx->tt->current_track->metadata.title     = bgav_strdup(priv->header.title);
  ctx->tt->current_track->metadata.author    = bgav_strdup(priv->header.author);
  ctx->tt->current_track->metadata.copyright = bgav_strdup(priv->header.copyright);
  ctx->tt->current_track->metadata.comment   = bgav_sprintf("Made with %s",
                                                            priv->header.producer);

  ctx->stream_description = bgav_sprintf("Vivo Version %d.x", priv->header.version);

  ctx->tt->current_track->duration = (GAVL_TIME_SCALE * (int64_t)(priv->header.duration)) / 1000;
  
  return 1;
  
  fail:
  return 0;
  }

static int next_packet_vivo(bgav_demuxer_context_t * ctx)
  {
  uint8_t c, h;
  int prefix = 0;
  int len;
  int seq;
  bgav_stream_t * stream = (bgav_stream_t*)0;
  vivo_priv_t * priv;
  int do_audio = 0;
  int do_video = 0;
  
  priv = (vivo_priv_t *)(ctx->priv);

  if(!bgav_input_read_data(ctx->input, &c, 1))
    return 0;

  if(c == 0x82)
    {
    prefix = 1;
    
    if(!bgav_input_read_data(ctx->input, &c, 1))
      return 0;
    
    }

  h = c;
  
  switch(h & 0xf0)
    {
    case 0x00: /* Thought we already have all headers */
      len = read_length(ctx->input);
      if(len < 0)
        return 0;
      bgav_input_skip(ctx->input, len);
      return 1;
      break;
    case 0x10: /* Video packet */
    case 0x20:
      if(prefix || ((h & 0xf0) == 0x20))
        {
        if(!bgav_input_read_data(ctx->input, &c, 1))
          return 0;
        len = c;
        }
      else
        len = 128;
      do_video = 1;
      break;
    case 0x30:
    case 0x40:
      if(prefix)
        {
        if(!bgav_input_read_data(ctx->input, &c, 1))
          return 0;
        len = c;
        }
      else if((h & 0xf0) == 0x30)
        len = 40;
      else
        len = 24;
      do_audio = 1;
      priv->audio_pos += len;
      break;
    default:
      fprintf(stderr, "Unknown packet type\n");
      return 0;
    }
    
  if(do_audio)
    stream = bgav_track_find_stream(ctx->tt->current_track, AUDIO_STREAM_ID);
  else if(do_video)
    stream = bgav_track_find_stream(ctx->tt->current_track, VIDEO_STREAM_ID);
  
  if(!stream)
    {
    bgav_input_skip(ctx->input, len);
    return 1;
    }
  
  seq = (h & 0x0f);
#if 0
  fprintf(stderr, "h: 0x%02x, %s, len: %d, prefix: %d, seq: %d\n",
          h, (do_audio ? "Audio" : "Video"), len, prefix, seq);
#endif
  /* Finish packet */

  if(stream->packet && (stream->packet_seq != seq))
    {
    bgav_packet_done_write(stream->packet);

    if(do_video)
      {
      stream->packet->pts = (priv->audio_pos * 8000) /
        ctx->tt->current_track->audio_streams[0].container_bitrate;
      }

    stream->packet = (bgav_packet_t*)0;
    }

  /* Get new packet */
  
  if(!stream->packet)
    {
    stream->packet = bgav_stream_get_packet_write(stream);
    stream->packet_seq = seq;
    stream->packet->data_size = 0;
    }

  /* Append data */
  bgav_packet_alloc(stream->packet,
                    stream->packet->data_size + len);
  if(bgav_input_read_data(ctx->input,
                          stream->packet->data + stream->packet->data_size,
                          len) < len)
    {
    return 0;
    }
  stream->packet->data_size += len;
  if((h & 0xf0) == 0x20)
    stream->packet_seq--;
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

