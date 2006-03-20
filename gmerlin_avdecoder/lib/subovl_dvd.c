/*****************************************************************
 
  subovl_dvd.c
 
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
#include <stdio.h>

#include <avdec_private.h>

typedef struct
  {
  uint8_t * buffer;
  int buffer_alloc;
  int buffer_size;
  int packet_size; /* Size of the entire packet (read from the first 2 bytes) */
  } dvdsub_t;

static int init_dvdsub(bgav_stream_t * s)
  {
  gavl_video_format_t * video_stream_format;
  dvdsub_t * priv;
  
  priv = calloc(1, sizeof(*priv));
  s->data.subtitle.decoder->priv = priv;

  /* Initialize format */
  video_stream_format = &(s->data.subtitle.video_stream->data.video.format);

  s->data.subtitle.format.image_width  = video_stream_format->image_width;
  s->data.subtitle.format.image_height = video_stream_format->image_height;

  s->data.subtitle.format.pixel_width  = video_stream_format->pixel_width;
  s->data.subtitle.format.pixel_height = video_stream_format->pixel_height;

  s->data.subtitle.format.frame_width  = video_stream_format->frame_width;
  s->data.subtitle.format.frame_height = video_stream_format->frame_height;
  s->data.subtitle.format.pixelformat = GAVL_YUVA_32;

  /* */
  
  return 1;
  }

/* Query if a subtitle is available */
static int has_subtitle_dvdsub(bgav_stream_t * s)
  {
  bgav_packet_t * p;
  dvdsub_t * priv;
  priv = (dvdsub_t *)s->data.subtitle.decoder->priv;
  
  /* Check if we have enough data */

  //  fprintf(stderr, "has_subtitle_dvdsub\n");
  
  while(1)
    {
    if(priv->packet_size && (priv->buffer_size >= priv->packet_size))
      return 1;

    if(!bgav_packet_buffer_peek_packet_read(s->packet_buffer))
      return 0;
    
    p = bgav_packet_buffer_get_packet_read(s->packet_buffer);
    
    }
  
  return 0;
  }

/*
 *  Decodes one frame. If frame is NULL;
 *  the frame is skipped
 */

static int decode_dvdsub(bgav_stream_t * s, gavl_overlay_t * ovl)
  {
  dvdsub_t * priv;
  priv = (dvdsub_t *)s->data.subtitle.decoder->priv;
  return 0;
  }

static void close_dvdsub(bgav_stream_t * s)
  {
  dvdsub_t * priv;
  priv = (dvdsub_t *)s->data.subtitle.decoder->priv;

  }

static void resync_dvdsub(bgav_stream_t * s)
  {
  dvdsub_t * priv;
  priv = (dvdsub_t *)s->data.subtitle.decoder->priv;
  
  }

static bgav_subtitle_overlay_decoder_t decoder =
  {
    fourccs: (uint32_t[]){ BGAV_MK_FOURCC('D', 'V', 'D', 'S'), 0 },
    name:    "DVD subtitle decoder",
    init:         init_dvdsub,
    has_subtitle: has_subtitle_dvdsub,
    decode:       decode_dvdsub,
    close:        close_dvdsub,
    resync:       resync_dvdsub,
  };

void bgav_init_subtitle_overlay_decoders_dvd()
  {
  bgav_subtitle_overlay_decoder_register(&decoder);
  }
