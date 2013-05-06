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

#include <gmerlin/upnp/device.h>
// #include <gmerlin/upnp/ssdp.h>

typedef struct bg_upnp_service_s bg_upnp_service_t;

/* We support just a subset of defined variable types */
typedef enum
  {
  BG_UPNP_SV_INT4,
  BG_UPNP_SV_STRING,
  } bg_upnp_sv_type_t;

/* Value for a state variable */

typedef struct
  {
  char * s;
  int i;
  } bg_upnp_sv_val_t;

char * bg_upnp_val_to_string(bg_upnp_sv_type_t type,
                             const bg_upnp_sv_val_t * val);
int bg_upnp_string_to_val(bg_upnp_sv_type_t type,
                          const char * str, bg_upnp_sv_val_t * val);
void bg_upnp_sv_val_free(bg_upnp_sv_val_t * val);
void bg_upnp_sv_val_copy(bg_upnp_sv_type_t type,
                         bg_upnp_sv_val_t * dst,
                         const bg_upnp_sv_val_t * src);

/* Range for a state variable */

typedef struct
  {
  bg_upnp_sv_val_t min;
  bg_upnp_sv_val_t max;
  bg_upnp_sv_val_t step;
  } bg_upnp_sv_range_t;

/* State variable */

// State variable exists only for defining the type of an argument (A_ARG_TYPE_*)
#define BG_UPNP_SV_ARG_ONLY    (1<<0)
#define BG_UPNP_SV_HAS_DEFAULT (1<<1)
#define BG_UPNP_SV_HAS_RANGE   (1<<2)
#define BG_UPNP_SV_EVENT       (1<<3)

typedef struct
  {
  char * name;
  int flags;
  bg_upnp_sv_type_t type;
  bg_upnp_sv_val_t def;
  
  bg_upnp_sv_range_t range;

  int num_allowed;
  int allowed_alloc;
  bg_upnp_sv_val_t * allowed;
  } bg_upnp_sv_desc_t;

void bg_upnp_sv_desc_set_default(bg_upnp_sv_desc_t * d, bg_upnp_sv_val_t * val);
void bg_upnp_sv_desc_set_range(bg_upnp_sv_desc_t * d, bg_upnp_sv_val_t * min,
                               bg_upnp_sv_val_t * max, bg_upnp_sv_val_t * step);
void bg_upnp_sv_desc_add_allowed(bg_upnp_sv_desc_t * d, bg_upnp_sv_val_t * val);
void bg_upnp_sv_desc_free(bg_upnp_sv_desc_t * d);


/* Argument for an action */

#define BG_UPNP_ARG_RETVAL (1<<0) // Return value

typedef struct
  {
  char * name;
  char * rsv_name;         // Related state variable  
  const bg_upnp_sv_desc_t * rsv;
  int flags;
  int id; // For lookup without second strcmp
  } bg_upnp_sa_arg_desc_t;

void bg_upnp_arg_desc_free(bg_upnp_sa_arg_desc_t * d);

/* Description of a service action */

typedef int (*bg_upnp_service_func)(bg_upnp_service_t * s);

typedef struct
  {
  bg_upnp_service_func func;
  
  char * name;
  int num_args_in;
  int args_in_alloc;
  bg_upnp_sa_arg_desc_t * args_in;

  int num_args_out;
  int args_out_alloc;
  bg_upnp_sa_arg_desc_t * args_out;
  } bg_upnp_sa_desc_t;

void
bg_upnp_sa_desc_add_arg_in(bg_upnp_sa_desc_t * d, const char * name,
                           const char * rsv_name, int flags, int id);

void
bg_upnp_sa_desc_add_arg_out(bg_upnp_sa_desc_t * d, const char * name,
                            const char * rsv_name, int flags, int id);

void bg_upnp_sa_desc_free(bg_upnp_sa_desc_t * d);

const bg_upnp_sa_arg_desc_t *
bg_upnp_sa_desc_in_arg_by_name(const bg_upnp_sa_desc_t * d, const char * name);

const bg_upnp_sa_arg_desc_t *
bg_upnp_sa_desc_out_arg_by_name(const bg_upnp_sa_desc_t * d, const char * name);


/* Description of a service */
typedef struct
  {
  int num_sv;
  int sv_alloc;
  bg_upnp_sv_desc_t * sv; /* State variables */

  int num_sa;
  int sa_alloc;
  bg_upnp_sa_desc_t * sa;  /* Actions */
  } bg_upnp_service_desc_t;

bg_upnp_sv_desc_t *
bg_upnp_service_desc_add_sv(bg_upnp_service_desc_t * d, const char * name,
                            int flags, bg_upnp_sv_type_t type);

bg_upnp_sa_desc_t *
bg_upnp_service_desc_add_action(bg_upnp_service_desc_t * d,
                                const char * name, bg_upnp_service_func func);

void bg_upnp_service_desc_free(bg_upnp_service_desc_t * d);

const bg_upnp_sv_desc_t *
bg_upnp_service_desc_sv_by_name(bg_upnp_service_desc_t * d, const char * name);

const bg_upnp_sa_desc_t *
bg_upnp_service_desc_sa_by_name(bg_upnp_service_desc_t * d, const char * name);

char * bg_upnp_service_desc_2_xml(bg_upnp_service_desc_t * d);

int bg_upnp_service_desc_resolve_refs(bg_upnp_service_desc_t * d);

/* */

typedef struct
  {
  bg_upnp_sv_val_t val;
  const bg_upnp_sa_arg_desc_t * desc;
  } bg_upnp_soap_arg_t;

typedef struct
  {
  bg_upnp_soap_arg_t * args_in;
  int num_args_in;
  int args_in_alloc;

  bg_upnp_soap_arg_t * args_out;
  int num_args_out;
  int args_out_alloc;
  
  const bg_upnp_sa_desc_t * action;
  
  xmlDocPtr result; // Result (or fault description) goes here
  } bg_upnp_soap_request_t;

int
bg_upnp_soap_request_from_xml(bg_upnp_service_t * s,
                              const char * xml, int len);

char *
bg_upnp_soap_response_to_xml(bg_upnp_service_t * s, int * len);

void bg_upnp_soap_request_cleanup(bg_upnp_soap_request_t * req);

/* Functions called by the service actions */

int
bg_upnp_service_get_arg_in_int(bg_upnp_soap_request_t * req,
                               int id);
const char *
bg_upnp_service_get_arg_in_string(bg_upnp_soap_request_t * req,
                                  int id);

void
bg_upnp_service_set_arg_out_int(bg_upnp_soap_request_t * req,
                                int id, int val);
void
bg_upnp_service_set_arg_out_string(bg_upnp_soap_request_t * req,
                                   int id, char * val);

typedef struct
  {
  void (*destroy)(void*priv);
  } bg_upnp_service_funcs_t;

/* */

typedef struct
  {
  bg_upnp_sv_val_t val;
  const bg_upnp_sv_desc_t * sv;
  } bg_upnp_event_var_t;

typedef struct
  {
  char uuid[37];
  char * url;   // For delivering
  uint32_t key; // Counter
  gavl_time_t expire_time;
  } bg_upnp_event_subscriber_t;

struct bg_upnp_service_s
  {
  /* Description */
  bg_upnp_service_desc_t desc;
  const bg_upnp_service_funcs_t * funcs;  

  char * name;        // For finding the service from the http request
  char * description; // xml
  char * type;        
  int version;
  
  /* Pointer to the device we belong to */
  bg_upnp_device_t * dev;

  /* Evented variable */
  bg_upnp_event_var_t * event_vars;
  int num_event_vars;
  
  /* Event subscribers */
  bg_upnp_event_subscriber_t * es;
  int num_es;
  int es_alloc;

  /* Request: Allocted once and used for all actions */
  bg_upnp_soap_request_t req;
  
  };

void bg_upnp_service_init(bg_upnp_service_t * ret,
                          const char * name,
                          const char * type, int version);

int
bg_upnp_service_start(bg_upnp_service_t * s);

int
bg_upnp_service_handle_request(bg_upnp_service_t * s, int fd,
                               const char * method,
                               const char * path,
                               const gavl_metadata_t * header);

int
bg_upnp_service_handle_event_request(bg_upnp_service_t * s, int fd,
                                     const char * method,
                                     const gavl_metadata_t * header);

int
bg_upnp_service_handle_action_request(bg_upnp_service_t * s, int fd,
                                      const char * method,
                                      const gavl_metadata_t * header);

/* ContentDirectory:1 */

void bg_upnp_service_init_content_directory(bg_upnp_service_t * ret,
                                            const char * name,
                                            bg_db_t * db);

/* ConnectionManager:1 */

void bg_upnp_service_init_connection_manager(bg_upnp_service_t * ret,
                                             const char * name);

