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

#include <stdlib.h>


#include <config.h>
#include <translation.h>

#include <utils.h>

static char * get_dB(float val)
  {
  if(val == 0.0)
    return bg_strdup((char*)0, TR("Zero"));
  else
    return bg_sprintf("%02f dB", val);
  }

char * bg_audio_format_to_string(gavl_audio_format_t * f, int use_tabs)
  {
  const char * format;
  char * channel_order = (char*)0;
  
  char * center_level;
  char * rear_level;

  char * ret;
  int i;

  center_level = get_dB(f->center_level);
  rear_level = get_dB(f->rear_level);
  
  /* Create channel order string */

  for(i = 0; i < f->num_channels; i++)
    {
    channel_order = bg_strcat(channel_order, gavl_channel_id_to_string(f->channel_locations[i]));
    if(i < f->num_channels - 1)
      channel_order = bg_strcat(channel_order, ", ");
    }
  
  if(!use_tabs)
    format = TR("Channels:          %d\nChannel order:     %s\nSamplerate:        %d\nSamples per frame: %d\nInterleave Mode:   %s\nSample format:     %s");
  else
    format = TR("Channels:\t %d\nChannel order\t %s\nSamplerate:\t %d\nSamples per frame:\t %d\nInterleave Mode:\t %s\nSample format:\t %s");
  ret =
    bg_sprintf(format,
               f->num_channels,
               channel_order,
               f->samplerate, f->samples_per_frame,
               gavl_interleave_mode_to_string(f->interleave_mode),
               gavl_sample_format_to_string(f->sample_format));

  free(channel_order);
  free(center_level);
  free(rear_level);
  return ret;

  }

char * bg_video_format_to_string(gavl_video_format_t * format, int use_tabs)
  {
  char * str, *ret;
  const char * s;
  if(!use_tabs)
    s = TR("Frame size:   %d x %d\nImage size:   %d x %d\nPixel size:   %d x %d\nPixel format: %s\n");
  else
    s = TR("Frame size:\t %d x %d\nImage size:\t %d x %d\nPixel size:\t %d x %d\nPixel format:\t %s\n");

  ret =
    bg_sprintf(s,
               format->frame_width, format->frame_height,
               format->image_width, format->image_height,
               format->pixel_width, format->pixel_height,
               gavl_pixelformat_to_string(format->pixelformat));
  
  if(format->framerate_mode == GAVL_FRAMERATE_STILL)
    {
    ret = bg_strcat(ret, TR("Still image\n"));
    }
  else
    {
    if(!use_tabs)
      s = TR("Framerate:    %f fps [%d / %d]\n             %s\n");
    else
      s = TR("Framerate:\t%f fps [%d / %d]\n\t%s\n");
    
    str =
      bg_sprintf(s,
                 (float)(format->timescale)/((float)format->frame_duration),
                 format->timescale, format->frame_duration,
                 ((format->framerate_mode == GAVL_FRAMERATE_CONSTANT) ? TR(" (constant)") : 
                  TR(" (variable)")));

    ret = bg_strcat(ret, str);
    free(str);
    }
  if(!use_tabs)
    s = TR("Interlace mode:   %s");
  else
    s = TR("Interlace mode:\t%s");
  
  str = bg_sprintf(s, gavl_interlace_mode_to_string(format->interlace_mode));
  ret = bg_strcat(ret, str);
  free(str);

  if(format->pixelformat == GAVL_YUV_420_P)
    {
    if(!use_tabs)
      s = TR("\nChroma placement: %s");
    else
      s = TR("\nChroma placement:\t%s");
    str = bg_sprintf(s, gavl_chroma_placement_to_string(format->chroma_placement));
    ret = bg_strcat(ret, str);
    free(str);
    }
  return ret;
  }
