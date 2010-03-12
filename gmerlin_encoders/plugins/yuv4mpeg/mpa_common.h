/*****************************************************************
 * gmerlin-encoders - encoder plugins for gmerlin
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

/* Common defintions and routines for driving mp2enc */

typedef struct
  {
  int bitrate;      /* -b (kbps) */
  int layer;        /* -l */
  int vcd; /* -V */

  gavl_audio_format_t format;
  bg_subprocess_t * mp2enc;
  
  sigset_t oldset;
  const gavl_compression_info_t * ci;
  FILE * out;
  } bg_mpa_common_t;

const bg_parameter_info_t * bg_mpa_get_parameters();

/* Must pass a bg_mpa_common_t for parameters */
void bg_mpa_set_parameter(void * data, const char * name,
                          const bg_parameter_value_t * val);

void bg_mpa_set_format(bg_mpa_common_t * com, const gavl_audio_format_t * format);
void bg_mpa_get_format(bg_mpa_common_t * com, gavl_audio_format_t * format);

int bg_mpa_start(bg_mpa_common_t * com, const char * filename);

int bg_mpa_write_audio_frame(bg_mpa_common_t * com, gavl_audio_frame_t * frame);

int bg_mpa_close(bg_mpa_common_t * com);

const char * bg_mpa_get_extension(bg_mpa_common_t * mpa);

void bg_mpa_set_ci(bg_mpa_common_t * com, const gavl_compression_info_t * ci);

int bg_mpa_write_audio_packet(bg_mpa_common_t * com,
                              gavl_packet_t * packet);
