/*****************************************************************
 
  player_video.c
 
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

#include <string.h>
#include <stdio.h>

#include "player.h"
#include "playerprivate.h"
void bg_player_video_create(bg_player_t * p)
  {
  p->video_stream.cnv = gavl_video_converter_create();
  gavl_video_default_options(&(p->video_stream.opt));
  }

void bg_player_video_destroy(bg_player_t * p)
  {
  gavl_video_converter_destroy(p->video_stream.cnv);
 
  }


int bg_player_video_init(bg_player_t * player, int video_stream)
  {
  bg_player_video_stream_t * s;
  s = &(player->video_stream);
  
  player->do_video = bg_player_input_set_video_stream(player->input_context,
                                                      video_stream);

  if(!player->do_video)
    return 0;

  bg_player_ov_init(player->ov_context);
  
  /* Initialize video fifo */

  player->video_stream.fifo = bg_fifo_create(NUM_VIDEO_FRAMES,
                                             bg_player_ov_create_frame,
                                             (void*)(player->ov_context));
                                      
  /* Initialize audio converter */

  //  fprintf(stderr, "Initializing video converter...");
  
  if(!gavl_video_init(s->cnv,
                      &(s->opt),
                      &(player->video_stream.input_format),
                      &(player->video_stream.output_format)))
    {
    s->do_convert = 0;
    }
  else
    {
    s->do_convert = 1;
    s->frame = gavl_video_frame_create(&(player->video_stream.input_format));
    }
  //  fprintf(stderr, "done\n");
  return 1;
  }

void bg_player_video_cleanup(bg_player_t * player)
  {
  if(player->video_stream.frame)
    {
    gavl_video_frame_destroy(player->video_stream.frame);
    player->video_stream.frame = (gavl_video_frame_t *)0;
    }
  if(player->video_stream.fifo)
    {
    bg_fifo_destroy(player->video_stream.fifo, bg_player_ov_destroy_frame,
                    (void*)(player->ov_context));
    player->video_stream.fifo = (bg_fifo_t*)0;
    }
  }

