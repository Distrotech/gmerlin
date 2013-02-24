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

#include <config.h>

#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <errno.h>

#include <gmerlin/utils.h>
#include <gmerlin/xmlutils.h>

#define BLOCK_SIZE 2048

#include <gmerlin/log.h>
#define LOG_DOMAIN "xmlutils"

typedef struct
  {
  int bytes_written;
  int bytes_allocated;
  char * buffer;
  } bg_xml_output_mem_t;

static int mem_write_callback(void * context, const char * buffer,
                          int len)
  {
  bg_xml_output_mem_t * o = context;

  if(o->bytes_allocated - o->bytes_written < len)
    {
    o->bytes_allocated += BLOCK_SIZE;
    while(o->bytes_allocated < o->bytes_written + len)
      o->bytes_allocated += BLOCK_SIZE;
    o->buffer = realloc(o->buffer, o->bytes_allocated);
    }
  memcpy(&o->buffer[o->bytes_written], buffer, len);
  o->bytes_written += len;
  return len;
  }

static int mem_close_callback(void * context)
  {
  bg_xml_output_mem_t * o = context;

  if(o->bytes_allocated == o->bytes_written)
    {
    o->bytes_allocated++;
    o->buffer = realloc(o->buffer, o->bytes_allocated);
    }
  o->buffer[o->bytes_written] = '\0';
  return 0;
  }

char * bg_xml_save_to_memory(xmlDocPtr doc)
  {
  xmlOutputBufferPtr b;
  bg_xml_output_mem_t ctx;
  memset(&ctx, 0, sizeof(ctx));
  
  b = xmlOutputBufferCreateIO (mem_write_callback,
                               mem_close_callback,
                               &ctx,
                               NULL);
  xmlSaveFileTo(b, doc, NULL);
  return ctx.buffer;
  }

xmlDocPtr bg_xml_parse_file(const char * filename)
  {
  struct stat st;

  if(stat(filename, &st))
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Cannot stat %s: %s",
           filename, strerror(errno));
    return NULL;
    }

  /* Return silently */
  if(!st.st_size)
    return NULL;
  return xmlParseFile(filename);
  }



xmlDocPtr bg_xml_load_FILE(FILE * f)
  {
  
  }

void bg_xml_save_FILE(xmlDocPtr doc, FILE * f)
  {
  
  }
