/*****************************************************************
 
  audio_pcm.c
 
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
#include <stdio.h>
#include <string.h>
#include <codecs.h>

#include <bswap.h>

#define FRAME_SAMPLES 1024

typedef struct
  {
  void (*decode_func)(bgav_stream_t * s);
  gavl_audio_frame_t * frame;
  int last_frame_samples;

  bgav_packet_t * p;
  int             bytes_in_packet;
  uint8_t *       packet_ptr;

  int block_align;
  } pcm_t;

/* Decode functions */

static void decode_8(bgav_stream_t * s)
  {
  pcm_t * priv;
  int num_samples, num_bytes;
  priv = (pcm_t*)(s->data.audio.decoder->priv);

  num_samples = priv->bytes_in_packet / (s->data.audio.format.num_channels);
  
  if(num_samples > FRAME_SAMPLES)
    num_samples = FRAME_SAMPLES;

  num_bytes   = num_samples * s->data.audio.format.num_channels;

  memcpy(priv->frame->samples.u_8, priv->packet_ptr, num_bytes);

  priv->packet_ptr += num_bytes;
  priv->bytes_in_packet -= num_bytes;
  priv->frame->valid_samples = num_samples;
  }

static void decode_s_16(bgav_stream_t * s)
  {
  pcm_t * priv;
  int num_samples, num_bytes;
  priv = (pcm_t*)(s->data.audio.decoder->priv);

  num_samples = priv->bytes_in_packet / (2 * s->data.audio.format.num_channels);
  
  if(num_samples > FRAME_SAMPLES)
    num_samples = FRAME_SAMPLES;

  num_bytes   = num_samples * 2 * s->data.audio.format.num_channels;

  memcpy(priv->frame->samples.s_16, priv->packet_ptr, num_bytes);

  priv->packet_ptr += num_bytes;
  priv->bytes_in_packet -= num_bytes;
  priv->frame->valid_samples = num_samples;
  }

static void decode_s_16_swap(bgav_stream_t * s)
  {
  pcm_t * priv;
  int num_samples, num_bytes, i;
  int16_t * src, *dst;
  
  priv = (pcm_t*)(s->data.audio.decoder->priv);

  num_samples = priv->bytes_in_packet / (2 * s->data.audio.format.num_channels);

  //  fprintf(stderr, "SAMPLES: %d\n", num_samples);
  
  if(num_samples > FRAME_SAMPLES)
    num_samples = FRAME_SAMPLES;

  num_bytes   = num_samples * 2 * s->data.audio.format.num_channels;

  src = (int16_t*)priv->packet_ptr;
  dst = priv->frame->samples.s_16;

  i = num_samples * s->data.audio.format.num_channels;

  while(i--)
    {
    *dst = bswap_16(*src);
    src++;
    dst++;
    }
  
  priv->packet_ptr += num_bytes;
  priv->bytes_in_packet -= num_bytes;
  priv->frame->valid_samples = num_samples;
  
  
  }

static void decode_s_24_le(bgav_stream_t * s)
  {
  pcm_t * priv;
  int num_samples, num_bytes, i;
  priv = (pcm_t*)(s->data.audio.decoder->priv);
  uint8_t * src;
  uint32_t * dst;

  num_samples = priv->bytes_in_packet / (3 * s->data.audio.format.num_channels);

  if(num_samples > FRAME_SAMPLES)
    num_samples = FRAME_SAMPLES;

  num_bytes   = num_samples * 3 * s->data.audio.format.num_channels;

  src = priv->packet_ptr;
  dst = (uint32_t*)(priv->frame->samples.s_32);

  i = num_samples * s->data.audio.format.num_channels;
  
  while(i--)
    {
    *dst =
      ((uint32_t)(src[0]) << 8)  |
      ((uint32_t)(src[1]) << 16)  |
      ((uint32_t)(src[2]) << 24);
    src+=3;
    dst++;
    }
  priv->packet_ptr += num_bytes;
  priv->bytes_in_packet -= num_bytes;
  priv->frame->valid_samples = num_samples;
  
  }

static void decode_s_24_be(bgav_stream_t * s)
  {
  pcm_t * priv;
  int num_samples, num_bytes, i;
  priv = (pcm_t*)(s->data.audio.decoder->priv);
  uint8_t * src;
  uint32_t * dst;

  num_samples = priv->bytes_in_packet / (3 * s->data.audio.format.num_channels);

  if(num_samples > FRAME_SAMPLES)
    num_samples = FRAME_SAMPLES;

  num_bytes   = num_samples * 3 * s->data.audio.format.num_channels;

  src = priv->packet_ptr;
  dst = (uint32_t*)(priv->frame->samples.s_32);

  i = num_samples * s->data.audio.format.num_channels;
  
  while(i--)
    {
    *dst =
      ((uint32_t)(src[2]) << 8)  |
      ((uint32_t)(src[1]) << 16)  |
      ((uint32_t)(src[0]) << 24);
    src+=3;
    dst++;
    }
  priv->packet_ptr += num_bytes;
  priv->bytes_in_packet -= num_bytes;
  priv->frame->valid_samples = num_samples;
  }

static void decode_s_32(bgav_stream_t * s)
  {
  pcm_t * priv;
  int num_samples, num_bytes;
  priv = (pcm_t*)(s->data.audio.decoder->priv);

  num_samples = priv->bytes_in_packet / (4 * s->data.audio.format.num_channels);

  if(num_samples > FRAME_SAMPLES)
    num_samples = FRAME_SAMPLES;

  num_bytes   = num_samples * 4 * s->data.audio.format.num_channels;
  memcpy(priv->frame->samples.s_32, priv->packet_ptr, num_bytes);
  
  priv->packet_ptr += num_bytes;
  priv->bytes_in_packet -= num_bytes;
  priv->frame->valid_samples = num_samples;
  //  fprintf(stderr, "Decode %d %d\n", num_bytes, num_samples);
  
  }

/* Integer 32 bit */

static void decode_s_32_swap(bgav_stream_t * s)
  {

  pcm_t * priv;
  int num_samples, num_bytes, i;
  int32_t * src, *dst;
  
  priv = (pcm_t*)(s->data.audio.decoder->priv);

  num_samples = priv->bytes_in_packet / (4 * s->data.audio.format.num_channels);
  
  if(num_samples > FRAME_SAMPLES)
    num_samples = FRAME_SAMPLES;

  num_bytes   = num_samples * 4 * s->data.audio.format.num_channels;

  src = (int32_t*)priv->packet_ptr;
  dst = priv->frame->samples.s_32;

  i = num_samples * s->data.audio.format.num_channels;

  while(i--)
    {
    *dst = bswap_32(*src);
    src++;
    dst++;
    }
  
  priv->packet_ptr += num_bytes;
  priv->bytes_in_packet -= num_bytes;
  priv->frame->valid_samples = num_samples;
  
  }

/* Big/Little endian floating point routines taken from libsndfile */

static float
float32_be_read (unsigned char *cptr)
{       int             exponent, mantissa, negative ;
        float   fvalue ;

        negative = cptr [0] & 0x80 ;
        exponent = ((cptr [0] & 0x7F) << 1) | ((cptr [1] & 0x80) ? 1 : 0) ;
        mantissa = ((cptr [1] & 0x7F) << 16) | (cptr [2] << 8) | (cptr [3]) ;

        if (! (exponent || mantissa))
                return 0.0 ;

        mantissa |= 0x800000 ;
        exponent = exponent ? exponent - 127 : 0 ;

        fvalue = mantissa ? ((float) mantissa) / ((float) 0x800000) : 0.0 ;

        if (negative)
                fvalue *= -1 ;

        if (exponent > 0)
                fvalue *= (1 << exponent) ;
        else if (exponent < 0)
                fvalue /= (1 << abs (exponent)) ;

        return fvalue ;
} /* float32_be_read */

static float
float32_le_read (unsigned char *cptr)
{       int             exponent, mantissa, negative ;
        float   fvalue ;

        negative = cptr [3] & 0x80 ;
        exponent = ((cptr [3] & 0x7F) << 1) | ((cptr [2] & 0x80) ? 1 : 0) ;
        mantissa = ((cptr [2] & 0x7F) << 16) | (cptr [1] << 8) | (cptr [0]) ;

        if (! (exponent || mantissa))
                return 0.0 ;

        mantissa |= 0x800000 ;
        exponent = exponent ? exponent - 127 : 0 ;

        fvalue = mantissa ? ((float) mantissa) / ((float) 0x800000) : 0.0 ;

        if (negative)
                fvalue *= -1 ;

        if (exponent > 0)
                fvalue *= (1 << exponent) ;
        else if (exponent < 0)
                fvalue /= (1 << abs (exponent)) ;

        return fvalue ;
} /* float32_le_read */

static double
double64_be_read (unsigned char *cptr)
{       int             exponent, negative ;
        double  dvalue ;

        negative = (cptr [0] & 0x80) ? 1 : 0 ;
        exponent = ((cptr [0] & 0x7F) << 4) | ((cptr [1] >> 4) & 0xF) ;

        /* Might not have a 64 bit long, so load the mantissa into a double. */
        dvalue = (((cptr [1] & 0xF) << 24) | (cptr [2] << 16) | (cptr [3] << 8) | cptr [4]) ;
        dvalue += ((cptr [5] << 16) | (cptr [6] << 8) | cptr [7]) / ((double) 0x1000000) ;

        if (exponent == 0 && dvalue == 0.0)
                return 0.0 ;

        dvalue += 0x10000000 ;

        exponent = exponent - 0x3FF ;

        dvalue = dvalue / ((double) 0x10000000) ;

        if (negative)
                dvalue *= -1 ;

        if (exponent > 0)
                dvalue *= (1 << exponent) ;
        else if (exponent < 0)
                dvalue /= (1 << abs (exponent)) ;

        return dvalue ;
} /* double64_be_read */

static double
double64_le_read (unsigned char *cptr)
{       int             exponent, negative ;
        double  dvalue ;

        negative = (cptr [7] & 0x80) ? 1 : 0 ;
        exponent = ((cptr [7] & 0x7F) << 4) | ((cptr [6] >> 4) & 0xF) ;

        /* Might not have a 64 bit long, so load the mantissa into a double. */
        dvalue = (((cptr [6] & 0xF) << 24) | (cptr [5] << 16) | (cptr [4] << 8) | cptr [3]) ;
        dvalue += ((cptr [2] << 16) | (cptr [1] << 8) | cptr [0]) / ((double) 0x1000000) ;

        if (exponent == 0 && dvalue == 0.0)
                return 0.0 ;

        dvalue += 0x10000000 ;

        exponent = exponent - 0x3FF ;

        dvalue = dvalue / ((double) 0x10000000) ;

        if (negative)
                dvalue *= -1 ;

        if (exponent > 0)
                dvalue *= (1 << exponent) ;
        else if (exponent < 0)
                dvalue /= (1 << abs (exponent)) ;

        return dvalue ;
} /* double64_le_read */

/* Corrsponding decoding functions */

static void decode_float_32_be(bgav_stream_t * s)
  {
  pcm_t * priv;
  int num_samples, num_bytes, i;
  priv = (pcm_t*)(s->data.audio.decoder->priv);
  uint8_t * src;
  float * dst;

  num_samples = priv->bytes_in_packet / (4 * s->data.audio.format.num_channels);

  if(num_samples > FRAME_SAMPLES)
    num_samples = FRAME_SAMPLES;

  num_bytes   = num_samples * 4 * s->data.audio.format.num_channels;

  src = priv->packet_ptr;
  dst = (float*)(priv->frame->samples.f);

  i = num_samples * s->data.audio.format.num_channels;
  
  while(i--)
    {
    *dst = float32_be_read(src);
    src+=4;
    dst++;
    }
  priv->packet_ptr += num_bytes;
  priv->bytes_in_packet -= num_bytes;
  priv->frame->valid_samples = num_samples;
  }

static void decode_float_32_le(bgav_stream_t * s)
  {
  pcm_t * priv;
  int num_samples, num_bytes, i;
  priv = (pcm_t*)(s->data.audio.decoder->priv);
  uint8_t * src;
  float * dst;

  num_samples = priv->bytes_in_packet / (4 * s->data.audio.format.num_channels);

  if(num_samples > FRAME_SAMPLES)
    num_samples = FRAME_SAMPLES;

  num_bytes   = num_samples * 4 * s->data.audio.format.num_channels;

  src = priv->packet_ptr;
  dst = (float*)(priv->frame->samples.f);

  i = num_samples * s->data.audio.format.num_channels;
  
  while(i--)
    {
    *dst = float32_le_read(src);
    src+=4;
    dst++;
    }
  priv->packet_ptr += num_bytes;
  priv->bytes_in_packet -= num_bytes;
  priv->frame->valid_samples = num_samples;
  }

static void decode_float_64_be(bgav_stream_t * s)
  {
  pcm_t * priv;
  int num_samples, num_bytes, i;
  priv = (pcm_t*)(s->data.audio.decoder->priv);
  uint8_t * src;
  float * dst;

  num_samples = priv->bytes_in_packet / (8 * s->data.audio.format.num_channels);

  if(num_samples > FRAME_SAMPLES)
    num_samples = FRAME_SAMPLES;

  num_bytes   = num_samples * 8 * s->data.audio.format.num_channels;

  src = priv->packet_ptr;
  dst = (float*)(priv->frame->samples.f);

  i = num_samples * s->data.audio.format.num_channels;
  
  while(i--)
    {
    *dst = double64_be_read(src);
    src+=8;
    dst++;
    }
  priv->packet_ptr += num_bytes;
  priv->bytes_in_packet -= num_bytes;
  priv->frame->valid_samples = num_samples;
  }

static void decode_float_64_le(bgav_stream_t * s)
  {
  pcm_t * priv;
  int num_samples, num_bytes, i;
  priv = (pcm_t*)(s->data.audio.decoder->priv);
  uint8_t * src;
  float * dst;

  num_samples = priv->bytes_in_packet / (8 * s->data.audio.format.num_channels);

  if(num_samples > FRAME_SAMPLES)
    num_samples = FRAME_SAMPLES;

  num_bytes   = num_samples * 8 * s->data.audio.format.num_channels;

  src = priv->packet_ptr;
  dst = (float*)(priv->frame->samples.f);

  i = num_samples * s->data.audio.format.num_channels;
  
  while(i--)
    {
    *dst = double64_le_read(src);
    src+=8;
    dst++;
    }
  priv->packet_ptr += num_bytes;
  priv->bytes_in_packet -= num_bytes;
  priv->frame->valid_samples = num_samples;
  }

/* U-Law */

static short ulaw_decode [256] =
{	-32124,	-31100,	-30076,	-29052,	-28028,	-27004,	-25980,	-24956,
	-23932,	-22908,	-21884,	-20860,	-19836,	-18812,	-17788,	-16764,
	-15996,	-15484,	-14972,	-14460,	-13948,	-13436,	-12924,	-12412,
	-11900,	-11388,	-10876,	-10364,	-9852,	-9340,	-8828,	-8316,
	-7932,	-7676,	-7420,	-7164,	-6908,	-6652,	-6396,	-6140,
	-5884,	-5628,	-5372,	-5116,	-4860,	-4604,	-4348,	-4092,
	-3900,	-3772,	-3644,	-3516,	-3388,	-3260,	-3132,	-3004,
	-2876,	-2748,	-2620,	-2492,	-2364,	-2236,	-2108,	-1980,
	-1884,	-1820,	-1756,	-1692,	-1628,	-1564,	-1500,	-1436,
	-1372,	-1308,	-1244,	-1180,	-1116,	-1052,	-988,	-924,
	-876,	-844,	-812,	-780,	-748,	-716,	-684,	-652,
	-620,	-588,	-556,	-524,	-492,	-460,	-428,	-396,
	-372,	-356,	-340,	-324,	-308,	-292,	-276,	-260,
	-244,	-228,	-212,	-196,	-180,	-164,	-148,	-132,
	-120,	-112,	-104,	-96,	-88,	-80,	-72,	-64,
	-56,	-48,	-40,	-32,	-24,	-16,	-8,		0,

	32124,	31100,	30076,	29052,	28028,	27004,	25980,	24956,
	23932,	22908,	21884,	20860,	19836,	18812,	17788,	16764,
	15996,	15484,	14972,	14460,	13948,	13436,	12924,	12412,
	11900,	11388,	10876,	10364,	9852,	9340,	8828,	8316,
	7932,	7676,	7420,	7164,	6908,	6652,	6396,	6140,
	5884,	5628,	5372,	5116,	4860,	4604,	4348,	4092,
	3900,	3772,	3644,	3516,	3388,	3260,	3132,	3004,
	2876,	2748,	2620,	2492,	2364,	2236,	2108,	1980,
	1884,	1820,	1756,	1692,	1628,	1564,	1500,	1436,
	1372,	1308,	1244,	1180,	1116,	1052,	988,	924,
	876,	844,	812,	780,	748,	716,	684,	652,
	620,	588,	556,	524,	492,	460,	428,	396,
	372,	356,	340,	324,	308,	292,	276,	260,
	244,	228,	212,	196,	180,	164,	148,	132,
	120,	112,	104,	96,		88,		80,		72,		64,
	56,		48,		40,		32,		24,		16,		8,		0
} ;

static void decode_ulaw(bgav_stream_t * s)
  {
  pcm_t * priv;
  int num_samples, num_bytes, i;
  priv = (pcm_t*)(s->data.audio.decoder->priv);
  uint8_t * src;
  int16_t * dst;

  num_samples = priv->bytes_in_packet / (s->data.audio.format.num_channels);

  if(num_samples > FRAME_SAMPLES)
    num_samples = FRAME_SAMPLES;

  num_bytes   = num_samples * s->data.audio.format.num_channels;

  src = priv->packet_ptr;
  dst = (int16_t*)(priv->frame->samples.s_16);

  i = num_samples * s->data.audio.format.num_channels;
  
  while(i--)
    {
    *dst = ulaw_decode[*src];
    src++;
    dst++;
    }
  priv->packet_ptr += num_bytes;
  priv->bytes_in_packet -= num_bytes;
  priv->frame->valid_samples = num_samples;
  }


/* A-Law */

static
short alaw_decode [256] =
{	-5504,	-5248,	-6016,	-5760,	-4480,	-4224,	-4992,	-4736,
	-7552,	-7296,	-8064,	-7808,	-6528,	-6272,	-7040,	-6784,
	-2752,	-2624,	-3008,	-2880,	-2240,	-2112,	-2496,	-2368,
	-3776,	-3648,	-4032,	-3904,	-3264,	-3136,	-3520,	-3392,
	-22016,	-20992,	-24064,	-23040,	-17920,	-16896,	-19968,	-18944,
	-30208,	-29184,	-32256,	-31232,	-26112,	-25088,	-28160,	-27136,
	-11008,	-10496,	-12032,	-11520,	-8960,	-8448,	-9984,	-9472,
	-15104,	-14592,	-16128,	-15616,	-13056,	-12544,	-14080,	-13568,
	-344,	-328,	-376,	-360,	-280,	-264,	-312,	-296,
	-472,	-456,	-504,	-488,	-408,	-392,	-440,	-424,
	-88,	-72,	-120,	-104,	-24,	-8,		-56,	-40,
	-216,	-200,	-248,	-232,	-152,	-136,	-184,	-168,
	-1376,	-1312,	-1504,	-1440,	-1120,	-1056,	-1248,	-1184,
	-1888,	-1824,	-2016,	-1952,	-1632,	-1568,	-1760,	-1696,
	-688,	-656,	-752,	-720,	-560,	-528,	-624,	-592,
	-944,	-912,	-1008,	-976,	-816,	-784,	-880,	-848,
	5504,	5248,	6016,	5760,	4480,	4224,	4992,	4736,
	7552,	7296,	8064,	7808,	6528,	6272,	7040,	6784,
	2752,	2624,	3008,	2880,	2240,	2112,	2496,	2368,
	3776,	3648,	4032,	3904,	3264,	3136,	3520,	3392,
	22016,	20992,	24064,	23040,	17920,	16896,	19968,	18944,
	30208,	29184,	32256,	31232,	26112,	25088,	28160,	27136,
	11008,	10496,	12032,	11520,	8960,	8448,	9984,	9472,
	15104,	14592,	16128,	15616,	13056,	12544,	14080,	13568,
	344,	328,	376,	360,	280,	264,	312,	296,
	472,	456,	504,	488,	408,	392,	440,	424,
	88,		72,		120,	104,	24,		8,		56,		40,
	216,	200,	248,	232,	152,	136,	184,	168,
	1376,	1312,	1504,	1440,	1120,	1056,	1248,	1184,
	1888,	1824,	2016,	1952,	1632,	1568,	1760,	1696,
	688,	656,	752,	720,	560,	528,	624,	592,
	944,	912,	1008,	976,	816,	784,	880,	848
} ; /* alaw_decode */

static void decode_alaw(bgav_stream_t * s)
  {
  pcm_t * priv;
  int num_samples, num_bytes, i;
  priv = (pcm_t*)(s->data.audio.decoder->priv);
  uint8_t * src;
  int16_t * dst;

  num_samples = priv->bytes_in_packet / (s->data.audio.format.num_channels);

  if(num_samples > FRAME_SAMPLES)
    num_samples = FRAME_SAMPLES;

  num_bytes   = num_samples * s->data.audio.format.num_channels;

  src = priv->packet_ptr;
  dst = (int16_t*)(priv->frame->samples.s_16);
  
  i = num_samples * s->data.audio.format.num_channels;
  
  while(i--)
    {
    *dst = alaw_decode[*src];
    src++;
    dst++;
    }
  priv->packet_ptr += num_bytes;
  priv->bytes_in_packet -= num_bytes;
  priv->frame->valid_samples = num_samples;
  }


static int init_pcm(bgav_stream_t * s)
  {
  pcm_t * priv;
  priv = calloc(1, sizeof(*priv));
  s->data.audio.decoder->priv = priv;

  switch(s->fourcc)
    {
    /* Big endian */
    case BGAV_MK_FOURCC('t', 'w', 'o', 's'):
    case BGAV_MK_FOURCC('a', 'i', 'f', 'f'):
      if(s->data.audio.bits_per_sample <= 8)
        {
        s->description = bgav_sprintf("%d bit PCM", s->data.audio.bits_per_sample);
        s->data.audio.format.sample_format = GAVL_SAMPLE_S8;
        priv->decode_func = decode_8;
        }
      else if(s->data.audio.bits_per_sample <= 16)
        {
        s->description = bgav_sprintf("%d bit PCM (big endian)", s->data.audio.bits_per_sample);
        s->data.audio.format.sample_format = GAVL_SAMPLE_S16;
#ifdef GAVL_PROCESSOR_LITTLE_ENDIAN
        priv->decode_func = decode_s_16_swap;
#else
        priv->decode_func = decode_s_16;
#endif
        }
      else if(s->data.audio.bits_per_sample <= 24)
        {
        s->description = bgav_sprintf("%d bit PCM (big endian)", s->data.audio.bits_per_sample);
        s->data.audio.format.sample_format = GAVL_SAMPLE_S32;
        priv->decode_func = decode_s_24_be;
        }
      else if(s->data.audio.bits_per_sample <= 32)
        {
        s->description = bgav_sprintf("%d bit PCM (big endian)", s->data.audio.bits_per_sample);
        s->data.audio.format.sample_format = GAVL_SAMPLE_S32;
#ifdef GAVL_PROCESSOR_LITTLE_ENDIAN
        priv->decode_func = decode_s_32_swap;
#else
        priv->decode_func = decode_s_32;
#endif
        }
      else
        {
        fprintf(stderr, "Audio bits %d not supported!!\n",
                s->data.audio.bits_per_sample);
        return 0;
        }
      break;
    /* Little endian */
    case BGAV_WAVID_2_FOURCC(0x01):
      if(s->data.audio.bits_per_sample <= 8)
        {
        s->description = bgav_sprintf("%d bit PCM", s->data.audio.bits_per_sample);
        s->data.audio.format.sample_format = GAVL_SAMPLE_U8;
        priv->decode_func = decode_8;
        }
      else if(s->data.audio.bits_per_sample <= 16)
        {
        s->description = bgav_sprintf("%d bit PCM (little endian)", s->data.audio.bits_per_sample);
        s->data.audio.format.sample_format = GAVL_SAMPLE_S16;
#ifdef GAVL_PROCESSOR_LITTLE_ENDIAN
        priv->decode_func = decode_s_16;
#else
        priv->decode_func = decode_s_16_swap;
#endif
        }
      else if(s->data.audio.bits_per_sample <= 24)
        {
        s->description = bgav_sprintf("%d bit PCM (little endian)", s->data.audio.bits_per_sample);
        s->data.audio.format.sample_format = GAVL_SAMPLE_S32;
        priv->decode_func = decode_s_24_le;
        }
      else if(s->data.audio.bits_per_sample <= 32)
        {
        s->description = bgav_sprintf("%d bit PCM (little endian)", s->data.audio.bits_per_sample);
        s->data.audio.format.sample_format = GAVL_SAMPLE_S32;
#ifdef GAVL_PROCESSOR_LITTLE_ENDIAN
        priv->decode_func = decode_s_32;
#else
        priv->decode_func = decode_s_32_swap;
#endif
        }
      break;
    case BGAV_MK_FOURCC('r', 'a', 'w', ' '):
    case BGAV_MK_FOURCC('s', 'o', 'w', 't'):
      switch(s->data.audio.bits_per_sample)
        {
        case 8:
          s->description = bgav_sprintf("%d bit PCM", s->data.audio.bits_per_sample);
          if(s->fourcc == BGAV_MK_FOURCC('s', 'o', 'w', 't'))
            s->data.audio.format.sample_format = GAVL_SAMPLE_S8;
          else
            s->data.audio.format.sample_format = GAVL_SAMPLE_U8;
          priv->decode_func = decode_8;
          break;
        case 16:
          s->description = bgav_sprintf("%d bit PCM (little endian)", s->data.audio.bits_per_sample);
          s->data.audio.format.sample_format = GAVL_SAMPLE_S16;
#ifdef GAVL_PROCESSOR_LITTLE_ENDIAN
          priv->decode_func = decode_s_16;
#else
          priv->decode_func = decode_s_16_swap;
#endif
          break;
        case 24:
          s->description = bgav_sprintf("%d bit PCM (little endian)", s->data.audio.bits_per_sample);
          s->data.audio.format.sample_format = GAVL_SAMPLE_S32;
          priv->decode_func = decode_s_24_le;
          break;
        case 32:
          s->description = bgav_sprintf("%d bit PCM (little endian)", s->data.audio.bits_per_sample);
          s->data.audio.format.sample_format = GAVL_SAMPLE_S32;
#ifdef GAVL_PROCESSOR_LITTLE_ENDIAN
          priv->decode_func = decode_s_32;
#else
          priv->decode_func = decode_s_32_swap;
#endif
          break;
        default:
          fprintf(stderr, "Audio bits %d not supported!!\n",
                  s->data.audio.bits_per_sample);
          return 0;
        }
      break;
    case BGAV_MK_FOURCC('l', 'p', 'c', 'm'):
      switch(s->data.audio.bits_per_sample)
        {
        case 16:
          s->data.audio.format.sample_format = GAVL_SAMPLE_S16;
#ifdef GAVL_PROCESSOR_LITTLE_ENDIAN
          priv->decode_func = decode_s_16_swap;
#else
          priv->decode_func = decode_s_16;
#endif
          break;
        default:
          fprintf(stderr, "Error: %d bit lpcm not supported\n", s->data.audio.bits_per_sample);
          return 0;
        }
     
      break;
      /* Quicktime 24/32 bit, can be either big or little endian */
    case BGAV_MK_FOURCC('i', 'n', '2', '4'):
      if(s->data.audio.endianess == BGAV_ENDIANESS_LITTLE)
        priv->decode_func = decode_s_24_le;
      else
        priv->decode_func = decode_s_24_be;
      
      s->description = bgav_sprintf("24 bit PCM (%s endian)",
                                    ((s->data.audio.endianess == BGAV_ENDIANESS_LITTLE) ? "little" : "big" ));
      s->data.audio.format.sample_format = GAVL_SAMPLE_S32;
      priv->block_align = s->data.audio.format.num_channels * 3;
      break;
    case BGAV_MK_FOURCC('i', 'n', '3', '2'):
      if(s->data.audio.endianess == BGAV_ENDIANESS_LITTLE)
        {
#ifdef GAVL_PROCESSOR_LITTLE_ENDIAN
        priv->decode_func = decode_s_32;
#else
        priv->decode_func = decode_s_32_swap;
#endif
        }
      else
        {
#ifdef GAVL_PROCESSOR_LITTLE_ENDIAN
        priv->decode_func = decode_s_32_swap;
#else
        priv->decode_func = decode_s_32;
#endif
        }
      
      s->description =
        bgav_sprintf("32 bit PCM (%s endian)",
                     ((s->data.audio.endianess == BGAV_ENDIANESS_LITTLE) ? "little" : "big" ));
      s->data.audio.format.sample_format = GAVL_SAMPLE_S32;
      priv->block_align = s->data.audio.format.num_channels * 4;
      break;
      /* Floating point formats */
    case BGAV_WAVID_2_FOURCC(0x0003):
      switch(s->data.audio.bits_per_sample)
        {
        case 32:
          priv->decode_func = decode_float_32_le;
          s->data.audio.format.sample_format = GAVL_SAMPLE_FLOAT;
          s->description = bgav_sprintf("Float 32 bit little endian");
          break;
        case 64:
          priv->decode_func = decode_float_64_le;
          s->data.audio.format.sample_format = GAVL_SAMPLE_FLOAT;
          s->description = bgav_sprintf("Float 64 bit little endian");
          break;
        default:
          return 0;
        }
      
      break;
    case BGAV_MK_FOURCC('f', 'l', '3', '2'):
      if(s->data.audio.endianess == BGAV_ENDIANESS_LITTLE)
        priv->decode_func = decode_float_32_le;
      else
        priv->decode_func = decode_float_32_be;
      s->data.audio.format.sample_format = GAVL_SAMPLE_FLOAT;
      s->description =
        bgav_sprintf("Float 32 bit %s endian",
                     ((s->data.audio.endianess == BGAV_ENDIANESS_LITTLE) ? "little" : "big" ));
      priv->block_align = s->data.audio.format.num_channels * 4;
      break;
    case BGAV_MK_FOURCC('f', 'l', '6', '4'):
      if(s->data.audio.endianess == BGAV_ENDIANESS_LITTLE)
        priv->decode_func = decode_float_64_le;
      else
        priv->decode_func = decode_float_64_be;
      s->data.audio.format.sample_format = GAVL_SAMPLE_FLOAT;
      s->description = bgav_sprintf("Float 64 bit big endian");
      priv->block_align = s->data.audio.format.num_channels * 8;
      break;
    case BGAV_MK_FOURCC('u', 'l', 'a', 'w'):
    case BGAV_MK_FOURCC('U', 'L', 'A', 'W'):
    case BGAV_WAVID_2_FOURCC(0x07):
      priv->decode_func = decode_ulaw;
      s->description = bgav_sprintf("u-Law 2:1");
      s->data.audio.format.sample_format = GAVL_SAMPLE_S16;
      priv->block_align = s->data.audio.format.num_channels;
      break;
    case BGAV_MK_FOURCC('a', 'l', 'a', 'w'):
    case BGAV_MK_FOURCC('A', 'L', 'A', 'W'):
    case BGAV_WAVID_2_FOURCC(0x06):
      priv->decode_func = decode_alaw;
      s->description = bgav_sprintf("a-Law 2:1");
      s->data.audio.format.sample_format = GAVL_SAMPLE_S16;
      priv->block_align = s->data.audio.format.num_channels;
      break;
    default:
      fprintf(stderr, "Unknown fourcc\n");
      return 0;
    }
  s->data.audio.format.interleave_mode = GAVL_INTERLEAVE_ALL;
  s->data.audio.format.samples_per_frame = FRAME_SAMPLES;
  gavl_set_channel_setup(&(s->data.audio.format));
  
  priv->frame = gavl_audio_frame_create(&s->data.audio.format);
  if(!priv->block_align)
    priv->block_align = s->data.audio.format.num_channels * ((s->data.audio.bits_per_sample+7)/8);
  
  return 1;
  }

static int decode_pcm(bgav_stream_t * s,
                      gavl_audio_frame_t * frame,
                      int num_samples)
  {
  pcm_t * priv;
  int samples_decoded;
  int samples_copied;
  
  priv = (pcm_t*)(s->data.audio.decoder->priv);
  samples_decoded = 0;
  while(samples_decoded <  num_samples)
    {
    if(!priv->frame->valid_samples)
      {
      if(!priv->p)      
        {
        priv->p = bgav_demuxer_get_packet_read(s->demuxer, s);

        /* EOF */
        
        if(!priv->p)
          {
          //          fprintf(stderr, "Reached EOF\n");
          break;
          }
        priv->bytes_in_packet = priv->p->data_size;

        //        fprintf(stderr, "Got packet %d samples, %d bytes\n", 
        
        if(priv->p->samples && (priv->p->samples * priv->block_align < priv->bytes_in_packet))
          priv->bytes_in_packet = priv->p->samples * priv->block_align;
        priv->packet_ptr = priv->p->data;
        }
      
      /* Decode stuff */

      priv->decode_func(s);

            
      
      priv->last_frame_samples = priv->frame->valid_samples;
      
      if(!priv->bytes_in_packet)
        {
        bgav_demuxer_done_packet_read(s->demuxer, priv->p);
        priv->p = (bgav_packet_t*)0;
        }
      if(!priv->last_frame_samples)
        break;
      }
    samples_copied =
      gavl_audio_frame_copy(&(s->data.audio.format),
                            frame,       /* dst */
                            priv->frame, /* src */
                            samples_decoded, /* int dst_pos */
                            priv->last_frame_samples - priv->frame->valid_samples, /* int src_pos */
                            num_samples - samples_decoded, /* int dst_size, */
                            priv->frame->valid_samples /* int src_size*/ );
    priv->frame->valid_samples -= samples_copied;
    samples_decoded += samples_copied;
    }
  if(frame)
    frame->valid_samples = samples_decoded;
  return samples_decoded;
  }

static void close_pcm(bgav_stream_t * s)
  {
  pcm_t * priv;
  priv = (pcm_t*)(s->data.audio.decoder->priv);

  gavl_audio_frame_destroy(priv->frame);
  free(priv);
  }

static void resync_pcm(bgav_stream_t * s)
  {
  pcm_t * priv;
  priv = (pcm_t*)(s->data.audio.decoder->priv);
  priv->frame->valid_samples = 0;
  }

static bgav_audio_decoder_t decoder =
  {
    fourccs: (uint32_t[]){ BGAV_WAVID_2_FOURCC(0x0001),
                           BGAV_WAVID_2_FOURCC(0x0003),
                           BGAV_MK_FOURCC('a', 'i', 'f', 'f'),
                           BGAV_MK_FOURCC('t', 'w', 'o', 's'),
                           BGAV_MK_FOURCC('s', 'o', 'w', 't'),
                           BGAV_MK_FOURCC('r', 'a', 'w', ' '),
                           BGAV_MK_FOURCC('l', 'p', 'c', 'm'),
                           BGAV_MK_FOURCC('f', 'l', '3', '2'),
                           BGAV_MK_FOURCC('f', 'l', '6', '4'),
                           BGAV_MK_FOURCC('i', 'n', '2', '4'),
                           BGAV_MK_FOURCC('i', 'n', '3', '2'),
                           BGAV_MK_FOURCC('u', 'l', 'a', 'w'),
                           BGAV_MK_FOURCC('U', 'L', 'A', 'W'),
                           BGAV_WAVID_2_FOURCC(0x07),
                           BGAV_MK_FOURCC('a', 'l', 'a', 'w'),
                           BGAV_MK_FOURCC('A', 'L', 'A', 'W'),
                           BGAV_WAVID_2_FOURCC(0x06),
                           0x00 },
    name: "PCM audio decoder",
    init: init_pcm,
    close: close_pcm,
    resync: resync_pcm,
    decode: decode_pcm
  };

void bgav_init_audio_decoders_pcm()
  {
  bgav_audio_decoder_register(&decoder);
  }
