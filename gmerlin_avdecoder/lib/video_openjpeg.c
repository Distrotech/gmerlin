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

/* Jpeg2000 decoder */

#include <stdlib.h>
#include <string.h>
#include <openjpeg.h>

#include <config.h>
#include <avdec_private.h>
#include <codecs.h>

#define LOG_DOMAIN "video_openjpeg"
#define LOG_DOMAIN_OJP "openjpeg"

#define J2K_CFMT 0
#define JP2_CFMT 1
#define JPT_CFMT 2

static void error_callback(const char *msg, void *client_data)
  {
  bgav_options_t * opt = (bgav_options_t*)client_data;
  bgav_log(opt, BGAV_LOG_ERROR, LOG_DOMAIN_OJP, msg);
  }

static void warning_callback(const char *msg, void *client_data)
  {
  bgav_options_t * opt = (bgav_options_t*)client_data;
  bgav_log(opt, BGAV_LOG_WARNING, LOG_DOMAIN_OJP, msg);
  }

static void info_callback(const char *msg, void *client_data)
  {
  bgav_options_t * opt = (bgav_options_t*)client_data;
  bgav_log(opt, BGAV_LOG_INFO, LOG_DOMAIN_OJP, msg);
  }

typedef struct
  {
  int have_frame;
  int need_format;

  opj_dparameters_t parameters;	/* decompression parameters */
  opj_event_mgr_t event_mgr;		/* event manager */
  opj_dinfo_t* dinfo;	/* handle to a decompressor */

  void (*debayer_func)(int ** planes, int width, int height,
                       float * out, int out_stride);
  opj_image_t *img;
  
  } openjpeg_priv_t;

/* Debayer functions (from blender) */

/* pretty simple but astonishingly very effective "debayer" function 
 */

static void
redcode_ycbcr2rgb_fullscale(int ** planes, int width, int height,
                            float * out, int out_stride)
  {
  int x,y;
  int pix_max = 4096;
  int mask = pix_max - 1;
  float *o;
  for (y = 0; y < height; y++)
    {
    fprintf(stderr, "y: %d\n", y);
    for (x = 0; x < width; x++)
      {
      int i = x + y*width;
      int i_p = (y > 0) ? i-width : i;
      int i_n = (y < (height-1)) ? i + width : i;
      float y1n = planes[0][i_n] & mask;
      float y1  = planes[0][i] & mask;
      float cb  = (planes[1][i] & mask)   - pix_max/2;
      float cr  = (planes[2][i] & mask)   - pix_max/2;
      float y2  = (planes[3][i] & mask);
      float y2p = (planes[3][i_p] & mask);

      float b_ = cb /(pix_max/2);
      float r_ = cr /(pix_max/2);
      float g_ = 0.0;
		
      float y_[4] = {y1 / pix_max, 
                     (y2 + y2p)/2 / pix_max, 
                     (y1 + y1n)/2 / pix_max, 
                     y2 / pix_max};

      int j;
      int yc = 0;

      o = out + (height-1-y)*(out_stride / sizeof(float))
        + x*3;

      for (j = 0; j < 6; j += 3)
        {
        o[j+0] = r_ + y_[yc];
        o[j+1] = g_ + y_[yc];
        o[j+2] = b_ + y_[yc];
        yc++;
        }

      if(height-1-y > 1)
        {
        o = out + (height-1-y)*(out_stride / sizeof(float))
          + x*3 - out_stride / sizeof(float);
        
        for (j = 0; j < 6; j += 3)
          {
          o[j+0] = r_ + y_[yc];
          o[j+1] = g_ + y_[yc];
          o[j+2] = b_ + y_[yc];
          yc++;
          }
        
        }
      }
    }
  }

static void
redcode_ycbcr2rgb_halfscale(int ** planes, int width, int height,
                            float * out, int out_stride)
  {
  int x,y;
  int pix_max = 4096;
  int mask = pix_max - 1;

  for (y = 0; y < height; y++)
    {
    float *o = out + width * (height - y - 1);
    for (x = 0; x < width; x++)
      {
      int i = y*height + x;
      float y1  = (planes[0][i] & mask);
      float cb  = (planes[1][i] & mask)  - pix_max/2;
      float cr  = (planes[2][i] & mask)  - pix_max/2;
      float y2  = (planes[3][i] & mask);

      float b_ = cb /(pix_max/2);
      float r_ = cr /(pix_max/2);
      float g_ = 0.0;
			
      float y = (y1 + y2)/2 / pix_max;

      *o++ = r_ + y;
      *o++ = g_ + y;
      *o++ = b_ + y;
      *o++ = 1.0;
      }
    }
  }

static void
redcode_ycbcr2rgb_quarterscale(int ** planes, int width, int height,
                               float * out, int out_stride)
  {
  int x,y;
  int pix_max = 4096;
  int mask = pix_max - 1;

  for (y = 0; y < height; y += 2)
    {
    float *o = out + (width/2) * (height/2 - y/2 - 1);
    for (x = 0; x < width; x += 2)
      {
      int i = y * width + x;
      float y1  = planes[0][i] & mask;
      float cb  = (planes[1][i] & mask)  - pix_max/2;
      float cr  = (planes[2][i] & mask)  - pix_max/2;
      float y2  = planes[3][i] & mask;

      float b_ = cb /(pix_max/2);
      float r_ = cr /(pix_max/2);
      float g_ = 0.0;
			
      float y = (y1 + y2)/2 / pix_max;
			
      *o++ = r_ + y;
      *o++ = g_ + y;
      *o++ = b_ + y;
      *o++ = 1.0;
      }
    }
  }

static int decode_openjpeg(bgav_stream_t * s, gavl_video_frame_t * f)
  {
  openjpeg_priv_t * priv;
  bgav_packet_t * p;
  opj_cio_t *cio;

  priv = (openjpeg_priv_t*)(s->data.video.decoder->priv);

  if(!priv->have_frame)
    {
    p = bgav_demuxer_get_packet_read(s->demuxer, s);
    if(!p)
      return 0;
    }
  else
    p = (bgav_packet_t*)0;
  
  if(f || priv->need_format)
    {
    if(!priv->have_frame)
      {
      /* open a byte stream */
      cio = opj_cio_open((opj_common_ptr)priv->dinfo, 
                         p->data, p->data_size);
      
      priv->img = opj_decode(priv->dinfo, cio);
      if(priv->need_format)
        {
        s->data.video.format.image_width  = priv->img->x1 - priv->img->x0; 
        s->data.video.format.image_height = priv->img->y1 - priv->img->y0; 
        s->data.video.format.frame_width  = s->data.video.format.image_width; 
        s->data.video.format.frame_height = s->data.video.format.image_height; 
        
        if(s->fourcc == BGAV_MK_FOURCC('R', '3', 'D', '1'))
          {
          s->data.video.format.pixelformat = GAVL_RGB_FLOAT;
          priv->debayer_func = redcode_ycbcr2rgb_fullscale;
          }
        }
      priv->have_frame = 1;
      }
    if(f)
      {
      if(priv->debayer_func)
        {
        int i;
        int* planes[4];
        for(i = 0; i < 4; i++)
          planes[i] = priv->img->comps[i].data;

        priv->debayer_func(planes, priv->img->comps[0].w, priv->img->comps[0].h,
                        (float*)f->planes[0], f->strides[0]);
        }
      else
        {
        
        }
      priv->have_frame = 0;
      }
      
    }
  if(p)
    bgav_demuxer_done_packet_read(s->demuxer, p);
  
  return 1;
  }

static int init_openjpeg(bgav_stream_t * s)
  {
  openjpeg_priv_t * priv;
  priv = calloc(1, sizeof(*priv));
  s->data.video.decoder->priv = priv;

  priv->event_mgr.error_handler = error_callback;
  priv->event_mgr.warning_handler = warning_callback;
  priv->event_mgr.info_handler = info_callback;

  opj_set_default_decoder_parameters(&priv->parameters);
  priv->parameters.decod_format = JP2_CFMT;

#if 0  
  if(scale == 2)
    {
    priv->parameters.cp_reduce = 1;
    }
  else if (scale == 4)
    {
    priv->parameters.cp_reduce = 2;
    }
  else if (scale == 8)
    {
    priv->parameters.cp_reduce = 3;
    }
#endif

  /* get a decoder handle */
  priv->dinfo = opj_create_decompress(CODEC_JP2);

  /* catch events using our callbacks and give a local context */
  opj_set_event_mgr((opj_common_ptr)priv->dinfo, &priv->event_mgr,
                    (bgav_options_t*)s->opt);
  
  /* setup the decoder decoding parameters using the current image 
     and user parameters */
  opj_setup_decoder(priv->dinfo, &priv->parameters);

  priv->need_format = 1;
  decode_openjpeg(s, (gavl_video_frame_t*)0);
  priv->need_format = 0;
  
  s->description = bgav_sprintf("JPEG-2000"); 
  return 1;
  }

static void close_openjpeg(bgav_stream_t * s)
  {
  openjpeg_priv_t * priv;
  priv = (openjpeg_priv_t*)(s->data.video.decoder->priv);
  free(priv);
  }

#if 0
static void resync_openjpeg(bgav_stream_t * s)
  {
  openjpeg_priv_t * priv;
  priv = (openjpeg_priv_t*)(s->data.video.decoder->priv);
  bgav_openjpeg_reader_reset(priv->openjpeg_reader);
  priv->have_header = 0;
  }
#endif

static bgav_video_decoder_t decoder =
  {
    .name =   "OPENJPEG video decoder",
    .fourccs =  (uint32_t[]){ BGAV_MK_FOURCC('R', '3', 'D', '1'),
                              0x00  },
    .init =   init_openjpeg,
    .decode = decode_openjpeg,
    //    .resync = resync_openjpeg,
    .close =  close_openjpeg,
    .resync = NULL,
  };

void bgav_init_video_decoders_openjpeg()
  {
  bgav_video_decoder_register(&decoder);
  }
