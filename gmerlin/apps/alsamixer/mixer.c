/*****************************************************************
 * gmerlin - a general purpose multimedia framework and applications
 *
 * Copyright (c) 2001 - 2012 Members of the Gmerlin project
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

#include "alsamixer.h"

alsa_mixer_t * alsa_mixer_create()
  {
  int i;
  alsa_mixer_t * ret;
  int card_index = -1;
  ret = calloc(1, sizeof(*ret));
  

  while(!snd_card_next(&card_index))
    {
    if(card_index > -1)
      ret->num_cards++;
    else
      break;
    }

  if(!ret->num_cards)
    {
    free(ret);
    return NULL;
    }

  ret->cards = calloc(ret->num_cards, sizeof(*ret->cards));

  for(i = 0; i < ret->num_cards; i++)
    {
    ret->cards[i] = alsa_card_create(i);
    }
  return ret;
  }

void alsa_mixer_destroy(alsa_mixer_t * m)
  {
  int i;
  for(i = 0; i < m->num_cards; i++)
    {
    alsa_card_destroy(m->cards[i]);
    }
  free(m->cards);
  free(m);
  }

void alsa_mixer_dump(alsa_mixer_t * m)
  {
  int i;

  for(i = 0; i < m->num_cards; i++)
    {
    alsa_card_dump(m->cards[i]);
    }
  }

