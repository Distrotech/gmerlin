/*****************************************************************
 
  cfg_registry.h
 
  Copyright (c) 2003-2004 by Burkhard Plaum - plaum@ipf.uni-stuttgart.de
 
  http://gmerlin.sourceforge.net
 
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
 
  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111, USA.
 
*****************************************************************/

#ifndef __BG_CFG_REGISTRY_H_
#define __BG_CFG_REGISTRY_H_

#include "parameter.h"

#include <libxml/tree.h>
#include <libxml/parser.h>


typedef enum
  {
    BG_CFG_INT,
    BG_CFG_FLOAT,
    BG_CFG_STRING,
    BG_CFG_COLOR,
    BG_CFG_TIME /* int64 */
  } bg_cfg_type_t;

typedef struct bg_cfg_item_s     bg_cfg_item_t;
typedef struct bg_cfg_section_s  bg_cfg_section_t;
typedef struct bg_cfg_registry_s bg_cfg_registry_t;

bg_cfg_registry_t * bg_cfg_registry_create();
void bg_cfg_registry_destroy(bg_cfg_registry_t *);

/* cfg_xml.c */

void bg_cfg_registry_load(bg_cfg_registry_t *, const char * filename);
void bg_cfg_registry_save(bg_cfg_registry_t *, const char * filename);

/* The name and xml tag of the section must be set before */

void bg_cfg_section_2_xml(bg_cfg_section_t * section, xmlNodePtr xml_section);

void bg_cfg_xml_2_section(xmlDocPtr xml_doc, xmlNodePtr xml_section,
                          bg_cfg_section_t * cfg_section);

/*
 *  Path looks like "section:subsection:subsubsection"
 */

bg_cfg_section_t * bg_cfg_registry_find_section(bg_cfg_registry_t *,
                                                const char * path);

bg_cfg_section_t * bg_cfg_section_find_subsection(bg_cfg_section_t *,
                                                  const char * name);

/* 
 *  Create/destroy config sections
 */

bg_cfg_section_t * bg_cfg_section_create(const char * name);

bg_cfg_section_t *
bg_cfg_section_create_from_parameters(const char * name,
                                      bg_parameter_info_t * parameters);
                        

void bg_cfg_section_destroy(bg_cfg_section_t * s);

bg_cfg_section_t * bg_cfg_section_copy(bg_cfg_section_t * src);

/*
 *  Get/Set section names
 */

const char * bg_cfg_section_get_name(bg_cfg_section_t * s);
void bg_cfg_section_set_name(bg_cfg_section_t * s, const char * name);


/*
 *  Get/Set values
 */

void bg_cfg_section_set_parameter(bg_cfg_section_t * section,
                                  bg_parameter_info_t * info,
                                  bg_parameter_value_t * value);

void bg_cfg_section_get_parameter(bg_cfg_section_t * section,
                                  bg_parameter_info_t * info,
                                  bg_parameter_value_t * value);

void bg_cfg_section_set_defaults(bg_cfg_section_t * section,
                                 bg_parameter_info_t * info);

/* Delete a subsection (useful for cleaning up */

void bg_cfg_section_delete_subsection(bg_cfg_section_t * section,
                                      bg_cfg_section_t * subsection);


/*
 *  Type specific get/set functions, which don't require
 *  an info structure
 */

void bg_cfg_section_set_parameter_int(bg_cfg_section_t * section,
                                      const char * name, int value);
void bg_cfg_section_set_parameter_float(bg_cfg_section_t * section,
                                        const char * name, float value);
void bg_cfg_section_set_parameter_string(bg_cfg_section_t * section,
                                         const char * name, const char * value);

/* Get parameter values, return 0 if no such entry */

int bg_cfg_section_get_parameter_int(bg_cfg_section_t * section,
                                      const char * name, int * value);

int bg_cfg_section_get_parameter_float(bg_cfg_section_t * section,
                                       const char * name, float * value);

int bg_cfg_section_get_parameter_string(bg_cfg_section_t * section,
                                        const char * name, const char ** value);


/* Apply all values found in the parameter info */

void bg_cfg_section_apply(bg_cfg_section_t * section,
                          bg_parameter_info_t * infos,
                          bg_set_parameter_func func,
                          void * callback_data);


void bg_cfg_section_get(bg_cfg_section_t * section,
                        bg_parameter_info_t * infos,
                        bg_get_parameter_func func,
                        void * callback_data);

int bg_cfg_section_has_subsection(bg_cfg_section_t * section,
                                  const char * name);

#endif /* __BG_CFG_REGISTRY_H_ */
