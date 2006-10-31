/*****************************************************************
 
  demux_gif.c
 
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

#define LOG_DOMAIN "gif"

static const char * gif_sig = "GIF89a";
#define SIG_LEN 6

#define HEADER_LEN 13 /* Signature + screen descriptor */

typedef struct
  {
  uint16_t width;
  uint16_t height;
  uint8_t  flags;
  uint8_t  bg_color;
  uint8_t  pixel_aspect_ratio;
  } screen_descriptor_t;

static void parse_screen_descriptor(uint8_t * data,
                                    screen_descriptor_t * ret)
  {
  data += SIG_LEN;
  ret->width               = BGAV_PTR_2_16LE(data); data += 2;
  ret->height              = BGAV_PTR_2_16LE(data); data += 2;
  ret->flags               = *data; data++;
  ret->bg_color            = *data; data++;
  ret->pixel_aspect_ratio  = *data; data++;
  }

static void dump_screen_descriptor(screen_descriptor_t * sd)
  {
  bgav_dprintf("GIF Screen descriptor\n");
  bgav_dprintf("  width:              %d\n",  sd->width);
  bgav_dprintf("  height:             %d\n", sd->height);
  bgav_dprintf("  flags:              %02x\n", sd->flags);
  bgav_dprintf("  bg_color:           %d\n", sd->bg_color);
  bgav_dprintf("  pixel_aspect_ratio: %d\n", sd->pixel_aspect_ratio);
  }

/* Private context */

typedef struct
  {
  uint8_t header[HEADER_LEN];
  uint8_t global_cmap[768];
  int global_cmap_bytes;
  
  } gif_priv_t;

static int probe_gif(bgav_input_context_t * input)
  {
  char probe_data[SIG_LEN];
  if(bgav_input_get_data(input, (uint8_t*)probe_data, SIG_LEN) < SIG_LEN)
    return 0;
  if(!memcmp(probe_data, gif_sig, SIG_LEN))
    return 1;
  return 1;
  }

static void skip_extension(bgav_input_context_t * input)
  {
  uint8_t u_8;

  bgav_input_skip(input, 2);
  while(1)
    {
    if(!bgav_input_read_data(input, &u_8, 1))
      return;
    if(!u_8)
      return;
    else
      bgav_input_skip(input, u_8);
    }
  
  }

static int open_gif(bgav_demuxer_context_t * ctx,
                   bgav_redirector_context_t ** redir)
  {
  bgav_stream_t * s;
  uint8_t ext_header[2];
  gif_priv_t * priv;
  screen_descriptor_t sd;
  int done;
  
  priv = calloc(1, sizeof(*priv));
  ctx->priv = priv;

  if(bgav_input_read_data(ctx->input, priv->header, HEADER_LEN) < HEADER_LEN)
    return 0;
  
  parse_screen_descriptor(priv->header, &sd);
  dump_screen_descriptor(&sd);

  if(sd.flags & 0x80)
    {
    /* Global colormap present */
    priv->global_cmap_bytes = 3*(1 << ((sd.flags & 0x07) + 1));
    if(bgav_input_read_data(ctx->input, priv->global_cmap,
                            priv->global_cmap_bytes) <
       priv->global_cmap_bytes)
      return 0;
    }
  
  /* Check for extensions: We skip all extensions until we reach
     the first image data *or* a Graphic Control Extension */
  done = 0;
  while(!done)
    {
    if(bgav_input_get_data(ctx->input, ext_header, 2) < 2)
      return 0;
    
    switch(ext_header[0])
      {
      case '!': // Extension block
        if(ext_header[1] == 0xF9) /* Graphic Control Extension */
          done = 1;
        else
          {
          fprintf(stderr, "Skipping extension 0x%02x\n", ext_header[1]);
          skip_extension(ctx->input);
          }
        break;
      case ',':
        /* Image data found but no Graphic Control Extension:
           Assume, this is a still image and refuse to read it */
        return 0;
        break;
      default:
        return 0; /* Invalid data */
        break;
      }
    }

  ctx->tt = bgav_track_table_create(1);
  s = bgav_track_add_video_stream(ctx->tt->current_track, ctx->opt);
  s->fourcc = BGAV_MK_FOURCC('g','i','f',' ');

  s->data.video.format.image_width  = sd.width;
  s->data.video.format.image_height = sd.height;

  s->data.video.format.frame_width  = sd.width;
  s->data.video.format.frame_height = sd.height;
  s->data.video.format.pixel_width = 1;
  s->data.video.format.pixel_height = 1;
  s->data.video.format.timescale = 100;
  s->data.video.format.frame_duration = 100; // Not reliable
  s->data.video.format.framerate_mode = GAVL_FRAMERATE_VARIABLE;
  return 1;
  }

static int next_packet_gif(bgav_demuxer_context_t * ctx)
  {
  uint8_t header[2];
  int done = 0;
  gif_priv_t * priv;
  priv = (gif_priv_t*)(ctx->priv);

  if(!bgav_input_get_data(ctx->input, header, 1))
    return 0;
  switch(header[0])
    {
    case ';':
      return 0; // Trailer
      break;
    case ',':
      break;
    }
  
  return 0;
  }


static void close_gif(bgav_demuxer_context_t * ctx)
  {
  gif_priv_t * priv;
  priv = (gif_priv_t*)(ctx->priv);
  
  }

bgav_demuxer_t bgav_demuxer_gif =
  {
    probe:       probe_gif,
    open:        open_gif,
    next_packet: next_packet_gif,
    close:       close_gif
  };
