/*****************************************************************
 
  cdrdao_common.h
 
  Copyright (c) 2006 by Burkhard Plaum - plaum@ipf.uni-stuttgart.de
 
  http://gmerlin.sourceforge.net
 
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
 
  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111, USA.
 
*****************************************************************/

#define CDRDAO_PARAMS                   \
  {                                     \
    name: "cdrdao",                     \
    long_name: TRS("Burn options"),        \
    type: BG_PARAMETER_SECTION,         \
  },                                    \
  {                                     \
    name: "cdrdao_run",                 \
    long_name: TRS("Run cdrdao"),            \
    type: BG_PARAMETER_CHECKBUTTON,     \
    val_default: { val_i: 0 },          \
  },                                    \
  {                                     \
    name:       "cdrdao_device",          \
    long_name:  TRS("Device"),                 \
    type: BG_PARAMETER_STRING,            \
    val_default: { val_str: (char*)0 },   \
    help_string: TRS("Device to use as burner. Type \"cdrdao scanbus\" at the commandline for supported devices. Leave this empty to use the default /dev/cdrecorder."), \
  }, \
  {                                     \
    name:       "cdrdao_driver",          \
    long_name:  TRS("Driver"),                 \
    type: BG_PARAMETER_STRING,            \
    val_default: { val_str: (char*)0 },   \
    help_string: TRS("Driver to use. Check the cdrdao manual page and the cdrdao README for available drivers/options. Leave this empty to attempt autodetection."), \
  }, \
  {                                     \
  name:       "cdrdao_eject",                           \
    long_name:  TRS("Eject after burning"),                       \
    type: BG_PARAMETER_CHECKBUTTON,                          \
    val_default: { val_i: 0 },                               \
  },                                                       \
  {                                                      \
    name:       "cdrdao_simulate",                           \
    long_name:  TRS("Simulate"),                  \
    type: BG_PARAMETER_CHECKBUTTON,                          \
    val_default: { val_i: 0 },                 \
    },                                                    \
  {                                                 \
    name:       "cdrdao_speed",                       \
    long_name:  TRS("Speed"),                  \
    type: BG_PARAMETER_INT,                          \
    val_default: { val_i: 0 },                 \
    val_min: { val_i: 0 },                 \
    val_max: { val_i: 1000 },                 \
    help_string: TRS("Set the writing speed. 0 means autodetect."), \
    }, \
    {                                                   \
    name:       "cdrdao_nopause",                       \
    long_name:  TRS("No pause"),                  \
    type: BG_PARAMETER_CHECKBUTTON,               \
    val_default: { val_i: 0 },                 \
    help_string: TRS("Skip the 10 second pause before writing or simulating starts."), \
    }



typedef struct bg_cdrdao_s bg_cdrdao_t;

bg_cdrdao_t * bg_cdrdao_create();
void bg_cdrdao_destroy(bg_cdrdao_t *);
void bg_cdrdao_set_callbacks(bg_cdrdao_t *, bg_e_pp_callbacks_t * callbacks);

void bg_cdrdao_set_parameter(void * data, const char * name,
                             const bg_parameter_value_t * val);

/* 1 if cdrdao was actually run */
int bg_cdrdao_run(bg_cdrdao_t *, const char * toc_file);

void bg_cdrdao_stop(bg_cdrdao_t *);
