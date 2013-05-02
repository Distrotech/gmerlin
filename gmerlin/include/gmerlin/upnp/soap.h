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

#include <gmerlin/xmlutils.h>

typedef struct
  {
  const gavl_metadata_t * req_hdr;
  gavl_metadata_t res_hdr;
  
  xmlDocPtr request;
  xmlDocPtr response;
  } bg_soap_t;

void bg_soap_init(bg_soap_t*);
void bg_soap_read_request(bg_soap_t*);
void bg_soap_write_response(bg_soap_t*);
void bg_soap_free(bg_soap_t*);

xmlDocPtr bg_soap_create_request(const char * function, const char * service, int version);
xmlDocPtr bg_soap_create_response(const char * function, const char * service, int version);

/* return the first named node within the Body element */
xmlNodePtr bg_soap_get_function(xmlDocPtr);

xmlNodePtr bg_soap_request_add_argument(xmlDocPtr doc, const char * name);

char * bg_soap_request_get_arg_string(xmlDocPtr doc, const char * name);
int bg_soap_request_get_arg_int(xmlDocPtr doc, const char * name, int * ret);

void bg_soap_request_add_arg_string(xmlDocPtr doc, const char * name, const char * val);
int bg_soap_request_add_arg_int(xmlDocPtr doc, const char * name, int * ret);
