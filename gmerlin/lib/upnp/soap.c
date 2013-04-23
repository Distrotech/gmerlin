#include <gmerlin/upnp/soap.h>
#include <gmerlin/utils.h>

#include <string.h>


static xmlDocPtr soap_create(const char * function, const char * service,
                             int version, int response)
  {
  char * tmp_string;
  xmlDocPtr  xml_doc;
  xmlNodePtr xml_env;
  xmlNodePtr xml_body;
  xmlNodePtr xml_action;

  xmlNsPtr soap_ns;
  xmlNsPtr upnp_ns;

  xml_doc = xmlNewDoc((xmlChar*)"1.0");
  xml_env = xmlNewDocRawNode(xml_doc, NULL, (xmlChar*)"Envelope", NULL);
  xmlDocSetRootElement(xml_doc, xml_env);
  soap_ns =
    xmlNewNs(xml_env,
             (xmlChar*)"http://schemas.xmlsoap.org/soap/envelope/",
             (xmlChar*)"s");
  xmlSetNs(xml_env, soap_ns);

  xmlSetNsProp(xml_env, soap_ns, (xmlChar*)"encodingStyle", 
               (xmlChar*)"http://schemas.xmlsoap.org/soap/encoding/");

  xml_body = xmlNewChild(xml_env, soap_ns, (xmlChar*)"Body", NULL);

  if(response)
    {
    tmp_string = bg_sprintf("%sResponse", function);
    xml_action = xmlNewChild(xml_body, NULL, (xmlChar*)tmp_string, NULL);
    free(tmp_string);
    }
  else
    xml_action = xmlNewChild(xml_body, NULL, (xmlChar*)function, NULL);

  tmp_string = bg_sprintf("urn:schemas-upnp-org:service:%s:%d",
                          service, version);

  upnp_ns = xmlNewNs(xml_action, (xmlChar*)tmp_string,  (xmlChar*)"u");  
  free(tmp_string);

  xmlSetNs(xml_action, upnp_ns);

  return xml_doc;
  }

xmlDocPtr bg_soap_create_request(const char * function,
                                 const char * service, int version)
  {
  return soap_create(function, service, version, 0);
  }

xmlDocPtr bg_soap_create_response(const char * function,
                                  const char * service, int version)
  {
  return soap_create(function, service, version, 1);
  }

xmlNodePtr bg_soap_get_function(xmlDocPtr doc)
  {
  xmlNodePtr node = doc->children;

  while(node && !node->name && strcmp((char*)node->name, "Envelope"))
    node = node->next;

  if(!node)
    return 0;

  node = node->children;

  while(node && !node->name && strcmp((char*)node->name, "Body"))
    node = node->next;

  if(!node)
    return 0;

  node = node->children;

  while(node && !node->name)
    node = node->next;

  return node;  
  }

xmlNodePtr bg_soap_request_add_argument(xmlDocPtr doc, const char * name)
  {
  xmlNodePtr ret;
  xmlNodePtr node = bg_soap_get_function(doc);
  ret = xmlNewChild(node, NULL, (xmlChar*)name, NULL);
  xmlSetNs(ret, NULL);
  return ret;
  }

