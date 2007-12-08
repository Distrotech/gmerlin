/* xml utilities */

#include <libxml/tree.h>
#include <libxml/parser.h>

/*  Macro, which calls strcmp, but casts the first argument to char*
 *  This is needed because libxml strings are uint8_t*
 */

#define BG_XML_STRCMP(a, b) strcmp((char*)a, b)
#define BG_XML_GET_PROP(a, b) (char*)xmlGetProp(a, (xmlChar*)b)
#define BG_XML_SET_PROP(a, b, c) xmlSetProp(a, (xmlChar*)b, (xmlChar*)c)
#define BG_XML_NEW_TEXT(a) xmlNewText((xmlChar*)a)

/* 
 * memory writer for libxml
 */

typedef struct bg_xml_output_mem_s
  {
  int bytes_written;
  int bytes_allocated;
  char * buffer;
  } bg_xml_output_mem_t;

int bg_xml_write_callback(void * context, const char * buffer,
                          int len);

int bg_xml_close_callback(void * context);

xmlDocPtr bg_xml_parse_file(const char * filename);

