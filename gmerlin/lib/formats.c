/*****************************************************************
 
  formats.c
 
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

#include <utils.h>



char * bg_audio_format_to_string(gavl_audio_format_t * f)
  {
  return
    bg_sprintf("Channels:          %d (%s%s)\nSamplerate:        %d\nSamples per frame: %d\nInterleave Mode:   %s\nSample format:     %s",
              f->num_channels,
              gavl_channel_setup_to_string(f->channel_setup),
               (f->lfe ? " + LFE" : ""),
              f->samplerate, f->samples_per_frame,
              gavl_interleave_mode_to_string(f->interleave_mode),
              gavl_sample_format_to_string(f->sample_format));
  }

char * bg_video_format_to_string(gavl_video_format_t * format)
  {
  return
    bg_sprintf("Frame size:   %d x %d\nImage size:   %d x %d\nPixel size:   %d x %d\nPixel format: %s\nFramerate:    %f fps [%d / %d]\n             %s",
              format->frame_width, format->frame_height,
              format->image_width, format->image_height,
              format->pixel_width, format->pixel_height,
              gavl_colorspace_to_string(format->colorspace),
              (float)(format->framerate_num)/((float)format->framerate_den),
              format->framerate_num, format->framerate_den,
              (!(format->free_framerate) ? " (Constant)" : 
               " (Not constant)"));
  }
