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

#include <config.h>
#include <gmerlin/plugin.h>
#include <gmerlin/translation.h>

#include <pulse/simple.h>
#include <pulse/error.h>

typedef struct
  {
  struct pa_simple *pa;
  char *server, *dev;
  int record;
  gavl_audio_format_t format;
  int block_align;
  int64_t sample_count;

  int num_channels;
  int bytes_per_sample;
  int samplerate;
  } bg_pa_t;

int bg_pa_open(bg_pa_t *, int record);
void bg_pa_close(void *);

void * bg_pa_create();
void bg_pa_destroy(void *);
