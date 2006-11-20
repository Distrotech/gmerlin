/*****************************************************************

  demux_gsm.c

  Copyright (c) 2005 by Michael Gruenert - one78@web.de

  http://gmerlin.sourceforge.net

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111, USA.

*****************************************************************/

#include <stdio.h>
#include <avdec_private.h>
#include <string.h>

#define GSM_BLOCK_SIZE 33
#define GSM_FRAME_SIZE 160

static int probe_gsm(bgav_input_context_t * input)
  {
    char * pos;
  /* There seems to be no way to do proper probing of the stream.
     Therefore, we accept only local files with .gsm as extension */

  if(input->filename)
    {
    pos = strrchr(input->filename, '.');
    if(!pos)
      return 0;
    if(!strcmp(pos, ".gsm"))
      return 1;
    }
  return 0;
  }

static int64_t bytes_2_samples(int64_t bytes)
  {
  return (bytes / GSM_BLOCK_SIZE) * GSM_FRAME_SIZE;
  }

static int open_gsm(bgav_demuxer_context_t * ctx,
                   bgav_redirector_context_t ** redir)
  {
  bgav_stream_t * as;
  int64_t total_samples;
  
  /* Create track */
  ctx->tt = bgav_track_table_create(1);

  as = bgav_track_add_audio_stream(ctx->tt->current_track, ctx->opt);
  as->fourcc = BGAV_MK_FOURCC('G', 'S', 'M', ' ');
  as->data.audio.format.samplerate = 8000;
  as->data.audio.format.num_channels = 1;

  
  as->data.audio.block_align = GSM_BLOCK_SIZE;
  
  if(ctx->input->total_bytes)
    {
    total_samples = bytes_2_samples(ctx->input->total_bytes); 
    ctx->tt->current_track->duration = 
      gavl_samples_to_time(as->data.audio.format.samplerate, total_samples);

    if(ctx->input->input->seek_byte)
      ctx->can_seek = 1;
    }

  ctx->stream_description = bgav_sprintf("RAW GSM");
  
  return 1;
  }

static int next_packet_gsm(bgav_demuxer_context_t * ctx)
  {
  bgav_packet_t * p;
  bgav_stream_t * s;
  int bytes_read;
  
  s = &(ctx->tt->current_track->audio_streams[0]);
  p = bgav_stream_get_packet_write(s);

  bgav_packet_alloc(p, s->data.audio.block_align);

  p->pts = bytes_2_samples(ctx->input->position);
  p->keyframe = 1;
  bytes_read = bgav_input_read_data(ctx->input, p->data,
                                    s->data.audio.block_align);
  p->data_size = bytes_read;

  if(bytes_read < s->data.audio.block_align)
    return 0;
  
  bgav_packet_done_write(p);
  return 1;
  }

static void seek_gsm(bgav_demuxer_context_t * ctx, gavl_time_t time)
  {
  bgav_stream_t * s;
  int64_t position;
  int64_t sample;
  
  s = &(ctx->tt->current_track->audio_streams[0]);
  
  sample = gavl_time_to_samples(s->data.audio.format.samplerate, time); 
  sample /= GSM_FRAME_SIZE;
  position = sample * GSM_BLOCK_SIZE;
  sample *= GSM_FRAME_SIZE;
  
  bgav_input_seek(ctx->input, position, SEEK_SET);

  s->time_scaled = sample;
  }

static void close_gsm(bgav_demuxer_context_t * ctx)
  {
  return;
  }

bgav_demuxer_t bgav_demuxer_gsm =
  {
    probe:       probe_gsm,
    open:        open_gsm,
    next_packet: next_packet_gsm,
    seek:        seek_gsm,
    close:       close_gsm
  };
