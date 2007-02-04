/*****************************************************************
 
  demux_daud.c
 
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
#include <string.h>
#include <stdio.h>

static int probe_daud(bgav_input_context_t * input)
  {
  char * pos;
  /* There seems to be no way to do proper probing of the stream.
     Therefore, we accept only local files with .dv as extension */

  if(input->filename)
    {
    pos = strrchr(input->filename, '.');
    if(!pos)
      return 0;
    if(!strcasecmp(pos, ".302"))
      return 1;
    }
  return 0;
  }


static int open_daud(bgav_demuxer_context_t * ctx,
                   bgav_redirector_context_t ** redir)
  {
  bgav_stream_t * s;

  ctx->tt = bgav_track_table_create(1);
  s = bgav_track_add_audio_stream(&ctx->tt->tracks[0], ctx->opt);

  s->fourcc = BGAV_MK_FOURCC('d', 'a', 'u', 'd');
  s->data.audio.format.num_channels = 6;
  s->data.audio.format.samplerate = 96000;
  s->container_bitrate = 3 * 6 * 96000 * 8;
  s->data.audio.block_align = 3 * 6;
  s->data.audio.bits_per_sample = 24;
  ctx->stream_description = bgav_sprintf("D-Cinema audio format");
  ctx->data_start = 0;
  ctx->flags |= BGAV_DEMUXER_HAS_DATA_START;
  if(ctx->input->input->seek_byte)
    ctx->flags |= BGAV_DEMUXER_CAN_SEEK;
  
  return 1;
  }

static int next_packet_daud(bgav_demuxer_context_t * ctx)
  {
  bgav_stream_t * s;
  bgav_packet_t * p;
  uint16_t size;

  if(!bgav_input_read_16_be(ctx->input, &size))
    return 0;
  bgav_input_skip(ctx->input, 2); // Unknown
  
  s = bgav_track_find_stream(ctx->tt->cur, 0);
  if(s)
    {
    p = bgav_stream_get_packet_write(s);
    bgav_packet_alloc(p, size);
    p->data_size = bgav_input_read_data(ctx->input, p->data, size);
    if(!p->data_size)
      return 0;
    bgav_packet_done_write(p);
    }
  else
    bgav_input_skip(ctx->input, size);
  
  return 1;
  }

static void close_daud(bgav_demuxer_context_t * ctx)
  {
  }

bgav_demuxer_t bgav_demuxer_daud =
  {
    probe:       probe_daud,
    open:        open_daud,
    next_packet: next_packet_daud,
    close:       close_daud
  };
