/*****************************************************************
 
  win32codec.h
 
  Copyright (c) 2005-2006 by Burkhard Plaum - plaum@ipf.uni-stuttgart.de
 
  http://gmerlin.sourceforge.net
 
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
 
  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111, USA.
 
*****************************************************************/

/* Win32 codecs work in an own thread to prevent crashes */

typedef struct bgav_win32_thread_s
  {
  int state;
  
  ldt_fs_t * ldt_fs;
  void * priv;
  
  sem_t input_ready;
  sem_t output_ready;

  bgav_stream_t * s;

  pthread_t thread;

  /* Input and output data */
  gavl_audio_frame_t * audio_frame;
  gavl_video_frame_t * video_frame;

  uint8_t * data;
  int data_len;
  int keyframe;
  int keyframe_seen;
  
  int last_frame_samples;

  /* Functions */
  int (*init)(struct bgav_win32_thread_s*);
  int (*decode)(struct bgav_win32_thread_s*);
  void (*cleanup)(struct bgav_win32_thread_s*);
  } bgav_win32_thread_t;

int bgav_win32_codec_thread_init(bgav_win32_thread_t*,bgav_stream_t*s);

int bgav_win32_codec_thread_decode_audio(bgav_win32_thread_t*,
                                         gavl_audio_frame_t*, uint8_t * data,
                                         int data_len);

int bgav_win32_codec_thread_decode_video(bgav_win32_thread_t*,
                                         gavl_video_frame_t*,
                                         uint8_t * data,
                                         int data_len, int keyframe);

void bgav_win32_codec_thread_cleanup(bgav_win32_thread_t*);

void bgav_windll_lock();
void bgav_windll_unlock();
