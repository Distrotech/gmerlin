/*****************************************************************
 
  demux_nsv.c
 
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <avdec_private.h>

#define AUDIO_ID 0
#define VIDEO_ID 1

/*
 *  Straight forward implementation of the NSV spec from
 *  http://ultravox.aol.com/
 */

#define NSV_FILE_HEADER BGAV_MK_FOURCC('N','S','V','f')
#define NSV_SYNC_HEADER BGAV_MK_FOURCC('N','S','V','s')

typedef struct
  {
  uint32_t file_size;
  uint32_t file_len; /* Milliseconds */
  uint32_t metadata_len;
  uint32_t toc_alloc;
  uint32_t toc_size;

  struct
    {
    char * name;
    char * url;
    char * creator;
    char * aspect;
    char * framerate;
    } metadata;

  struct
    {
    uint32_t * offsets;
    uint32_t * frames; /* Only presend for Version 2 TOCs */
    } toc;
  
  } nsv_file_header_t;

static int nsv_file_header_read(bgav_input_context_t * ctx,
                                nsv_file_header_t * ret)
  {
  return 0;
  }

static void nsv_file_header_free(nsv_file_header_t * h)
  {
  if(h->metadata.name)      free(h->metadata.name);
  if(h->metadata.url)       free(h->metadata.url);
  if(h->metadata.creator)   free(h->metadata.creator);
  if(h->metadata.aspect)    free(h->metadata.aspect);
  if(h->metadata.framerate) free(h->metadata.framerate);

  if(h->toc.offsets)        free(h->toc.offsets);
  if(h->toc.frames)         free(h->toc.frames);
  }

typedef struct
  {
  uint32_t vidfmt;
  uint32_t audfmt;
  uint16_t width;
  uint16_t height;
  uint8_t  framerate;
  int16_t  syncoffs;
  } nsv_sync_header_t;

static int nsv_sync_header_read(bgav_input_context_t * ctx,
                                nsv_sync_header_t * ret)
  {
  bgav_input_skip(ctx, 4); /* NSVs */

  if(!bgav_input_read_fourcc(ctx, &ret->vidfmt) ||
     !bgav_input_read_fourcc(ctx, &ret->audfmt) ||
     !bgav_input_read_16_le(ctx, &ret->width) ||
     !bgav_input_read_16_le(ctx, &ret->height) ||
     !bgav_input_read_8(ctx, &ret->framerate) ||
     !bgav_input_read_16_le(ctx, (uint16_t*)(&ret->syncoffs)))
    return 0;
  return 1;
  }

static void nsv_sync_header_dump(nsv_sync_header_t * h)
  {
  fprintf(stderr, "sync_header\n");
  fprintf(stderr, "  vidfmt: ");
  bgav_dump_fourcc(h->vidfmt);
  fprintf(stderr, "\n");

  fprintf(stderr, "  audfmt: ");
  bgav_dump_fourcc(h->audfmt);
  fprintf(stderr, "\n");

  fprintf(stderr, "  width:         %d\n", h->width);
  fprintf(stderr, "  height:        %d\n", h->height);
  fprintf(stderr, "  framerate_idx: %d\n", h->framerate);
  fprintf(stderr, "  syncoffs:      %d\n", h->syncoffs);
  }
  
typedef struct
  {
  nsv_file_header_t fh;
  
  int payload_follows; /* Header already read, payload follows */

  int64_t frame_counter;
  } nsv_priv_t;

static int probe_nsv(bgav_input_context_t * input)
  {
  uint32_t fourcc;
  /* Check for video/nsv */

  if(input->mimetype && !strcmp(input->mimetype, "video/nsv"))
    return 1;

  if(!bgav_input_get_fourcc(input, &fourcc))
    return 0;

  if((fourcc != NSV_FILE_HEADER) &&
     (fourcc != NSV_SYNC_HEADER))
    return 0;

  return 1;
  }

static void simplify_rational(int * num, int * den)
  {
  int i = 2;

  while(i <= *den)
    {
    if(!(*num % i) && !(*den % i))
      {
      *num /= i;
      *den /= i;
      }
    else
      i++;
    }
  }

static void calc_framerate(int code, int * num, int * den)
  {
  int t, s_num, s_den;
  if(!(code & 0x80))
    {
    *num = code;
    *den = 1;
    return;
    }
  t = (code & 0x7f) >> 2;
  if(t < 16)
    {
    s_num = 1;
    s_den = t+1;
    }
  else
    {
    s_num = t - 15;
    s_den = 1;
    }

  if(code & 1)
    {
    s_num *= 1000;
    s_den *= 1001;
    }

  if((code & 3) == 3)
    {
    *num = s_num * 24;
    *den = s_den;
    }
  else if((code & 3) == 2)
    {
    *num = s_num * 25;
    *den = s_den;
    }
  else
    {
    *num = s_num * 30;
    *den = s_den;
    }
  simplify_rational(num, den);
  }
#if 0
static void test_framerate()
  {
  int i, num, den;
  for(i = 0; i < 256; i++)
    {
    calc_framerate(i, &num, &den);
    fprintf(stderr, "%d=%.4f [%d:%d]\n", i, (float)num/(float)den, num, den);
    }
  }
#endif
static int open_nsv(bgav_demuxer_context_t * ctx,
                    bgav_redirector_context_t ** redir)
  {
  bgav_stream_t * s;
  nsv_priv_t * p;

  nsv_sync_header_t sh;
  uint32_t fourcc;
  int done = 0;

  //  test_framerate();
  
  p = calloc(1, sizeof(*p));
  ctx->priv = p;
  
  while(!done)
    {
    /* Find the first chunk we can handle */
    while(1)
      {
      if(!bgav_input_get_fourcc(ctx->input, &fourcc))
        return 0;
      
      if((fourcc == NSV_FILE_HEADER) || (fourcc == NSV_SYNC_HEADER))
        break;
      bgav_input_skip(ctx->input, 1);
      }

    if(fourcc == NSV_FILE_HEADER)
      {
      fprintf(stderr, "Got file header\n");
      }
    else if(fourcc == NSV_SYNC_HEADER)
      {
      fprintf(stderr, "Got sync header\n");

      if(!nsv_sync_header_read(ctx->input, &sh))
        return 0;
      nsv_sync_header_dump(&sh);
      done = 1;
      }
    }
  ctx->tt = bgav_track_table_create(1);

  /* Set up streams */

  if(sh.vidfmt != BGAV_MK_FOURCC('N','O','N','E'))
    {
    s = bgav_track_add_video_stream(ctx->tt->current_track);
    s->fourcc = sh.vidfmt;
    
    s->data.video.format.image_width  = sh.width;
    s->data.video.format.image_height = sh.height;
    
    s->data.video.format.frame_width =
      s->data.video.format.image_width;
    s->data.video.format.frame_height =
      s->data.video.format.image_height;
    
    s->data.video.format.pixel_width  = 1;
    s->data.video.format.pixel_height = 1;
    s->stream_id = VIDEO_ID;

    /* Calculate framerate */

    calc_framerate(sh.framerate,
                   &s->data.video.format.timescale,
                   &s->data.video.format.frame_duration);
    }
  
  if(sh.audfmt != BGAV_MK_FOURCC('N','O','N','E'))
    {
    s = bgav_track_add_audio_stream(ctx->tt->current_track);
    s->fourcc = sh.audfmt;
    s->stream_id = AUDIO_ID;
    }

  /* TODO: Check for file header */

  p->payload_follows = 1;
  
  return 1;
  }

static int next_packet_nsv(bgav_demuxer_context_t * ctx)
  {
  //  uint8_t test_data[32];
  int i;
  bgav_packet_t * p;
  bgav_stream_t * s;
  nsv_priv_t * priv;
  uint8_t num_aux;
  uint16_t tmp_16;
  uint32_t aux_plus_video_len;
  uint32_t video_len;
  uint16_t audio_len;

  uint16_t aux_chunk_len;
  uint32_t aux_chunk_type;
  int skipped_bytes;
  
  nsv_sync_header_t sh;
  uint32_t fourcc;
  int have_sync_header = 0;
  
  priv = (nsv_priv_t *)(ctx->priv);

  if(!priv->payload_follows)
    {
    /* Read header */

    if(!bgav_input_get_16_le(ctx->input, &tmp_16))
      return 0;

    if(tmp_16 != 0xbeef)
      {
      skipped_bytes = 0;
      while(1)
        {
        if(!bgav_input_get_fourcc(ctx->input, &fourcc))
          return 0;
        
        if(fourcc == NSV_SYNC_HEADER)
          break;
        bgav_input_skip(ctx->input, 1);
        skipped_bytes++;
        }
      if(!nsv_sync_header_read(ctx->input, &sh))
        return 0;
      //      nsv_sync_header_dump(&sh);
      have_sync_header = 1;
      }
    else
      bgav_input_skip(ctx->input, 2); /* Skip beef */
    }


  //  bgav_input_get_data(ctx->input, test_data, 32);
  //  bgav_hexdump(test_data, 32, 16);
  /* Parse payload */
  
  if(!bgav_input_read_8(ctx->input, &num_aux))
    return 0;

  if(!bgav_input_read_16_le(ctx->input, &tmp_16))
    return 0;

  if(!bgav_input_read_16_le(ctx->input, &audio_len))
    return 0;

  aux_plus_video_len = tmp_16;
  
  aux_plus_video_len = (aux_plus_video_len << 4) | (num_aux >> 4);
  num_aux &= 0x0f;
  
  //  fprintf(stderr, "num_aux: %d, aux_plus_video_len: %d, audio_len: %d\n",
  //          num_aux, aux_plus_video_len, audio_len);

  video_len = aux_plus_video_len;

  /* Skip aux packets */
  for(i = 0; i < num_aux; i++)
    {
    if(!bgav_input_read_16_le(ctx->input, &aux_chunk_len) ||
       !bgav_input_read_fourcc(ctx->input, &aux_chunk_type))
      return 0;

    fprintf(stderr, "Skipping %d bytes of aux data type ", aux_chunk_len);
    bgav_dump_fourcc(aux_chunk_type);
    fprintf(stderr, "\n");
    bgav_input_skip(ctx->input, aux_chunk_len);

    video_len -= (aux_chunk_len+6);
    }

  /* Video data */

  if(video_len)
    {
    s = bgav_track_find_stream(ctx->tt->current_track, VIDEO_ID);
    if(s)
      {
      p = bgav_packet_buffer_get_packet_write(s->packet_buffer, s);
      bgav_packet_alloc(p, video_len);
      if(bgav_input_read_data(ctx->input, p->data, video_len) < video_len)
        return 0;
      p->data_size = video_len;
      p->timestamp_scaled =
        priv->frame_counter * s->data.video.format.frame_duration;
      bgav_packet_done_write(p);
      priv->frame_counter++;
      }
    else
      bgav_input_skip(ctx->input, video_len);
    }

  if(audio_len)
    {
    s = bgav_track_find_stream(ctx->tt->current_track, AUDIO_ID);
    if(s)
      {
      p = bgav_packet_buffer_get_packet_write(s->packet_buffer, s);
      bgav_packet_alloc(p, audio_len);
      if(bgav_input_read_data(ctx->input, p->data, audio_len) < audio_len)
        return 0;
      p->data_size = audio_len;
      bgav_packet_done_write(p);
      }
    else
      bgav_input_skip(ctx->input, audio_len);
    }
  priv->payload_follows = 0;
  return 1;
  }

static void seek_nsv(bgav_demuxer_context_t * ctx, gavl_time_t time)
  {
  }

static void close_nsv(bgav_demuxer_context_t * ctx)
  {
  }

bgav_demuxer_t bgav_demuxer_nsv =
  {
    probe:       probe_nsv,
    open:        open_nsv,
    next_packet: next_packet_nsv,
    seek:        seek_nsv,
    close:       close_nsv
  };

