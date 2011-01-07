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

#include <avdec_private.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

static int probe_yml_qtl(bgav_yml_node_t * node)
  {
  if(bgav_yml_find_by_pi(node, "quicktime") &&
     bgav_yml_find_by_name(node, "embed"))
    return 1;
  return 0;
  }

static void add_url(bgav_redirector_context_t * r)
  {
  r->num_urls++;
  r->urls = realloc(r->urls, r->num_urls * sizeof(*(r->urls)));
  memset(r->urls + r->num_urls - 1, 0, sizeof(*(r->urls)));
  }

static int parse_qtl(bgav_redirector_context_t * r)
  {
  char * filename_base, *pos;
  const char * url;
  bgav_yml_node_t * node;
  
  node = bgav_yml_find_by_name(r->yml, "embed");
  if(!node)
    return 0;
  add_url(r);

  url = bgav_yml_get_attribute(node, "src");
  
  if(r->input->filename &&
     !strstr(url, "://") &&
     (r->input->filename[0] != '/'))
    {
    filename_base = bgav_strdup(r->input->filename);
    pos = strrchr(filename_base, '/');
    if(pos)
      {
      *pos = '\0';
      r->urls[r->num_urls-1].url = bgav_sprintf("%s/%s", filename_base,
                                                url);
      return 1;
      }
    }
  
  r->urls[r->num_urls-1].url = bgav_strdup(url);
  return 1;
  }

const bgav_redirector_t bgav_redirector_qtl = 
  {
    .name =  "qtl",
    .probe_yml = probe_yml_qtl,
    .parse = parse_qtl,
  };
