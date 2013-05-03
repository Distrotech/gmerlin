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

#ifndef __BG_UPNP_DEVICE_DESC_H_
#define __BG_UPNP_DEVICE_DESC_H_

#include <uuid/uuid.h>
#include <gmerlin/xmlutils.h>
#include <gmerlin/bgsocket.h>

// #include <gmerlin/.h>

xmlDocPtr bg_upnp_device_description_create(bg_socket_address_t * addr,
                                            const char * type, int version);

/*
  Required. Short description for end user. Should be localized (cf. ACCEPT-LANGUAGE and CONTENT-
  LANGUAGE headers). Specified by UPnP vendor. String. Should be < 64 characters.
*/

void bg_upnp_device_description_set_name(xmlDocPtr ptr, const char * name);

/*
  Required. Manufacturer's name. May be localized (cf. ACCEPT-LANGUAGE and CONTENT-LANGUAGE headers).
  Specified by UPnP vendor. String. Should be < 64 characters.
*/

void bg_upnp_device_description_set_manufacturer(xmlDocPtr ptr, const char * name);

/*
  Optional. Web site for Manufacturer. May be localized (cf. ACCEPT-LANGUAGE and CONTENT-LANGUAGE
  headers). May be relative to base URL. Specified by UPnP vendor. Single URL.
*/

void bg_upnp_device_description_set_manufacturer_url(xmlDocPtr ptr, const char * name);

/*
  Recommended. Long description for end user. Should be localized (cf. ACCEPT-LANGUAGE and CONTENT-
  LANGUAGE headers). Specified by UPnP vendor. String. Should be < 128 characters.
 */

void bg_upnp_device_description_set_model_description(xmlDocPtr ptr, const char * name);

/*
  Required. Model name. May be localized (cf. ACCEPT-LANGUAGE and CONTENT-LANGUAGE headers).
  Specified by UPnP vendor. String. Should be < 32 characters.
 */

void bg_upnp_device_description_set_model_name(xmlDocPtr ptr, const char * name);

/*
  Recommended. Model number. May be localized (cf. ACCEPT-LANGUAGE and CONTENT-LANGUAGE headers).
  Specified by UPnP vendor. String. Should be < 32 characters.
 */

void bg_upnp_device_description_set_model_number(xmlDocPtr ptr, const char * name);

/*
  Optional. Web site for model. May be localized (cf. ACCEPT-LANGUAGE and CONTENT-LANGUAGE headers).
  May be relative to base URL. Specified by UPnP vendor. Single URL.
*/

void bg_upnp_device_description_set_model_url(xmlDocPtr ptr, const char * name);

/*
  Recommended. Serial number. May be localized (cf. ACCEPT-LANGUAGE and CONTENT-LANGUAGE headers).
  Specified by UPnP vendor. String. Should be < 64 characters.
*/

void bg_upnp_device_description_set_serial_number(xmlDocPtr ptr, const char * name);

/*
  Required. Unique Device Name. Universally-unique identifier for the device, whether root or
  embedded. Must be the same over time for a specific device instance (i.e., must survive reboots).
  Must match the value of the NT header in device discovery messages. Must match the prefix of the
  USN header in all discovery messages. (The section on Discovery explains the NT and USN headers.)
  Must begin with uuid: followed by a UUID suffix
  specified by a UPnP vendor. Single URI.
 */

void bg_upnp_device_description_set_uuid(xmlDocPtr ptr, uuid_t uuid);

/*
  Optional. Universal Product Code. 12-digit, all-numeric code that identifies the consumer
  package. Managed by the Uniform Code Council. Specified by UPnP vendor. Single UPC.
 */

void bg_upnp_device_description_set_upc(xmlDocPtr ptr, const char * name);

void bg_upnp_device_description_add_icon(xmlDocPtr ptr,
                                         const char * mimetype,
                                         int width, int height, int depth, const char * url);

/*
 *  Add a service.
 *  Description URL will be <root>/upnp/<name>/desc.xml
 *  control URL will be     <root>/upnp/<name>/control
 *  event URL will be       <root>/upnp/<name>/event
 */

void bg_upnp_device_description_add_service(xmlDocPtr ptr,
                                            const char * type, int version, const char * name);

#endif // __BG_UPNP_DEVICE_DESC_H_
