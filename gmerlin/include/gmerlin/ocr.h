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

#ifndef __BG_OCR_H_
#define __BG_OCR_H_

#include <gmerlin/pluginregistry.h>

typedef struct bg_ocr_s bg_ocr_t;

bg_ocr_t * bg_ocr_create(bg_plugin_registry_t * plugin_reg);
const bg_parameter_info_t * bg_ocr_get_parameters();
int bg_ocr_set_parameter(void * ocr, const char * name,
                          const bg_parameter_value_t * val);
int bg_ocr_init(bg_ocr_t *,
                const gavl_video_format_t * format,
                const char * language);
int bg_ocr_run(bg_ocr_t *,
               const gavl_video_format_t * format,
               gavl_video_frame_t * frame,
               char ** ret);

void bg_ocr_destroy(bg_ocr_t *);

#endif // __BG_OCR_H_
