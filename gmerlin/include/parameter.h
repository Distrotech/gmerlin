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

#ifndef __BG_PARAMETER_H_
#define __BG_PARAMETER_H_

#include <libxml/tree.h>
#include <libxml/parser.h>

#include <gavl/gavl.h>

/**  \defgroup parameter Parameter description
 *
 *   Parameters are universal data containers, which
 *   are the basis for all configuration mechanisms.
 *
 *   A configurable module foo, should provide at least 2 functions.
 *   One, which lets the application get a null-terminated array of parameter description
 *   and one of type \ref bg_set_parameter_func_t. It's up to the module, if the parameter array
 *   is allocated per instance or if it's just a static array. Some parameters (e.g. window
 *   coordinates) are not configured by a dialog. Instead, they are changed by the module.
 *   For these parameters, set \ref BG_PARAMETER_HIDE_DIALOG for the flags and provide another
 *   function of type \ref bg_get_parameter_func_t, which lets the core read the updated value.
 */

/* Universal Parameter setting mechanism */

/** \ingroup parameter
 *  \brief Parameter type
 *
 *  These define both the data type and
 *  the appearance of an eventual configuration widget.
 */

typedef enum
  {
    BG_PARAMETER_SECTION, //!< Dummy type. It contains no data but acts as a separator in notebook style configuration windows
    BG_PARAMETER_CHECKBUTTON, //!< Bool
    BG_PARAMETER_INT,         //!< Integer spinbutton
    BG_PARAMETER_FLOAT,       //!< Float spinbutton
    BG_PARAMETER_SLIDER_INT,  //!< Integer slider
    BG_PARAMETER_SLIDER_FLOAT, //!< Float slider
    BG_PARAMETER_STRING,      //!< String (one line only)
    BG_PARAMETER_STRING_HIDDEN, //!< Encrypted string (displays as ***)
    BG_PARAMETER_STRINGLIST,  //!< Popdown menu with string values
    BG_PARAMETER_COLOR_RGB,   //!< RGB Color
    BG_PARAMETER_COLOR_RGBA,  //!< RGBA Color
    BG_PARAMETER_FONT,        //!< Font (contains fontconfig compatible fontname)
    BG_PARAMETER_DEVICE,      //!< Device
    BG_PARAMETER_FILE,        //!< File
    BG_PARAMETER_DIRECTORY,   //!< Directory
    BG_PARAMETER_MULTI_MENU,  //!< Menu with config- and infobutton
    BG_PARAMETER_MULTI_LIST,  //!< List with config- and infobutton
    BG_PARAMETER_MULTI_CHAIN, //!< Several subitems (including suboptions) can be arranged in a chain
    BG_PARAMETER_TIME         //!< Time
  } bg_parameter_type_t;

/** \ingroup parameter
 * \brief Container for a parameter value
 */

typedef union
  {
  double  val_f; //!< For BG_PARAMETER_FLOAT and BG_PARAMETER_SLIDER_FLOAT
  int     val_i; //!< For BG_PARAMETER_CHECKBUTTON, BG_PARAMETER_INT and BG_PARAMETER_SLIDER_INT
  char *  val_str; //!< For BG_PARAMETER_STRING, BG_PARAMETER_STRING_HIDDEN, BG_PARAMETER_STRINGLIST, BG_PARAMETER_FONT, BG_PARAMETER_FILE, BG_PARAMETER_DIRECTORY, BG_PARAMETER_MULTI_MENU, BG_PARAMETER_MULTI_LIST
  float val_color[4];  //!< RGBA values (0.0..1.0) for BG_PARAMETER_COLOR_RGB and BG_PARAMETER_COLOR_RGBA 
  gavl_time_t val_time; //!< For BG_PARAMETER_TIME
  } bg_parameter_value_t;

/* Flags */

/** \ingroup parameter
 */

#define BG_PARAMETER_SYNC         (1<<0) //!< Apply the value whenever the widgets value changes

/** \ingroup parameter
 */

#define BG_PARAMETER_HIDE_DIALOG  (1<<1) //!< Don't make a configuration widget (for objects, which change values themselves)

/** \ingroup parameter
 *  \brief Parmeter description
 *
 *  Usually, parameter infos are passed around as NULL-terminated arrays.
 */

typedef struct bg_parameter_info_s
  {
  char * name; //!< Unique name. Can contain alphanumeric characters plus underscore.
  char * long_name; //!< Long name (for labels)
  char * opt; //!< ultrashort name (optional for commandline). If missing, the name will be used.

  char * gettext_domain; //!< First argument for bindtextdomain(). In an array, it's valid for subsequent entries too.
  char * gettext_directory; //!< Second argument for bindtextdomain(). In an array, it's valid for subsequent entries too.
  
  bg_parameter_type_t type; //!< Type

  int flags; //!< Mask of BG_PARAMETER_* defines
  
  bg_parameter_value_t val_default; //!< Default value
  bg_parameter_value_t val_min; //!< Minimum value (for arithmetic types)
  bg_parameter_value_t val_max; //!< Maximum value (for arithmetic types)
  
  /* Names which can be passed to set_parameter (NULL terminated) */

  char const * const * multi_names; //!< Names for multi option parameters (NULL terminated)

  /* Long names are optional, if they are NULL,
     the short names are used */

  char const * const * multi_labels; //!< Optional labels for multi option parameters
  char const * const * multi_descriptions; //!< Optional descriptions (will be displayed by info buttons)
    
  /*
   *  These are parameters for each codec.
   *  The name members of these MUST be unique with respect to the rest
   *  of the parameters passed to the same set_parameter func
   */

  struct bg_parameter_info_s const * const * multi_parameters; //!< Parameters for each option. The name members of these MUST be unique with respect to the rest of the parameters passed to the same set_parameter func
  
  int num_digits; //!< Number of digits for floating point parameters
  
  char * help_string; //!< Help strings for tooltips or --help option 
  
  char ** multi_names_nc; //!< When allocating dynamically, use this instead of multi_names and call \ref bg_parameter_info_set_const_ptrs at the end

  char ** multi_labels_nc; //!< When allocating dynamically, use this instead of multi_labels and call \ref bg_parameter_info_set_const_ptrs at the end

  char ** multi_descriptions_nc; //!< When allocating dynamically, use this instead of multi_descriptions and call \ref bg_parameter_info_set_const_ptrs at the end 

  struct bg_parameter_info_s ** multi_parameters_nc; //!< When allocating dynamically, use this instead of multi_parameters and call \ref bg_parameter_info_set_const_ptrs at the end 

  } bg_parameter_info_t;

/* Prototype for setting/getting parameters */

/*
 *  NOTE: All applications MUST call a bg_set_parameter_func with
 *  a NULL name argument to signal, that all parameters are set now
 */

/** \ingroup parameter
 *  \brief Generic prototype for setting parameters in a module
 *  \param data Instance
 *  \param name Name of the parameter
 *  \param v Value
 *
 *  This function is usually called from "Apply" buttons in config dialogs.
 *  It's called subsequently for all defined püarameters. After that, it *must*
 *  be called with a NULL argument for the name to signal, that all parameters
 *  are set. Modules can do some additional setup stuff then. If not, the name == NULL
 *  case must be handled nevertheless.
 */

typedef void (*bg_set_parameter_func_t)(void * data, const char * name,
                                        const bg_parameter_value_t * v);

/** \ingroup parameter
 *  \brief Generic prototype for getting parameters from a module
 *  \param data Instance
 *  \param name Name of the parameter
 *  \param v Value
 *
 *  \returns 1 if a parameter was found and set, 0 else.
 *
 *  Provide this function, if your module changes parameters by itself.
 *  Set the \ref BG_PARAMETER_HIDE_DIALOG to prevent building config
 *  dialogs for those parameters.
 */

typedef int (*bg_get_parameter_func_t)(void * data, const char * name,
                                       bg_parameter_value_t * v);

/** \ingroup parameter
 *  \brief Copy a NULL terminated parameter array
 *  \param src Source array
 *
 *  \returns A newly allocated parameter array, whose contents are copied from src.
 *
 *  Use \ref bg_parameter_info_destroy_array to free the returned array.
 */

bg_parameter_info_t *
bg_parameter_info_copy_array(const bg_parameter_info_t * src);

/** \ingroup parameter
 *  \brief Set the const pointers of a dynamically allocated parameter info
 *  \param info A parameter info
 *
 *  This copied the adresses of the *_nc pointers to their constant equivalents.
 *  Use this for each parameter in routines, which dynamically allocate parameter infos.
 */

void
bg_parameter_info_set_const_ptrs(bg_parameter_info_t * info);


/** \ingroup parameter
 *  \brief Copy a single parameter description
 *  \param dst Destination parameter description
 *  \param src Source parameter description
 *
 *  Make sure, that src is memset to 0 before calling this.
 */

void bg_parameter_info_copy(bg_parameter_info_t * dst,
                            const bg_parameter_info_t * src);

/** \ingroup parameter
 *  \brief Free a NULL terminated parameter array
 *  \param info Parameter array
 */

void bg_parameter_info_destroy_array(bg_parameter_info_t * info);

/** \ingroup parameter
 *  \brief Copy a parameter value
 *  \param dst Destination
 *  \param src Source
 *  \param info Parameter description
 *
 *  Make sure, that dst is either memset to 0 or contains
 *  data, which was created by \ref bg_parameter_value_copy
 */

void bg_parameter_value_copy(bg_parameter_value_t * dst,
                             const bg_parameter_value_t * src,
                             const bg_parameter_info_t * info);

/** \ingroup parameter
 *  \brief Free a parameter value
 *  \param val A parameter value
 *  \param type Type of the parameter
 */

void bg_parameter_value_free(bg_parameter_value_t * val,
                             bg_parameter_type_t type);


/** \ingroup parameter
 *  \brief Concatenate multiple arrays into one
 *  \param srcs NULL terminated array of source arrays
 *  \returns A newly allocated array
 */


bg_parameter_info_t *
bg_parameter_info_concat_arrays(bg_parameter_info_t const ** srcs);

/** \ingroup parameter
 *  \brief Get the index for a multi-options parameter
 *  \param info A parameter info
 *  \param val The value
 *  \returns The index of val in the multi_names array
 *
 *  If val does not occur in the multi_names[] array,
 *  try the default value. If that fails as well, return 0.
 */

int bg_parameter_get_selected(const bg_parameter_info_t * info,
                              const char * val);


/** \ingroup parameter
 *  \brief Find a parameter info
 *  \param info A parameter info
 *  \param name The name of the the parameter
 *  \returns Parameter info matching name or NULL
 *
 *  This function looks for a parameter info with the
 *  given name in an array or parameters. Sub-parameters
 *  are also searched.
 */

const bg_parameter_info_t *
bg_parameter_find(const bg_parameter_info_t * info,
                  const char * name);


/** \ingroup parameter
 *  \brief Convert a libxml2 node into a parameter array
 *  \param xml_doc Pointer to the xml document
 *  \param xml_parameters Pointer to the xml node containing the parameters
 *  \returns A newly allocated array
 *
 *  See the libxml2 documentation for more infos
 */


bg_parameter_info_t * bg_xml_2_parameters(xmlDocPtr xml_doc,
                                          xmlNodePtr xml_parameters);

/** \ingroup parameter
 *  \brief Convert a parameter array into a libxml2 node
 *  \param info Parameter array
 *  \param xml_parameters Pointer to the xml node for the parameters
 *
 *  See the libxml2 documentation for more infos
 */


void
bg_parameters_2_xml(const bg_parameter_info_t * info, xmlNodePtr xml_parameters);



#endif /* __BG_PARAMETER_H_ */

