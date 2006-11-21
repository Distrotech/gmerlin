
/*****************************************************************
 
  metadata.c
 
  Copyright (c) 2003-2006 by Burkhard Plaum - plaum@ipf.uni-stuttgart.de
 
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
#include <stdio.h>
#include <string.h>

#define MY_FREE(ptr) if(ptr)free(ptr);ptr=NULL;

void bgav_metadata_free(bgav_metadata_t * m)
  {
  MY_FREE(m->author);
  MY_FREE(m->title);
  MY_FREE(m->comment);
  MY_FREE(m->copyright);
  MY_FREE(m->album);
  MY_FREE(m->artist);
  MY_FREE(m->date);
  MY_FREE(m->genre);
  memset(m, 0, sizeof(*m));
  }

#define MERGE_S(s) \
if(dst->s) free(dst->s);\
if(src1->s) \
  dst->s=bgav_strdup(src1->s);\
else if(src2->s) \
  dst->s=bgav_strdup(src2->s);\
else \
  dst->s=NULL;

#define MERGE_I(s) \
if(src1->s) \
  dst->s=src1->s;\
else if(src2->s) \
  dst->s=src2->s;\
else \
  dst->s=0;

void bgav_metadata_merge(bgav_metadata_t * dst,
                         bgav_metadata_t * src1,
                         bgav_metadata_t * src2)
  {
  MERGE_S(author);
  MERGE_S(title);
  MERGE_S(comment);
  MERGE_S(copyright);
  MERGE_S(album);
  MERGE_S(artist);
  MERGE_S(date);
  MERGE_S(genre);

  MERGE_I(track);
  }

#define MERGE2_S(s) \
if((!dst->s) && (src->s)) dst->s=bgav_strdup(src->s)

#define MERGE2_I(s) \
if((!dst->s) && (src->s)) dst->s=src->s

void bgav_metadata_merge2(bgav_metadata_t * dst,
                          bgav_metadata_t * src)
  {
  MERGE2_S(author);
  MERGE2_S(title);
  MERGE2_S(comment);
  MERGE2_S(copyright);
  MERGE2_S(album);
  MERGE2_S(artist);
  MERGE2_S(date);
  MERGE2_S(genre);

  MERGE2_I(track);
  }



#define PS(label, str) if(str)bgav_dprintf("%s%s\n", label, str);
#define PI(label, i)   if(i) bgav_dprintf("%s%d\n", label, i);

void bgav_metadata_dump(bgav_metadata_t*m)
  {
  bgav_dprintf("Metadata:\n");
  
  PS("  Author:    ", m->author);
  PS("  Title:     ", m->title);
  PS("  Comment:   ", m->comment);
  PS("  Copyright: ", m->copyright);
  PS("  Album:     ", m->album);
  PS("  Artist:    ", m->artist);
  PS("  Genre:     ", m->genre);
  PI("  Track:     ", m->track);
  PS("  Date:      ", m->date);
  }

const char * bgav_metadata_get_author(const bgav_metadata_t*m)
  {
  return m->author;
  }

const char * bgav_metadata_get_title(const bgav_metadata_t*m)
  {
  return m->title;
  }

const char * bgav_metadata_get_comment(const bgav_metadata_t*m)
  {
  return m->comment;
  }

const char * bgav_metadata_get_copyright(const bgav_metadata_t*m)
  {
  return m->copyright;
  }

const char * bgav_metadata_get_album(const bgav_metadata_t*m)
  {
  return m->album;
  }

const char * bgav_metadata_get_artist(const bgav_metadata_t*m)
  {
  return m->artist;
  }

const char * bgav_metadata_get_genre(const bgav_metadata_t*m)
  {
  return m->genre;
  }

const char * bgav_metadata_get_date(const bgav_metadata_t*m)
  {
  return m->date;
  }

int bgav_metadata_get_track(const bgav_metadata_t*m)
  {
  return m->track;
  }
