/*****************************************************************
 * gmerlin-avdecoder - a general purpose multimedia decoding library
 *
 * Copyright (c) 2001 - 2008 Members of the Gmerlin project
 * gmerlin-general@lists.sourceforge.net
 * http://gmerlin.sourceforge.net
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 * *****************************************************************/

#include <avdec_private.h>
#include <codecs.h>

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <schroedinger/schro.h>
#include <schroedinger/schrovideoformat.h>

#define LOG_DOMAIN "video_schroedinger"

#define MAX_PTS 16

typedef struct
  {
  int used;
  uint32_t pic_num;
  int64_t pts;
  int duration;
  } pts_cache_t;

static void pts_cache_clear(pts_cache_t * p)
  {
  int i;
  for(i = 0; i < MAX_PTS; i++)
    p[i].used = 0;
  }

static void pts_cache_put(pts_cache_t * p,
                          int64_t pts, int duration,
                          uint32_t pic_num)
  {
  int i;
  fprintf(stderr, "Cache put: %d %ld\n", pic_num, pts);
  for(i = 0; i < MAX_PTS; i++)
    {
    if(!p[i].used)
      {
      p[i].pic_num = pic_num;
      p[i].pts = pts;
      p[i].duration = duration;
      p[i].used = 1;
      }
    }
  }

static int64_t pts_cache_get(pts_cache_t * p, int * duration,
                             uint32_t pic_num)
  {
  int i;
  fprintf(stderr, "Cache get: %d\n", pic_num);
  for(i = 0; i < MAX_PTS; i++)
    {
    if(p[i].used && (p[i].pic_num == pic_num))
      {
      p[i].used = 0;
      *duration = p[i].duration;
      return p[i].pts;
      }
    }
  fprintf(stderr, "Cache get failed\n");
  *duration = -1;
  return BGAV_TIMESTAMP_UNDEFINED;
  }

typedef struct
  {
  SchroDecoder * dec;
  SchroFrame * dec_frame;
  uint8_t * buffer_ptr;
  int buffer_size;
  
  gavl_video_frame_t * frame;
  
  SchroFrameFormat frame_format;
  
  int eof;

  int64_t last_pts;
  
  bgav_packet_t * p;

  pts_cache_t pts_cache[MAX_PTS];
  } schroedinger_priv_t;

/* Pixelformat stuff */

typedef struct
  {
  gavl_pixelformat_t pixelformat;
  SchroChromaFormat chroma_format;
  SchroFrameFormat  frame_format;
  SchroSignalRange  signal_range;
  int bits;
  } pixel_format_t;

static const pixel_format_t
pixel_format_map[] =
  {
    { GAVL_YUV_420_P, SCHRO_CHROMA_420, SCHRO_FRAME_FORMAT_U8_420, SCHRO_SIGNAL_RANGE_8BIT_VIDEO, 8 },
    { GAVL_YUV_422_P, SCHRO_CHROMA_422, SCHRO_FRAME_FORMAT_U8_422, SCHRO_SIGNAL_RANGE_8BIT_VIDEO, 8 },
    { GAVL_YUV_444_P, SCHRO_CHROMA_444, SCHRO_FRAME_FORMAT_U8_444, SCHRO_SIGNAL_RANGE_8BIT_VIDEO, 8 },
    { GAVL_YUVJ_420_P, SCHRO_CHROMA_420, SCHRO_FRAME_FORMAT_U8_420, SCHRO_SIGNAL_RANGE_8BIT_FULL, 8 },
    { GAVL_YUVJ_422_P, SCHRO_CHROMA_422, SCHRO_FRAME_FORMAT_U8_422, SCHRO_SIGNAL_RANGE_8BIT_FULL, 8 },
    { GAVL_YUVJ_444_P, SCHRO_CHROMA_444, SCHRO_FRAME_FORMAT_U8_444, SCHRO_SIGNAL_RANGE_8BIT_FULL, 8 },
  };

static const int num_pixel_formats = sizeof(pixel_format_map)/sizeof(pixel_format_map[0]);

static const pixel_format_t * pixelformat_from_schro(SchroVideoFormat *format)
  {
  int i;
  SchroSignalRange signal_range;
  
  signal_range = schro_video_format_get_std_signal_range(format);
  
  for(i = 0; i < num_pixel_formats; i++)
    {
    if((pixel_format_map[i].signal_range == signal_range) &&
       (pixel_format_map[i].chroma_format == format->chroma_format))
      return &pixel_format_map[i];
    }
  return NULL;
  }

static gavl_pixelformat_t get_pixelformat(SchroVideoFormat *format)
  {
  const pixel_format_t * pfmt = pixelformat_from_schro(format);
  if(pfmt)
    return pfmt->pixelformat;
  else
    return GAVL_PIXELFORMAT_NONE;
  }

static SchroFrameFormat
get_frame_format(SchroVideoFormat *format)
  {
  const pixel_format_t * pfmt = pixelformat_from_schro(format);
  if(pfmt)
    return pfmt->frame_format;
  else
    return 0;
  }

/* Get data */ 

static int next_startcode(uint8_t * data, int len)
  {
  int ret;
  ret = (data[5]<<24) | (data[6]<<16) | (data[7]<<8) | (data[8]);
  if(!ret)
    ret = 13;
  return ret;
  }

static void
buffer_free (SchroBuffer *buf, void *priv)
  {
  free (priv);
  }

static SchroBuffer * get_data(bgav_stream_t * s)
  {
  schroedinger_priv_t* priv;
  SchroBuffer * ret;
  int size;
  uint8_t * data;
  priv = (schroedinger_priv_t*)(s->data.video.decoder->priv);
  
  if(priv->eof)
    return NULL;
  
  if(priv->buffer_size < 13)
    {
    if(priv->p)
      bgav_demuxer_done_packet_read(s->demuxer, priv->p);

    priv->p = bgav_demuxer_get_packet_read(s->demuxer, s);
    if(!priv->p)
      {
      priv->eof = 1;
      schro_decoder_push_end_of_stream(priv->dec);
      return NULL;
      }
    priv->buffer_size = priv->p->data_size;
    priv->buffer_ptr = priv->p->data;
    
    }
  
  size = next_startcode(priv->buffer_ptr, priv->buffer_size);
  
  if(SCHRO_PARSE_CODE_IS_PICTURE(priv->buffer_ptr[4]))
    {
    if(priv->p->pts != priv->last_pts)
      {
      uint32_t pic_num;
      pic_num = BGAV_PTR_2_32BE(priv->buffer_ptr + 13);
      pts_cache_put(priv->pts_cache,
                    priv->p->pts, priv->p->duration, pic_num);
      priv->last_pts = priv->p->pts;
      }
    //    codec->dec_delay++;
    //    fprintf(stderr, "** Delay++: %d\n", codec->dec_delay);
    }
  data = malloc(size);
  memcpy(data, priv->buffer_ptr, size);

  //  fprintf(stderr, "Buffer %d\n", size);
  //  lqt_hexdump(data, 128 > size ? size : 128, 16);
  
  ret = schro_buffer_new_with_data(data, size);
  
  ret->free = buffer_free;
  ret->priv = data;
  
  priv->buffer_size -= size;
  priv->buffer_ptr += size;
  
  return ret;
  }

/* Get format */

static void get_format(bgav_stream_t * s)
  {
  SchroVideoFormat * format;
  schroedinger_priv_t* priv;
  priv = (schroedinger_priv_t*)(s->data.video.decoder->priv);
  
  format = schro_decoder_get_video_format(priv->dec);

  /* Get colormodel */
  s->data.video.format.pixelformat = get_pixelformat(format);

  /* Get size */
  s->data.video.format.image_width = format->width;
  s->data.video.format.image_height = format->height;

  s->data.video.format.frame_width = format->width;
  s->data.video.format.frame_height = format->height;
  
#if 0
  if((vtrack->stream_cmodel == BC_YUV422P16) ||
     (vtrack->stream_cmodel == BC_YUV444P16))
    {
    
    }
  else
    codec->dec_copy_frame = copy_frame_8;
#endif
  
  priv->frame_format = get_frame_format(format);
  
  /* Get interlace mode */
  if(format->interlaced)
    {
    if(format->top_field_first)
      s->data.video.format.interlace_mode = GAVL_INTERLACE_TOP_FIRST;
    else
      s->data.video.format.interlace_mode = GAVL_INTERLACE_BOTTOM_FIRST;
    }
  else
    s->data.video.format.interlace_mode = GAVL_INTERLACE_NONE;
  
  /* Get pixel aspect */
  s->data.video.format.pixel_width = 
    format->aspect_ratio_numerator;
  s->data.video.format.pixel_height = 
    format->aspect_ratio_denominator;
  
  free(format);
  
  }


/* Decode */

static int decode_picture(bgav_stream_t * s)
  {
  int duration;
  uint32_t pic_num;
  int64_t pts;
  int state;
  SchroBuffer * buf = NULL;
  SchroFrame * frame = NULL;
  schroedinger_priv_t* priv;
  priv = (schroedinger_priv_t*)(s->data.video.decoder->priv);

  while(1)
    {
    state = schro_decoder_wait(priv->dec);
    
    switch (state)
      {
      case SCHRO_DECODER_FIRST_ACCESS_UNIT:
        //        fprintf(stderr, "State: SCHRO_DECODER_FIRST_ACCESS_UNIT\n");

        get_format(s);
        
        // libschroedinger_handle_first_access_unit (avccontext);
        break;

      case SCHRO_DECODER_NEED_BITS:
        /* Need more input data - stop iterating over what we have. */
        //        fprintf(stderr, "State: SCHRO_DECODER_NEED_BITS\n");

        buf = get_data(s);
#if 1
        if(buf)
          {
          state = schro_decoder_push(priv->dec, buf);
          if(state == SCHRO_DECODER_FIRST_ACCESS_UNIT)
            {
            //            fprintf(stderr, "State: SCHRO_DECODER_FIRST_ACCESS_UNIT\n");
            get_format(s);
            }
          }
#endif
        break;

      case SCHRO_DECODER_NEED_FRAME:
        /* Decoder needs a frame - create one and push it in. */
        //        fprintf(stderr, "State: SCHRO_DECODER_NEED_FRAME\n");
        frame = schro_frame_new_and_alloc(NULL,
                                          priv->frame_format,
                                          s->data.video.format.frame_width,
                                          s->data.video.format.frame_height);
        schro_decoder_add_output_picture (priv->dec, frame);
        //        fprintf(stderr, "Need frame %p\n", frame);
        break;

      case SCHRO_DECODER_OK:

        pic_num = schro_decoder_get_picture_number(priv->dec);

        /* Pull a frame out of the decoder. */
        //        fprintf(stderr, "State: SCHRO_DECODER_OK %d\n",
        //                schro_decoder_get_picture_number(codec->dec));
        
        // if(codec->dec_delay)
        //          {
        priv->dec_frame = schro_decoder_pull(priv->dec);
        
        pts = pts_cache_get(priv->pts_cache, &duration, pic_num);
        
        s->out_time =
          gavl_time_rescale(s->timescale,
                            s->data.video.format.timescale,
                            pts);
        
        return 1;
          //          }
        break;
      case SCHRO_DECODER_EOS:
        //        fprintf(stderr, "State: SCHRO_DECODER_EOS\n");
        // p_schro_params->eos_pulled = 1;
        //        schro_decoder_reset (decoder);
        //        outer = 0;
        return 0;
        break;
      case SCHRO_DECODER_ERROR:
        fprintf(stderr, "State: SCHRO_DECODER_ERROR\n");
        return 0;
        break;
      }
    }
  return 0;
  }

static int init_schroedinger(bgav_stream_t * s)
  {
  schroedinger_priv_t * priv;

  schro_init();
  
  priv = calloc(1, sizeof(*priv));
  priv->last_pts = BGAV_TIMESTAMP_UNDEFINED;
  
  s->data.video.decoder->priv = priv;

  priv->dec = schro_decoder_new();

  priv->frame = gavl_video_frame_create(NULL);

  if(!decode_picture(s)) /* Get format */
    return 0;

  s->description = bgav_sprintf("Dirac"); 
  return 1;
  }

// static int64_t frame_counter = 0;

static int decode_schroedinger(bgav_stream_t * s, gavl_video_frame_t * frame)
  {
  schroedinger_priv_t * priv;
  priv = (schroedinger_priv_t*)(s->data.video.decoder->priv);

  if(!priv->dec_frame && !decode_picture(s))
    return 0;

  /* Copy frame */

  priv->frame->planes[0] = priv->dec_frame->components[0].data;
  priv->frame->planes[1] = priv->dec_frame->components[1].data;
  priv->frame->planes[2] = priv->dec_frame->components[2].data;
  
  priv->frame->strides[0] = priv->dec_frame->components[0].stride;
  priv->frame->strides[1] = priv->dec_frame->components[1].stride;
  priv->frame->strides[2] = priv->dec_frame->components[2].stride;

  gavl_video_frame_copy(&s->data.video.format,
                        frame, priv->frame);
  
  schro_frame_unref(priv->dec_frame);
  priv->dec_frame = NULL;
  
  return 1;
  }

static void close_schroedinger(bgav_stream_t * s)
  {
  schroedinger_priv_t * priv;
  priv = (schroedinger_priv_t*)(s->data.video.decoder->priv);

  if(priv->dec)
    schro_decoder_free(priv->dec);
  
  gavl_video_frame_null(priv->frame);
  gavl_video_frame_destroy(priv->frame);

  free(priv);
  }

static void resync_schroedinger(bgav_stream_t * s)
  {
  schroedinger_priv_t * priv;
  priv = (schroedinger_priv_t*)(s->data.video.decoder->priv);

  schro_decoder_reset(priv->dec);
  
  if(priv->dec_frame)
    {
    schro_frame_unref(priv->dec_frame);
    priv->dec_frame = NULL;
    }
  priv->eof = 0;
  priv->buffer_size = 0;
  priv->last_pts = BGAV_TIMESTAMP_UNDEFINED;
  }

static bgav_video_decoder_t decoder =
  {
    .name =   "Schroedinger decoder",
    .fourccs =  (uint32_t[]){ BGAV_MK_FOURCC('d', 'r', 'a', 'c'),
                            0x00  },
    .init =   init_schroedinger,
    .decode = decode_schroedinger,
    .close =  close_schroedinger,
    .resync = resync_schroedinger,
  };

void bgav_init_video_decoders_schroedinger()
  {
  bgav_video_decoder_register(&decoder);
  }
