/*****************************************************************
 * gmerlin-avdecoder - a general purpose multimedia decoding library
 *
 * Copyright (c) 2001 - 2011 Members of the Gmerlin project
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

/* Shoutcast redirector (.pls) */

#include <avdec_private.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

static int probe_pls(bgav_input_context_t * input)
  {
  char probe_data[10];
  
  if(input->mimetype &&
     (!strcmp(input->mimetype, "audio/x-scpls") ||
      !strcmp(input->mimetype, "audio/scpls")))
    return 1;

  
  if(bgav_input_get_data(input, (uint8_t*)probe_data, 10) < 10)
    return 0;
  
  if(!strncasecmp(probe_data, "[playlist]", 10))
    return 1;
  return 0;
  }

/*
 *  This routine parses a .pls file.
 *  We completely ignore all fields except File= and Title=
 */

static int parse_pls(bgav_redirector_context_t * r)
  {
  char * buffer = (char*)0;
  int buffer_alloc = 0;
  int index;
  char * pos;
  int ret = 0;
  
  /* Get the first nonempty line */
  while(1)
    {
    if(!bgav_input_read_line(r->input, &buffer, &buffer_alloc, 0, (int*)0))
      goto fail;
    pos = buffer;
    while(isspace(*pos))
      pos++;
    if(*pos != '\0')
      break;
    }
  
  if(strncasecmp(buffer, "[playlist]", 10))
    goto fail;
  
  /* Get number of entries */
  
  while(1)
    {
    if(!bgav_input_read_line(r->input, &buffer, &buffer_alloc, 0, (int*)0))
      break;

    if(!strncasecmp(buffer, "Title", 5))
      {
      index = atoi(buffer + 5);

      if(index > r->num_urls)
        {
        r->urls = realloc(r->urls, index * sizeof(*(r->urls)));
        memset(r->urls + r->num_urls, 0,
               (index - r->num_urls) * sizeof(*(r->urls)));
        r->num_urls = index;
        }
      pos = strchr(buffer, '=');
      if(pos)
        {
        pos++;
        r->urls[index-1].name = bgav_strdup(pos);
        }
      }
    else if(!strncasecmp(buffer, "File", 4))
      {
      index = atoi(buffer + 4);
      
      if(index > r->num_urls)
        {
        r->urls = realloc(r->urls, index * sizeof(*(r->urls)));
        memset(r->urls + r->num_urls, 0,
               (index - r->num_urls) * sizeof(*(r->urls)));
        r->num_urls = index;
        }

      pos = strchr(buffer, '=');
      if(pos)
        {
        pos++;
        r->urls[index-1].url = bgav_strdup(pos);
        }
      }
    }
  ret = 1;

  fail:
  if(buffer)
    free(buffer);
  
  return ret;
  }

const bgav_redirector_t bgav_redirector_pls = 
  {
    .name =  "pls (Shoutcast)",
    .probe = probe_pls,
    .parse = parse_pls
  };
