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

void bgav_BITMAPINFOHEADER_read(bgav_BITMAPINFOHEADER * ret, uint8_t ** data)
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

#if 0
  uint16_t  wFormatTag;     /* value that identifies compression format */
  uint16_t  nChannels;
  uint32_t  nSamplesPerSec;
  uint32_t  nAvgBytesPerSec;
  uint16_t  nBlockAlign;    /* size of a data sample */
  uint16_t  wBitsPerSample;
  uint16_t  cbSize;         /* size of format-specific data */
#endif

void bgav_WAVEFORMATEX_read(bgav_WAVEFORMATEX * ret, uint8_t ** data)
  {
  uint8_t * ptr;
  ptr = *data;
  /* value that identifies compression format */
  ret->wFormatTag      = BGAV_PTR_2_16LE(ptr);ptr+=2;
  ret->nChannels       = BGAV_PTR_2_16LE(ptr);ptr+=2;
  ret->nSamplesPerSec  = BGAV_PTR_2_32LE(ptr);ptr+=4;
  ret->nAvgBytesPerSec = BGAV_PTR_2_32LE(ptr);ptr+=4;
  /* size of a data sample */
  ret->nBlockAlign     = BGAV_PTR_2_16LE(ptr);ptr+=2;
  ret->wBitsPerSample  = BGAV_PTR_2_16LE(ptr);ptr+=2;
    /* size of format-specific data */
  ret->cbSize          = BGAV_PTR_2_16LE(ptr);ptr+=2;
  *data = ptr;
  }

void bgav_WAVEFORMATEX_get_format(bgav_WAVEFORMATEX * wf,
                                  bgav_stream_t * s)
  {
  s->data.audio.format.num_channels = wf->nChannels;
  s->data.audio.format.samplerate   = wf->nSamplesPerSec;
  s->codec_bitrate                  = wf->nAvgBytesPerSec * 8;
  s->data.audio.block_align         = wf->nBlockAlign;
  s->data.audio.bits_per_sample     = wf->wBitsPerSample;
  s->fourcc                         = BGAV_WAVID_2_FOURCC(wf->wFormatTag);
  }

void bgav_WAVEFORMATEX_set_format(bgav_WAVEFORMATEX * wf,
                                  bgav_stream_t * s)
  {
  wf->nChannels             = s->data.audio.format.num_channels;
  wf->nSamplesPerSec        = s->data.audio.format.samplerate;
  wf->nAvgBytesPerSec       = s->codec_bitrate / 8;
  wf->nBlockAlign           = s->data.audio.block_align;
  wf->wFormatTag            = BGAV_FOURCC_2_WAVID(s->fourcc);
  wf->wBitsPerSample        = s->data.audio.bits_per_sample;
  wf->cbSize                = 0;
  }

void bgav_BITMAPINFOHEADER_get_format(bgav_BITMAPINFOHEADER * bh,
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
  }

void bgav_BITMAPINFOHEADER_set_format(bgav_BITMAPINFOHEADER * bh,
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


void bgav_BITMAPINFOHEADER_dump(bgav_BITMAPINFOHEADER * ret)
  {
  uint32_t fourcc_be;
  fprintf(stderr, "biSize: %d\n", ret->biSize); /* sizeof(BITMAPINFOHEADER) */
  fprintf(stderr, "biWidth: %d\n", ret->biWidth);
  fprintf(stderr, "biHeight: %d\n", ret->biHeight);
  fprintf(stderr, "biPlanes: %d\n", ret->biPlanes);
  fprintf(stderr, "biBitCount: %d\n", ret->biBitCount);
  fourcc_be = swap_endian(ret->biCompression);
  fprintf(stderr, "biCompression: ");
  bgav_dump_fourcc(fourcc_be);
  fprintf(stderr, "\n");
  fprintf(stderr, "biSizeImage: %d\n", ret->biSizeImage);
  fprintf(stderr, "biXPelsPerMeter: %d\n", ret->biXPelsPerMeter);
  fprintf(stderr, "biYPelsPerMeter: %d\n", ret->biXPelsPerMeter);
  fprintf(stderr, "biClrUsed: %d\n", ret->biClrUsed);
  fprintf(stderr, "biClrImportant: %d\n", ret->biClrImportant);
  }

void bgav_WAVEFORMATEX_dump(bgav_WAVEFORMATEX * ret)
  {
  fprintf(stderr, "wFormatTag:      %04x\n", ret->wFormatTag);
  fprintf(stderr, "nChannels:       %d\n",   ret->nChannels);
  fprintf(stderr, "nSamplesPerSec:  %d\n", ret->nSamplesPerSec);
  fprintf(stderr, "nAvgBytesPerSec: %d\n", ret->nAvgBytesPerSec);
  fprintf(stderr, "nBlockAlign:     %d\n", ret->nBlockAlign);
  fprintf(stderr, "wBitsPerSample:  %d\n", ret->wBitsPerSample);
  fprintf(stderr, "cbSize:          %d\n", ret->cbSize);
  }
