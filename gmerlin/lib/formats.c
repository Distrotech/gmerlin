/*****************************************************************
 * gmerlin - a general purpose multimedia framework and applications
 *
 * Copyright (c) 2001 - 2008 Members of the Gmerlin project
 * gmerlin-general@lists.sourceforge.net
 * http://gmerlin.sourceforge.net
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 * *****************************************************************/

#include <stdlib.h>


#include <config.h>
#include <gmerlin/translation.h>

#include <gmerlin/utils.h>

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
    channel_order = bg_strcat(channel_order,
                              TRD(gavl_channel_id_to_string(f->channel_locations[i]), NULL));
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
               TRD(gavl_interleave_mode_to_string(f->interleave_mode), NULL),
               TRD(gavl_sample_format_to_string(f->sample_format), NULL));
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
               TRD(gavl_pixelformat_to_string(format->pixelformat), NULL));
  
  if(format->framerate_mode == GAVL_FRAMERATE_STILL)
    {
    ret = bg_strcat(ret, TR("Still image\n"));
    }
  else if(!format->frame_duration &&
          (format->framerate_mode == GAVL_FRAMERATE_VARIABLE))
    {
    if(!use_tabs)
      s = TR("Framerate:    Variable (timescale: %d)\n");
    else
      s = TR("Framerate:\tVariable (timescale: %d)\n");

    str = bg_sprintf(s, format->timescale);
    ret = bg_strcat(ret, str);
    free(str);
    }
  else
    {
    if(!use_tabs)
      s = TR("Framerate:    %f fps [%d / %d]%s\n");
    else
      s = TR("Framerate:\t%f fps [%d / %d]%s\n");
    
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
  
  str =
    bg_sprintf(s, TRD(gavl_interlace_mode_to_string(format->interlace_mode),
                      NULL));
  ret = bg_strcat(ret, str);
  free(str);

  if(format->pixelformat == GAVL_YUV_420_P)
    {
    if(!use_tabs)
      s = TR("\nChroma placement: %s");
    else
      s = TR("\nChroma placement:\t%s");
    str = bg_sprintf(s, TRD(gavl_chroma_placement_to_string(format->chroma_placement), NULL));
    ret = bg_strcat(ret, str);
    free(str);
    }

  if(format->timecode_format.int_framerate)
    {
    if(!use_tabs)
      s = TR("\nTimecode rate:    %d");
    else
      s = TR("\nTimecode rate:\t%d");
    str = bg_sprintf(s, format->timecode_format.int_framerate);
    ret = bg_strcat(ret, str);
    free(str);

    if(format->timecode_format.flags)
      {
      if(!use_tabs)
        s = TR("\nTimecode flags: ");
      else
        s = TR("\nTimecode flags:\t");
      ret = bg_strcat(ret, s);
      
      if(format->timecode_format.flags &
         GAVL_TIMECODE_DROP_FRAME)
        ret = bg_strcat(ret, TR("Drop "));
      }
    }
  return ret;
  }

/*
 *  When gavl will be gettextized (probably never), the
 *  following stuff must be removed
 */

// gavl_channel_id_t

TRU("Unknown channel")
TRU("Front C")
TRU("Front L")
TRU("Front R")
TRU("Front CL")
TRU("Front CR")
TRU("Rear C")
TRU("Rear L")
TRU("Rear R")
TRU("Side L")
TRU("Side R")
TRU("LFE")
TRU("AUX")

// gavl_sample_format_t

TRU("Not specified")
TRU("Unsigned 8 bit")
TRU("Signed 8 bit")
TRU("Unsigned 16 bit")
TRU("Signed 16 bit")
TRU("Signed 32 bit")
TRU("Floating point")
TRU("Double precision")
// gavl_interleave_mode_t

TRU("Not interleaved")
TRU("Interleaved channel pairs")
TRU("All channels interleaved")

// gavl_interlace_mode_t
     
TRU("None (Progressive)")
TRU("Top field first")
TRU("Bottom field first")
TRU("Mixed")

// gavl_chroma_placement_t
TRU("MPEG-1/JPEG")
TRU("MPEG-2")
TRU("DV PAL")

// gavl_pixelformat_t

TRU("15 bpp RGB")
TRU("15 bpp BGR")
TRU("16 bpp RGB")
TRU("16 bpp BGR")
TRU("24 bpp RGB")
TRU("24 bpp BGR")
TRU("32 bpp RGB")
TRU("32 bpp BGR")
TRU("32 bpp RGBA")
TRU("48 bpp RGB")
TRU("64 bpp RGBA")
TRU("Float RGB")
TRU("Float RGBA")
TRU("YUV 422 (YUY2)")
TRU("YUV 422 (UYVY)")
TRU("YUVA 4444")
TRU("YUV 420 Planar")
TRU("YUV 410 Planar")
TRU("YUV 411 Planar")
TRU("YUV 422 Planar")
TRU("YUV 422 Planar (16 bit)")
TRU("YUV 444 Planar")
TRU("YUV 444 Planar (16 bit)")
TRU("YUVJ 420 Planar")
TRU("YUVJ 422 Planar")
TRU("YUVJ 444 Planar")
TRU("Undefined")
