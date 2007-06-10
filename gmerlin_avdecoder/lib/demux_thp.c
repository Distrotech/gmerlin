/*****************************************************************
 
  demux_thp.c
 
  Copyright (c) 2007 by Burkhard Plaum - plaum@ipf.uni-stuttgart.de
 
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


#include <avdec_private.h>

#define VERSION_1_0 0x00010000
#define VERSION_1_1 0x00011000

#define LOG_DOMAIN "thp"

#define AUDIO_ID 0
#define VIDEO_ID 1

// #define DUMP_HEADER

typedef struct
  {
  char tag[4]; //'THP\0'

  uint32_t version; //0x00011000 = 1.1, 0x00010000 = 1.0
  uint32_t maxBufferSize; //maximal buffer size needed for one complete frame (header + video + audio)
  uint32_t maxAudioSamples; //!= 0 if sound is stored in file, maximal number of samples in one frame.
                       //you can use this field to check if file contains audio.


  float fps; //usually 29.something (=0x41efc28f) for ntsc
  uint32_t numFrames; //number of frames in the thp file
  uint32_t firstFrameSize; //size of first frame (header + video + audio)

  uint32_t dataSize; //size of all frames (not counting the thp header structures)

  uint32_t componentDataOffset; //ThpComponents stored here (see below)
  uint32_t offsetsDataOffset; //?? if != 0, offset to table with offsets of all frames?

  uint32_t firstFrameOffset; //offset to first frame's data
  uint32_t lastFrameOffset; //offset to last frame's data
  } ThpHeader;

static int read_header(bgav_input_context_t * input, ThpHeader * ret)
  {
  if((bgav_input_read_data(input, (uint8_t*)ret->tag, 4) < 4) ||
     !bgav_input_read_32_be(input, &ret->version) ||
     !bgav_input_read_32_be(input, &ret->maxBufferSize) ||
     !bgav_input_read_32_be(input, &ret->maxAudioSamples) ||
     !bgav_input_read_float_32_be(input, &ret->fps) ||
     !bgav_input_read_32_be(input, &ret->numFrames) ||
     !bgav_input_read_32_be(input, &ret->firstFrameSize) ||
     !bgav_input_read_32_be(input, &ret->dataSize) ||
     !bgav_input_read_32_be(input, &ret->componentDataOffset) ||
     !bgav_input_read_32_be(input, &ret->offsetsDataOffset) ||
     !bgav_input_read_32_be(input, &ret->firstFrameOffset) ||
     !bgav_input_read_32_be(input, &ret->lastFrameOffset))
    return 0;
  return 1;
  }

#ifdef DUMP_HEADER

static void dump_header(ThpHeader * h)
  {
  bgav_dprintf("ThpHeader\n");
  bgav_dprintf("  tag:                 %c%c%c%c (%02x%02x%02x%02x)\n",
               h->tag[0], h->tag[1], h->tag[2], h->tag[3],
               h->tag[0], h->tag[1], h->tag[2], h->tag[3]);
  bgav_dprintf("  version:             0x%08x\n", h->version);
  
  bgav_dprintf("  maxBufferSize:       %d\n", h->maxBufferSize);
  bgav_dprintf("  maxAudioSamples:     %d\n", h->maxAudioSamples);
   
  bgav_dprintf("  fps:                 %f\n", h->fps);
  bgav_dprintf("  numFrames:           %d\n", h->numFrames);
  bgav_dprintf("  firstFrameSize:      %d\n", h->firstFrameSize);
  
  bgav_dprintf("  dataSize:            %d\n", h->dataSize);
  
  bgav_dprintf("  componentDataOffset: %d\n", h->componentDataOffset);
  bgav_dprintf("  offsetsDataOffset:   %d\n", h->offsetsDataOffset);
  
  bgav_dprintf("  firstFrameOffset:    %d\n", h->firstFrameOffset);
  bgav_dprintf("  lastFrameOffset:     %d\n", h->lastFrameOffset);
  }

#endif

static int probe_thp(bgav_input_context_t * input)
  {
  uint8_t probe_buffer[4];
  if(bgav_input_get_data(input, probe_buffer, 4) < 4)
    return 0;

  if((probe_buffer[0] == 'T') &&
     (probe_buffer[1] == 'H') &&
     (probe_buffer[2] == 'P') &&
     (probe_buffer[3] == 0x00))
    return 1;
  
  return 0;
  }


typedef struct
  {
  ThpHeader h;
  
  uint32_t next_frame_offset;
  uint32_t next_frame_size;
  uint32_t next_frame;
  } thp_t;

static int open_thp(bgav_demuxer_context_t * ctx,
                    bgav_redirector_context_t ** redir)
  {
  thp_t * priv;
  uint8_t components[16];
  uint32_t num_components;
  int i;
  bgav_stream_t * s;
  
  if(!ctx->input->input->seek_byte)
    {
    bgav_log(ctx->opt, BGAV_LOG_ERROR, LOG_DOMAIN,
             "Cannot decode from nonseekable source");
    return 0;
    }
  
  priv = calloc(1, sizeof(*priv));
  ctx->priv = priv;

  if(!read_header(ctx->input, &priv->h))
    return 0;

#ifdef DUMP_HEADER  
  dump_header(&priv->h);
#endif
  
  bgav_input_seek(ctx->input, priv->h.componentDataOffset, SEEK_SET);

  if(!bgav_input_read_32_be(ctx->input, &num_components) ||
     (bgav_input_read_data(ctx->input, components, 16) < 16))
    return 0;
  
  ctx->tt = bgav_track_table_create(1);
  
  for(i = 0; i < num_components; i++)
    {
    if(components[i] == 0) // Video
      {
      uint32_t width, height;
      
      if(ctx->tt->cur->num_video_streams)
        break;
      
      s = bgav_track_add_video_stream(ctx->tt->cur, ctx->opt);
      s->fourcc = BGAV_MK_FOURCC('T','H','P','V');
      s->stream_id = VIDEO_ID;
      
      s->data.video.format.timescale      = (int)(priv->h.fps * 1000000.0 + 0.5);
      s->data.video.format.frame_duration = 1000000;

      if(!bgav_input_read_32_be(ctx->input, &width) ||
         !bgav_input_read_32_be(ctx->input, &height))
        return 0;

      s->data.video.format.image_width = width;
      s->data.video.format.frame_width = width;

      s->data.video.format.image_height = height;
      s->data.video.format.frame_height = height;

      s->data.video.format.pixel_width  = 1;
      s->data.video.format.pixel_height = 1;

      if(priv->h.version == VERSION_1_1)
        bgav_input_skip(ctx->input, 4); // unknown

      ctx->tt->cur->duration =
        gavl_time_unscale(s->data.video.format.timescale,
                          (int64_t)(priv->h.numFrames) * 
                          s->data.video.format.frame_duration);

      }
    else if(components[i] == 1) // Audio
      {
      uint32_t samplerate, num_channels;

      if(ctx->tt->cur->num_audio_streams)
        break;
      s = bgav_track_add_audio_stream(ctx->tt->cur, ctx->opt);
      s->fourcc = BGAV_MK_FOURCC('T','H','P','A');
      s->stream_id = AUDIO_ID;

      if(!bgav_input_read_32_be(ctx->input, &num_channels) ||
         !bgav_input_read_32_be(ctx->input, &samplerate))
        return 0;
      
      s->data.audio.format.samplerate   = samplerate;
      s->data.audio.format.num_channels = num_channels;

      bgav_input_skip(ctx->input, 4); // numSamples
      if(priv->h.version == VERSION_1_1)
        bgav_input_skip(ctx->input, 4); // numData
      }
    }
  priv->next_frame_offset = priv->h.firstFrameOffset;
  priv->next_frame_size   = priv->h.firstFrameSize;

  ctx->stream_description = bgav_sprintf("THP");
  
  return 1;
  }

static int next_packet_thp(bgav_demuxer_context_t * ctx)
  {
  bgav_stream_t * s;
  thp_t * priv = (thp_t*)(ctx->priv);
  uint32_t audio_size = 0, video_size;
  bgav_packet_t * p;
  
  /* Check for EOF */
  if(priv->next_frame >= priv->h.numFrames)
    return 0;

  /* Update positions */
  bgav_input_seek(ctx->input, priv->next_frame_offset, SEEK_SET);
  priv->next_frame_offset += priv->next_frame_size;

  /* Read offsets */
  if(!bgav_input_read_32_be(ctx->input, &priv->next_frame_size))
    return 0;

  bgav_input_skip(ctx->input, 4); // prevTotalSize

  if(!bgav_input_read_32_be(ctx->input, &video_size))
    return 0;

  if(priv->h.maxAudioSamples &&
     !bgav_input_read_32_be(ctx->input, &audio_size))
    return 0;

  /* Read video frame */
  s = bgav_track_find_stream(ctx->tt->cur, VIDEO_ID);
  if(s)
    {
    p = bgav_stream_get_packet_write(s);
    bgav_packet_alloc(p, video_size);
    p->data_size = bgav_input_read_data(ctx->input, p->data, video_size);

    if(p->data_size < video_size)
      return 0;

    p->pts = priv->next_frame * s->data.video.format.frame_duration;
    
    bgav_packet_done_write(p);
    }
  else
    bgav_input_skip(ctx->input, video_size);
  
  priv->next_frame++;
  
  if(!audio_size)
    return 1;
  
  /* Read audio frame */
  s = bgav_track_find_stream(ctx->tt->cur, AUDIO_ID);
  if(s)
    {
    p = bgav_stream_get_packet_write(s);
    bgav_packet_alloc(p, audio_size);
    p->data_size = bgav_input_read_data(ctx->input, p->data, audio_size);

    if(p->data_size < audio_size)
      return 0;
    
    bgav_packet_done_write(p);
    }
  else
    bgav_input_skip(ctx->input, audio_size);
  
  return 1;
  }

static int select_track_thp(bgav_demuxer_context_t * ctx, int track)
  {
  thp_t * priv = (thp_t*)(ctx->priv);
  priv->next_frame_offset = priv->h.firstFrameOffset;
  priv->next_frame_size   = priv->h.firstFrameSize;
  priv->next_frame = 0;
  return 1;
  }

static void close_thp(bgav_demuxer_context_t * ctx)
  {
  thp_t * priv = (thp_t*)(ctx->priv);
  if(priv)
    free(priv);
  }

bgav_demuxer_t bgav_demuxer_thp =
  {
    probe:        probe_thp,
    open:         open_thp,
    select_track: select_track_thp,
    next_packet:  next_packet_thp,
    close:        close_thp
  };
