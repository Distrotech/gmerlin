/*****************************************************************
  
  bggavl.c
  
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

/* Useful code for gluing gavl and gmerlin */

#define BG_GAVL_PARAM_CONVERSION_QUALITY \
  {                                      \
  name:        "conversion_quality",     \
    long_name:   "Conversion Quality",          \
    type:        BG_PARAMETER_SLIDER_INT,               \
    val_min:     { val_i: GAVL_QUALITY_FASTEST },       \
    val_max:     { val_i: GAVL_QUALITY_BEST    },       \
    val_default: { val_i: GAVL_QUALITY_DEFAULT },                      \
    help_string: "Set the conversion quality for format conversions. \
Lower quality means more speed. Values above 3 enable slow high quality calculations" \
    },

#define BG_GAVL_PARAM_FRAMERATE                                         \
  {                                                                     \
  name:      "fixed_framerate",                                         \
  long_name: "Fixed framerate",                                       \
  type:      BG_PARAMETER_CHECKBUTTON,                                \
  val_default: { val_i: 0 },                                          \
  help_string: "If disabled, the output framerate is taken from the source.\
If enabled, the framerate you specify below us used"\
  },                                              \
  {                                           \
  name:      "timescale",                     \
  long_name: "Timescale",                   \
  type:      BG_PARAMETER_INT,              \
  val_min:     { val_i: 1 },                \
  val_max:     { val_i: 100000 },           \
  val_default: { val_i: 25 },                                         \
  help_string: "Timescale for fixed output framerate (Framerate = timescale / frame_duration)", \
  },                                                                  \
  {                                                                 \
  name:      "frame_duration",                                      \
  long_name: "Frame duration",                                    \
  type:      BG_PARAMETER_INT,                                    \
  val_min:     { val_i: 1 },                                      \
  val_max:     { val_i: 100000 },                                 \
  val_default: { val_i: 1 },                                      \
  help_string: "Frame duration for fixed output framerate (Framerate = timescale / frame_duration)", \
  },


#define BG_GAVL_PARAM_CROP                                         \
 {                                                                \
  name:      "crop_left",                                          \
    long_name: "Crop left",                                        \
    type:      BG_PARAMETER_INT,                                   \
    val_min:     { val_i: 0 },                                     \
      val_max:     { val_i: 100000 },\
      val_default: { val_i: 0 },\
      help_string: "Cut this many pixels from the left border"\
    },\
    {\
      name:      "crop_right",\
      long_name: "Crop right",\
      type:      BG_PARAMETER_INT,\
      val_min:     { val_i: 0 },\
      val_max:     { val_i: 100000 },\
      val_default: { val_i: 0 },\
      help_string: "Cut this many pixels from the right border"\
    },\
    {\
      name:      "crop_top",\
      long_name: "Crop top",\
      type:      BG_PARAMETER_INT,\
      val_min:     { val_i: 0 },\
      val_max:     { val_i: 100000 },\
      val_default: { val_i: 0 },\
      help_string: "Cut this many pixels from the top border"\
    },\
    {\
      name:      "crop_bottom",\
      long_name: "Crop bottom",\
      type:      BG_PARAMETER_INT,\
      val_min:     { val_i: 0 },\
      val_max:     { val_i: 100000 },\
      val_default: { val_i: 0 },\
      help_string: "Cut this many pixels from the bottom border"\
    },



#define BG_GAVL_PARAM_SAMPLERATE                \
    {\
      name:      "fixed_samplerate",\
      long_name: "Fixed samplerate",\
      type:      BG_PARAMETER_CHECKBUTTON,\
      val_default: { val_i: 0 },\
      help_string: "If disabled, the output samplerate is taken from the source.\
If enabled, the samplerate you specify below us used"\
    },\
    {\
      name:        "samplerate",\
      long_name:   "Samplerate",\
      type:        BG_PARAMETER_INT,\
      val_min:     { val_i: 8000 },\
      val_max:     { val_i: 192000 },\
      val_default: { val_i: 44100 },\
      help_string: "Fixed output samplerate",\
    },
