/*****************************************************************
 
  video_ffmpeg.c
 
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

#include <stdlib.h>
#include <string.h>
#include <ffmpeg/avcodec.h>

#include <config.h>
#include <bswap.h>
#include <codecs.h>
#include <avdec_private.h>

#include <stdio.h>

#ifdef HAVE_LIBDV
#include <dvframe.h>
#endif

/* Map of ffmpeg codecs to fourccs (from ffmpeg's avienc.c) */

typedef struct
  {
  const char * decoder_name;
  const char * format_name;
  enum CodecID ffmpeg_id;
  uint32_t * fourccs;
  } codec_info_t;

static codec_info_t codec_infos[] =
  {
    /************************************************************
     * Uncompressed "decoders"
     ************************************************************/

    { "FFmpeg Raw decoder", "Uncompressed", CODEC_ID_RAWVIDEO,
      (uint32_t[]){ BGAV_MK_FOURCC('Y', '4', '2', '2'),
               BGAV_MK_FOURCC('I', '4', '2', '0'),
               0x00 } },
        
    /************************************************************
     * Simple, mostly old decoders
     ************************************************************/
    
    { "FFmpeg 8BPS decoder", "Quicktime Planar RGB (8BPS)", CODEC_ID_8BPS,
      (uint32_t[]){ BGAV_MK_FOURCC('8', 'B', 'P', 'S'),
               0x00 } },
    { "FFmpeg Inteo 3 decoder", "Intel Indeo 3", CODEC_ID_INDEO3,
      (uint32_t[]){ BGAV_MK_FOURCC('I', 'V', '3', '1'),
                    BGAV_MK_FOURCC('I', 'V', '3', '2'),
                    BGAV_MK_FOURCC('i', 'v', '3', '1'),
                    BGAV_MK_FOURCC('i', 'v', '3', '2'),
                    0x00 } },
    { "FFmpeg rpza decoder", "Apple Video", CODEC_ID_RPZA,
      (uint32_t[]){ BGAV_MK_FOURCC('r', 'p', 'z', 'a'),
               BGAV_MK_FOURCC('a', 'z', 'p', 'r'),
               0x00 } },
    { "FFmpeg SMC decoder", "Apple Graphics", CODEC_ID_SMC,
      (uint32_t[]){ BGAV_MK_FOURCC('s', 'm', 'c', ' '),
               0x00 } },
    { "FFmpeg cinepak decoder", "Cinepak", CODEC_ID_CINEPAK,
      (uint32_t[]){ BGAV_MK_FOURCC('c', 'v', 'i', 'd'),
               0x00 } },
    { "FFmpeg MSVideo 1 decoder", "Microsoft Video 1", CODEC_ID_MSVIDEO1,
      (uint32_t[]){ BGAV_MK_FOURCC('M', 'S', 'V', 'C'),
               BGAV_MK_FOURCC('m', 's', 'v', 'c'),
               BGAV_MK_FOURCC('C', 'R', 'A', 'M'),
               BGAV_MK_FOURCC('c', 'r', 'a', 'm'),
               BGAV_MK_FOURCC('W', 'H', 'A', 'M'),
               BGAV_MK_FOURCC('w', 'h', 'a', 'm'),
               0x00 } },

    { "FFmpeg Creative YUV decoder", "Creative YUV", CODEC_ID_CYUV,
      (uint32_t[]){ BGAV_MK_FOURCC('C', 'Y', 'U', 'V'),
               0x00 } },

    { "FFmpeg MSRLE Decoder", "Microsoft RLE", CODEC_ID_MSRLE,
      (uint32_t[]){ BGAV_MK_FOURCC('W', 'R', 'L', 'E'),
               BGAV_MK_FOURCC('m', 'r', 'l', 'e'),
               BGAV_MK_FOURCC(0x1, 0x0, 0x0, 0x0),
               0x00 } },

    { "FFmpeg QT rle Decoder", "Quicktime RLE", CODEC_ID_QTRLE,
      (uint32_t[]){ BGAV_MK_FOURCC('r', 'l', 'e', ' '),
               0x00 } },

    { "FFmpeg FLI/FLC Decoder", "FLI/FLC Animation", CODEC_ID_FLIC,
      (uint32_t[]){ BGAV_MK_FOURCC('F', 'L', 'I', 'C'),
               0x00 } },
#if 0
    /************************************************************
     * H261 Variants
     ************************************************************/
    
    { "FFmpeg H261 decoder", "H261", CODEC_ID_H261,
      (uint32_t[]){ BGAV_MK_FOURCC('H', '2', '6', '1'),
               BGAV_MK_FOURCC('h', '2', '6', '1'),
               0x00 } },
#endif    
    /************************************************************
     * H263 Variants
     ************************************************************/
    
    { "FFmpeg H263 decoder", "H263", CODEC_ID_H263,
      (uint32_t[]){ BGAV_MK_FOURCC('H', '2', '6', '3'),
               BGAV_MK_FOURCC('h', '2', '6', '3'),
               BGAV_MK_FOURCC('s', '2', '6', '3'),
               BGAV_MK_FOURCC('u', '2', '6', '3'),
               BGAV_MK_FOURCC('U', '2', '6', '3'),
               BGAV_MK_FOURCC('v', 'i', 'v', '1'),
               0x00 } },
    
    { "FFmpeg H263I decoder", "I263", CODEC_ID_H263I,
      (uint32_t[]){ BGAV_MK_FOURCC('I', '2', '6', '3'), /* intel h263 */
               0x00 } },

    /************************************************************
     * MPEG-4 Variants
     ************************************************************/
        
    { "FFmpeg MPEG-4 decoder", "MPEG-4", CODEC_ID_MPEG4,
      (uint32_t[]){ BGAV_MK_FOURCC('D', 'I', 'V', 'X'),
               BGAV_MK_FOURCC('D', 'X', '5', '0'),
               BGAV_MK_FOURCC('X', 'V', 'I', 'D'),
               BGAV_MK_FOURCC('X', 'V', 'I', 'D'),
               BGAV_MK_FOURCC('M', 'P', '4', 'S'),
               BGAV_MK_FOURCC('M', '4', 'S', '2'),
               BGAV_MK_FOURCC(0x04, 0, 0, 0), /* some broken avi use this */
               BGAV_MK_FOURCC('D', 'I', 'V', '1'),
               BGAV_MK_FOURCC('B', 'L', 'Z', '0'),
               BGAV_MK_FOURCC('m', 'p', '4', 'v'),
               BGAV_MK_FOURCC('U', 'M', 'P', '4'),
               0x00 } },
    
    { "FFmpeg MSMPEG4V3 decoder", "Microsoft MPEG-4 V3", CODEC_ID_MSMPEG4V3,
      (uint32_t[]){ BGAV_MK_FOURCC('D', 'I', 'V', '3'),
               BGAV_MK_FOURCC('M', 'P', '4', '3'), 
               BGAV_MK_FOURCC('M', 'P', 'G', '3'), 
               BGAV_MK_FOURCC('D', 'I', 'V', '5'), 
               BGAV_MK_FOURCC('D', 'I', 'V', '6'), 
               BGAV_MK_FOURCC('D', 'I', 'V', '4'), 
               BGAV_MK_FOURCC('A', 'P', '4', '1'),
               BGAV_MK_FOURCC('C', 'O', 'L', '1'),
               BGAV_MK_FOURCC('C', 'O', 'L', '0'),
               0x00 } },
    
    { "FFmpeg MSMPEG4V2 decoder", "Microsoft MPEG-4 V2", CODEC_ID_MSMPEG4V2,
      (uint32_t[]){ BGAV_MK_FOURCC('M', 'P', '4', '2'),
               BGAV_MK_FOURCC('D', 'I', 'V', '2'),
               0x00 } },
    
    { "FFmpeg MSMPEG4V1 decoder", "Microsoft MPEG-4 V1", CODEC_ID_MSMPEG4V1,
      (uint32_t[]){ BGAV_MK_FOURCC('M', 'P', 'G', '4'),
               BGAV_MK_FOURCC('D', 'I', 'V', '4'),
               0x00 } },
    
    { "FFmpeg WMV1 decoder", "Window Media Video 7", CODEC_ID_WMV1,
      (uint32_t[]){ BGAV_MK_FOURCC('W', 'M', 'V', '1'),
               0x00 } }, 
    
    { "FFmpeg WMV2 decoder", "Window Media Video 8", CODEC_ID_WMV2,
      (uint32_t[]){ BGAV_MK_FOURCC('W', 'M', 'V', '2'),
               0x00 } }, 

    /************************************************************
     * DV Decoder
     ************************************************************/
        
    { "FFmpeg DV decoder", "DV Video", CODEC_ID_DVVIDEO,
      (uint32_t[]){ BGAV_MK_FOURCC('d', 'v', 's', 'd'), 
               BGAV_MK_FOURCC('D', 'V', 'S', 'D'), 
               BGAV_MK_FOURCC('d', 'v', 'h', 'd'), 
               BGAV_MK_FOURCC('d', 'v', 'c', ' '), 
               BGAV_MK_FOURCC('d', 'v', 'c', 'p'), 
               BGAV_MK_FOURCC('d', 'v', 'p', 'p'), 
               BGAV_MK_FOURCC('d', 'v', 's', 'l'), 
               BGAV_MK_FOURCC('d', 'v', '2', '5'),
               //               BGAV_MK_FOURCC('d', 'v', '5', 'n'),
               //               BGAV_MK_FOURCC('d', 'v', '5', 'p'),
               0x00 } },

    /*************************************************************
     * Motion JPEG Variants
     *************************************************************/

    { "FFmpeg motion Jpeg decoder", "Motion Jpeg", CODEC_ID_MJPEG,
      (uint32_t[]){ BGAV_MK_FOURCC('L', 'J', 'P', 'G'),
               BGAV_MK_FOURCC('A', 'V', 'R', 'n'),
               BGAV_MK_FOURCC('J', 'P', 'G', 'L'),
               BGAV_MK_FOURCC('j', 'p', 'e', 'g'),
               BGAV_MK_FOURCC('m', 'j', 'p', 'a'),
               BGAV_MK_FOURCC('A', 'V', 'D', 'J'),
               BGAV_MK_FOURCC('M', 'J', 'P', 'G'),
               0x00 } },

    { "FFmpeg motion Jpeg-B decoder", "Motion Jpeg B", CODEC_ID_MJPEGB,
      (uint32_t[]){ BGAV_MK_FOURCC('m', 'j', 'p', 'b'),
               0x00 } },
    
    { "FFmpeg Ljpeg decoder", "Lossless Motion Jpeg", CODEC_ID_LJPEG,
      (uint32_t[]){ BGAV_MK_FOURCC('L', 'J', 'P', 'G'),
               0x00 } },

    /*************************************************************
     * Proprietary Codecs
     *************************************************************/
        
    { "FFmpeg Real Video 1.0 decoder", "Real Video 1.0", CODEC_ID_RV10,
      (uint32_t[]){ BGAV_MK_FOURCC('R', 'V', '1', '0'),
               BGAV_MK_FOURCC('R', 'V', '1', '3'),
               0x00 } },
#if 1
    { "FFmpeg Real Video 2.0 decoder", "Real Video 2.0", CODEC_ID_RV20,
      (uint32_t[]){ BGAV_MK_FOURCC('R', 'V', '2', '0'),
               0x00 } },
#endif
    { "FFmpeg Sorenson 1 decoder", "Sorenson Video 1", CODEC_ID_SVQ1,
      (uint32_t[]){ BGAV_MK_FOURCC('S', 'V', 'Q', '1'),
               BGAV_MK_FOURCC('s', 'v', 'q', '1'),
               BGAV_MK_FOURCC('s', 'v', 'q', 'i'),
               0x00 } },
    
    { "FFmpeg Sorenson 3 decoder", "Sorenson Video 3", CODEC_ID_SVQ3,
      (uint32_t[]){ BGAV_MK_FOURCC('S', 'V', 'Q', '3'),
               0x00 } },

    
    /*************************************************************
     * Misc other stuff
     *************************************************************/
#if 0        
    { "FFmpeg Mpeg1 decoder", "MPEG-1", CODEC_ID_MPEG1VIDEO,
      (uint32_t[]){ BGAV_MK_FOURCC('m', 'p', 'g', '1'), 
               BGAV_MK_FOURCC('m', 'p', 'g', '2'),
               BGAV_MK_FOURCC('P', 'I', 'M', '1'), 
               BGAV_MK_FOURCC('V', 'C', 'R', '2'),
               0x00 } }, 
#endif    
    
    { "FFmpeg Hufyuv decoder", "Huff YUV", CODEC_ID_HUFFYUV,
      (uint32_t[]){ BGAV_MK_FOURCC('H', 'F', 'Y', 'U'),
               0x00 } },
    
    { "FFmpeg VP3 decoder", "On2 VP3", CODEC_ID_VP3,
      (uint32_t[]){ BGAV_MK_FOURCC('V', 'P', '3', '1'),
                    BGAV_MK_FOURCC('V', 'P', '3', ' '),
               0x00 } },

    { "FFmpeg ASV1 decoder", "Asus v1", CODEC_ID_ASV1,
      (uint32_t[]){ BGAV_MK_FOURCC('A', 'S', 'V', '1'),
               0x00 } },

    { "FFmpeg ASV2 decoder", "Asus v2", CODEC_ID_ASV2,
      (uint32_t[]){ BGAV_MK_FOURCC('A', 'S', 'V', '2'),
               0x00 } },

    { "FFmpeg VCR1 decoder", "ATI VCR1", CODEC_ID_VCR1,
      (uint32_t[]){ BGAV_MK_FOURCC('V', 'C', 'R', '1'),
               0x00 } },

    { "FFmpeg CLJR decoder", "Cirrus Logic AccuPak", CODEC_ID_CLJR,
      (uint32_t[]){ BGAV_MK_FOURCC('C', 'L', 'J', 'R'),
               0x00 } },

    { "FFmpeg TSCC decoder", "TechSmith Camtasia", CODEC_ID_TSCC,
      (uint32_t[]){ BGAV_MK_FOURCC('T', 'S', 'C', 'C'),
               BGAV_MK_FOURCC('t', 's', 'c', 'c'),
               0x00 } },
    
    { "FFmpeg FFV1 decoder", "FFmpeg Video 1", CODEC_ID_FFV1,
      (uint32_t[]){ BGAV_MK_FOURCC('F', 'F', 'V', '1'),
               0x00 } },

    { "FFmpeg Xxan decoder", "Xan/WC3", CODEC_ID_XAN_WC4,
      (uint32_t[]){ BGAV_MK_FOURCC('X', 'x', 'a', 'n'),
               0x00 } },

    { "FFmpeg QDraw decoder", "Apple QuickDraw", CODEC_ID_QDRAW,
      (uint32_t[]){ BGAV_MK_FOURCC('q', 'd', 'r', 'w'),
               0x00 } },

    { "FFmpeg ULTI decoder", "IBM Ultimotion", CODEC_ID_ULTI,
      (uint32_t[]){ BGAV_MK_FOURCC('U', 'L', 'T', 'I'),
               0x00 } },

    { "FFmpeg H264 decoder", "H264", CODEC_ID_H264,
      (uint32_t[]){ BGAV_MK_FOURCC('a', 'v', 'c', '1'),
               BGAV_MK_FOURCC('H', '2', '6', '4'),
               0x00 } },
  };

/* Pixel formats */

static struct
  {
  enum PixelFormat  ffmpeg_csp;
  gavl_pixelformat_t gavl_csp;
  } pixelformats[] =
  {
    { PIX_FMT_YUV420P,       GAVL_YUV_420_P },  ///< Planar YUV 4:2:0 (1 Cr & Cb sample per 2x2 Y samples)
    { PIX_FMT_YUV422,        GAVL_YUY2      },
    { PIX_FMT_RGB24,         GAVL_RGB_24    },  ///< Packed pixel, 3 bytes per pixel, RGBRGB...
    { PIX_FMT_BGR24,         GAVL_BGR_24    },  ///< Packed pixel, 3 bytes per pixel, BGRBGR...
    { PIX_FMT_YUV422P,       GAVL_YUV_422_P },  ///< Planar YUV 4:2:2 (1 Cr & Cb sample per 2x1 Y samples)
    { PIX_FMT_YUV444P,       GAVL_YUV_444_P }, ///< Planar YUV 4:4:4 (1 Cr & Cb sample per 1x1 Y samples)
    { PIX_FMT_RGBA32,        GAVL_RGBA_32   },  ///< Packed pixel, 4 bytes per pixel, BGRABGRA..., stored in cpu endianness
    { PIX_FMT_YUV410P,       GAVL_YUV_410_P }, ///< Planar YUV 4:1:0 (1 Cr & Cb sample per 4x4 Y samples)
    { PIX_FMT_YUV411P,       GAVL_YUV_411_P }, ///< Planar YUV 4:1:1 (1 Cr & Cb sample per 4x1 Y samples)
    { PIX_FMT_RGB565,        GAVL_RGB_16 }, ///< always stored in cpu endianness
    { PIX_FMT_RGB555,        GAVL_RGB_15 }, ///< always stored in cpu endianness, most significant bit to 1
    { PIX_FMT_GRAY8,         GAVL_PIXELFORMAT_NONE },
    { PIX_FMT_MONOWHITE,     GAVL_PIXELFORMAT_NONE }, ///< 0 is white
    { PIX_FMT_MONOBLACK,     GAVL_PIXELFORMAT_NONE }, ///< 0 is black
    { PIX_FMT_PAL8,          GAVL_RGB_24     }, ///< 8 bit with RGBA palette
    { PIX_FMT_YUVJ420P,      GAVL_YUVJ_420_P }, ///< Planar YUV 4:2:0 full scale (jpeg)
    { PIX_FMT_YUVJ422P,      GAVL_YUVJ_422_P }, ///< Planar YUV 4:2:2 full scale (jpeg)
    { PIX_FMT_YUVJ444P,      GAVL_YUVJ_444_P }, ///< Planar YUV 4:4:4 full scale (jpeg)
    { PIX_FMT_XVMC_MPEG2_MC, GAVL_PIXELFORMAT_NONE }, ///< XVideo Motion Acceleration via common packet passing(xvmc_render.h)
    { PIX_FMT_XVMC_MPEG2_IDCT, GAVL_PIXELFORMAT_NONE },
    { PIX_FMT_NB, GAVL_PIXELFORMAT_NONE }
};

static void pal8_to_rgb24(gavl_video_frame_t * dst, AVFrame * src,
                          int width, int height)
  {
  int i, j;
  uint32_t pixel;
  uint8_t * dst_ptr;
  uint8_t * dst_save;

  uint8_t * src_ptr;
  uint8_t * src_save;

  uint32_t * palette;

  palette = (uint32_t*)(src->data[1]);

  src_save = src->data[0];
  dst_save = dst->planes[0];
  for(i = 0; i < height; i++)
    {
    src_ptr = src_save;
    dst_ptr = dst_save;
    
    for(j = 0; j < width; j++)
      {
      pixel = palette[*src_ptr];
      dst_ptr[0] = (pixel & 0x00FF0000) >> 16;
      dst_ptr[1] = (pixel & 0x0000FF00) >> 8;
      dst_ptr[2] = (pixel & 0x000000FF);
      
      dst_ptr+=3;
      src_ptr++;
      }

    src_save += src->linesize[0];
    dst_save += dst->strides[0];
    }
  }

/* Real stupid rgba format conversion */

static void rgba32_to_rgba32(gavl_video_frame_t * dst, AVFrame * src,
                             int width, int height)
  {
  int i, j;
  uint32_t r, g, b, a;
  uint8_t * dst_ptr;
  uint8_t * dst_save;
  
  uint32_t * src_ptr;
  uint8_t  * src_save;
  
  src_save = src->data[0];
  dst_save = dst->planes[0];
  for(i = 0; i < height; i++)
    {
    src_ptr = (uint32_t*)src_save;
    dst_ptr = dst_save;
    
    for(j = 0; j < width; j++)
      {
      a = ((*src_ptr) & 0xff000000) >> 24;
      r = ((*src_ptr) & 0x00ff0000) >> 16;
      g = ((*src_ptr) & 0x0000ff00) >> 8;
      b = ((*src_ptr) & 0x000000ff);
      dst_ptr[0] = r;
      dst_ptr[1] = g;
      dst_ptr[2] = b;
      dst_ptr[3] = a;
      
      dst_ptr+=4;
      src_ptr++;
      }
    src_save += src->linesize[0];
    dst_save += dst->strides[0];
    }
  }

static gavl_pixelformat_t get_pixelformat(enum PixelFormat p)
  {
  int i;
  for(i = 0; i < sizeof(pixelformats)/sizeof(pixelformats[0]); i++)
    {
    if(pixelformats[i].ffmpeg_csp == p)
      return pixelformats[i].gavl_csp;
    }
  return GAVL_PIXELFORMAT_NONE;
  }


#define NUM_CODECS sizeof(codec_infos)/sizeof(codec_infos[0])

static int real_num_codecs;

static struct
  {
  codec_info_t * info;
  bgav_video_decoder_t decoder;
  } codecs[NUM_CODECS];

typedef struct
  {
  AVCodecContext * ctx;
  AVFrame * frame;
  //  enum CodecID ffmpeg_id;
  codec_info_t * info;
  gavl_video_frame_t * gavl_frame;
  uint8_t * packet_buffer;
  uint8_t * packet_buffer_ptr;
  int packet_buffer_alloc;
  int bytes_in_packet_buffer;
  int do_convert;
  enum PixelFormat dst_format;

  uint32_t rv_extradata[2+FF_INPUT_BUFFER_PADDING_SIZE/4];
  AVPaletteControl palette;

  int need_first_frame;
  int have_first_frame;

  uint8_t * extradata;
  int extradata_size;

  int64_t old_pts[FF_MAX_B_FRAMES+1];
  int num_old_pts;
  int delay;
  
  } ffmpeg_video_priv;

static codec_info_t * lookup_codec(bgav_stream_t * s)
  {
  int i;
  for(i = 0; i < real_num_codecs; i++)
    {
    if(s->data.video.decoder->decoder == &(codecs[i].decoder))
      return codecs[i].info;
    }
  return (codec_info_t *)0;
  }

/* This MUST match demux_rm.c!! */

typedef struct dp_hdr_s {
    uint32_t chunks;    // number of chunks
    uint32_t timestamp; // timestamp from packet header
    uint32_t len;       // length of actual data
    uint32_t chunktab;  // offset to chunk offset array
} dp_hdr_t;

// static int data_dumped = 0;

static int decode(bgav_stream_t * s, gavl_video_frame_t * f)
  {
  int i;
  int got_picture = 0;
  int bytes_used;
  int len;
  AVPicture ffmpeg_frame;
  dp_hdr_t *hdr;
  ffmpeg_video_priv * priv;
  bgav_packet_t * p;
#ifdef HAVE_LIBDV /* We get the DV format info from libdv, since the values
                     ffmpeg returns are not reliable */
  bgav_dv_dec_t * dvdec;
#endif  
  priv = (ffmpeg_video_priv*)(s->data.video.decoder->priv);
  
  
  //  fprintf(stderr, "Decode Video 1: %d\n", priv->bytes_in_packet_buffer);
  
  if(priv->have_first_frame)
    {
    got_picture = 1;
    priv->have_first_frame = 0;
    //    fprintf(stderr, "Have first frame %p\n", f);
    }
  
  while(!got_picture)
    {
    /* Read data if necessary */
    while(!priv->bytes_in_packet_buffer)
      {
      p = bgav_demuxer_get_packet_read(s->demuxer, s);
      if(!p)
        {
        priv->packet_buffer_ptr = (uint8_t*)0;
        //        fprintf(stderr, "EOF %d\n", priv->has_b_frames);
        break;
        }
      if(!p->data_size)
        s->position++;

      priv->old_pts[priv->num_old_pts] = p->timestamp_scaled;
      priv->num_old_pts++;
      
      if(p->data_size + FF_INPUT_BUFFER_PADDING_SIZE > priv->packet_buffer_alloc)
        {
        priv->packet_buffer_alloc = p->data_size + FF_INPUT_BUFFER_PADDING_SIZE + 32;
        priv->packet_buffer = realloc(priv->packet_buffer, priv->packet_buffer_alloc);
        }
      priv->bytes_in_packet_buffer = p->data_size;
      memcpy(priv->packet_buffer, p->data, p->data_size);
      priv->packet_buffer_ptr = priv->packet_buffer;
    
      memset(&(priv->packet_buffer[p->data_size]), 0, FF_INPUT_BUFFER_PADDING_SIZE);
      bgav_demuxer_done_packet_read(s->demuxer, p);
      //      fprintf(stderr, "Got packet, %d bytes\n", priv->bytes_in_packet_buffer);
      }
    len = priv->bytes_in_packet_buffer;

    //    fprintf(stderr, "%d %p\n", priv->have_last_frame, priv->packet_buffer_ptr);
    
    /* If we are out of data and the codec has no B-Frames, we are done */
    if(!priv->delay && !priv->packet_buffer_ptr)
      return 0;

    /* Other Real Video oddities */
    
    if((s->fourcc == BGAV_MK_FOURCC('R', 'V', '1', '0')) ||
       (s->fourcc == BGAV_MK_FOURCC('R', 'V', '1', '3')) ||
       (s->fourcc == BGAV_MK_FOURCC('R', 'V', '2', '0')) ||
       (s->fourcc == BGAV_MK_FOURCC('R', 'V', '3', '0')) ||
       (s->fourcc == BGAV_MK_FOURCC('R', 'V', '4', '0')))
      {
      if(priv->ctx->extradata_size == 8)
        {
        hdr= (dp_hdr_t*)(priv->packet_buffer_ptr);
        if(priv->ctx->slice_offset==NULL)
          priv->ctx->slice_offset= malloc(sizeof(int)*1000);
        priv->ctx->slice_count= hdr->chunks+1;
        for(i=0; i<priv->ctx->slice_count; i++)
          priv->ctx->slice_offset[i]= ((uint32_t*)(priv->packet_buffer_ptr+hdr->chunktab))[2*i+1];
        len=hdr->len;
        priv->packet_buffer_ptr += sizeof(dp_hdr_t);
        priv->bytes_in_packet_buffer = len;
        }
      }
    
    /* Decode one frame */

    //    fprintf(stderr, "Decode Video: %d %p\n", len, priv->packet_buffer_ptr);
    //    bgav_hexdump(priv->packet_buffer_ptr, 128, 16);
    
    if(!f && !priv->need_first_frame)
      priv->ctx->hurry_up = 1;
    else
      priv->ctx->hurry_up = 0;
#if 0
    fprintf(stderr, "Decode: %lld %d\n", s->position, len);
    //    bgav_hexdump(priv->packet_buffer_ptr, 16, 16);
#endif
    //    fprintf(stderr, "Decode %d...", priv->ctx->pix_fmt);

#ifdef HAVE_LIBDV
    if(priv->need_first_frame && (priv->info->ffmpeg_id == CODEC_ID_DVVIDEO))
      {
      dvdec = bgav_dv_dec_create();
      bgav_dv_dec_set_header(dvdec, priv->packet_buffer_ptr);
      bgav_dv_dec_set_frame(dvdec, priv->packet_buffer_ptr);

      bgav_dv_dec_get_pixel_aspect(dvdec, &s->data.video.format.pixel_width,
                                   &s->data.video.format.pixel_height);
      
      bgav_dv_dec_destroy(dvdec);
      }
#endif
    
    bytes_used = avcodec_decode_video(priv->ctx,
                                      priv->frame,
                                      &got_picture,
                                      priv->packet_buffer_ptr,
                                      len);

    //    fprintf(stderr, "%d\n", priv->ctx->pix_fmt);

    //    fprintf(stderr, "Sample aspect ratio: %d %d\n", priv->ctx->sample_aspect_ratio.num,
    //            priv->ctx->sample_aspect_ratio.den);

    //    fprintf(stderr, "Image size: %d %d\n", priv->ctx->width, priv->ctx->height);

#if 0
    fprintf(stderr, "Used %d/%d bytes, got picture: %d, delay: %d, old_pts: %d\n",
            bytes_used, len, got_picture, priv->delay, priv->num_old_pts);
#endif

    if(!got_picture)
      {
      /* Decoding delay */
      if(priv->delay == FF_MAX_B_FRAMES)
        {
        fprintf(stderr, "Decoding delay too large\n");
        return 0;
        }
      priv->old_pts[priv->delay] = p->timestamp_scaled;
      priv->delay++;
      }
    
#if 0
    if(!got_picture)
      {
      fprintf(stderr, "Didn't get picture: bytes: %d, bytes_used: %d, last_frame_time: %lld\n",
              len, bytes_used, s->data.video.last_frame_time);
      bgav_hexdump(priv->packet_buffer_ptr, 16, 16);
      
      }
#endif
    if(priv->packet_buffer_ptr)
      {
      /* Check for error */
      //      if((bytes_used <= 0) || !(got_picture))
      if((bytes_used <= 0) || !got_picture)
        {
        //      fprintf(stderr, "Skipping corrupted frame\n");
        //        priv->packet_buffer_ptr += len;
        priv->bytes_in_packet_buffer = 0;
        }
      /* Advance packet buffer */
      else
        {
        priv->packet_buffer_ptr += bytes_used;
        priv->bytes_in_packet_buffer -= bytes_used;
        if(priv->bytes_in_packet_buffer < 0)
          priv->bytes_in_packet_buffer = 0;
        }
      }

    }

  if(got_picture)
    {

    if(priv->need_first_frame)
      {
      //      fprintf(stderr, "First frame\n");
      
      s->data.video.format.pixelformat = get_pixelformat(priv->ctx->pix_fmt);
      priv->need_first_frame = 0;
      priv->have_first_frame = 1;
      
      if(priv->info->ffmpeg_id == CODEC_ID_DVVIDEO)
        {
        if(s->data.video.format.pixelformat == GAVL_YUV_420_P)
          s->data.video.format.chroma_placement = GAVL_CHROMA_PLACEMENT_DVPAL;

        if(!s->data.video.format.interlace_mode)
          s->data.video.format.interlace_mode = GAVL_INTERLACE_BOTTOM_FIRST;

        /* We completely ignore the frame size of the container */
        s->data.video.format.image_width = priv->ctx->width;
        s->data.video.format.frame_width = priv->ctx->width;
        
        s->data.video.format.image_height = priv->ctx->height;
        s->data.video.format.frame_height = priv->ctx->height;
        }
      else
        {
        if((priv->ctx->sample_aspect_ratio.num > 1) ||
           (priv->ctx->sample_aspect_ratio.den > 1))
          {
          s->data.video.format.pixel_width  = priv->ctx->sample_aspect_ratio.num;
          s->data.video.format.pixel_height = priv->ctx->sample_aspect_ratio.den;
          }
        /* Some demuxers don't know the frame dimensions */
        if(!s->data.video.format.image_width)
          {
          s->data.video.format.image_width = priv->ctx->width;
          s->data.video.format.frame_width = priv->ctx->width;

          s->data.video.format.image_height = priv->ctx->height;
          s->data.video.format.frame_height = priv->ctx->height;
          //        fprintf(stderr, "Got size: %d x %d\n", priv->ctx->width, priv->ctx->height);
          }
        /* Sometimes, the size encoded in some mp4 (vol?) headers is different from
           what is found in the container. In this case, the image must be scaled. */
        else if(s->data.video.format.image_width &&
                (priv->ctx->width < s->data.video.format.image_width))
          {
          s->data.video.format.pixel_width  = s->data.video.format.image_width;
          s->data.video.format.pixel_height = priv->ctx->width;
          s->data.video.format.image_width = priv->ctx->width;
          }
        
        }
      
      return 1;
      }

    if(f)
      {
      //      fprintf(stderr, "Got picture\n");
      //        fprintf(stderr, "Timestamp: %lld\n", timestamp);
      if(priv->ctx->pix_fmt == PIX_FMT_PAL8)
        {
        pal8_to_rgb24(f, priv->frame,
                      s->data.video.format.image_width, s->data.video.format.image_height);
        }
      else if(priv->ctx->pix_fmt == PIX_FMT_RGBA32)
        {
        rgba32_to_rgba32(f, priv->frame,
                         s->data.video.format.image_width, s->data.video.format.image_height);
        }
      else if(!priv->do_convert)
        {
        priv->gavl_frame->planes[0]  = priv->frame->data[0];
        priv->gavl_frame->planes[1]  = priv->frame->data[1];
        priv->gavl_frame->planes[2]  = priv->frame->data[2];
      
        priv->gavl_frame->strides[0] = priv->frame->linesize[0];
        priv->gavl_frame->strides[1] = priv->frame->linesize[1];
        priv->gavl_frame->strides[2] = priv->frame->linesize[2];
#if 0
        fprintf(stderr, "gavl_video_frame_copy %d %d %d %lld\n",
                priv->frame->linesize[0],
                priv->frame->linesize[1],
                priv->frame->linesize[2],
                s->position);
#endif   
        gavl_video_frame_copy(&(s->data.video.format), f, priv->gavl_frame);
        }
      else
        {
        ffmpeg_frame.data[0]     = f->planes[0];
        ffmpeg_frame.data[1]     = f->planes[1];
        ffmpeg_frame.data[2]     = f->planes[2];
        ffmpeg_frame.linesize[0] = f->strides[0];
        ffmpeg_frame.linesize[1] = f->strides[1];
        ffmpeg_frame.linesize[2] = f->strides[2];
        img_convert(&ffmpeg_frame, priv->dst_format,
                    (AVPicture*)(priv->frame), priv->ctx->pix_fmt,
                    s->data.video.format.image_width,
                    s->data.video.format.image_height);
        //          fprintf(stderr, "img_convert\n");
        }
      }

    /* Set correct stream timestams */
    if(priv->delay)
      {
      s->time_scaled = priv->old_pts[0];
      s->data.video.last_frame_time = priv->old_pts[0];
      if(priv->num_old_pts > 1)
        s->data.video.last_frame_duration = priv->old_pts[1] - priv->old_pts[0];
      }

    priv->num_old_pts--;
    if(priv->num_old_pts)
      memmove(&(priv->old_pts[0]), &(priv->old_pts[1]), sizeof(int64_t) * priv->num_old_pts);
    
    //      fprintf(stderr, "Decode %p %d\n", priv->packet_buffer_ptr, priv->have_last_frame);
    if(!priv->packet_buffer_ptr)
      {
      priv->delay--;
      }
    }
  else /* !got_picture */
    {
    //      fprintf(stderr, "Got no picture\n");
    if(!priv->packet_buffer_ptr)
      return 0; /* EOF */
    }

  return 1;
  }

static int init(bgav_stream_t * s)
  {
  int i, imax;
  AVCodec * codec;
  
  ffmpeg_video_priv * priv;
  priv = calloc(1, sizeof(*priv));
  priv->info = lookup_codec(s);
  codec = avcodec_find_decoder(priv->info->ffmpeg_id);
  priv->ctx = avcodec_alloc_context();
  priv->ctx->width = s->data.video.format.frame_width;
  priv->ctx->height = s->data.video.format.frame_height;
  priv->ctx->bits_per_sample = s->data.video.depth;

  if(s->ext_data)
    {
    priv->extradata = calloc(s->ext_size + FF_INPUT_BUFFER_PADDING_SIZE, 1);
    memcpy(priv->extradata, s->ext_data, s->ext_size);
    priv->extradata_size = s->ext_size;
    }
  
  priv->ctx->extradata      = priv->extradata;
  priv->ctx->extradata_size = priv->extradata_size;
  priv->ctx->codec_type = CODEC_TYPE_VIDEO;
  
  priv->ctx->bit_rate = 0;
#if LIBAVCODEC_BUILD >= 4754
  priv->ctx->time_base.den = s->data.video.format.timescale;
  priv->ctx->time_base.num = s->data.video.format.frame_duration;
#else
  priv->ctx->frame_rate = s->data.video.format.timescale;
  priv->ctx->frame_rate_base = s->data.video.format.frame_duration;
#endif
  /* Build the palette from the stream info */
  
  if(s->data.video.palette_size)
    {
    //    fprintf(stderr, "Building Palette...");
    priv->ctx->palctrl = &(priv->palette);
    priv->palette.palette_changed = 1;
    imax =
      (s->data.video.palette_size > AVPALETTE_COUNT)
      ? AVPALETTE_COUNT : s->data.video.palette_size;
    for(i = 0; i < imax; i++)
      {
      priv->palette.palette[i] =
        ((s->data.video.palette[i].a >> 8) << 24) |
        ((s->data.video.palette[i].r >> 8) << 16) |
        ((s->data.video.palette[i].g >> 8) << 8) |
        ((s->data.video.palette[i].b >> 8));

      }
    
    //    fprintf(stderr, "done %d colors\n", imax);
    }
  
  //  fprintf(stderr, "Setting extradata: %d bytes\n", s->ext_size);

  //  bgav_hexdump(s->ext_data, s->ext_size, 16);

  priv->ctx->codec_tag   =
    ((s->fourcc & 0x000000ff) << 24) |
    ((s->fourcc & 0x0000ff00) << 8) |
    ((s->fourcc & 0x00ff0000) >> 8) |
    ((s->fourcc & 0xff000000) >> 24);

  priv->ctx->codec_id =
    codec->id;
  //  fprintf(stderr, "Codec tag: %08x\n", priv->ctx->codec_tag);
  //  fprintf(stderr, "FF: %d %s\n", priv->ctx->extradata_size, priv->ctx->extradata);
  
  priv->frame = avcodec_alloc_frame();
  priv->gavl_frame = gavl_video_frame_create(NULL);
  
  /* Some codecs need extra stuff */

  /* Huffman tables for Motion jpeg */

  if(((s->fourcc == BGAV_MK_FOURCC('A','V','R','n')) ||
      (s->fourcc == BGAV_MK_FOURCC('M','J','P','G'))) &&
     priv->ctx->extradata_size)
    priv->ctx->flags |= CODEC_FLAG_EXTERN_HUFF;
  
  
  /* RealVideo oddities */

  if((s->fourcc == BGAV_MK_FOURCC('R','V','1','0')) ||
     (s->fourcc == BGAV_MK_FOURCC('R','V','1','3')) ||
     (s->fourcc == BGAV_MK_FOURCC('R','V','2','0')))
    {
    priv->ctx->extradata_size= 8;
    priv->ctx->extradata = priv->rv_extradata;
    
    if(priv->ctx->extradata_size != 8)
      {
      /* only 1 packet per frame & sub_id from fourcc */
      priv->rv_extradata[0] = 0;
      priv->rv_extradata[1] =
        (s->fourcc == BGAV_MK_FOURCC('R','V','1','3')) ? 0x10003001 :
        0x10000000;
      priv->ctx->sub_id = priv->rv_extradata[1];
      }
    else
      {
      /* has extra slice header (demux_rm or rm->avi streamcopy) */
      unsigned int* extrahdr=(unsigned int*)(s->ext_data);
      priv->rv_extradata[0] = be2me_32(extrahdr[0]);

      priv->ctx->sub_id = extrahdr[1];

      
      priv->rv_extradata[1] = be2me_32(extrahdr[1]);
      }

    //    fprintf(stderr, "subid: %08x\n", priv->ctx->sub_id);
    }

  if(codec->capabilities & CODEC_CAP_TRUNCATED)
    priv->ctx->flags &= ~CODEC_FLAG_TRUNCATED;
  priv->ctx->workaround_bugs = FF_BUG_AUTODETECT;
  priv->ctx->error_concealment = 3;
  
  //  priv->ctx->error_resilience = 3;
  
  /* Open this thing */
  
  if(avcodec_open(priv->ctx, codec) != 0)
    return 0;
  s->data.video.decoder->priv = priv;
  
  /* Set missing format values */

  //  fprintf(stderr, "Pixelformat: %s\n",
  //          avcodec_get_pix_fmt_name(priv->ctx->pix_fmt));

  priv->need_first_frame = 1;

  decode(s, NULL);
  
  /* TODO Handle unsupported colormodels */
  if(s->data.video.format.pixelformat == GAVL_PIXELFORMAT_NONE)
    {
    //    fprintf(stderr, "Unsupported pixel format %s\n",
    //            avcodec_get_pix_fmt_name(priv->ctx->pix_fmt));
    s->data.video.format.pixelformat = GAVL_YUV_420_P;
    priv->do_convert = 1;
    priv->dst_format = PIX_FMT_YUV420P;
    }
  s->description = bgav_sprintf("%s", priv->info->format_name);
  return 1;
  }



static void resync_ffmpeg(bgav_stream_t * s)
  {
  ffmpeg_video_priv * priv;
  priv = (ffmpeg_video_priv*)(s->data.video.decoder->priv);
  avcodec_flush_buffers(priv->ctx);
  priv->bytes_in_packet_buffer = 0;
  }

static void close_ffmpeg(bgav_stream_t * s)
  {
  ffmpeg_video_priv * priv;
  priv= (ffmpeg_video_priv*)(s->data.video.decoder->priv);

  if(priv->packet_buffer)
    free(priv->packet_buffer);
  
  if(priv->ctx)
    {
    avcodec_close(priv->ctx);
    free(priv->ctx);
    }
  if(priv->gavl_frame)
    {
    gavl_video_frame_null(priv->gavl_frame);
    gavl_video_frame_destroy(priv->gavl_frame);
    }
  if(priv->extradata)
    free(priv->extradata);
  
  free(priv->frame);
  free(priv);
  }

void bgav_init_video_decoders_ffmpeg()
  {
  int i;
  real_num_codecs = 0;
  for(i = 0; i < NUM_CODECS; i++)
    {
    if(avcodec_find_decoder(codec_infos[i].ffmpeg_id))
      {
      // fprintf(stderr, "Trying %s\n", codec_map_template[i].name);
      codecs[real_num_codecs].info = &(codec_infos[i]);
      codecs[real_num_codecs].decoder.name = codecs[real_num_codecs].info->decoder_name;
      codecs[real_num_codecs].decoder.fourccs = codecs[real_num_codecs].info->fourccs;
      codecs[real_num_codecs].decoder.init = init;
      codecs[real_num_codecs].decoder.decode = decode;
      codecs[real_num_codecs].decoder.close = close_ffmpeg;
      codecs[real_num_codecs].decoder.resync = resync_ffmpeg;
      //      fprintf(stderr, "Registering Codec %s\n", codecs[real_num_codecs].decoder.name);
      bgav_video_decoder_register(&codecs[real_num_codecs].decoder);
      real_num_codecs++;
      }
    }

  }

