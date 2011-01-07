/*****************************************************************
 * gmerlin - a general purpose multimedia framework and applications
 *
 * Copyright (c) 2001 - 2011 Members of the Gmerlin project
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

#include <alsa/asoundlib.h>

typedef struct
  {
  snd_hctl_elem_t * hctl;
  snd_ctl_elem_value_t * val;
  } alsa_mixer_control_t;

int alsa_mixer_control_read(alsa_mixer_control_t*);
int alsa_mixer_control_write(alsa_mixer_control_t*);

typedef struct
  {
  char * label;

  alsa_mixer_control_t * playback_volume;
  alsa_mixer_control_t * playback_switch;

  alsa_mixer_control_t * capture_volume;
  alsa_mixer_control_t * capture_switch;

  alsa_mixer_control_t * tone_treble;
  alsa_mixer_control_t * tone_bass;
  alsa_mixer_control_t * tone_switch;
  
  alsa_mixer_control_t * ctl; /* For all others */
  } alsa_mixer_group_t;

typedef struct
  {
  char * name;
  int num_groups;
  alsa_mixer_group_t * groups;

  snd_ctl_card_info_t * card_info;
  snd_hctl_t * hctl;
  } alsa_card_t;

alsa_card_t * alsa_card_create(int index);
void alsa_card_destroy(alsa_card_t *);
void alsa_card_dump(alsa_card_t *);

typedef struct
  {
  int num_cards;
  alsa_card_t ** cards;
  } alsa_mixer_t;

alsa_mixer_t * alsa_mixer_create();
void alsa_mixer_destroy(alsa_mixer_t *);
void alsa_mixer_dump(alsa_mixer_t *);

