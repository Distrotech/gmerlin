/*****************************************************************
 
  dvb_channels.h
 
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


#ifdef HAVE_LINUXDVB

typedef struct
  {
  char * name;
  int tone;
  
  int audio_pid; // Unused
  int video_pid; // Unused
  int service_id;
    
  struct dvb_frontend_parameters front_param;
  
  int pol;    // 1: Vertical, 0 horizontal
  int sat_no; // satellite number
  int initialized;

  int pcr_pid;
  int extra_pcr_pid; // 1 if the PCR is contained in a non-A/V stream
  int num_ac3_streams;
  } bgav_dvb_channel_info_t;

#else

typedef struct
  {
  int dummy;
  }

#endif

bgav_dvb_channel_info_t *
bgav_dvb_channels_load(const bgav_options_t * opt,
                       fe_type_t type, int * num);

void dvb_channels_destroy(bgav_dvb_channel_info_t *, int num);

void dvb_channels_dump(bgav_dvb_channel_info_t *, fe_type_t type, int num);

