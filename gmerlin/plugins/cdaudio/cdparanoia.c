/*****************************************************************
 * gmerlin - a general purpose multimedia framework and applications
 *
 * Copyright (c) 2001 - 2008 Members of the Gmerlin project
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

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <config.h>
#include <translation.h>

#include "cdaudio.h"
#define DO_NOT_WANT_PARANOIA_COMPATIBILITY
#include <cdio/cdda.h>
#include <cdio/paranoia.h>

/*
 *  Ripping support
 *  Several versions (cdparanoia, simple linux ripper) can go here
 */

typedef struct
  {
  cdrom_drive_t *drive;
  cdrom_paranoia_t *paranoia;
  
  /* Configuration options (mostly correspond to commandline options) */
  
  int speed;

  int disable_paranoia;       // -Z
  int disable_extra_paranoia; // -Y
  int max_retries;            // (0: unlimited)
  } cdparanoia_priv_t;


void * bg_cdaudio_rip_create()
  {
  cdparanoia_priv_t * ret;
  ret = calloc(1, sizeof(*ret));
  return ret;
  }

int bg_cdaudio_rip_init(void * data,
                        CdIo_t *cdio, int start_sector, int start_sector_lba,
                        int * frames_per_read)
  {
  char * msg = (char*)0;
  int paranoia_mode;
  cdparanoia_priv_t * priv;
  priv = (cdparanoia_priv_t *)data;
  
  priv->drive = cdio_cddap_identify_cdio(cdio, 1, &msg);

  if(!priv->drive)
    return 0;

  cdio_cddap_verbose_set(priv->drive,CDDA_MESSAGE_FORGETIT,CDDA_MESSAGE_FORGETIT);

  if(priv->speed != -1)
  cdio_cddap_speed_set(priv->drive, priv->speed);

  cdio_cddap_open(priv->drive);
  
  paranoia_mode=PARANOIA_MODE_FULL^PARANOIA_MODE_NEVERSKIP;

  if(priv->disable_paranoia)
    paranoia_mode=PARANOIA_MODE_DISABLE;
  if(priv->disable_extra_paranoia)
    {
    paranoia_mode|=PARANOIA_MODE_OVERLAP; /* cdda2wav style overlap
                                             check only */
    paranoia_mode&=~PARANOIA_MODE_VERIFY;
    }

  priv->paranoia = cdio_paranoia_init(priv->drive);
  cdio_paranoia_seek(priv->paranoia, start_sector, SEEK_SET);
  cdio_paranoia_modeset(priv->paranoia,paranoia_mode);

  *frames_per_read = priv->drive->nsectors;
  return 1;
  }

static void paranoia_callback(long inpos, paranoia_cb_mode_t function)
  {
  
  }

int bg_cdaudio_rip_rip(void * data, gavl_audio_frame_t * f)
  {
  int i;
  int16_t * samples;
  cdparanoia_priv_t * priv;
  priv = (cdparanoia_priv_t *)data;

  for(i = 0; i < priv->drive->nsectors; i++)
    {
    samples = cdio_paranoia_read(priv->paranoia, paranoia_callback);
    memcpy(f->samples.s_16 + i * (588 * 2), samples, 588 * 4);
    }
  return 1;
  }

/* Sector is absolute */

void bg_cdaudio_rip_seek(void * data, int sector, int sector_lba)
  {
  cdparanoia_priv_t * priv;
  priv = (cdparanoia_priv_t *)data;
  cdio_paranoia_seek(priv->paranoia, sector, SEEK_SET);
  }

void bg_cdaudio_rip_close(void * data)
  {
  cdparanoia_priv_t * priv;
  priv = (cdparanoia_priv_t *)data;

  
  if(priv->paranoia)
    {
    cdio_paranoia_free(priv->paranoia);
    priv->paranoia = NULL;
    }
  if(priv->drive)
    {
    cdio_cddap_close(priv->drive);
    priv->drive = NULL;
    }
  }


void bg_cdaudio_rip_destroy(void * data)
  {
  cdparanoia_priv_t * priv;
  priv = (cdparanoia_priv_t *)data;
  free(priv);
  }

static bg_parameter_info_t parameters[] = 
  {
    {
      .name =       "cdparanoia",
      .long_name =  TRS("Cdparanoia"),
      .type =       BG_PARAMETER_SECTION,
    },
    {
      .name =       "cdparanoia_speed",
      .long_name =  TRS("Speed"),
      .type =       BG_PARAMETER_STRINGLIST,
      .val_default = { .val_str = "Auto" },
      .multi_names = (char*[]){ TRS("Auto"),
                              "4",
                              "8",
                              "16",
                              "32",
                              (char*)0 },
    },
    {
      .name =        "cdparanoia_max_retries",
      .long_name =   TRS("Maximum retries"),
      .type =        BG_PARAMETER_INT,
      .val_min =     { .val_i = 0 },
      .val_max =     { .val_i = 200 },
      .val_default = { .val_i = 20 },
      .help_string = TRS("Maximum number of retries, 0 = infinite")
    },
    {
      .name =        "cdparanoia_disable_paranoia",
      .long_name =   TRS("Disable paranoia"),
      .type =        BG_PARAMETER_CHECKBUTTON,
      .val_default = { .val_i = 0 },
      .help_string = TRS("Disable all data verification and correction features.")
    },
    {
      .name =        "cdparanoia_disable_extra_paranoia",
      .long_name =   TRS("Disable extra paranoia"),
      .type =        BG_PARAMETER_CHECKBUTTON,
      .val_default = { .val_i = 0 },
      .help_string = TRS("Disables intra-read data verification; only overlap checking at\
read boundaries is performed. It can wedge if errors  occur  in \
the attempted overlap area. Not recommended.")
    },
    { /* End of parameters */ }
  };

bg_parameter_info_t * bg_cdaudio_rip_get_parameters()
  {
  return parameters;
  }


int
bg_cdaudio_rip_set_parameter(void * data, const char * name,
                             const bg_parameter_value_t * val)
  {
  cdparanoia_priv_t * priv;
  priv = (cdparanoia_priv_t *)data;

  if(!name)
    return 0;

  if(!strcmp(name, "cdparanoia_speed"))
    {
    if(!strcmp(val->val_str, "Auto"))
      priv->speed = -1;
    else
      priv->speed = atoi(val->val_str);
    return 1;
    }
  else if(!strcmp(name, "cdparanoia_max_retries"))
    {
    priv->max_retries = val->val_i;
    return 1;
    }
  else if(!strcmp(name, "cdparanoia_disable_paranoia"))
    {
    priv->disable_paranoia = val->val_i;
    return 1;
    }
  else if(!strcmp(name, "cdparanoia_disable_extra_paranoia"))
    {
    priv->disable_extra_paranoia = val->val_i;
    return 1;
    }
  return 0;
  }
