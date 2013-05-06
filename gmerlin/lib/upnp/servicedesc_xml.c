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

#include <upnp/servicepriv.h>
#include <gmerlin/utils.h>




/* XML Stuff */

static xmlDocPtr bg_upnp_service_description_create()
  {
  xmlDocPtr ret;
  xmlNodePtr root;
  xmlNsPtr ns;
  xmlNodePtr node;
  
  
  ret = xmlNewDoc((xmlChar*)"1.0");
  root = xmlNewDocRawNode(ret, NULL, (xmlChar*)"scpd", NULL);
  xmlDocSetRootElement(ret, root);

  ns =
    xmlNewNs(root,
             (xmlChar*)"urn:schemas-upnp-org:service-1-0",
             NULL);
  xmlSetNs(root, ns);

  node = xmlNewTextChild(root, NULL, (xmlChar*)"specVersion", NULL);
  bg_xml_append_child_node(node, "major", "1");
  bg_xml_append_child_node(node, "minor", "0");

  node = xmlNewTextChild(root, NULL, (xmlChar*)"actionList", NULL);
  node = xmlNewTextChild(root, NULL, (xmlChar*)"serviceStateTable", NULL);
  return ret;
  }

static xmlNodePtr bg_upnp_service_description_add_action(xmlDocPtr doc, const char * name)
  {
  xmlNodePtr node;

  if(!(node = bg_xml_find_doc_child(doc, "scpd")) ||
     !(node = bg_xml_find_node_child(node, "actionList")))
    return NULL;

  node = xmlNewTextChild(node, NULL, (xmlChar*)"action", NULL);
  bg_xml_append_child_node(node, "name", name);
  xmlNewTextChild(node, NULL, (xmlChar*)"argumentList", NULL);
  return node;
  }

static void bg_upnp_service_action_add_argument(xmlNodePtr node,
                                                const char * name, int out, int retval,
                                                const char * related_statevar)
  {
  if(!(node = bg_xml_find_node_child(node, "argumentList")))
    return;

  node = xmlNewTextChild(node, NULL, (xmlChar*)"argument", NULL);
  bg_xml_append_child_node(node, "name", name);

  bg_xml_append_child_node(node, "direction", (out ? "out" : "in"));

  if(retval)
    xmlNewTextChild(node, NULL, (xmlChar*)"retval", NULL);

  bg_xml_append_child_node(node, "relatedStateVariable", related_statevar);
  }

static xmlNodePtr
bg_upnp_service_description_add_statevar(xmlDocPtr doc,
                                         const char * name,
                                         int events,
                                         const char * data_type)
  {
  xmlNodePtr node;
  
  if(!(node = bg_xml_find_doc_child(doc, "scpd")) ||
     !(node = bg_xml_find_node_child(node, "serviceStateTable")))
    return NULL;

  node = xmlNewTextChild(node, NULL, (xmlChar*)"stateVariable", NULL);

  BG_XML_SET_PROP(node, "sendEvents", (events ? "yes" : "no"));

  bg_xml_append_child_node(node, "name", name);
  bg_xml_append_child_node(node, "dataType", data_type);
  return node;
  }

static void
bg_upnp_service_statevar_add_allowed_value(xmlNodePtr node,
                                           const char * val)
  {
  xmlNodePtr allowedlist = bg_xml_find_node_child(node, "allowedValueList");
  
  if(!allowedlist)
    allowedlist = xmlNewTextChild(node, NULL, (xmlChar*)"allowedValueList", NULL);
  bg_xml_append_child_node(allowedlist, "allowedValue", val);
  }

static void
bg_upnp_service_statevar_set_allowed_value_range(xmlNodePtr node, const char * min, 
                                                 const char * max, const char * step)
  {
  xmlNodePtr allowedrange = xmlNewTextChild(node, NULL, (xmlChar*)"allowedValueRange", NULL);
  bg_xml_append_child_node(allowedrange, "minimum", min);
  bg_xml_append_child_node(allowedrange, "maximum", max);
  bg_xml_append_child_node(allowedrange, "step", step);
  }

static void
bg_upnp_service_statevar_set_default(xmlNodePtr node, const char * def)
  {
  bg_xml_append_child_node(node, "defaultValue", def);
  }


static const struct
  {
  const char * name;
  bg_upnp_sv_type_t t;
  } sv_types[] =
  {
     { "i4",     BG_UPNP_SV_INT4 },
     { "string", BG_UPNP_SV_STRING },
     { },
  };

static const char * sv_type_to_string(bg_upnp_sv_type_t t)
   {
   int i = 0;
   while(sv_types[i].name)
     {
     if(sv_types[i].t == t)
       return sv_types[i].name;
     i++;
     }
   return NULL;
   }

char * bg_upnp_service_desc_2_xml(bg_upnp_service_desc_t * d)
  {
  int i, j;
  xmlDocPtr doc;
  char * ret;

  doc = bg_upnp_service_description_create();
  /* Actions */
  
  for(i = 0; i < d->num_sa; i++)
    {
    xmlNodePtr action;
    action = bg_upnp_service_description_add_action(doc, d->sa[i].name);
    
    for(j = 0; j < d->sa[i].num_args_in; j++)
      {
      bg_upnp_service_action_add_argument(action, 
                                          d->sa[i].args_in[j].name, 0, 0,
                                          d->sa[i].args_in[j].rsv_name);
      }
    for(j = 0; j < d->sa[i].num_args_out; j++)
      {
      bg_upnp_service_action_add_argument(action, 
                                          d->sa[i].args_out[j].name, 1, 
                                          !!(d->sa[i].args_out[j].flags & BG_UPNP_ARG_RETVAL),
                                          d->sa[i].args_out[j].rsv_name);
      }
    }
  for(i = 0; i < d->num_sv; i++)
    {
    xmlNodePtr sv;    
    sv = bg_upnp_service_description_add_statevar(doc, d->sv[i].name,
                                                  d->sv[i].flags & BG_UPNP_SV_EVENT,
                                                  sv_type_to_string(d->sv[i].type));
    if(d->sv[i].flags & BG_UPNP_SV_HAS_DEFAULT)
      {
      char * tmp_string = bg_upnp_val_to_string(d->sv[i].type, &d->sv[i].def);
      bg_upnp_service_statevar_set_default(sv, tmp_string);
      free(tmp_string);
      }
    for(j = 0; j < d->sv[i].num_allowed; j++)
      {
      char * tmp_string = bg_upnp_val_to_string(d->sv[i].type, &d->sv[i].allowed[j]);
      bg_upnp_service_statevar_add_allowed_value(sv, tmp_string);
      free(tmp_string);
      }
    if(d->sv[i].flags & BG_UPNP_SV_HAS_RANGE)
      {
      char * min, * max, *step;
      min = bg_upnp_val_to_string(d->sv[i].type, &d->sv[i].range.min);
      max = bg_upnp_val_to_string(d->sv[i].type, &d->sv[i].range.max);
      step = bg_upnp_val_to_string(d->sv[i].type, &d->sv[i].range.step);
      bg_upnp_service_statevar_set_allowed_value_range(sv, min, max, step);
      free(min);
      free(max);
      free(step);
      }
    }
  ret = bg_xml_save_to_memory(doc);
  xmlFreeDoc(doc);
  return ret;
  }
