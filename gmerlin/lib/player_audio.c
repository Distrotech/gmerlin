/*****************************************************************
 
  player_audio.c
 
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
#include <stdlib.h>
#include <stdio.h>
#include "player.h"
#include "playerprivate.h"

void bg_player_audio_create(bg_player_t * p)
  {
  p->audio_stream.cnv = gavl_audio_converter_create();
  gavl_audio_default_options(&(p->audio_stream.opt));
  }

void bg_player_audio_destroy(bg_player_t * p)
  {
  gavl_audio_converter_destroy(p->audio_stream.cnv);
  }

int bg_player_audio_init(bg_player_t * player, int audio_stream)
  {
  player->do_audio =
    bg_player_input_set_audio_stream(player->input_context, audio_stream);

  if(!player->do_audio)
    return 0;

#if 1
  fprintf(stderr, "======= Input format: ===========\n");
  gavl_audio_format_dump(&(player->audio_format_i));
#endif
  
  /* Set up formats */
  
  bg_player_oa_init(player->oa_context);
  
  /* Initialize audio fifo */

  player->audio_stream.fifo =
    bg_fifo_create(NUM_AUDIO_FRAMES, bg_player_oa_create_frame,
                   (void*)player->oa_context);
  
  /* Initialize audio converter */

#if 0
  fprintf(stderr, "======= Output format: ==========\n");
  gavl_audio_format_dump(&(player->audio_format_o));
  fprintf(stderr, "=================================\n");
#endif
  
  if(!gavl_audio_init(player->audio_stream.cnv, &(player->audio_stream.opt),
                      &(player->audio_format_i), &(player->audio_format_o)))
    {
    player->audio_stream.do_convert = 0;
    //    fprintf(stderr, "**** No Conversion ****\n");
    }
  else
    {
    player->audio_stream.do_convert = 1;
    player->audio_stream.frame =
      gavl_audio_frame_create(&(player->audio_format_i));
    //    fprintf(stderr, "**** Doing Conversion %p ****\n",
    //            player->audio_stream.frame);
    }
  
  return 1;
  }

void bg_player_audio_cleanup(bg_player_t * player)
  {
  if(player->audio_stream.fifo)
    {
    bg_fifo_destroy(player->audio_stream.fifo,
                    bg_player_oa_destroy_frame, NULL);
    player->audio_stream.fifo = (bg_fifo_t *)0;
    }
  if(player->audio_stream.frame)
    {
    gavl_audio_frame_destroy(player->audio_stream.frame);
    player->audio_stream.frame = (gavl_audio_frame_t*)0;
    }
  }

