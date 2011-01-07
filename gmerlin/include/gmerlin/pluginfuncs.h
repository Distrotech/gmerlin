/*****************************************************************
 * gmerlin - a general purpose multimedia framework and applications
 *
 * Copyright (c) 2001 - 2011 Members of the Gmerlin project
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

int bg_encoder_cb_create_output_file(bg_encoder_callbacks_t * cb,
                                     const char * filename);

int bg_encoder_cb_create_temp_file(bg_encoder_callbacks_t * cb,
                                   const char * filename);

int bg_iw_cb_create_output_file(bg_iw_callbacks_t * cb,
                                const char * filename);

typedef struct
  {
  int timescale;
  int frame_duration;
  } bg_encoder_framerate_t;

#define BG_ENCODER_FRAMERATE_PARAMS \
  { \
    .name      = "default_timescale", \
    .long_name = TRS("Default timescale"), \
    .type      = BG_PARAMETER_INT, \
    .val_default = { .val_i = 25 }, \
    .help_string = TRS("For formats, which support only constant framerates, set the default timescale here"), \
  }, \
  { \
    .name =      "default_frame_duration", \
    .long_name = TRS("Default frame duration"), \
    .type      = BG_PARAMETER_INT, \
    .val_default = { .val_i = 1 }, \
    .help_string = TRS("For formats, which support only constant framerates, set the default frame duration here"), \
  }

int bg_encoder_set_framerate_parameter(bg_encoder_framerate_t * f,
                                       const char * name,
                                       const bg_parameter_value_t * val);

void bg_encoder_set_framerate(const bg_encoder_framerate_t * f,
                              gavl_video_format_t * format);

void
bg_encoder_set_framerate_nearest(const bg_encoder_framerate_t * rate_default,
                                 const bg_encoder_framerate_t * rates_supported,
                                 gavl_video_format_t * format);
