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

#include <avdec_private.h>
#include <stdlib.h>
#include <mpeg2dec/mpeg2.h>

typedef struct
  {
  const mpeg2_info_t * info;
  mpeg2dec_t * dec;
  gavl_video_frame_t * frame;
  bgav_packet_t * p;
  
  gavl_time_t picture_duration;
  gavl_time_t picture_timestamp;

  int intra_slice_refresh;
  } mpeg2_priv_t;

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
  
  mpeg2_buffer(priv->dec, priv->p->data, priv->p->data + priv->p->data_size);
  
  if(priv->p->timestamp >= 0)
    {
    //    fprintf(stderr, "Got pts %f\n", gavl_time_to_seconds(priv->p->timestamp));
    mpeg2_tag_picture(priv->dec,
                      (priv->p->timestamp) >> 32,
                      priv->p->timestamp & 0xffffffff);
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
    case 1126125:
      ret->timescale = 24000;
      ret->frame_duration = 1001;
      break;
    case 1125000:
      ret->timescale = 24;
      ret->frame_duration = 1;
      break;
    case 1080000:
      ret->timescale = 25;
      ret->frame_duration = 1;
      break;
    case 900900:
      ret->timescale = 30000;
      ret->frame_duration = 1001;
      break;
    case 900000:
      ret->timescale = 30;
      ret->frame_duration = 1;
      break;
    case 540000:
      ret->timescale = 50;
      ret->frame_duration = 1;
      break;
    case 450450:
      ret->timescale = 60000;
      ret->frame_duration = 1001;
      break;
    case 450000:
      ret->timescale = 60;
      ret->frame_duration = 1;
      break;
    }

  ret->image_width  = sequence->picture_width;
  ret->image_height = sequence->picture_height;
                                                                               
  ret->frame_width  = sequence->width;
  ret->frame_height = sequence->height;
                                                                               
  ret->pixel_width  = sequence->pixel_width;
  ret->pixel_height = sequence->pixel_height;

  if(sequence->chroma_height == sequence->height/2)
    ret->colorspace = GAVL_YUV_420_P;
  else if(sequence->chroma_height == sequence->height)
    ret->colorspace = GAVL_YUV_422_P;
  
  if(sequence->flags & SEQ_FLAG_MPEG2)
    ret->free_framerate = 1;
  }

int init_mpeg2(bgav_stream_t*s)
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
   *  If the first frame after a sequence header is a P-Frame, we have most likely an
   *  intra slice refresh stream
   */

  if(priv->info->current_picture->flags & PIC_FLAG_CODING_TYPE_P)
    priv->intra_slice_refresh = 1;
  
  return 1;
  }

int decode_mpeg2(bgav_stream_t*s, gavl_video_frame_t*f)
  {
  mpeg2_priv_t * priv;
  mpeg2_state_t state;
  priv = (mpeg2_priv_t*)(s->data.video.decoder->priv);
  /* Decode frame */

  if(f)
    mpeg2_skip(priv->dec, 0);
  else
    mpeg2_skip(priv->dec, 1);

  while(1)
    {
    if(!parse(s, &state))
      return 0;
    if((state == STATE_END) || (state == STATE_SLICE) ||
       (state == STATE_INVALID_END))
      break;
    }
  
  /* Copy frame */

  if(priv->info->display_fbuf)
    {
    /* Calculate timestamp */
    
    if(priv->info->display_picture->flags & PIC_FLAG_TAGS)
      {
      priv->picture_timestamp = priv->info->display_picture->tag;
      priv->picture_timestamp <<= 32;
      priv->picture_timestamp |= priv->info->display_picture->tag2;
      }
    else
      {
      priv->picture_timestamp += priv->picture_duration;
      }
    /* Get this pictures duration */

    priv->picture_duration =
      (GAVL_TIME_SCALE * s->data.video.format.frame_duration) /
      s->data.video.format.timescale;
    
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
    s->time = priv->picture_timestamp;
    }
  //  fprintf(stderr, "Timestamp: %f\n",
  //          gavl_time_to_seconds(priv->picture_timestamp));
  
  return 1;
  }

void resync_mpeg2(bgav_stream_t*s)
  {
  mpeg2_state_t state;
  mpeg2_priv_t * priv;
  priv = (mpeg2_priv_t*)(s->data.video.decoder->priv);
  mpeg2_reset(priv->dec, 0);
  mpeg2_buffer(priv->dec, NULL, NULL);
  mpeg2_skip(priv->dec, 1);
  while(1)
    {
    while(1)
      {
      if(!parse(s, &state))
        return;
      if(state == STATE_PICTURE)
        break;
      }
    if((priv->intra_slice_refresh)  &&
       (priv->info->current_picture->flags & PIC_FLAG_CODING_TYPE_P))
      break;
    else if(priv->info->current_picture->flags & PIC_FLAG_CODING_TYPE_I)
      break;
    }
  mpeg2_skip(priv->dec, 0);
  }

  
void close_mpeg2(bgav_stream_t*s)
  {
  mpeg2_priv_t * priv;
  priv = (mpeg2_priv_t*)(s->data.video.decoder->priv);
  
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
