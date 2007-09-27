#include <utils.h>
#include <stdlib.h>
#include <string.h>

#define BLOCK_SIZE 2048

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
