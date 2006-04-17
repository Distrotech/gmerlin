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
    long_name: "Burn options",        \
    type: BG_PARAMETER_SECTION,         \
  },                                    \
  {                                     \
    name: "cdrdao_run",                 \
    long_name: "Run cdrdao",            \
    type: BG_PARAMETER_CHECKBUTTON,     \
    val_default: { val_i: 0 },          \
  },                                    \
  {                                     \
    name:       "cdrdao_device",          \
    long_name:  "Device",                 \
    type: BG_PARAMETER_STRING,            \
    val_default: { val_str: (char*)0 },   \
    help_string: "Device to use as burner. Type \"cdrdao scanbus\" at the commandline for supported devices. Leave this empty to attempt autodetection.", \
  }, \
  {                                     \
  name:       "cdrdao_eject",                           \
    long_name:  "Eject after burning",                       \
    type: BG_PARAMETER_CHECKBUTTON,                          \
    val_default: { val_i: 0 },                               \
  },                                                       \
  {                                                      \
    name:       "cdrdao_simulate",                           \
    long_name:  "Simulate",                  \
    type: BG_PARAMETER_CHECKBUTTON,                          \
    val_default: { val_i: 0 },                 \
    },                                                    \
  {                                                 \
    name:       "cdrdao_speed",                       \
    long_name:  "Speed",                  \
    type: BG_PARAMETER_INT,                          \
    val_default: { val_i: 0 },                 \
    val_min: { val_i: 0 },                 \
    val_max: { val_i: 1000 },                 \
    help_string: "Set the writing speed. 0 means autodetect", \
    }



typedef struct bg_cdrdao_s bg_cdrdao_t;

bg_cdrdao_t * bg_cdrdao_create();
void bg_cdrdao_destroy(bg_cdrdao_t *);
void bg_cdrdao_set_parameter(void * data, char * name, bg_parameter_value_t * val);

void bg_cdrdao_run(bg_cdrdao_t *, const char * toc_file);
