/*****************************************************************
 
  a52_header.c
 
  Copyright (c) 2007 by Burkhard Plaum - plaum@ipf.uni-stuttgart.de
 
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
#include <a52_header.h>

#define LEVEL_3DB 0.7071067811865476
#define LEVEL_45DB 0.5946035575013605
#define LEVEL_6DB 0.5

static uint8_t halfrate[12] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 3};
static int rate[] = { 32,  40,  48,  56,  64,  80,  96, 112,
                      128, 160, 192, 224, 256, 320, 384, 448,
                      512, 576, 640};
static uint8_t lfeon[8] = {0x10, 0x10, 0x04, 0x04, 0x04, 0x01, 0x04, 0x01};

static float clev[4] = {LEVEL_3DB, LEVEL_45DB, LEVEL_6DB, LEVEL_45DB};
static float slev[4] = {LEVEL_3DB, LEVEL_6DB,          0, LEVEL_6DB};

int bgav_a52_header_read(bgav_a52_header_t * ret, uint8_t * buf)
  {
  int half;
  int frmsizecod;
  int bitrate;

  int cmixlev;
  int smixlev;
  
  
  if ((buf[0] != 0x0b) || (buf[1] != 0x77))   /* syncword */
    {
    return 0;
    }
  if (buf[5] >= 0x60)         /* bsid >= 12 */
    {
    return 0;
    }
  half = halfrate[buf[5] >> 3];

  /* acmod, dsurmod and lfeon */
  ret->acmod = buf[6] >> 5;

  /* cmixlev and surmixlev */

  if((ret->acmod & 0x01) && (ret->acmod != 0x01))
    {
    cmixlev = (buf[6] & 0x18) >> 3;
    }
  else
    cmixlev = -1;
  
  if(ret->acmod & 0x04)
    {
    if((cmixlev) == -1)
      smixlev = (buf[6] & 0x18) >> 3;
    else
      smixlev = (buf[6] & 0x06) >> 1;
    }
  else
    smixlev = -1;

  if(smixlev >= 0)
    ret->smixlev = slev[smixlev];
  if(cmixlev >= 0)
    ret->cmixlev = clev[cmixlev];
  
  if((buf[6] & 0xf8) == 0x50)
    ret->dolby = 1;

  if(buf[6] & lfeon[ret->acmod])
    ret->lfe = 1;

  frmsizecod = buf[4] & 63;
  if (frmsizecod >= 38)
    return 0;
  bitrate = rate[frmsizecod >> 1];
  ret->bitrate = (bitrate * 1000) >> half;

  switch (buf[4] & 0xc0)
    {
    case 0:
      ret->samplerate = 48000 >> half;
      ret->total_bytes = 4 * bitrate;
      break;
    case 0x40:
      ret->samplerate = 44100 >> half;
      ret->total_bytes =  2 * (320 * bitrate / 147 + (frmsizecod & 1));
      break;
    case 0x80:
      ret->samplerate = 32000 >> half;
      ret->total_bytes =  6 * bitrate;
      break;
    default:
      return 0;
    }
  
  return 1;
  }

void bgav_a52_header_dump(bgav_a52_header_t * h)
  {
  bgav_dprintf("A52 header:\n");
  bgav_dprintf("  Frame bytes: %d\n", h->total_bytes);
  bgav_dprintf("  Samplerate:  %d\n", h->samplerate);
  bgav_dprintf("  acmod:  0x%0x\n",   h->acmod);
  if(h->smixlev >= 0.0)
    bgav_dprintf("  smixlev: %f\n", h->smixlev);
  if(h->cmixlev >= 0.0)
    bgav_dprintf("  cmixlev: %f\n", h->cmixlev);
  }
