/*****************************************************************
 
  parameter.h
 
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

#ifndef __BG_PARAMETER_H_
#define __BG_PARAMETER_H_

#include <libxml/tree.h>
#include <libxml/parser.h>

#include <gavl/gavl.h>

/* Universal Parameter setting mechanism */

/* Parameter type: These define both the data type and
 * the appearance of the configuration widget
 */

typedef enum
  {
    BG_PARAMETER_SECTION,
    BG_PARAMETER_CHECKBUTTON,
    BG_PARAMETER_INT,
    BG_PARAMETER_FLOAT,
    BG_PARAMETER_SLIDER_INT,
    BG_PARAMETER_SLIDER_FLOAT,
    BG_PARAMETER_STRING,
    BG_PARAMETER_STRING_HIDDEN,
    BG_PARAMETER_STRINGLIST,
    BG_PARAMETER_COLOR_RGB,
    BG_PARAMETER_COLOR_RGBA,
    BG_PARAMETER_FONT,
    BG_PARAMETER_DEVICE,
    BG_PARAMETER_FILE,
    BG_PARAMETER_DIRECTORY,
    BG_PARAMETER_MULTI_MENU,
    BG_PARAMETER_MULTI_LIST,
    BG_PARAMETER_TIME
  } bg_parameter_type_t;

/* Container for a parameter value */

typedef union
  {
  float   val_f;
  int     val_i;
  char *  val_str;
  float * val_color;  /* RGBA Color */
  gavl_time_t val_time;
  } bg_parameter_value_t;

/* Flags */

/* Apply the value whenever the widgets value changes */
#define BG_PARAMETER_SYNC         (1<<0)

/*
 *  Don't make a configuration widget (for objects,
 *  which change values themselves)
 */

#define BG_PARAMETER_HIDE_DIALOG  (1<<1)

typedef struct bg_parameter_info_s
  {
  char * name;
  char * long_name;
  char * opt; /* For commandline */
  
  bg_parameter_type_t type;

  int flags;
  
  bg_parameter_value_t val_default;
  bg_parameter_value_t val_min;
  bg_parameter_value_t val_max;
  
  /* Names which can be passed to set_parameter (NULL terminated) */

  char ** multi_names;

  /* Long names are optional, if they are NULL,
     the short names are used */

  char ** multi_labels;
  char ** multi_descriptions;
    
  /*
   *  These are parameters for each codec.
   *  The name members of these MUST be unique with respect to the rest
   *  of the parameters passed to the same set_parameter func
   */

  struct bg_parameter_info_s ** multi_parameters;
  
  /* For floating point parameters */
  
  int num_digits;
  
  /* For tooltips and such */
  char * help_string;
  } bg_parameter_info_t;

/* Prototype for setting/getting parameters */

/*
 *  NOTE: All applications MUST call a bg_set_parameter_func with
 *  a NULL name argument to signal, that all parameters are set now
 */

typedef void (*bg_set_parameter_func)(void * data, char * name,
                                      bg_parameter_value_t * v);

typedef int (*bg_get_parameter_func)(void * data, char * name,
                                     bg_parameter_value_t * v);


bg_parameter_info_t *
bg_parameter_info_copy_array(const bg_parameter_info_t *);

void bg_parameter_info_copy(bg_parameter_info_t * dst,
                            const bg_parameter_info_t * src);

void bg_parameter_info_destroy_array(bg_parameter_info_t * info);

void bg_parameter_value_copy(bg_parameter_value_t * dst,
                             const bg_parameter_value_t * src,
                             bg_parameter_info_t * info);


bg_parameter_info_t *
bg_parameter_info_merge_arrays(bg_parameter_info_t ** srcs);

bg_parameter_info_t * bg_xml_2_parameters(xmlDocPtr xml_doc,
                                          xmlNodePtr xml_parameters);

void bg_parameters_2_xml(bg_parameter_info_t * info, xmlNodePtr xml_parameters);





#endif /* __BG_PARAMETER_H_ */

