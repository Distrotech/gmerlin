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

// #define MAKE_VERSION(maj,min,pat) ((maj<<16)|(min<<8)|pat)
// #define BGAV_FLAC_VERSION_INT MAKE_VERSION(BGAV_FLAC_MAJOR,BGAV_FLAC_MINOR,BGAV_FLAC_PATCHLEVEL)

#include <FLAC/stream_encoder.h> /* Flac in Ogg */
#include <FLAC/metadata.h>

#define BG_FLAC_HEADER_SIZE (4+38)

typedef struct bg_flac_s bg_flac_t;


bg_flac_t * bg_flac_create();

const bg_parameter_info_t * bg_flac_get_parameters(void);
  
void bg_flac_set_parameter(void * data, const char * name, const bg_parameter_value_t * val);

gavl_packet_sink_t *
bg_flac_start_compressed(bg_flac_t * flac,
                         gavl_audio_format_t * fmt, gavl_compression_info_t * ci,
                         gavl_metadata_t * stream_metadata);

gavl_audio_sink_t *
bg_flac_start_uncompressed(bg_flac_t * flac,
                           gavl_audio_format_t * fmt, gavl_compression_info_t * ci,
                           gavl_metadata_t * stream_metadata);

#if 0
int bg_flac_start(bg_flac_t * flac,
                  gavl_audio_format_t * fmt, gavl_compression_info_t * ci,
                  gavl_metadata_t * stream_metadata);
#endif

// gavl_audio_sink_t * bg_flac_get_audio_sink(bg_flac_t * flac);
// gavl_packet_sink_t * bg_flac_get_packet_sink(bg_flac_t * flac);

// int bg_flac_encode_audio_frame(bg_flac_t * flac, gavl_audio_frame_t * frame);

void bg_flac_free(bg_flac_t * flac);

void bg_flac_set_callbacks(bg_flac_t * flac,
                           int (*streaminfo_callback)(void*, uint8_t *, int),
                           void * priv);

void bg_flac_set_sink(bg_flac_t * flac, gavl_packet_sink_t * psink);
