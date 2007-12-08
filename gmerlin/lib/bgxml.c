#include <config.h>

#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <errno.h>

#include <utils.h>
#include <xmlutils.h>

#define BLOCK_SIZE 2048

#include <log.h>
#define LOG_DOMAIN "xmlutils"

int bg_xml_write_callback(void * context, const char * buffer,
                       int len)
  {
  bg_xml_output_mem_t * o = (bg_xml_output_mem_t*)context;

  if(o->bytes_allocated - o->bytes_written < len)
    {
    o->bytes_allocated += BLOCK_SIZE;
    while(o->bytes_allocated < o->bytes_written + len)
      o->bytes_allocated += BLOCK_SIZE;
    o->buffer = realloc(o->buffer, o->bytes_allocated);
    }
  memcpy(&(o->buffer[o->bytes_written]), buffer, len);
  o->bytes_written += len;
  return len;
  }

int bg_xml_close_callback(void * context)
  {
  bg_xml_output_mem_t * o = (bg_xml_output_mem_t*)context;

  if(o->bytes_allocated == o->bytes_written)
    {
    o->bytes_allocated++;
    o->buffer = realloc(o->buffer, o->bytes_allocated);
    }
  o->buffer[o->bytes_written] = '\0';
  return 0;
  }

xmlDocPtr bg_xml_parse_file(const char * filename)
  {
  struct stat st;

  if(stat(filename, &st))
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Cannot stat %s: %s",
           filename, strerror(errno));
    }

  /* Return silently */
  if(!st.st_size)
    return (xmlDocPtr)0;
  return xmlParseFile(filename);
  }
