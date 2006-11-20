/*****************************************************************
 
  demux_psxstr.c
 
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
#include <stdio.h>
#include <string.h>

#define LOG_DOMAIN "psxstr"

#define RIFF_TAG BGAV_MK_FOURCC('R', 'I', 'F', 'F')
#define CDXA_TAG BGAV_MK_FOURCC('C', 'D', 'X', 'A')

#define RAW_CD_SECTOR_SIZE 2352
#define RAW_CD_SECTOR_DATA_SIZE 2304
#define VIDEO_DATA_CHUNK_SIZE 0x7E0
#define VIDEO_DATA_HEADER_SIZE 0x38
#define RIFF_HEADER_SIZE 0x2C

#define CDXA_TYPE_MASK     0x0E
#define CDXA_TYPE_DATA     0x08
#define CDXA_TYPE_AUDIO    0x04
#define CDXA_TYPE_VIDEO    0x02

#define STR_MAGIC (0x80010160)

#define SCAN_SECTORS 32

#define AUDIO_OFFSET 0
#define VIDEO_OFFSET 32

static uint8_t sync_header[12] =
{0x00,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0x00};

typedef struct
  {
  uint32_t video_pts;
  } psxstr_priv_t;

static int probe_psxstr(bgav_input_context_t * input)
  {
  uint8_t * ptr;
  uint8_t data[28+RIFF_HEADER_SIZE];
  
  if(bgav_input_get_data(input, data, 28+RIFF_HEADER_SIZE) < 28+RIFF_HEADER_SIZE)
    return 0;

  if((data[0] == 'R') &&
     (data[1] == 'I') &&
     (data[2] == 'F') &&
     (data[3] == 'F') &&
     (data[8] ==  'C') &&
     (data[9] ==  'D') &&
     (data[10] == 'X') &&
     (data[11] == 'A'))
    ptr = &data[RIFF_HEADER_SIZE];
  else
    ptr = &data[0];

  if(memcmp(ptr, sync_header, 12))
    return 0;

  if(BGAV_PTR_2_32LE(&ptr[0x18]) != STR_MAGIC)
    return 0;
  return 1;
  }

static int open_psxstr(bgav_demuxer_context_t * ctx,
                   bgav_redirector_context_t ** redir)
  {
  int i;
  int ret = 0;
  uint32_t fourcc;
  uint8_t * buffer = (uint8_t*)0;
  uint8_t * sector;
  int channel;
  bgav_stream_t * s;
  psxstr_priv_t * priv;

  priv = calloc(1, sizeof(*priv));
  ctx->priv = priv;
  
  if(!bgav_input_get_fourcc(ctx->input, &fourcc))
    goto fail;

  if(fourcc == RIFF_TAG)
    bgav_input_skip(ctx->input, RIFF_HEADER_SIZE);

  buffer = malloc(SCAN_SECTORS*RAW_CD_SECTOR_SIZE);

  if(bgav_input_get_data(ctx->input, buffer, SCAN_SECTORS*RAW_CD_SECTOR_SIZE) <
     SCAN_SECTORS*RAW_CD_SECTOR_SIZE)
    return 0;

  ctx->tt = bgav_track_table_create(1);
    
  for(i = 0; i < SCAN_SECTORS; i++)
    {
    sector = buffer + i * RAW_CD_SECTOR_SIZE;

    channel = sector[0x11];
    if(channel >= 32)
      {
      bgav_log(ctx->opt, BGAV_LOG_ERROR, LOG_DOMAIN, "Invalid channel number %d",
               channel);
      goto fail;
      }
    switch(sector[0x12] & CDXA_TYPE_MASK)
      {
      case CDXA_TYPE_DATA:
      case CDXA_TYPE_VIDEO:
        /* qualify the magic number */
        if(BGAV_PTR_2_32LE(&sector[0x18]) != STR_MAGIC)
          break;

        if(bgav_track_find_stream_all(ctx->tt->current_track, channel + VIDEO_OFFSET))
          break;
        
        s = bgav_track_add_video_stream(ctx->tt->current_track, ctx->opt);
        s->fourcc = BGAV_MK_FOURCC('M','D','E','C');

        s->data.video.format.image_width  = BGAV_PTR_2_16LE(&sector[0x28]);
        s->data.video.format.image_height = BGAV_PTR_2_16LE(&sector[0x2A]);
        
        s->data.video.format.frame_width  = s->data.video.format.image_width;
        s->data.video.format.frame_height = s->data.video.format.image_height;

        s->data.video.format.pixel_width = 1;
        s->data.video.format.pixel_height = 1;

        s->data.video.format.timescale = 15;
        s->data.video.format.frame_duration = 1;
                
        s->stream_id = channel + VIDEO_OFFSET;
               
        break;
      case CDXA_TYPE_AUDIO:
        if(bgav_track_find_stream_all(ctx->tt->current_track, channel + AUDIO_OFFSET))
          break;
        
        s = bgav_track_add_audio_stream(ctx->tt->current_track, ctx->opt);
        s->fourcc = BGAV_MK_FOURCC('A','D','X','A');
        
        s->data.audio.format.samplerate      = (sector[0x13] & 0x04) ? 18900 : 37800;
        s->data.audio.format.num_channels    = (sector[0x13] & 0x01) ? 2 : 1;
        s->data.audio.bits_per_sample = (sector[0x13] & 0x10) ? 8 : 4;
        s->data.audio.block_align     = 128;

        s->stream_id = channel + AUDIO_OFFSET;
        
        break;
      }
    }
  ctx->stream_description = bgav_sprintf("Sony Playstation (PSX) STR");
  
  ret = 1;
  fail:
  if(buffer)
    free(buffer);
      
  return ret;
  }

static int next_packet_psxstr(bgav_demuxer_context_t * ctx)
  {
  int channel;
  uint8_t sector[RAW_CD_SECTOR_SIZE];
  psxstr_priv_t * priv;
  bgav_stream_t * s;
  bgav_packet_t * p;
  int current_sector;
  int sector_count;
  int frame_size;
  int bytes_to_copy;
  
  priv = (psxstr_priv_t *)(ctx->priv);

  if(bgav_input_read_data(ctx->input, sector, RAW_CD_SECTOR_SIZE) < RAW_CD_SECTOR_SIZE)
    return 0;

  channel = sector[0x11];
  
  switch (sector[0x12] & CDXA_TYPE_MASK)
    {
    case CDXA_TYPE_DATA:
    case CDXA_TYPE_VIDEO:

      s = bgav_track_find_stream(ctx->tt->current_track, channel + VIDEO_OFFSET);
      if(!s)
        break;
      
      current_sector = BGAV_PTR_2_16LE(&sector[0x1C]);
      sector_count   = BGAV_PTR_2_16LE(&sector[0x1E]);
      frame_size     = BGAV_PTR_2_32LE(&sector[0x24]);
#if 0
      fprintf(stderr, "current_sector: %d, sector_count: %d, frame_size: %d\n",
              current_sector,sector_count,frame_size);
#endif
      if(!s->packet)
        {
        s->packet = bgav_stream_get_packet_write(s);
        bgav_packet_alloc(s->packet, frame_size);
        }
      bytes_to_copy = frame_size - current_sector*VIDEO_DATA_CHUNK_SIZE;

      bytes_to_copy = frame_size - current_sector*VIDEO_DATA_CHUNK_SIZE;
      
      if(bytes_to_copy > 0)
        {
        if(bytes_to_copy > VIDEO_DATA_CHUNK_SIZE)
          bytes_to_copy=VIDEO_DATA_CHUNK_SIZE;
        memcpy(s->packet->data + current_sector*VIDEO_DATA_CHUNK_SIZE,
               sector + VIDEO_DATA_HEADER_SIZE, bytes_to_copy);
        s->packet->data_size += bytes_to_copy;
        }
      
      if(current_sector == sector_count-1)
        {
        s->packet->pts = priv->video_pts++;
        bgav_packet_done_write(s->packet);
        s->packet = (bgav_packet_t*)0;
        }
      break;
    case CDXA_TYPE_AUDIO:
      
      s = bgav_track_find_stream(ctx->tt->current_track, channel + AUDIO_OFFSET);
      if(!s)
        break;

      p = bgav_stream_get_packet_write(s);
      bgav_packet_alloc(p, RAW_CD_SECTOR_DATA_SIZE);

      memcpy(p->data, sector + 24, RAW_CD_SECTOR_DATA_SIZE);
      p->data_size = RAW_CD_SECTOR_DATA_SIZE;
      bgav_packet_done_write(p);
      
      break;
      
    }
  return 1;
  }

static void close_psxstr(bgav_demuxer_context_t * ctx)
  {
  psxstr_priv_t * priv;
  priv = (psxstr_priv_t *)(ctx->priv);
  free(priv);
  }

bgav_demuxer_t bgav_demuxer_psxstr =
  {
    probe:       probe_psxstr,
    open:        open_psxstr,
    next_packet: next_packet_psxstr,
    close:       close_psxstr
  };
