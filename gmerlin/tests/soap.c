#include <gmerlin/upnp/soap.h>

int main(int argc, char ** argv)
  {
  xmlDocPtr doc = bg_soap_create_request("myfunction", "myservice", 1);

  bg_soap_request_add_argument(doc, "myarg1");

  bg_xml_save_FILE(doc, stdout);
  xmlFreeDoc(doc);
  }

