/*****************************************************************
 * gmerlin-avdecoder - a general purpose multimedia decoding library
 *
 * Copyright (c) 2001 - 2008 Members of the Gmerlin project
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


#include <avdec_private.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#define PROBE_BYTES 8

static int probe_rtsptext(bgav_input_context_t * input)
  {
  char probe_buffer[PROBE_BYTES];
  /* Most likely, we get this via http, so we can check the mimetype */

  if(bgav_input_get_data(input, (uint8_t*)probe_buffer, PROBE_BYTES) < PROBE_BYTES)
    return 0;
  
  if(!strncasecmp(probe_buffer, "rtsptext", PROBE_BYTES))
    return 1;
  return 0;
  }

static void add_url(bgav_redirector_context_t * r)
  {
  r->num_urls++;
  r->urls = realloc(r->urls, r->num_urls * sizeof(*(r->urls)));
  memset(r->urls + r->num_urls - 1, 0, sizeof(*(r->urls)));
  }

static int parse_rtsptext(bgav_redirector_context_t * r)
  {
  char * buffer = (char*)0;
  int buffer_alloc = 0;
  char * pos;
  
  if(!bgav_input_read_line(r->input, &buffer, &buffer_alloc, 0, (int*)0))
    return 0;

  pos = buffer + 8;

  while(isspace(*pos) && (*pos != '\0'))
    pos++;

  if(*pos != '\0')
    {
    add_url(r);
    r->urls[r->num_urls-1].url = bgav_strdup(pos);
    }
  else
    {
    if(!bgav_input_read_line(r->input, &buffer, &buffer_alloc, 0, (int*)0))
      return 0;
    add_url(r);
    r->urls[r->num_urls-1].url = bgav_strdup(buffer);
    
    }
  
  if(buffer)
    free(buffer);
  return !!(r->num_urls);
  }

const bgav_redirector_t bgav_redirector_rtsptext = 
  {
    .name =  "rtsptext",
    .probe = probe_rtsptext,
    .parse = parse_rtsptext
  };
