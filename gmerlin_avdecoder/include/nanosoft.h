/*****************************************************************
 
  nanosoft.h
 
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

/* Stuff from redmont */

typedef struct
  {
  uint32_t  biSize;  /* sizeof(BITMAPINFOHEADER) */
  uint32_t  biWidth;
  uint32_t  biHeight;
  uint16_t  biPlanes; /* unused */
  uint16_t  biBitCount;
  uint32_t  biCompression; /* fourcc of image */
  uint32_t  biSizeImage;   /* size of image. For uncompressed images */
                           /* ( biCompression 0 or 3 ) can be zero.  */
  
  uint32_t  biXPelsPerMeter; /* unused */
  uint32_t  biYPelsPerMeter; /* unused */
  uint32_t  biClrUsed;       /* valid only for palettized images. */
  /* Number of colors in palette. */
  uint32_t  biClrImportant;
  } bgav_BITMAPINFOHEADER;

typedef struct
  {
  uint16_t  wFormatTag;     /* value that identifies compression format */
  uint16_t  nChannels;
  uint32_t  nSamplesPerSec;
  uint32_t  nAvgBytesPerSec;
  uint16_t  nBlockAlign;    /* size of a data sample */
  uint16_t  wBitsPerSample;
  uint16_t  cbSize;         /* size of format-specific data */
  } bgav_WAVEFORMATEX;

void bgav_BITMAPINFOHEADER_read(bgav_BITMAPINFOHEADER * ret, uint8_t ** data);
void bgav_WAVEFORMATEX_read(bgav_WAVEFORMATEX * ret, uint8_t ** data);

void bgav_BITMAPINFOHEADER_dump(bgav_BITMAPINFOHEADER * ret);
void bgav_WAVEFORMATEX_dump(bgav_WAVEFORMATEX * ret);

void bgav_WAVEFORMATEX_get_format(bgav_WAVEFORMATEX * wf,
                                  bgav_stream_t * f);

void bgav_BITMAPINFOHEADER_get_format(bgav_BITMAPINFOHEADER * bh,
                                      bgav_stream_t * f);

void bgav_BITMAPINFOHEADER_set_format(bgav_BITMAPINFOHEADER * bh,
                                      bgav_stream_t * f);

