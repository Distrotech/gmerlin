/*****************************************************************
 * gmerlin-encoders - encoder plugins for gmerlin
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

#include <faac.h>

typedef struct bg_faac_s bg_faac_t;

bg_faac_t * bg_faac_create(void);

const bg_parameter_info_t * bg_faac_get_parameters(bg_faac_t*);

void bg_faac_set_parameter(bg_faac_t * ctx, const char * name,
                           const bg_parameter_value_t * val);

gavl_audio_sink_t * bg_faac_open(bg_faac_t * ctx,
                                 gavl_compression_info_t * ci,
                                 gavl_audio_format_t * fmt,
                                 gavl_metadata_t * m);

void bg_faac_set_packet_sink(bg_faac_t * ctx,
                             gavl_packet_sink_t * psink);

void bg_faac_destroy(bg_faac_t * ctx);
