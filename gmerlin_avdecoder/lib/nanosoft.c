/*****************************************************************
 
  nanosoft.c
 
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <avdec_private.h>
#include <nanosoft.h>

static uint32_t swap_endian(uint32_t val)
  {
  return ((val & 0x000000FF) << 24) |
    ((val & 0x0000FF00) << 8) |
    ((val & 0x00FF0000) >> 8) |
    ((val & 0xFF000000) >> 24);
  }

/* Speaker configurations for WAVEFORMATEXTENSIBLE */

#define SPEAKER_FRONT_LEFT 	        0x1
#define SPEAKER_FRONT_RIGHT 	        0x2
#define SPEAKER_FRONT_CENTER 	        0x4
#define SPEAKER_LOW_FREQUENCY 	        0x8
#define SPEAKER_BACK_LEFT 	        0x10
#define SPEAKER_BACK_RIGHT 	        0x20
#define SPEAKER_FRONT_LEFT_OF_CENTER 	0x40
#define SPEAKER_FRONT_RIGHT_OF_CENTER 	0x80
#define SPEAKER_BACK_CENTER 	        0x100
#define SPEAKER_SIDE_LEFT 	        0x200
#define SPEAKER_SIDE_RIGHT 	        0x400
#define SPEAKER_TOP_CENTER 	        0x800
#define SPEAKER_TOP_FRONT_LEFT 	        0x1000
#define SPEAKER_TOP_FRONT_CENTER 	0x2000
#define SPEAKER_TOP_FRONT_RIGHT 	0x4000
#define SPEAKER_TOP_BACK_LEFT 	        0x8000
#define SPEAKER_TOP_BACK_CENTER 	0x10000
#define SPEAKER_TOP_BACK_RIGHT 	        0x20000

struct
  {
  uint32_t channel_mask;

  int num_channels;
  gavl_channel_setup_t channel_setup;
  gavl_channel_id_t channel_locations[GAVL_MAX_CHANNELS];
  int lfe;
  }
channel_setups[] =
  {
    /* Without lfe */
    {
      channel_mask:      SPEAKER_FRONT_CENTER,
      num_channels:      1,
      channel_setup:     GAVL_CHANNEL_MONO,
      channel_locations: { GAVL_CHID_FRONT },
      lfe:               0
    },
    {
      channel_mask:      SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT,
      num_channels:      2,
      channel_setup:     GAVL_CHANNEL_STEREO,
      channel_locations: { GAVL_CHID_FRONT_LEFT, GAVL_CHID_FRONT_RIGHT },
      lfe:               0
    },
    {
      channel_mask:      SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT | SPEAKER_FRONT_CENTER,
      num_channels:      3,
      channel_setup:     GAVL_CHANNEL_3F,
      channel_locations: { GAVL_CHID_FRONT_LEFT, GAVL_CHID_FRONT_RIGHT, GAVL_CHID_FRONT_CENTER },
      lfe:               0
    },
    {
      channel_mask:      SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT | SPEAKER_BACK_CENTER,
      num_channels:      3,
      channel_setup:     GAVL_CHANNEL_2F1R,
      channel_locations: { GAVL_CHID_FRONT_LEFT, GAVL_CHID_FRONT_RIGHT, GAVL_CHID_REAR },
      lfe:               0
    },
    {
      channel_mask:      SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT | SPEAKER_FRONT_CENTER | SPEAKER_BACK_CENTER,
      num_channels:      4,
      channel_setup:     GAVL_CHANNEL_3F1R,
      channel_locations: { GAVL_CHID_FRONT_LEFT, GAVL_CHID_FRONT_RIGHT, GAVL_CHID_FRONT_CENTER,  GAVL_CHID_REAR },
      lfe:               0
    },
    {
      channel_mask:      SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT | SPEAKER_BACK_LEFT | SPEAKER_BACK_RIGHT,
      num_channels:      4,
      channel_setup:     GAVL_CHANNEL_2F2R,
      channel_locations: { GAVL_CHID_FRONT_LEFT, GAVL_CHID_FRONT_RIGHT, GAVL_CHID_REAR_LEFT, GAVL_CHID_REAR_RIGHT },
      lfe:               0
    },
    {
      channel_mask:      SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT | SPEAKER_FRONT_CENTER| SPEAKER_BACK_LEFT | SPEAKER_BACK_RIGHT,
      num_channels:      5,
      channel_setup:     GAVL_CHANNEL_3F2R,
      channel_locations: { GAVL_CHID_FRONT_LEFT, SPEAKER_FRONT_RIGHT, SPEAKER_FRONT_CENTER },
      lfe:               0
    },
    /* With LFE */
    {
      channel_mask:      SPEAKER_FRONT_CENTER | SPEAKER_LOW_FREQUENCY,
      num_channels:      2,
      channel_setup:     GAVL_CHANNEL_MONO,
      channel_locations: { GAVL_CHID_FRONT, GAVL_CHID_LFE },
      lfe:               1
    },
    {
      channel_mask:      SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT | SPEAKER_LOW_FREQUENCY,
      num_channels:      3,
      channel_setup:     GAVL_CHANNEL_STEREO,
      channel_locations: { GAVL_CHID_FRONT_LEFT, GAVL_CHID_FRONT_RIGHT, GAVL_CHID_LFE  },
      lfe:               1
    },
    {
      channel_mask:      SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT | SPEAKER_FRONT_CENTER | SPEAKER_LOW_FREQUENCY,
      num_channels:      4,
      channel_setup:     GAVL_CHANNEL_3F,
      channel_locations: { GAVL_CHID_FRONT_LEFT, GAVL_CHID_FRONT_RIGHT, GAVL_CHID_FRONT_CENTER, GAVL_CHID_LFE  },
      lfe:               1
    },
    {
      channel_mask:      SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT | SPEAKER_LOW_FREQUENCY | SPEAKER_BACK_CENTER,
      num_channels:      4,
      channel_setup:     GAVL_CHANNEL_2F1R,
      channel_locations: { GAVL_CHID_FRONT_LEFT, GAVL_CHID_FRONT_RIGHT, GAVL_CHID_LFE , GAVL_CHID_REAR },
      lfe:               1
    },
    {
      channel_mask:      SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT | SPEAKER_FRONT_CENTER | SPEAKER_LOW_FREQUENCY | SPEAKER_BACK_CENTER,
      num_channels:      5,
      channel_setup:     GAVL_CHANNEL_3F1R,
      channel_locations: { GAVL_CHID_FRONT_LEFT, GAVL_CHID_FRONT_RIGHT, GAVL_CHID_FRONT_CENTER, GAVL_CHID_LFE , GAVL_CHID_REAR },
      lfe:               1
    },
    {
      channel_mask:      SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT | SPEAKER_LOW_FREQUENCY | SPEAKER_BACK_LEFT | SPEAKER_BACK_RIGHT,
      num_channels:      5,
      channel_setup:     GAVL_CHANNEL_2F2R,
      channel_locations: { GAVL_CHID_FRONT_LEFT, GAVL_CHID_FRONT_RIGHT, GAVL_CHID_LFE , GAVL_CHID_REAR_LEFT, GAVL_CHID_REAR_RIGHT },
      lfe:               1
    },
    {
      channel_mask:      SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT | SPEAKER_FRONT_CENTER | SPEAKER_LOW_FREQUENCY | SPEAKER_BACK_LEFT | SPEAKER_BACK_RIGHT,
      num_channels:      6,
      channel_setup:     GAVL_CHANNEL_3F2R,
      channel_locations: { GAVL_CHID_FRONT_LEFT, GAVL_CHID_FRONT_RIGHT, GAVL_CHID_FRONT_CENTER, GAVL_CHID_LFE , GAVL_CHID_REAR_LEFT, GAVL_CHID_REAR_RIGHT},
      lfe:               1
    },
  };

static void channel_mask_2_format(uint32_t channel_mask, gavl_audio_format_t * format)
  {
  int i;

  for(i = 0; i < sizeof(channel_setups)/sizeof(channel_setups[0]); i++)
    {
    if(channel_mask == channel_setups[i].channel_mask)
      {
      format->channel_setup = channel_setups[i].channel_setup;
      format->lfe           = channel_setups[i].lfe;
      
      if(format->num_channels != channel_setups[i].num_channels)
        {
        fprintf(stderr, "Channel number mismatch\n");
        break;
        }
      
      memcpy(format->channel_locations, channel_setups[i].channel_locations,
             channel_setups[i].num_channels * sizeof(channel_setups[i].channel_locations[0]));
      return;
      }
    }
  gavl_set_channel_setup(format);
  }

void bgav_WAVEFORMAT_read(bgav_WAVEFORMAT_t * ret, uint8_t * data, int len)
  {
  uint8_t * ptr;
  memset(ret, 0, sizeof(*ret));

  if(len < 12)
    return;

  //  fprintf(stderr, "bgav_VAVEFORMAT_read: %d bytes ", len);
  //  bgav_hexdump(data, 16, 16);
  
  ptr = data;
  ret->type = BGAV_WAVEFORMAT_WAVEFORMAT;
  ret->f.WAVEFORMAT.wFormatTag      = BGAV_PTR_2_16LE(ptr);ptr+=2;
  ret->f.WAVEFORMAT.nChannels       = BGAV_PTR_2_16LE(ptr);ptr+=2;
  ret->f.WAVEFORMAT.nSamplesPerSec  = BGAV_PTR_2_32LE(ptr);ptr+=4;
  ret->f.WAVEFORMAT.nAvgBytesPerSec = BGAV_PTR_2_32LE(ptr);ptr+=4;
  /* size of a data sample */
  ret->f.WAVEFORMAT.nBlockAlign     = BGAV_PTR_2_16LE(ptr);ptr+=2;

  if(len >= 14)
    {
    ret->type = BGAV_WAVEFORMAT_PCMWAVEFORMAT;
    ret->f.PCMWAVEFORMAT.wBitsPerSample     = BGAV_PTR_2_16LE(ptr);ptr+=2;
    }
  if(len >= 16)
    {
    ret->type = BGAV_WAVEFORMAT_WAVEFORMATEX;
    ret->f.WAVEFORMATEX.cbSize = BGAV_PTR_2_16LE(ptr);ptr+=2;
    if(ret->f.WAVEFORMATEX.cbSize)
      {
      if(ret->f.WAVEFORMAT.wFormatTag == 0xfffe && (ret->f.WAVEFORMATEX.cbSize >= 22))
        {
        ret->type = BGAV_WAVEFORMAT_WAVEFORMATEXTENSIBLE;
        ret->f.WAVEFORMATEXTENSIBLE.Samples.wValidBitsPerSample = BGAV_PTR_2_16LE(ptr);ptr+=2;
        ret->f.WAVEFORMATEXTENSIBLE.dwChannelMask = BGAV_PTR_2_32LE(ptr);ptr+=4;
        bgav_GUID_from_ptr(&(ret->f.WAVEFORMATEXTENSIBLE.SubFormat), ptr);ptr+=16;

        if((ret->f.WAVEFORMATEX.cbSize > 22) && (len >= (int)(ptr - data) + ret->f.WAVEFORMATEX.cbSize - 22))
          {
          ret->f.WAVEFORMATEX.ext_data = malloc(ret->f.WAVEFORMATEX.cbSize - 22);
          ret->f.WAVEFORMATEX.ext_size = ret->f.WAVEFORMATEX.cbSize - 22;
          memcpy(ret->f.WAVEFORMATEX.ext_data, ptr, ret->f.WAVEFORMATEX.cbSize - 22);
          ptr += (ret->f.WAVEFORMATEX.cbSize - 22);
          }
        
        }
      else
        {
        if(ret->f.WAVEFORMATEX.cbSize && (len >= (int)(ptr - data) + ret->f.WAVEFORMATEX.cbSize))
          {
          ret->f.WAVEFORMATEX.ext_data = malloc(ret->f.WAVEFORMATEX.cbSize);
          ret->f.WAVEFORMATEX.ext_size = ret->f.WAVEFORMATEX.cbSize;
          memcpy(ret->f.WAVEFORMATEX.ext_data, ptr, ret->f.WAVEFORMATEX.cbSize);
          ptr += ret->f.WAVEFORMATEX.cbSize;
          }
        }

      }
    }
  }

static struct
  {
  uint32_t fourcc;
  bgav_GUID_t guid;
  }
wav_guids[] = 
  {
    {
      BGAV_MK_FOURCC('V','O','R','B'),
      { 0x8a0566ac, 0x42b3, 0x4ad9, { 0xac, 0xa3, 0x93, 0xb9, 0x6, 0xdd, 0xf9, 0x8a }}
    },
    {
      BGAV_MK_FOURCC('V','O','R','B'),
      { 0x6ba47966, 0x3f83, 0x4178, { 0x96, 0x65, 0x00, 0xf0, 0xbf, 0x62, 0x92, 0xe5 } }
    }
  };

static uint32_t guid_2_fourcc(bgav_GUID_t * g)
  {
  int i;
  /* Generic check */

  if(!(g->v1 & 0xffff0000) && 
     (g->v2 == 0x0000) &&
     (g->v3 == 0x0010) &&
     (g->v4[0] == 0x80) &&
     (g->v4[1] == 0x00) &&
     (g->v4[2] == 0x00) &&
     (g->v4[3] == 0xaa) &&
     (g->v4[4] == 0x00) &&
     (g->v4[5] == 0x38) &&
     (g->v4[6] == 0x9b) &&
     (g->v4[7] == 0x71))
    {
    return BGAV_WAVID_2_FOURCC(g->v1);
    }
  for(i = 0; i < sizeof(wav_guids)/sizeof(wav_guids[0]); i++)
    {
    if(bgav_GUID_equal(g, &(wav_guids[i].guid)))
      return wav_guids[i].fourcc;
    }
  return 0;
  }

void bgav_WAVEFORMAT_get_format(bgav_WAVEFORMAT_t * wf,
                                bgav_stream_t * s)
  {
  bgav_WAVEFORMAT_dump(wf);
  
  s->data.audio.format.num_channels = wf->f.WAVEFORMAT.nChannels;
  s->data.audio.format.samplerate   = wf->f.WAVEFORMAT.nSamplesPerSec;
  s->codec_bitrate                  = wf->f.WAVEFORMAT.nAvgBytesPerSec * 8;
  s->container_bitrate              = wf->f.WAVEFORMAT.nAvgBytesPerSec * 8;
  s->data.audio.block_align         = wf->f.WAVEFORMAT.nBlockAlign;
  
  s->timescale = s->data.audio.format.samplerate;

  switch(wf->type)
    {
    case BGAV_WAVEFORMAT_WAVEFORMAT:
      s->fourcc                         = BGAV_WAVID_2_FOURCC(wf->f.WAVEFORMAT.wFormatTag);
      s->data.audio.bits_per_sample     = 8;
      gavl_set_channel_setup(&(s->data.audio.format));
      break;
    case BGAV_WAVEFORMAT_PCMWAVEFORMAT:
      s->fourcc                         = BGAV_WAVID_2_FOURCC(wf->f.WAVEFORMAT.wFormatTag);
      s->data.audio.bits_per_sample     = wf->f.PCMWAVEFORMAT.wBitsPerSample;
      gavl_set_channel_setup(&(s->data.audio.format));
      break;
    case BGAV_WAVEFORMAT_WAVEFORMATEX:
      s->fourcc                         = BGAV_WAVID_2_FOURCC(wf->f.WAVEFORMAT.wFormatTag);
      s->data.audio.bits_per_sample     = wf->f.PCMWAVEFORMAT.wBitsPerSample;

      if(wf->f.WAVEFORMATEX.ext_size)
        {
        s->ext_data = malloc(wf->f.WAVEFORMATEX.ext_size);
        s->ext_size = wf->f.WAVEFORMATEX.ext_size;
        memcpy(s->ext_data, wf->f.WAVEFORMATEX.ext_data, s->ext_size);
        }
      
      gavl_set_channel_setup(&(s->data.audio.format));
      break;
    case BGAV_WAVEFORMAT_WAVEFORMATEXTENSIBLE:
      s->data.audio.bits_per_sample     = wf->f.PCMWAVEFORMAT.wBitsPerSample;
      s->fourcc = guid_2_fourcc(&(wf->f.WAVEFORMATEXTENSIBLE.SubFormat));
      channel_mask_2_format(wf->f.WAVEFORMATEXTENSIBLE.dwChannelMask, &(s->data.audio.format));

      if(wf->f.WAVEFORMATEX.ext_size)
        {
        s->ext_data = malloc(wf->f.WAVEFORMATEX.ext_size);
        s->ext_size = wf->f.WAVEFORMATEX.ext_size;
        memcpy(s->ext_data, wf->f.WAVEFORMATEX.ext_data, s->ext_size);
        }


      break;
    }
  }

void bgav_WAVEFORMAT_set_format(bgav_WAVEFORMAT_t * wf,
                                  bgav_stream_t * s)
  {
  memset(wf, 0, sizeof(*wf));
  wf->type = BGAV_WAVEFORMAT_WAVEFORMATEX;
  wf->f.WAVEFORMAT.nChannels         = s->data.audio.format.num_channels;
  wf->f.WAVEFORMAT.nSamplesPerSec    = s->data.audio.format.samplerate;
  wf->f.WAVEFORMAT.nAvgBytesPerSec   = s->codec_bitrate / 8;
  wf->f.WAVEFORMAT.nBlockAlign       = s->data.audio.block_align;
  wf->f.WAVEFORMAT.wFormatTag        = BGAV_FOURCC_2_WAVID(s->fourcc);
  wf->f.PCMWAVEFORMAT.wBitsPerSample = s->data.audio.bits_per_sample;
  wf->f.WAVEFORMATEX.cbSize          = 0;
  }


void bgav_WAVEFORMAT_dump(bgav_WAVEFORMAT_t * ret)
  {
  switch(ret->type)
    {
    case BGAV_WAVEFORMAT_WAVEFORMAT:
      fprintf(stderr, "WAVEFORMAT\n");
      break;
    case BGAV_WAVEFORMAT_PCMWAVEFORMAT:
      fprintf(stderr, "PCMWAVEFORMAT\n");
      break;
    case BGAV_WAVEFORMAT_WAVEFORMATEX:
      fprintf(stderr, "WAVEFORMATEX\n");
      break;
    case BGAV_WAVEFORMAT_WAVEFORMATEXTENSIBLE:
      fprintf(stderr, "WAVEFORMATEXTENSIBLE\n");
      break;
    }
  fprintf(stderr, "  wFormatTag:      %04x\n", ret->f.WAVEFORMAT.wFormatTag);
  fprintf(stderr, "  nChannels:       %d\n",   ret->f.WAVEFORMAT.nChannels);
  fprintf(stderr, "  nSamplesPerSec:  %d\n",   ret->f.WAVEFORMAT.nSamplesPerSec);
  fprintf(stderr, "  nAvgBytesPerSec: %d\n",   ret->f.WAVEFORMAT.nAvgBytesPerSec);
  fprintf(stderr, "  nBlockAlign:     %d\n",   ret->f.WAVEFORMAT.nBlockAlign);

  switch(ret->type)
    {
    case BGAV_WAVEFORMAT_WAVEFORMAT:
      break;
    case BGAV_WAVEFORMAT_PCMWAVEFORMAT:
      fprintf(stderr, "  wBitsPerSample:  %d\n",   ret->f.PCMWAVEFORMAT.wBitsPerSample);
      break;
    case BGAV_WAVEFORMAT_WAVEFORMATEX:
      fprintf(stderr, "  wBitsPerSample:  %d\n",   ret->f.PCMWAVEFORMAT.wBitsPerSample);
      fprintf(stderr, "  cbSize:          %d\n",   ret->f.WAVEFORMATEX.cbSize);
      if(ret->f.WAVEFORMATEX.ext_size)
        {
        fprintf(stderr, "Extradata %d bytes, hexdump follows\n", ret->f.WAVEFORMATEX.ext_size);
        bgav_hexdump(ret->f.WAVEFORMATEX.ext_data, ret->f.WAVEFORMATEX.ext_size, 16);
        }
      break;
    case BGAV_WAVEFORMAT_WAVEFORMATEXTENSIBLE:
      fprintf(stderr, "  wBitsPerSample:  %d\n",   ret->f.PCMWAVEFORMAT.wBitsPerSample);
      fprintf(stderr, "  cbSize:          %d\n",   ret->f.WAVEFORMATEX.cbSize);
      
      fprintf(stderr, "  wValidBitsPerSample: %d\n", ret->f.WAVEFORMATEXTENSIBLE.Samples.wValidBitsPerSample);
      fprintf(stderr, "  dwChannelMask:       %08x\n", ret->f.WAVEFORMATEXTENSIBLE.dwChannelMask);
      fprintf(stderr, "  SubFormat:           ");
      bgav_GUID_dump(&(ret->f.WAVEFORMATEXTENSIBLE.SubFormat));
      if(ret->f.WAVEFORMATEX.ext_size)
        {
        fprintf(stderr, "Extradata %d bytes, hexdump follows\n", ret->f.WAVEFORMATEX.ext_size);
        bgav_hexdump(ret->f.WAVEFORMATEX.ext_data, ret->f.WAVEFORMATEX.ext_size, 16);
        }
      break;
    }
  }

void bgav_WAVEFORMAT_free(bgav_WAVEFORMAT_t * wf)
  {
  if(wf->f.WAVEFORMATEX.ext_data)
    free(wf->f.WAVEFORMATEX.ext_data);
  }


/* BITMAPINFOHEADER */

void bgav_BITMAPINFOHEADER_read(bgav_BITMAPINFOHEADER_t * ret, uint8_t ** data)
  {
  ret->biSize          = BGAV_PTR_2_32LE((*data));
  ret->biWidth         = BGAV_PTR_2_32LE((*data)+4);
  ret->biHeight        = BGAV_PTR_2_32LE((*data)+8);
  ret->biPlanes        = BGAV_PTR_2_16LE((*data)+12);
  ret->biBitCount      = BGAV_PTR_2_16LE((*data)+14);
  ret->biCompression   = BGAV_PTR_2_32LE((*data)+16);
  ret->biSizeImage     = BGAV_PTR_2_32LE((*data)+20);
  ret->biXPelsPerMeter = BGAV_PTR_2_32LE((*data)+24);
  ret->biYPelsPerMeter = BGAV_PTR_2_32LE((*data)+28);
  ret->biClrUsed       = BGAV_PTR_2_32LE((*data)+32);
  ret->biClrImportant  = BGAV_PTR_2_32LE((*data)+36);
  *data += 40;
  }

void bgav_BITMAPINFOHEADER_get_format(bgav_BITMAPINFOHEADER_t * bh,
                                      bgav_stream_t * s)
  {
  s->data.video.format.frame_width  = bh->biWidth;
  s->data.video.format.frame_height = bh->biHeight;

  s->data.video.format.image_width  = bh->biWidth;
  s->data.video.format.image_height = bh->biHeight;

  s->data.video.format.pixel_width  = 1;
  s->data.video.format.pixel_height = 1;
  s->data.video.depth               = bh->biBitCount;
  s->data.video.image_size          = bh->biSizeImage;
  s->data.video.planes              = bh->biPlanes;
  
  s->fourcc =                        swap_endian(bh->biCompression);

  /* Introduce a fourcc for RGB */
  
  if(!s->fourcc)
    s->fourcc = BGAV_MK_FOURCC('R', 'G', 'B', ' ');
  }

void bgav_BITMAPINFOHEADER_set_format(bgav_BITMAPINFOHEADER_t * bh,
                                      bgav_stream_t * s)
  {
  memset(bh, 0, sizeof(*bh));
  bh->biWidth = s->data.video.format.image_width;
  bh->biHeight = s->data.video.format.image_height;
  bh->biCompression = swap_endian(s->fourcc);
  bh->biBitCount    = s->data.video.depth;
  //  if(bh->biBitCount > 24)
  //    bh->biBitCount = 24;
  bh->biSizeImage   = s->data.video.image_size;
  bh->biPlanes      = s->data.video.planes;
  bh->biSize        = 40;
  if(!bh->biPlanes)
    bh->biPlanes = 1;
  }


void bgav_BITMAPINFOHEADER_dump(bgav_BITMAPINFOHEADER_t * ret)
  {
  uint32_t fourcc_be;
  fprintf(stderr, "BITMAPINFOHEADER:\n");
  fprintf(stderr, "  biSize: %d\n", ret->biSize); /* sizeof(BITMAPINFOHEADER) */
  fprintf(stderr, "  biWidth: %d\n", ret->biWidth);
  fprintf(stderr, "  biHeight: %d\n", ret->biHeight);
  fprintf(stderr, "  biPlanes: %d\n", ret->biPlanes);
  fprintf(stderr, "  biBitCount: %d\n", ret->biBitCount);
  fourcc_be = swap_endian(ret->biCompression);
  fprintf(stderr, "  biCompression: ");
  bgav_dump_fourcc(fourcc_be);
  fprintf(stderr, "\n");
  fprintf(stderr, "  biSizeImage: %d\n", ret->biSizeImage);
  fprintf(stderr, "  biXPelsPerMeter: %d\n", ret->biXPelsPerMeter);
  fprintf(stderr, "  biYPelsPerMeter: %d\n", ret->biXPelsPerMeter);
  fprintf(stderr, "  biClrUsed: %d\n", ret->biClrUsed);
  fprintf(stderr, "  biClrImportant: %d\n", ret->biClrImportant);
  }

/* RIFF INFO chunk */

int bgav_RIFFINFO_probe(bgav_input_context_t * input)
  {
  uint8_t test_data[12];

  if(bgav_input_get_data(input, test_data, 12) < 12)
    return 0;

  if((test_data[0] == 'L') &&
     (test_data[1] == 'I') &&
     (test_data[2] == 'S') &&
     (test_data[3] == 'T') &&
     (test_data[8] == 'I') &&
     (test_data[9] == 'N') &&
     (test_data[10] == 'F') &&
     (test_data[11] == 'O'))
    return 1;

  return 0;
  }

/* RS == read_string */

#define RS(tag) \
  if(!strncmp(ptr, #tag, 4)) \
    { \
    ret->tag = bgav_strndup(ptr + 8, NULL);     \
    ptr += string_len + 8; \
    if(string_len % 2) \
      ptr++; \
    continue; \
    }

bgav_RIFFINFO_t * bgav_RIFFINFO_read_without_header(bgav_input_context_t * input, int size)
  {
  int string_len;
  bgav_RIFFINFO_t * ret;
  uint8_t *buf, * ptr, *end_ptr;
  buf = malloc(size);
  ptr = buf;
  
  if(bgav_input_read_data(input, ptr, size) < size)
    {
    free(buf);
    return (bgav_RIFFINFO_t*)0;
    }
  end_ptr = ptr + size;

  ret = calloc(1, sizeof(*ret));

  while(ptr < end_ptr)
    {
    string_len = BGAV_PTR_2_32LE(ptr + 4);

    //    bgav_hexdump(ptr, 8, 8);
    
    RS(IARL);
    RS(IART);
    RS(ICMS);
    RS(ICMT);
    RS(ICOP);
    RS(ICRD);
    RS(ICRP);
    RS(IDIM);
    RS(IDPI);
    RS(IENG);
    RS(IGNR);
    RS(IKEY);
    RS(ILGT);
    RS(IMED);
    RS(INAM);
    RS(IPLT);
    RS(IPRD);
    RS(ISBJ);
    RS(ISFT);
    RS(ISHP);
    RS(ISRC);
    RS(ISRF);
    RS(ITCH);
    
    ptr += string_len + 8;
    if(string_len % 2)
      ptr++;
    }
  free(buf);
  return ret;
  }

#undef RS

bgav_RIFFINFO_t * bgav_RIFFINFO_read(bgav_input_context_t * input)
  {
  uint32_t size;

  //  fprintf(stderr, "bgav_RIFFINFO_read...");
  
  /*
   *  Read 12 byte header. We assume that it's already verified with
   *  bgav_RIFFINFO_probe
   */

  bgav_input_skip(input, 4);
  bgav_input_read_32_le(input, &size);
  bgav_input_skip(input, 4);

  size -= 4; /* INFO */

  return bgav_RIFFINFO_read_without_header(input, size);
  }


/* DS == dump_string */

#define DS(tag) if(info->tag) fprintf(stderr, "  %s: %s\n", #tag, info->tag)

void bgav_RIFFINFO_dump(bgav_RIFFINFO_t * info)
  {
  fprintf(stderr, "INFO\n");

  DS(IARL);
  DS(IART);
  DS(ICMS);
  DS(ICMT);
  DS(ICOP);
  DS(ICRD);
  DS(ICRP);
  DS(IDIM);
  DS(IDPI);
  DS(IENG);
  DS(IGNR);
  DS(IKEY);
  DS(ILGT);
  DS(IMED);
  DS(INAM);
  DS(IPLT);
  DS(IPRD);
  DS(ISBJ);
  DS(ISFT);
  DS(ISHP);
  DS(ISRC);
  DS(ISRF);
  DS(ITCH);
  
  }

#undef DS

/* FS = free_string */

#define FS(tag) if(info->tag) { free(info->tag); info->tag = (char*)0; }

void bgav_RIFFINFO_destroy(bgav_RIFFINFO_t * info)
  {

  FS(IARL);
  FS(IART);
  FS(ICMS);
  FS(ICMT);
  FS(ICOP);
  FS(ICRD);
  FS(ICRP);
  FS(IDIM);
  FS(IDPI);
  FS(IENG);
  FS(IGNR);
  FS(IKEY);
  FS(ILGT);
  FS(IMED);
  FS(INAM);
  FS(IPLT);
  FS(IPRD);
  FS(ISBJ);
  FS(ISFT);
  FS(ISHP);
  FS(ISRC);
  FS(ISRF);
  FS(ITCH);

  free(info);
  }
#undef FS

/* CS == copy_string */

#define CS(meta, tag) if(!m->meta) m->meta = bgav_strndup(info->tag, NULL);

void bgav_RIFFINFO_get_metadata(bgav_RIFFINFO_t * info, bgav_metadata_t * m)
  {
  CS(artist,    IART);
  CS(title,     INAM);
  CS(comment,   ICMT);
  CS(copyright, ICOP);
  CS(genre,     IGNR);
  CS(date,      ICRD);

  /*
   * Create comment from Software and Engineer
   * This one is for verifying, that some wav samples in Windows XP are
   * made with a cracked version of Soundforge (registered under the name
   * Deepz0ne)
   */
  
  if(!m->comment)
    {
    if(info->IENG && info->ISFT)
      m->comment = bgav_sprintf("Made by %s with %s", info->IENG, info->ISFT);
    else if(info->IENG)
      m->comment = bgav_sprintf("Made by %s", info->IENG, info->ISFT);
    else if(info->ISFT)
      m->comment = bgav_sprintf("Made with %s", info->ISFT);
    }
  }

#undef CS

/* GUID */

void bgav_GUID_dump(bgav_GUID_t * g)
  {
  fprintf(stderr,
          "%08x-%04x-%04x-%02x-%02x-%02x-%02x-%02x-%02x-%02x-%02x\n",
          g->v1, g->v2, g->v3, g->v4[0], g->v4[1], g->v4[2], g->v4[3],
          g->v4[4], g->v4[5], g->v4[6], g->v4[7]);
  }

int bgav_GUID_equal(const bgav_GUID_t * g1, const bgav_GUID_t * g2)
  {
  return (g1->v1 == g2->v1) &&
    (g1->v2 == g2->v2) &&
    (g1->v3 == g2->v3) &&
    !memcmp(g1->v4, g2->v4, 8);
  }

int bgav_GUID_read(bgav_GUID_t * ret, bgav_input_context_t * input)
  {
  return bgav_input_read_32_le(input, &(ret->v1)) &&
    bgav_input_read_16_le(input, &(ret->v2)) &&
    bgav_input_read_16_le(input, &(ret->v3)) &&
    (bgav_input_read_data(input, ret->v4, 8) == 8);
  }

void bgav_GUID_from_ptr(bgav_GUID_t * ret, uint8_t * ptr)
  {
  ret->v1 = BGAV_PTR_2_32LE(ptr);
  ret->v2 = BGAV_PTR_2_16LE(ptr+4);
  ret->v3 = BGAV_PTR_2_16LE(ptr+6);
  memcpy(ret->v4, ptr + 8, 8);
  }

int bgav_GUID_get(bgav_GUID_t * ret, bgav_input_context_t * input)
  {
  uint8_t data[16];

  if(bgav_input_get_data(input, data, 16) < 16)
    return 0;
  bgav_GUID_from_ptr(ret, data);
  return 1;
  }
