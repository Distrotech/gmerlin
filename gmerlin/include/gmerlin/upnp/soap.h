#include <gmerlin/xmlutils.h>

xmlDocPtr bg_soap_create_request(const char * function, const char * service, int version);
xmlDocPtr bg_soap_create_response(const char * function, const char * service, int version);

/* return the first named node within the Body element */
xmlNodePtr bg_soap_get_function(xmlDocPtr);

xmlNodePtr bg_soap_request_add_argument(xmlDocPtr doc, const char * name);
