/*****************************************************************
 
  demux_mpegps.c
 
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

#include <stdlib.h>

#include <avdec_private.h>
#include <pes_packet.h>

#define SYSTEM_HEADER 0x000001bb
#define PACK_HEADER   0x000001ba
#define PROGRAM_END   0x000001b9

/* Synchronization routines */

#define IS_START_CODE(h)  ((h&0xffffff00)==0x00000100)

uint32_t next_start_code(bgav_input_context_t * ctx)
  {
  uint32_t c;
  while(1)
    {
    if(!bgav_input_get_32_be(ctx, &c))
      return 0;
    if(IS_START_CODE(c))
      return c;
    bgav_input_skip(ctx, 1);
    }
  return 0;
  }

uint32_t previous_start_code(bgav_input_context_t * ctx)
  {
  uint32_t c;
  bgav_input_seek(ctx, -1, SEEK_CUR);
  while(ctx->position > 0)
    {
    if(!bgav_input_get_32_be(ctx, &c))
      return 0;
    if(IS_START_CODE(c))
      return c;
    bgav_input_seek(ctx, -1, SEEK_CUR);
    }
  return 0;
  }

/* Probe */

static int probe_mpegps(bgav_input_context_t * input)
  {
  uint32_t h;
  if(!bgav_input_get_32_be(input, &h))
    return 0;
  if(h == PACK_HEADER)
    return 1;
  return 0;
  }


/* Pack header */

typedef struct
  {
  int64_t scr;
  int mux_rate;
  int version;
  } pack_header_t;

static int pack_header_read(bgav_input_context_t * input,
                            pack_header_t * ret)
  {
  uint8_t c;
  uint16_t tmp_16;
  
  if(bgav_input_read_8(input, &c))
    return 0;

  if((c & 0xf0) == 0x20) /* MPEG-1 */
    {
    bgav_input_read_8(input, &c);
    
    ret->scr = ((c >> 1) & 7) << 30;
    bgav_input_read_16_be(input, &tmp_16);
    ret->scr |= ((tmp_16 >> 1) << 15);
    bgav_input_read_16_be(input, &tmp_16);
    ret->scr |= (tmp_16 >> 1);
    
    bgav_input_read_8(input, &c);
    ret->mux_rate = (c & 0x7F) << 15;
                                                                                
    bgav_input_read_8(input, &c);
    ret->mux_rate |= ((c & 0x7F) << 7);
                                                                                
    bgav_input_read_8(input, &c);
    ret->mux_rate |= (((c & 0xFE)) >> 1);
    ret->version = 1;
    //    fprintf(stderr, "SCR: %f\n", demuxer->pack_header.scr / 90000.0);
    }
  else
    {
    
    }
  

  return 0;
  }

static void pack_header_dump(pack_header_t * h)
  {
  
  }

/* System header */

typedef struct
  {
  int dummy;
  } system_header_t;

static int system_header_read(bgav_input_context_t * input,
                              system_header_t * ret)
  {
  int16_t len;
  if(!bgav_input_read_16_le(input, &len))
    return 0;
  bgav_input_skip(input, len);
  return 1;
  }

static void system_header_dump(system_header_t * h)
  {
  fprintf(stderr, "System header (skipped)\n");
  }

/* Demuxer structure */

typedef struct
  {
  /* Actions for next_packet */
  
  int add_streams;
  int do_seek;

  /* Headers */

  pack_header_t   pack_header;
  
  } mpegps_priv_t;

/* Get one packet */

static int next_packet_mpegps(bgav_demuxer_context_t * ctx)
  {
  mpegps_priv_t * priv;
  system_header_t system_header;
  
  int got_packet = 0;
  uint32_t start_code;

  priv = (mpegps_priv_t*)(ctx->priv);
  
  while(!got_packet)
    {
    if(!(start_code = next_start_code(ctx->input)))
      return 0;

    if(start_code == SYSTEM_HEADER)
      {
      if(!system_header_read(ctx->input, &system_header))
        return 0;
      system_header_dump(&system_header);
      }
    else if(start_code == PACK_HEADER)
      {
      if(!pack_header_read(ctx->input, &(priv->pack_header)))
        return 0;
      pack_header_dump(&(priv->pack_header));
      }
    
    }
  
  return 1;
  }

#define NUM_PACKETS 200

static int find_streams(bgav_demuxer_context_t * ctx)
  {
  return 0;
  }

static int open_mpegps(bgav_demuxer_context_t * ctx,
                       bgav_redirector_context_t ** redir)
  {
  mpegps_priv_t * priv;

  priv = calloc(1, sizeof(*priv));
  ctx->priv = priv;
  
  if(!ctx->tt)
    {
    ctx->tt = bgav_track_table_create(1);
    //    find_streams(
    }
    
  return 0;
  }

static void seek_mpegps(bgav_demuxer_context_t * ctx, gavl_time_t time)
  {
  
  }

static void close_mpegps(bgav_demuxer_context_t * ctx)
  {
  
  }


bgav_demuxer_t bgav_demuxer_mpegps =
  {
    probe:       probe_mpegps,
    open:        open_mpegps,
    next_packet: next_packet_mpegps,
    seek:        seek_mpegps,
    close:       close_mpegps
  };
