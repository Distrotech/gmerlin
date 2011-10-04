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
#include <stdio.h>
#include <stdlib.h>

#define LOG_DOMAIN "redirector"

extern const bgav_redirector_t bgav_redirector_asx;
extern const bgav_redirector_t bgav_redirector_m3u;
extern const bgav_redirector_t bgav_redirector_pls;
extern const bgav_redirector_t bgav_redirector_ref;
extern const bgav_redirector_t bgav_redirector_smil;
extern const bgav_redirector_t bgav_redirector_rtsptext;
extern const bgav_redirector_t bgav_redirector_qtl;

void bgav_redirectors_dump()
  {
  bgav_dprintf( "<h2>Redirectors</h2>\n");
  bgav_dprintf( "<ul>\n");
  bgav_dprintf( "<li>%s\n", bgav_redirector_asx.name);
  bgav_dprintf( "<li>%s\n", bgav_redirector_m3u.name);
  bgav_dprintf( "<li>%s\n", bgav_redirector_pls.name);
  bgav_dprintf( "<li>%s\n", bgav_redirector_ref.name);
  bgav_dprintf( "<li>%s\n", bgav_redirector_smil.name);
  bgav_dprintf( "<li>%s\n", bgav_redirector_rtsptext.name);
  bgav_dprintf( "<li>%s\n", bgav_redirector_qtl.name);
  bgav_dprintf( "</ul>\n");
  }

typedef struct
  {
  const bgav_redirector_t * r;
  char * format_name;
  } redir_t;


const redir_t redirectors[] =
  {
    { &bgav_redirector_asx, "asx" },
    { &bgav_redirector_pls, "pls" },
    { &bgav_redirector_ref, "MS Referece" },
    { &bgav_redirector_smil, "smil" },
    { &bgav_redirector_m3u, "m3u" },
    { &bgav_redirector_rtsptext, "rtsptext" },
  };

const redir_t yml_redirectors[] =
  {
    { &bgav_redirector_qtl, "qtl" },
    //    { &bgav_redirector_rtsptext, "rtsptext" },
  };

static const int num_redirectors = sizeof(redirectors)/sizeof(redirectors[0]);

static const int num_yml_redirectors = sizeof(yml_redirectors)/sizeof(yml_redirectors[0]);

const bgav_redirector_t * bgav_redirector_probe(bgav_input_context_t * input,
                                                bgav_yml_node_t ** yml)
  {
  int i;

  for(i = 0; i < num_redirectors; i++)
    {
    if(redirectors[i].r->probe(input))
      {
      bgav_log(input->opt, BGAV_LOG_INFO, LOG_DOMAIN,
               "Detected %s redirector", redirectors[i].format_name);
      return redirectors[i].r;
      }
    }
  /* Check if this is an xml like file */
  if(bgav_yml_probe(input))
    {
    *yml = bgav_yml_parse(input);
    if(!(*yml))
      return NULL;

#if 1
    
    for(i = 0; i < num_yml_redirectors; i++)
      {
      if(yml_redirectors[i].r->probe_yml(*yml))
        {
        bgav_log(input->opt, BGAV_LOG_INFO, LOG_DOMAIN,
                 "Detected %s redirector",
                 yml_redirectors[i].format_name);
        return yml_redirectors[i].r;
        }
      }
#endif
    }
  return NULL;
  }

static bgav_redirector_context_t * get_redir(bgav_t * b)
  {
  if(b->redirector)
    return b->redirector;
  else if(b->demuxer)
    return b->demuxer->redirector;
  else
    return NULL;
  }

int bgav_is_redirector(bgav_t * b)
  {
  bgav_redirector_context_t * r;
  r = get_redir(b);

  if(r)
    return 1;
  return 0;
  }

int bgav_redirector_get_num_urls(bgav_t * b)
  {
  bgav_redirector_context_t * r;
  r = get_redir(b);
  if(!r)
    return 0;
  return r->num_urls;
  }

const char * bgav_redirector_get_url(bgav_t * b, int index)
  {
  bgav_redirector_context_t * r;
  r = get_redir(b);
  if(!r)
    return NULL;
  return r->urls[index].url;
  }

const char * bgav_redirector_get_name(bgav_t * b, int index)
  {
  bgav_redirector_context_t * r;
  r = get_redir(b);
  if(!r)
    return NULL;
  return r->urls[index].name;
  }

void bgav_redirector_destroy(bgav_redirector_context_t*r)
  {
  int i;
  if(!r)
    return;
  for(i = 0; i < r->num_urls; i++)
    {
    if(r->urls[i].url)
      free(r->urls[i].url);
    if(r->urls[i].name)
      free(r->urls[i].name);
    }
  free(r->urls);
  free(r);
  }
