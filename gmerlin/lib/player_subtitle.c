/*****************************************************************
 
  player_subtitle.c
 
  Copyright (c) 2006 by Burkhard Plaum - plaum@ipf.uni-stuttgart.de
 
  http://gmerlin.sourceforge.net
 
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
 
  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111, USA.
 
*****************************************************************/

#include <string.h>
#include <stdio.h>

#include "player.h"
#include "playerprivate.h"

// #define NUM_SUBTITLE_FRAMES 2 /* How many overlays are in the fifo */

void bg_player_subtitle_create(bg_player_t * p)
  {
  p->subtitle_stream.cnv = gavl_video_converter_create();
  p->subtitle_stream.renderer = bg_text_renderer_create();
  pthread_mutex_init(&(p->subtitle_stream.config_mutex),(pthread_mutexattr_t *)0);

  }

void bg_player_subtitle_destroy(bg_player_t * p)
  {
  gavl_video_converter_destroy(p->subtitle_stream.cnv);
  pthread_mutex_destroy(&(p->subtitle_stream.config_mutex));
  bg_text_renderer_destroy(p->subtitle_stream.renderer);
  }

static void * create_frame(void * data)
  {
  gavl_overlay_t * ret;
  bg_player_subtitle_stream_t * s;
  s = (bg_player_subtitle_stream_t *)data;
  
  ret = calloc(1, sizeof(*ret));
#if 0
  fprintf(stderr, "Creating overlay frame\n");
  gavl_video_format_dump(&s->format);
#endif
  ret->frame = gavl_video_frame_create(&s->format);
  
  return ret;
  }

static void destroy_frame(void * data, void * frame)
  {
  gavl_overlay_t * ovl;
  ovl = (gavl_overlay_t*)frame;
  gavl_video_frame_destroy(ovl->frame);
  free(frame);
  }

int bg_player_subtitle_init(bg_player_t * player, int subtitle_stream)
  {
  bg_player_subtitle_stream_t * s;
  
  s = &(player->subtitle_stream);

  player->do_subtitle_overlay = 0;
  player->do_subtitle_text    = 0;
  
  if(player->track_info->num_subtitle_streams)
    {
    if(bg_player_input_set_subtitle_stream(player->input_context,
                                           subtitle_stream))
      {
      if(player->track_info->subtitle_streams[subtitle_stream].is_text)
        {
        player->do_subtitle_text = 1;
        /* Initialize text renderer */
        bg_text_renderer_init(player->subtitle_stream.renderer,
                              &(player->video_stream.output_format),
                              &(player->subtitle_stream.format));
        }
      else
        player->do_subtitle_overlay = 1;
      }
    else
      {
      return 1;
      }
    }
  else
    return 1;
  
  /* Initialize subtitle fifo */

  player->subtitle_stream.fifo = bg_fifo_create(NUM_SUBTITLE_FRAMES,
                                                create_frame, (void*)(s));
  bg_player_ov_set_subtitle_format(player->ov_context,
                                   &(player->subtitle_stream.format));

  /* Initialize video converter */

  //  fprintf(stderr, "Initializing video converter...");

  pthread_mutex_lock(&(player->subtitle_stream.config_mutex));

  

  pthread_mutex_unlock(&(player->subtitle_stream.config_mutex));
  return 1;
  }

void bg_player_subtitle_cleanup(bg_player_t * player)
  {
  if(player->subtitle_stream.fifo)
    {
    bg_fifo_destroy(player->subtitle_stream.fifo, destroy_frame,
                    (void*)(&(player->subtitle_stream)));
    
    player->subtitle_stream.fifo = (bg_fifo_t*)0;
    }
  }

/* Configuration stuff */

bg_parameter_info_t * bg_player_get_subtitle_parameters(bg_player_t * p)
  {
  return bg_text_renderer_get_parameters();
  }

void bg_player_set_subtitle_parameter(void * data, char * name,
                                      bg_parameter_value_t * val)
  {
  bg_player_t * player = (bg_player_t*)data;

  if(!name)
    return;
  
  pthread_mutex_lock(&(player->subtitle_stream.config_mutex));
  
  bg_text_renderer_set_parameter(player->subtitle_stream.renderer,
                                 name, val);
  
  pthread_mutex_unlock(&(player->subtitle_stream.config_mutex));
  }
