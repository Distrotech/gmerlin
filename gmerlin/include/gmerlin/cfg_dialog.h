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

#ifndef __BG_CFG_DIALOG_H_
#define __BG_CFG_DIALOG_H_

#include <gmerlin/cfg_registry.h>

/* Opaque pointer, will look different with all toolkits */

typedef struct bg_dialog_s bg_dialog_t;

/* These function prototypes must be defined by the toolkit */

/* Create a dialog from simple configuration data */

bg_dialog_t * bg_dialog_create(bg_cfg_section_t * config,
                               bg_set_parameter_func_t set_param,
                               void * callback_data,
                               const bg_parameter_info_t * info,
                               const char * title);

/* Create a dialog, add sections later */

bg_dialog_t * bg_dialog_create_multi(const char * label);

/* Add sections to a dialog */

void bg_dialog_add(bg_dialog_t *d,
                   const char * label,
                   bg_cfg_section_t * section,
                   bg_set_parameter_func_t set_param,
                   void * callback_data,
                   const bg_parameter_info_t * info);

/* Add child notebook to the dialog. You can pass the returned
   void pointer to subsequent calls of bg_dialog_add_child */

void * bg_dialog_add_parent(bg_dialog_t *d, void * parent, const char * label);

void bg_dialog_add_child(bg_dialog_t *d, void * parent,
                         const char * label,
                         bg_cfg_section_t * section,
                         bg_set_parameter_func_t set_param,
                         void * callback_data,
                         const bg_parameter_info_t * info);

void bg_dialog_show(bg_dialog_t *, void * parent);

void bg_dialog_destroy(bg_dialog_t *);
#endif // __BG_CFG_DIALOG_H_
