
/*****************************************************************
 
  redirect.c
 
  Copyright (c) 2003-2004 by Burkhard Plaum - plaum@ipf.uni-stuttgart.de
 
  http://gmerlin.sourceforge.net
 
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
 
  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111, USA.
 
*****************************************************************/

#include <avdec_private.h>
#include <stdlib.h>

extern bgav_redirector_t bgav_redirector_asx;
extern bgav_redirector_t bgav_redirector_m3u;
extern bgav_redirector_t bgav_redirector_pls;
extern bgav_redirector_t bgav_redirector_ref;

static struct
  {
  bgav_redirector_t * r;
  char * format_name;
  }
redirectors[] =
  {
    { &bgav_redirector_asx, "asx" },
    { &bgav_redirector_m3u, "m3u" },
    { &bgav_redirector_pls, "pls" },
    { &bgav_redirector_ref, "MS Referece" },
  };

static int num_redirectors = sizeof(redirectors)/sizeof(redirectors[0]);

bgav_redirector_t * bgav_redirector_probe(bgav_input_context_t * input)
  {
  int i;
  for(i = 0; i < num_redirectors; i++)
    {
    if(redirectors[i].r->probe(input))
      {
      fprintf(stderr, "Detected %s redirector\n", redirectors[i].format_name);
      return redirectors[i].r;
      }
    }
  return (bgav_redirector_t*)0;
  }

int bgav_is_redirector(bgav_t * b)
  {
  if(b->redirector)
    return 1;
  return 0;
  }

int bgav_redirector_get_num_urls(bgav_t * b)
  {
  if(!b->redirector)
    return 0;
  return b->redirector->num_urls;
  }

const char * bgav_redirector_get_url(bgav_t * b, int index)
  {
  if(!b->redirector)
    return 0;
  return b->redirector->urls[index].url;
  }

const char * bgav_redirector_get_name(bgav_t * b, int index)
  {
  if(!b->redirector)
    return 0;
  return b->redirector->urls[index].name;
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
