#include <gmerlin/upnp/soap.h>
#include <gmerlin/utils.h>

#include <string.h>
#include <upnp_service.h>


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
  xmlNodePtr node;

  if(!(node = bg_xml_find_doc_child(doc, "Envelope")) ||
     !(node = bg_xml_find_node_child(node, "Body")) ||
     !(node = bg_xml_find_next_node_child(node, NULL)))
    return NULL;
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

xmlNodePtr bg_soap_request_next_argument(xmlNodePtr function, xmlNodePtr arg)
  {
  return bg_xml_find_next_node_child(function, arg);
  }

int
bg_upnp_soap_request_from_xml(bg_upnp_service_t * s,
                              const char * xml, int len)
  {
  xmlDocPtr doc;
  xmlNodePtr function;
  xmlNodePtr arg;
  int i;
  
  doc = xmlParseMemory(xml, len);
  if(!doc || (function = bg_soap_get_function(doc)))
    {
    /* Error */
    }

  s->req.action =
    bg_upnp_service_desc_sa_by_name(&s->desc, (char*)function->name);

  if(s->req.action)
    {
    /* Error */
    // 401 Invalid Action 
    
    }
  
  arg = NULL;

  while((arg = bg_soap_request_next_argument(function, arg)))
    {
    if(s->req.num_args_in == s->req.action->num_args_in)
      break;
    
    fprintf(stderr, "Got arg: %s, value: %s\n", arg->name,
            bg_xml_node_get_text_content(arg));
    
    s->req.args_in[s->req.num_args_in].desc = 
      bg_upnp_sa_desc_in_arg_by_name(s->req.action, (char*)arg->name);
    
    if(s->req.args_in[s->req.num_args_in].desc)
      {
      /* Extract value */
      if(!bg_upnp_string_to_val(s->req.args_in[s->req.num_args_in].desc->rsv->type,
                                bg_xml_node_get_text_content(arg),
                                &s->req.args_in[s->req.num_args_in].val))
        {
        /* 402 Invalid Args */
        }
      
      s->req.num_args_in++;
      }
    else // Unknown arg
      {
      /* Warning */
      }
    }

  /* Prepare output arguments */
  for(i = 0; i < s->req.action->num_args_out; i++)
    s->req.args_out[i].desc = &s->req.action->args_out[i];
  
  
  return 1;
  }

char *
bg_upnp_soap_response_to_xml(bg_upnp_service_t * s, int * len)
  {
  xmlDocPtr doc = soap_create(s->req.action->name,
                              s->type, s->version, 1);
  
  }
