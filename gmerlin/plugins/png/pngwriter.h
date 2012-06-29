/*****************************************************************
 * gmerlin - a general purpose multimedia framework and applications
 *
 * Copyright (c) 2001 - 2012 Members of the Gmerlin project
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

#define BITS_AUTO  0
#define BITS_8     8
#define BITS_16   16

typedef struct
  {
  png_structp png_ptr;
  png_infop   info_ptr;
  int transform_flags;
  FILE * output;
  int bit_mode;
  int compression_level;
  gavl_video_format_t format;

  png_text * text;
  int num_text;
  int dont_force_extension;
  
  } bg_pngwriter_t;

int bg_pngwriter_write_header(void * priv, const char * filename,
                              gavl_video_format_t * format,
                              const gavl_metadata_t * metadata);

int bg_pngwriter_write_image(void * priv, gavl_video_frame_t * frame);

void bg_pngwriter_set_parameter(void * p, const char * name,
                                const bg_parameter_value_t * val);


