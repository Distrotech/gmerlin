/*****************************************************************
 * gmerlin - a general purpose multimedia framework and applications
 *
 * Copyright (c) 2001 - 2010 Members of the Gmerlin project
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

#include <gavl/gavl.h>
#include <alsa/asoundlib.h>

/* For reading, the sample format, num_channels and samplerate must be set */

snd_pcm_t * bg_alsa_open_read(const char * card, gavl_audio_format_t * format,
                              gavl_time_t buffer_time);

/* For writing, the complete format must be set, values will be changed if not compatible */

snd_pcm_t * bg_alsa_open_write(const char * card, gavl_audio_format_t * format,
                               gavl_time_t buffer_time,
                               int * convert_3_4);

/* Builds a parameter array for all available cards */

void bg_alsa_create_card_parameters(bg_parameter_info_t * ret, int record);

