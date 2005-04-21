/*****************************************************************
 
  video_libmpeg2.c
 
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

#include <inttypes.h>
#include <mpeg2dec/mpeg2.h>

#include <config.h>
#include <codecs.h>
#include <avdec_private.h>

/* Debug function */

void dump_sequence_header(const mpeg2_sequence_t * s)
  {
  fprintf(stderr, "Sequence header:\n");
  fprintf(stderr, "size:         %d x %d\n", s->width, s->height);
  fprintf(stderr, "chroma_size:  %d x %d\n", s->chroma_width,
          s->chroma_height);

  fprintf(stderr, "picture_size: %d x %d\n", s->picture_width,
          s->picture_height);
  fprintf(stderr, "display_size: %d x %d\n", s->display_width,
          s->display_height);
  fprintf(stderr, "pixel_size:   %d x %d\n", s->pixel_width,
          s->pixel_height);
  }


typedef struct
  {
  const mpeg2_info_t * info;
  mpeg2dec_t * dec;
  gavl_video_frame_t * frame;
  bgav_packet_t * p;
  
  gavl_time_t picture_duration;
  gavl_time_t picture_timestamp;

  int intra_slice_refresh;
  
  int do_resync;

  const mpeg2_picture_t * first_iframe;
  } mpeg2_priv_t;

int dump_packet = 1;

static int get_data(bgav_stream_t*s)
  {
  mpeg2_priv_t * priv;
  priv = (mpeg2_priv_t*)(s->data.video.decoder->priv);
  
  if(priv->p)
    {
    bgav_demuxer_done_packet_read(s->demuxer, priv->p);
    }
  priv->p = bgav_demuxer_get_packet_read(s->demuxer, s);
  if(!priv->p)
    return 0;

  //  fprintf(stderr, "Got data %d\n", priv->p->data_size);
  //  bgav_hexdump(priv->p->data, 16, 16);
  
  mpeg2_buffer(priv->dec, priv->p->data, priv->p->data + priv->p->data_size);
  
  if(priv->p->timestamp_scaled >= 0)
    {
    //    fprintf(stderr, "Got pts %lld\n", priv->p->timestamp_scaled);
    mpeg2_tag_picture(priv->dec,
                      (priv->p->timestamp_scaled) >> 32,
                      priv->p->timestamp_scaled & 0xffffffff);
    if(priv->do_resync)
      priv->picture_timestamp =
        (priv->p->timestamp_scaled * s->data.video.format.timescale) / s->timescale;
    }
  
  return 1;
  }

static int parse(bgav_stream_t*s, mpeg2_state_t * state)
  {
  mpeg2_priv_t * priv;
  priv = (mpeg2_priv_t*)(s->data.video.decoder->priv);

  while(1)
    {
    *state = mpeg2_parse(priv->dec);
    if(*state == STATE_BUFFER)
      {
      if(!get_data(s))
        return 0;
      }
    else
      return 1;
    }
  }

static void get_format(gavl_video_format_t * ret,
                       const mpeg2_sequence_t * sequence)
  {
  switch(sequence->frame_period)
    {
    /* Original timscale is 27.000.000, a bit too much for us */

    /* We choose duration/scale so that the furation is always even.
       Since the MPEG-2 repeat stuff happens at half-frametime level,
       we nevertheless get 100% precise timestamps
    */
    
    case 1126125: /* 24000 / 1001 */
      ret->timescale = 48000;
      ret->frame_duration = 2002;
      break;
    case 1125000: /* 24 / 1 */
      ret->timescale = 48;
      ret->frame_duration = 2;
      break;
    case 1080000: /* 25 / 1 */
      ret->timescale = 50;
      ret->frame_duration = 2;
      break;
    case 900900: /* 30000 / 1001 */
      ret->timescale = 60000;
      ret->frame_duration = 2002;
      break;
    case 900000: /* 30 / 1 */
      ret->timescale = 60;
      ret->frame_duration = 2;
      break;
    case 540000: /* 50 / 1 */
      ret->timescale = 100;
      ret->frame_duration = 2;
      break;
    case 450450: /* 60000 / 1001 */
      ret->timescale = 120000;
      ret->frame_duration = 2002;
      break;
    case 450000: /* 60 / 1 */
      ret->timescale = 120;
      ret->frame_duration = 2;
      break;
    }

  ret->image_width  = sequence->picture_width;
  ret->image_height = sequence->picture_height;
                                                                               
  ret->frame_width  = sequence->width;
  ret->frame_height = sequence->height;

#if 1
  ret->pixel_width  = sequence->pixel_width;
  ret->pixel_height = sequence->pixel_height;
#else
  mpeg2_guess_aspect(sequence,
                     &(ret->pixel_width),
                     &(ret->pixel_height));
#endif
  if(sequence->chroma_height == sequence->height/2)
    ret->colorspace = GAVL_YUV_420_P;
  else if(sequence->chroma_height == sequence->height)
    ret->colorspace = GAVL_YUV_422_P;
  
  if(sequence->flags & SEQ_FLAG_MPEG2)
    ret->free_framerate = 1;
  dump_sequence_header(sequence);
  }

static int init_mpeg2(bgav_stream_t*s)
  {
  mpeg2_state_t state;
  mpeg2_priv_t * priv;

  priv = calloc(1, sizeof(*priv));
  s->data.video.decoder->priv = priv;
  
  priv->dec  = mpeg2_init();
  priv->info = mpeg2_info(priv->dec);
  
  while(1)
    {
    if(!parse(s, &state))
      return 0;
    if(state == STATE_SEQUENCE)
      break;
    }
  
  /* Get format */
  
  get_format(&(s->data.video.format), priv->info->sequence);
  priv->frame = gavl_video_frame_create(NULL);

  priv->frame->strides[0] = priv->info->sequence->width;
  priv->frame->strides[1] = priv->info->sequence->chroma_width;
  priv->frame->strides[2] = priv->info->sequence->chroma_width;
  
  s->description =
    bgav_sprintf("MPEG-%d, %.2f kb/sec", 
                 (priv->info->sequence->flags & SEQ_FLAG_MPEG2) ? 2 : 1,
                 (float)priv->info->sequence->byte_rate * 8.0 / 1000.0);
  s->codec_bitrate = priv->info->sequence->byte_rate * 8;
  
  while(1)
    {
    if(!parse(s, &state))
      return 0;
    if(state == STATE_PICTURE)
      break;
    }
  /*
   *  If the first frame after a sequence header is a P-Frame,
   *  we have most likely an
   *  intra slice refresh stream
   */

  if(priv->info->current_picture->flags & PIC_FLAG_CODING_TYPE_P)
    priv->intra_slice_refresh = 1;
  
  return 1;
  }

int decode_mpeg2(bgav_stream_t*s, gavl_video_frame_t*f)
  {
  int64_t tmp;
  mpeg2_priv_t * priv;
  mpeg2_state_t state;
  priv = (mpeg2_priv_t*)(s->data.video.decoder->priv);
  /* Decode frame */

  //  if(!f)
  //    fprintf(stderr, "Skipping frame\n");
#if 0
  if(f)
    mpeg2_skip(priv->dec, 0);
  else
    mpeg2_skip(priv->dec, 1);
#endif
  while(1)
    {
    if(!parse(s, &state))
      return 0;
    if(((state == STATE_END) || (state == STATE_SLICE) ||
        (state == STATE_INVALID_END)) && priv->info->display_fbuf)
      {
      if(priv->first_iframe)
        {
        if(priv->info->display_picture == priv->first_iframe)
          {
          // fprintf(stderr, "Got I Frame 1: %p\n", priv->info->display_picture); 
          priv->first_iframe = (mpeg2_picture_t*)0;
          break;
          }
        }
      else
        break;
      }
    }
#if 0
  if(priv->info->display_picture->flags & PIC_FLAG_SKIP)
    fprintf(stderr, "BUG: Displaying skipped picture\n");
#endif
  /* Calculate timestamp */
    
  if(priv->info->display_picture->flags & PIC_FLAG_TAGS)
    {
    tmp = priv->info->display_picture->tag;
    tmp <<= 32;
    tmp |= priv->info->display_picture->tag2;
    priv->picture_timestamp = (tmp * s->data.video.format.timescale) / s->timescale;
    //    fprintf(stderr, "Got time from pts: %lld\n", priv->picture_timestamp);
    }
  else
    {
    priv->picture_timestamp += priv->picture_duration;
    //    fprintf(stderr, "Calculated time: %lld\n", priv->picture_timestamp);
    }
  /* Get this pictures duration */
  
  priv->picture_duration = s->data.video.format.frame_duration;
  
  if((priv->info->display_picture->flags & PIC_FLAG_TOP_FIELD_FIRST) &&
     (priv->info->display_picture->nb_fields > 2))
    {
    priv->picture_duration =
      (priv->picture_duration * priv->info->current_picture->nb_fields) / 2;
    }
  
  if(f)
    {
    priv->frame->planes[0] = priv->info->display_fbuf->buf[0];
    priv->frame->planes[1] = priv->info->display_fbuf->buf[1];
    priv->frame->planes[2] = priv->info->display_fbuf->buf[2];
    gavl_video_frame_copy(&(s->data.video.format), f, priv->frame);
    }
  s->time_scaled = priv->picture_timestamp;
    
  s->data.video.last_frame_time     = priv->picture_timestamp;
  s->data.video.last_frame_duration = priv->picture_duration;
#if 0
  if((priv->info->display_picture->flags & PIC_MASK_CODING_TYPE) == PIC_FLAG_CODING_TYPE_I)
    fprintf(stderr, "I-Frame\n");
  else if((priv->info->display_picture->flags & PIC_MASK_CODING_TYPE) == PIC_FLAG_CODING_TYPE_P)
    fprintf(stderr, "P-Frame\n");
  else if((priv->info->display_picture->flags & PIC_MASK_CODING_TYPE) == PIC_FLAG_CODING_TYPE_B)
    fprintf(stderr, "B-Frame\n");
#endif
  
  //  fprintf(stderr, "Timestamp: %lld, duration: %d\n",
  //          s->data.video.last_frame_time,
  //          s->data.video.last_frame_duration);
  return 1;
  }

static void resync_mpeg2(bgav_stream_t*s)
  {
  mpeg2_state_t state;
  mpeg2_priv_t * priv;
  priv = (mpeg2_priv_t*)(s->data.video.decoder->priv);
  mpeg2_reset(priv->dec, 0);
  mpeg2_buffer(priv->dec, NULL, NULL);
  mpeg2_skip(priv->dec, 1);

  priv->do_resync = 1;

  while(1)
    {
    /* Get the next picture header */
    while(1)
      {
      if(!parse(s, &state))
        return;
      if(state == STATE_PICTURE)
        break;
      }
    //    fprintf(stderr, "Resync: Current: %p display: %p\n",
    //            priv->info->current_picture, priv->info->display_picture);
    
    /* Check if we can start decoding again */
    if((priv->intra_slice_refresh)  &&
       ((priv->info->current_picture->flags & PIC_MASK_CODING_TYPE) == PIC_FLAG_CODING_TYPE_P))
      break;
    else if(priv->info->current_picture &&
            ((priv->info->current_picture->flags & PIC_MASK_CODING_TYPE) == PIC_FLAG_CODING_TYPE_I))
      {
      // fprintf(stderr, "resync_mpeg2: Got I Frame %p\n", priv->info->current_picture);
      priv->first_iframe = priv->info->current_picture;
      break;
      }
    }
  mpeg2_skip(priv->dec, 0);
  priv->do_resync = 0;
  }

  
void close_mpeg2(bgav_stream_t*s)
  {
  mpeg2_priv_t * priv;
  priv = (mpeg2_priv_t*)(s->data.video.decoder->priv);

  if(priv->frame)
    {
    gavl_video_frame_null(priv->frame);
    gavl_video_frame_destroy(priv->frame);
    }

  if(priv->dec)
    mpeg2_close(priv->dec);
  
  free(priv);
  
  }

static bgav_video_decoder_t decoder =
  {
    fourccs: (uint32_t[]){ BGAV_MK_FOURCC('m','p','g','v'), 0x00 },
    name:    "libmpeg2 decoder",

    init:    init_mpeg2,
    decode:  decode_mpeg2,
    close:   close_mpeg2,
    resync:  resync_mpeg2,
  };

void bgav_init_video_decoders_libmpeg2()
  {
  bgav_video_decoder_register(&decoder);
  }
