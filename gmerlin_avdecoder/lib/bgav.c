/*****************************************************************
 
  bgav.c
 
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

#include <stdlib.h>

#include <avdec_private.h>

bgav_t * bgav_open(const char * location, int milliseconds)
  {
  bgav_t * ret;
  
  ret = calloc(1, sizeof(*ret));

  ret->input = bgav_input_open(location, milliseconds);
  if(!ret->input)
    {
    free(ret);
    return (bgav_t *)0;
    }
  bgav_codecs_init();
    
  ret->demuxer = bgav_demuxer_create(ret->input);
  if(!ret->demuxer)
    goto fail;

  return ret;

  fail:
  
  if(ret->demuxer)
    bgav_demuxer_destroy(ret->demuxer);
    
  free(ret);
  return (bgav_t *)0;
  }

void bgav_close(bgav_t * b)
  {
  bgav_close_audio_decoders(b);
  bgav_close_video_decoders(b);

  if(b->demuxer)
    bgav_demuxer_destroy(b->demuxer);

  if(b->input)
    bgav_input_close(b->input);
  
  free(b);
  }

#define CS(str) ((str) ? str : "(null)")

void bgav_dump(bgav_t * bgav)
  {
  int i;
  fprintf(stderr, "Stream Info:\n");

  fprintf(stderr, "Author:    %s\n", CS(bgav->demuxer->metadata.author));
  fprintf(stderr, "Title:     %s\n", CS(bgav->demuxer->metadata.title));
  fprintf(stderr, "Copyright: %s\n", CS(bgav->demuxer->metadata.copyright));
  fprintf(stderr, "Comment:   %s\n", CS(bgav->demuxer->metadata.comment));
  
  
  fprintf(stderr, "Duration: %f\n",
          gavl_time_to_seconds(bgav_get_duration(bgav)));
  for(i = 0; i < bgav->demuxer->num_audio_streams; i++)
    {
    bgav_audio_dump(&(bgav->demuxer->audio_streams[i]));
    }
  for(i = 0; i < bgav->demuxer->num_video_streams; i++)
    {
    bgav_video_dump(&(bgav->demuxer->video_streams[i]));
    }
  }

gavl_time_t bgav_get_duration(bgav_t * bgav)
  {
  return bgav->demuxer->duration;
  }

void bgav_start_decoders(bgav_t * b)
  {
  bgav_demuxer_create_buffers(b->demuxer);
  bgav_start_audio_decoders(b);
  bgav_start_video_decoders(b);
  }

int bgav_can_seek(bgav_t * b)
  {
  return b->demuxer->can_seek;
  }

const char * bgav_get_author(bgav_t * b)
  {
  return b->demuxer->metadata.author;
  }

const char * bgav_get_title(bgav_t* b)
  {
  return b->demuxer->metadata.title;
  }

const char * bgav_get_copyright(bgav_t* b)
  {
  return b->demuxer->metadata.copyright;
  }

const char * bgav_get_comment(bgav_t* b)
  {
  return b->demuxer->metadata.comment;
  }

const char * bgav_get_audio_description(bgav_t * b, int s)
  {
  return b->demuxer->audio_streams[s].description;
  }

const char * bgav_get_video_description(bgav_t * b, int s)
  {
  return b->demuxer->video_streams[s].description;
  }

const char * bgav_get_description(bgav_t * b)
  {
  return b->demuxer->stream_description;
  }
