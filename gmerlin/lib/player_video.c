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
  pthread_mutex_init(&(p->video_stream.config_mutex),(pthread_mutexattr_t *)0);

  p->video_stream.options.opt =
    gavl_video_converter_get_options(p->video_stream.cnv);
  
  bg_gavl_video_options_init(&(p->video_stream.options));
  }

void bg_player_video_destroy(bg_player_t * p)
  {
  gavl_video_converter_destroy(p->video_stream.cnv);
  pthread_mutex_destroy(&(p->video_stream.config_mutex));
  }

int bg_player_video_init(bg_player_t * player, int video_stream)
  {
  bg_player_video_stream_t * s;
  
  s = &(player->video_stream);

  
  if(player->track_info->num_video_streams)
    {
    player->do_video = bg_player_input_set_video_stream(player->input_context,
                                                        video_stream);
    if(!player->do_video)
      return 1;
    }
  else if(player->track_info->num_still_streams)
    {
    player->do_still = bg_player_input_set_still_stream(player->input_context,
                                                        player->current_still_stream);
    if(!player->do_still)
      return 1;
    }
  else
    return 1;
  
  if(!bg_player_ov_init(player->ov_context))
    {
    player->video_stream.error_msg = bg_player_ov_get_error(player->ov_context);
    return 0;
    }
  /* Initialize video fifo */
  
  player->video_stream.fifo = bg_fifo_create(NUM_VIDEO_FRAMES,
                                             bg_player_ov_create_frame,
                                             (void*)(player->ov_context));
  
  /* Initialize audio converter */

  //  fprintf(stderr, "Initializing video converter...");

  pthread_mutex_lock(&(player->video_stream.config_mutex));
  if(!gavl_video_converter_init(s->cnv,
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
  pthread_mutex_unlock(&(player->video_stream.config_mutex));
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

/* Configuration stuff */

static bg_parameter_info_t parameters[] =
  {
    {
      name:      "video",
      long_name: "Video",
      type:      BG_PARAMETER_SECTION,
    },
    BG_GAVL_PARAM_CONVERSION_QUALITY,
    BG_GAVL_PARAM_ALPHA,
    { /* End of parameters */ }
  };

bg_parameter_info_t * bg_player_get_video_parameters(bg_player_t * p)
  {
  return parameters;
  }

void bg_player_set_video_parameter(void * data, char * name,
                                   bg_parameter_value_t * val)
  {
  bg_player_t * player = (bg_player_t*)data;

  if(!name)
    return;

  pthread_mutex_lock(&(player->video_stream.config_mutex));

  bg_gavl_video_set_parameter(&(player->video_stream.options),
                              name, val);
  
  pthread_mutex_unlock(&(player->video_stream.config_mutex));

  }
