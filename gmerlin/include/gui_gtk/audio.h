/*****************************************************************
 
  audio.h
 
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

#include <gavl/gavl.h>
#include <cfg_registry.h>

/* Audio monitor, will have level meters and other stuff */

typedef struct bg_gtk_vumeter_s bg_gtk_vumeter_t;

bg_gtk_vumeter_t *
bg_gtk_vumeter_create(int max_num_channels);

GtkWidget *
bg_gtk_vumeter_get_widget(bg_gtk_vumeter_t * m);

void bg_gtk_vumeter_set_format(bg_gtk_vumeter_t * m,
                                     gavl_audio_format_t * format);

void bg_gtk_vumeter_update(bg_gtk_vumeter_t * m,
                           gavl_audio_frame_t * frame);

void bg_gtk_vumeter_reset_overflow(bg_gtk_vumeter_t *);


void bg_gtk_vumeter_draw(bg_gtk_vumeter_t * m);

void bg_gtk_vumeter_destroy(bg_gtk_vumeter_t * m);

bg_parameter_info_t *
bg_gtk_vumeter_get_parameters(bg_gtk_vumeter_t * m);

void bg_gtk_vumeter_set_parameter(void * vumeter, const char * name,
                                  bg_parameter_value_t * val);


/* Mixer control */

typedef struct bg_gtk_mixer_control_s bg_gtk_mixer_control_t;

typedef struct
  {
  void (*change)(bg_gtk_mixer_control_t *, float*, void*);
  void (*set_record)(bg_gtk_mixer_control_t *, int, void*);
  void (*set_mute)(bg_gtk_mixer_control_t *, int, void*);
  } bg_gtk_mixer_callbacks_t;

bg_gtk_mixer_control_t *
bg_gtk_mixer_control_create(const char * label, int stereo, int record,
                            bg_gtk_mixer_callbacks_t * callbacks,
                            void * callback_data);

void bg_gtk_mixer_control_configure(bg_gtk_mixer_control_t * ctrl,
                                    bg_cfg_section_t * section);

void
bg_gtk_mixer_control_destroy(bg_gtk_mixer_control_t *);

void
bg_gtk_mixer_control_set_value(bg_gtk_mixer_control_t *, float * value,
                                int send_callback);

void
bg_gtk_mixer_control_set_record(bg_gtk_mixer_control_t *, int record);

void bg_gtk_mixer_control_get_value(bg_gtk_mixer_control_t *, float*);

GtkWidget *
bg_gtk_mixer_control_get_widget(bg_gtk_mixer_control_t*);

