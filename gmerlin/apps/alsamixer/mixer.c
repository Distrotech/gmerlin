#include "alsamixer.h"

alsa_mixer_t * alsa_mixer_create()
  {
  int i;
  alsa_mixer_t * ret;
  ret = calloc(1, sizeof(*ret));
  
  int card_index = -1;

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
    return (alsa_mixer_t*)0;
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

