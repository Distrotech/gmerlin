/*****************************************************************

  volume.h

  Copyright (c) 2004 by Burkhard Plaum - plaum@ipf.uni-stuttgart.de

  http://gmerlin.sourceforge.net

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111, USA.

*****************************************************************/

typedef struct
  {
  void (*set_volume_s8)(gavl_volume_control_t * v, void * samples,
                        int num_samples);
  void (*set_volume_u8)(gavl_volume_control_t * v, void * samples,
                        int num_samples);

  void (*set_volume_s16)(gavl_volume_control_t * v, void * samples,
                         int num_samples);
  void (*set_volume_u16)(gavl_volume_control_t * v, void * samples,
                         int num_samples);

  void (*set_volume_s32)(gavl_volume_control_t * v, void * samples,
                         int num_samples);

  void (*set_volume_float)(gavl_volume_control_t * v, void * samples,
                         int num_samples);
  } gavl_volume_funcs_t;

struct gavl_volume_control_s
  {
  gavl_audio_format_t format;
  
  float factor_f;
  int64_t factor_i;
  
  void (*set_volume)(gavl_volume_control_t * v,
                     gavl_audio_frame_t * frame);
  
  void (*set_volume_channel)(gavl_volume_control_t * v,
                             void * samples,
                             int num_samples);
  };


gavl_volume_funcs_t * gavl_volume_funcs_create();
void gavl_volume_funcs_destroy(gavl_volume_funcs_t *);

/* Get specific functions */

void gavl_init_volume_funcs_c(gavl_volume_funcs_t*);

/* TODO */
#ifdef ARCH_X86
// void gavl_init_volume_funcs_mmx(gavl_volume_funcs_t*);
#endif
