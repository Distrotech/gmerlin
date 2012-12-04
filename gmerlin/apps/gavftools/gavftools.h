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
#include <gmerlin/pluginregistry.h>
#include <gmerlin/utils.h>
#include <gmerlin/cmdline.h>
#include <gmerlin/log.h>
#include <gmerlin/translation.h>
#include <gmerlin/bgplug.h>
#include <gmerlin/mediaconnector.h>

extern bg_plugin_registry_t * plugin_reg;
extern bg_cfg_registry_t * cfg_reg;

void gavftools_init_registries();

/* Program */

typedef struct bg_program_s bg_program_t;

bg_program_t * bg_program_create();
void bg_program_destroy(bg_program_t *);

void bg_program_add_audio_stream(bg_program_t *,
                                 gavl_audio_source_t * asrc,
                                 gavl_packet_source_t * psrc,
                                 const gavl_metadata_t * m);

void bg_program_add_video_stream(bg_program_t *,
                                 gavl_video_source_t * vsrc,
                                 gavl_packet_source_t * psrc,
                                 const gavl_metadata_t * m);

void bg_program_add_text_stream(bg_program_t *,
                                gavl_packet_source_t * psrc,
                                uint32_t timescale,
                                const gavl_metadata_t * m);

int bg_program_num_audio_streams(bg_program_t *);
int bg_program_num_video_streams(bg_program_t *);
int bg_program_num_text_streams(bg_program_t *);

/* So one iteration */
int bg_program_process(bg_program_t *);

void bg_program_connect_src_plug(bg_program_t *, bg_plug_t * p);
void bg_program_connect_dst_plug(bg_program_t *, bg_plug_t * p);
