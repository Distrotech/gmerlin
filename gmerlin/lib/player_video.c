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

  player->do_video = 0;
  
  if(player->track_info->num_video_streams)
    player->do_video = bg_player_input_set_video_stream(player->input_context,
                                                        video_stream);
  
  if(!player->do_video)
    return 1;
  
  if(!bg_player_ov_init(player->ov_context))
    return 0;
    
  /* Initialize video fifo */
  
  player->video_stream.fifo = bg_fifo_create(NUM_VIDEO_FRAMES,
                                             bg_player_ov_create_frame,
                                             (void*)(player->ov_context));
  
  /* Initialize audio converter */

  //  fprintf(stderr, "Initializing video converter...");

  pthread_mutex_lock(&(player->video_stream.config_mutex));
  if(!gavl_video_converter_init(s->cnv,
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
    {
      name:        "conversion_quality",
      long_name:   "Conversion Quality",
      type:        BG_PARAMETER_SLIDER_INT,
      val_min:     { val_i: GAVL_QUALITY_FASTEST },
      val_max:     { val_i: GAVL_QUALITY_BEST },
      val_default: { val_i: 2 }
    },
    {
      name:        "alpha_mode",
      long_name:   "Alpha mode",
      type:        BG_PARAMETER_STRINGLIST,
      val_default: { val_str: "Ignore" },
      options:     (char*[]){"Ignore", "Blend background color", (char*)0}
    },
    {
      name:        "background_color",
      long_name:   "Background color",
      type:      BG_PARAMETER_COLOR_RGB,
      val_default: { val_color: (float[]){ 0.0, 0.0, 0.0 } },
    },
    { /* End of parameters */ }
  };

bg_parameter_info_t * bg_player_get_video_parameters(bg_player_t * p)
  {
  return parameters;
  }

void bg_player_set_video_parameter(void * data, char * name,
                                   bg_parameter_value_t * val)
  {
  int i_tmp;
  bg_player_t * player = (bg_player_t*)data;

  if(!name)
    return;

  pthread_mutex_lock(&(player->video_stream.config_mutex));

  if(!strcmp(name, "conversion_quality"))
    {
    player->video_stream.opt.quality = val->val_i;
    }
  else if(!strcmp(name, "alpha_mode"))
    {
    if(!strcmp(val->val_str, "Ignore"))
      {
      player->video_stream.opt.alpha_mode = GAVL_ALPHA_IGNORE;
      }
    else if(!strcmp(val->val_str, "Blend background color"))
      {
      player->video_stream.opt.alpha_mode = GAVL_ALPHA_BLEND_COLOR;
      }
    }
  else if(!strcmp(name, "background_color"))
    {
    i_tmp = (int)(val->val_color[0] * 65535.0 + 0.5);
    if(i_tmp < 0)      i_tmp = 0;
    if(i_tmp > 0xffff) i_tmp = 0xffff;
    player->video_stream.opt.background_red = i_tmp;

    i_tmp = (int)(val->val_color[1] * 65535.0 + 0.5);
    if(i_tmp < 0)      i_tmp = 0;
    if(i_tmp > 0xffff) i_tmp = 0xffff;
    player->video_stream.opt.background_green = i_tmp;

    i_tmp = (int)(val->val_color[2] * 65535.0 + 0.5);
    if(i_tmp < 0)      i_tmp = 0;
    if(i_tmp > 0xffff) i_tmp = 0xffff;
    player->video_stream.opt.background_blue = i_tmp;
    }
  
  pthread_mutex_unlock(&(player->video_stream.config_mutex));

  }
