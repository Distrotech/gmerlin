/*****************************************************************
 
  demux_vmd.c
 
  Copyright (c) 2003-2006 by Burkhard Plaum - plaum@ipf.uni-stuttgart.de
 
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
#include <string.h>
#include <stdio.h>

#define VMD_HEADER_SIZE 0x0330
#define BYTES_PER_FRAME_RECORD 16

#define LOG_DOMAIN "vmd"

#define AUDIO_ID 0
#define VIDEO_ID 1

typedef struct
  {
  int stream_index;
  uint32_t frame_offset;
  unsigned int frame_size;
  unsigned int frames_per_block;
  int64_t pts;
  int keyframe;
  unsigned char frame_record[BYTES_PER_FRAME_RECORD];
  } vmd_frame_t;

typedef struct
  {
  uint8_t header[VMD_HEADER_SIZE];

  vmd_frame_t *frame_table;
  int current_frame;
  uint32_t frame_count;
  uint32_t frames_per_block;
  
  
  } vmd_priv_t;

static int probe_vmd(bgav_input_context_t * input)
  {
  uint16_t size;
  char * pos;
  if(input->filename)
    {
    pos = strrchr(input->filename, '.');
    if(!pos)
      return 0;
    if(strcasecmp(pos, ".vmd"))
      return 0;
    }
  if(!bgav_input_get_16_le(input, &size) ||
     (size != VMD_HEADER_SIZE - 2))
    return 0;
  return 1;
  }

static int open_vmd(bgav_demuxer_context_t * ctx,
                   bgav_redirector_context_t ** redir)
  {
  vmd_priv_t * priv;
  bgav_stream_t * vs = (bgav_stream_t *)0;
  bgav_stream_t * as = (bgav_stream_t *)0;
  int samplerate;
  uint32_t toc_offset;
  uint8_t * raw_frame_table = (uint8_t*)0;
  int raw_frame_table_size;
  int ret = 0;
  int i, j;
  uint8_t chunk[BYTES_PER_FRAME_RECORD];
  uint32_t current_offset;

  uint8_t type;
  uint32_t size;

  int64_t video_pts_inc = 0;
  int64_t current_video_pts = 0;
  int frame_index = 0;
   
  if(!ctx->input->input->seek_byte)
    {
    bgav_log(ctx->opt, BGAV_LOG_ERROR, LOG_DOMAIN,
             "Cannot open VMD file from nonseekable source");
    goto fail;
    }
  
  priv = calloc(1, sizeof(*priv));
  ctx->priv = priv;

  if(bgav_input_read_data(ctx->input, priv->header, VMD_HEADER_SIZE) <
     VMD_HEADER_SIZE)
    goto fail;

  ctx->tt = bgav_track_table_create(1);

  /* Initialize video stream */
  
  vs = bgav_track_add_video_stream(ctx->tt->current_track, ctx->opt);

  vs->stream_id = VIDEO_ID;
  vs->fourcc = BGAV_MK_FOURCC('V','M','D','V');
  vs->data.video.format.image_width  = BGAV_PTR_2_16LE(&priv->header[12]);
  vs->data.video.format.image_height = BGAV_PTR_2_16LE(&priv->header[14]);

  vs->data.video.format.frame_width  = vs->data.video.format.image_width;
  vs->data.video.format.frame_height = vs->data.video.format.image_height;
  vs->data.video.format.pixel_width  = 1;
  vs->data.video.format.pixel_height = 1;
  vs->ext_data = priv->header;
  vs->ext_size = VMD_HEADER_SIZE;

  /* Initialize audio stream */
  samplerate = BGAV_PTR_2_16LE(&priv->header[804]);

  if(samplerate)
    {
    as = bgav_track_add_audio_stream(ctx->tt->current_track, ctx->opt);
    as->stream_id = AUDIO_ID;
    as->fourcc = BGAV_MK_FOURCC('V','M','D','A');
    as->data.audio.format.samplerate = samplerate;
    as->data.audio.format.num_channels =
      (priv->header[811] & 0x80) ? 2 : 1;
    as->data.audio.block_align = BGAV_PTR_2_16LE(&priv->header[806]);
    if(as->data.audio.block_align & 0x8000)
      {
      as->data.audio.bits_per_sample = 16;
      as->data.audio.block_align = -(as->data.audio.block_align - 0x10000);
      }
    else
      as->data.audio.bits_per_sample = 8;

    video_pts_inc = 90000;
    video_pts_inc *= as->data.audio.block_align;
    video_pts_inc /= as->data.audio.format.samplerate;
    video_pts_inc /= as->data.audio.format.num_channels;
    }
  else
    {
    /* Assume 10 fps */
    video_pts_inc = 90000 / 10;
    }

  vs->data.video.format.frame_duration = video_pts_inc;
  vs->data.video.format.timescale = 90000;

  /* Get table of contents */

  toc_offset = BGAV_PTR_2_32LE(&priv->header[812]);
  priv->frame_count = BGAV_PTR_2_16LE(&priv->header[6]);
  priv->frames_per_block = BGAV_PTR_2_16LE(&priv->header[18]);
  
  bgav_input_seek(ctx->input, toc_offset, SEEK_SET);

  raw_frame_table_size = priv->frame_count * 6;
  raw_frame_table = malloc(raw_frame_table_size);
  priv->frame_table =
    malloc(priv->frame_count*priv->frames_per_block*
           sizeof(*priv->frame_table));

  if(bgav_input_read_data(ctx->input, raw_frame_table, raw_frame_table_size)
     < raw_frame_table_size)
    goto fail;

  for(i = 0; i < priv->frame_count; i++)
    {
    current_offset = BGAV_PTR_2_32LE(&raw_frame_table[6*i+2]);
    for (j = 0; j < priv->frames_per_block; j++)
      {
      if(bgav_input_read_data(ctx->input, chunk, BYTES_PER_FRAME_RECORD) <
         BYTES_PER_FRAME_RECORD)
        {
        fprintf(stderr, "Unexpected end of file %d %d\n", i, j);
        goto fail;
        }
      type = chunk[0];
      size = BGAV_PTR_2_32LE(&chunk[2]);

      /* Common fields */
      priv->frame_table[frame_index].frame_offset = current_offset;
      priv->frame_table[frame_index].frame_size = size;
      memcpy(priv->frame_table[frame_index].frame_record,
             chunk, BYTES_PER_FRAME_RECORD);
      
      switch(type)
        {
        case 1: /* Audio */
          priv->frame_table[frame_index].stream_index = AUDIO_ID;
          break;
        case 2: /* Video */
          priv->frame_table[frame_index].stream_index = VIDEO_ID;
          priv->frame_table[frame_index].pts = current_video_pts;
          break;
        }
      current_offset += size;
      frame_index++;
      }
    current_video_pts += video_pts_inc;
    }

  fprintf(stderr, "Frame count: %d, frame_index: %d\n",
          priv->frame_count, frame_index);
  priv->frame_count = frame_index;
  /* */
  ctx->stream_description = bgav_sprintf("Sierra VMD");
  ret = 1;
  
  fail:
  if(raw_frame_table)
    free(raw_frame_table);
  
  return ret;
  }


static int next_packet_vmd(bgav_demuxer_context_t * ctx)
  {
  vmd_priv_t * priv;
  vmd_frame_t *frame;
  bgav_stream_t * s;
  bgav_packet_t * p;
  
  priv = (vmd_priv_t*)(ctx->priv);

  if(priv->current_frame >= priv->frame_count)
    return 0;

  frame = &priv->frame_table[priv->current_frame];

  s = bgav_track_find_stream(ctx->tt->current_track, frame->stream_index);
  if(s)
    {
    bgav_input_seek(ctx->input, frame->frame_offset, SEEK_SET);
    p = bgav_packet_buffer_get_packet_write(s->packet_buffer, s);

    bgav_packet_alloc(p, frame->frame_size + BYTES_PER_FRAME_RECORD);
    memcpy(p->data, frame->frame_record, BYTES_PER_FRAME_RECORD);
    if(bgav_input_read_data(ctx->input, p->data + BYTES_PER_FRAME_RECORD,
                            frame->frame_size) < frame->frame_size)
      return 0;
    if(s->type == BGAV_STREAM_VIDEO)
      p->pts = frame->pts;
    p->data_size = frame->frame_size + BYTES_PER_FRAME_RECORD;
    bgav_packet_done_write(p);
    }
  priv->current_frame++;
  return 1;
  }

static void close_vmd(bgav_demuxer_context_t * ctx)
  {
  vmd_priv_t * priv;
  priv = (vmd_priv_t*)(ctx->priv);
  free(priv);
  }

bgav_demuxer_t bgav_demuxer_vmd =
  {
    probe:       probe_vmd,
    open:        open_vmd,
    next_packet: next_packet_vmd,
    close:       close_vmd
  };
