/*****************************************************************

  deinterlace_blend_c.c

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

#include <gavl/gavl.h>
#include <video.h>

#include <deinterlace.h>
#include <accel.h>
#include <colorspace_tables.h>
#include <colorspace_macros.h>
static void blend_func_packed_15_c(uint8_t * t1,
                                   uint8_t * m1,
                                   uint8_t * b1,
                                   uint8_t * dst1,
                                   int num)
  {
  int i;

  uint16_t * t   = (uint16_t*)t1;
  uint16_t * m   = (uint16_t*)m1;
  uint16_t * b   = (uint16_t*)b1;
  uint16_t * dst = (uint16_t*)dst1;
  
  for(i = 0; i < num; i++)
    {
    *dst = 0;
    
    *dst |=
      (((*t & RGB15_LOWER_MASK) +
        (*b & RGB15_LOWER_MASK) +
        ((*m & RGB15_LOWER_MASK) << 1)) >> 2);
    
    *dst |=
      (((*t & RGB15_MIDDLE_MASK) +
        (*b & RGB15_MIDDLE_MASK) +
        ((*m & RGB15_MIDDLE_MASK) << 1)) >> 2) &
      RGB15_MIDDLE_MASK;

    *dst |=
      (((*t & RGB15_UPPER_MASK) +
        (*b & RGB15_UPPER_MASK) +
        ((*m & RGB15_UPPER_MASK) << 1)) >> 2) &
      RGB15_UPPER_MASK;
    
    dst++;
    t++;
    m++;
    b++;
    }
  }

static void blend_func_packed_16_c(uint8_t * t1,
                                   uint8_t * m1,
                                   uint8_t * b1,
                                   uint8_t * dst1,
                                   int num)
  {
  int i;

  uint16_t * t   = (uint16_t*)t1;
  uint16_t * m   = (uint16_t*)m1;
  uint16_t * b   = (uint16_t*)b1;
  uint16_t * dst = (uint16_t*)dst1;
  
  for(i = 0; i < num; i++)
    {
    *dst = 0;
     
    *dst |=
      (((*t & RGB16_LOWER_MASK) +
        (*b & RGB16_LOWER_MASK) +
        ((*m & RGB16_LOWER_MASK) << 1)) >> 2);
    
    *dst |=
      (((*t & RGB16_MIDDLE_MASK) +
        (*b & RGB16_MIDDLE_MASK) +
        ((*m & RGB16_MIDDLE_MASK) << 1)) >> 2) &
      RGB16_MIDDLE_MASK;

    *dst |=
      (((*t & RGB16_UPPER_MASK) +
        (*b & RGB16_UPPER_MASK) +
        ((*m & RGB16_UPPER_MASK) << 1)) >> 2) &
      RGB16_UPPER_MASK;
    
    dst++;
    t++;
    m++;
    b++;
    }
  }

static void blend_func_8_c(uint8_t * t,
                           uint8_t * m,
                           uint8_t * b,
                           uint8_t * dst,
                           int num)
  {
  int i;
  for(i = 0; i < num; i++)
    {
    *(dst++) = (*(t++) + (*(m++) << 1) + *(b++)) >> 2;
    }
  }

static void blend_func_16_c(uint8_t * t1,
                            uint8_t * m1,
                            uint8_t * b1,
                            uint8_t * dst1,
                            int num)
  {
  int i;

  uint16_t * t   = (uint16_t*)t1;
  uint16_t * m   = (uint16_t*)m1;
  uint16_t * b   = (uint16_t*)b1;
  uint16_t * dst = (uint16_t*)dst1;
  
  for(i = 0; i < num; i++)
    {
    *(dst++) = (*(t++) + (*(m++) << 1) + *(b++)) >> 2;
    }
  }



static void blend_func_float_c(uint8_t * t1,
                               uint8_t * m1,
                               uint8_t * b1,
                               uint8_t * dst1,
                               int num)
  {
  int i;

  float * t   = (float*)t1;
  float * m   = (float*)m1;
  float * b   = (float*)b1;
  float * dst = (float*)dst1;
  
  for(i = 0; i < num; i++)
    *(dst++) = (*(t++) + *(b++) + 2.0 * (*(m++))) * 0.25;
  }
void
gavl_find_deinterlacer_blend_funcs_c(gavl_video_deinterlace_blend_func_table_t * tab,
                                     const gavl_video_options_t * opt,
                                     const gavl_video_format_t * format)
  {
  tab->func_packed_15 = blend_func_packed_15_c;
  tab->func_packed_16 = blend_func_packed_16_c;
  tab->func_8 = blend_func_8_c;
  tab->func_16 = blend_func_16_c;
  tab->func_float = blend_func_float_c;
  }
