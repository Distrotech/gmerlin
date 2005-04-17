/*****************************************************************

  cdparanoia.c

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
#include <string.h>
#include <stdio.h>

#include "cdaudio.h"


#ifdef HAVE_CDDA_CDDA_INTERFACE
#include <cdda/cdda_interface.h>
#include <cdda/cdda_paranoia.h>
#else
#include <cdda_interface.h>
#include <cdda_paranoia.h>
#endif


/*
 *  Ripping support
 *  Several versions (cdparanoia, simple linux ripper) can go here
 */

typedef struct
  {
  cdrom_drive *drive;
  cdrom_paranoia *paranoia;
  
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
                        char * device, int start_sector, int start_sector_lba,
                        int * frames_per_read)
  {
  int paranoia_mode;
  cdparanoia_priv_t * priv;
  priv = (cdparanoia_priv_t *)data;
  
  //  fprintf(stderr, "cdparanoia: cdda_identify\n");
  priv->drive = cdda_identify(device, 1, NULL);
  //  fprintf(stderr, "cdparanoia: cdda_identify done, drive: %p\n", priv->drive);

  if(!priv->drive)
    return 0;

  cdda_verbose_set(priv->drive,CDDA_MESSAGE_FORGETIT,CDDA_MESSAGE_FORGETIT);

  if(priv->speed != -1)
    cdda_speed_set(priv->drive, priv->speed);

  cdda_open(priv->drive);
  
  paranoia_mode=PARANOIA_MODE_FULL^PARANOIA_MODE_NEVERSKIP;

  if(priv->disable_paranoia)
    paranoia_mode=PARANOIA_MODE_DISABLE;
  if(priv->disable_extra_paranoia)
    {
    paranoia_mode|=PARANOIA_MODE_OVERLAP; /* cdda2wav style overlap
                                             check only */
    paranoia_mode&=~PARANOIA_MODE_VERIFY;
    }

  priv->paranoia = paranoia_init(priv->drive);
  paranoia_seek(priv->paranoia, start_sector_lba, SEEK_SET);
  paranoia_modeset(priv->paranoia,paranoia_mode);

  *frames_per_read = priv->drive->nsectors;
  return 1;
  }

static void paranoia_callback(long inpos, int function)
  {
  //  fprintf(stderr, "Paranoia callback: %ld\n", inpos);
  
  }

int bg_cdaudio_rip_rip(void * data, gavl_audio_frame_t * f)
  {
  int i;
  int16_t * samples;
  cdparanoia_priv_t * priv;
  priv = (cdparanoia_priv_t *)data;

  for(i = 0; i < priv->drive->nsectors; i++)
    {
    samples = paranoia_read(priv->paranoia, paranoia_callback);
    memcpy(f->samples.s_16 + i * (588 * 2), samples, 588 * 4);
    }
  return 1;
  }

/* Sector is absolute */

void bg_cdaudio_rip_seek(void * data, int sector, int sector_lba)
  {
  cdparanoia_priv_t * priv;
  priv = (cdparanoia_priv_t *)data;
  fprintf(stderr, "Paranoia seek: %d\n", sector);
  paranoia_seek(priv->paranoia, sector_lba, SEEK_SET);
  }

void bg_cdaudio_rip_close(void * data)
  {
  cdparanoia_priv_t * priv;
  priv = (cdparanoia_priv_t *)data;

  
  if(priv->paranoia)
    {
    paranoia_free(priv->paranoia);
    priv->paranoia = NULL;
    }
  if(priv->drive)
    {
    cdda_close(priv->drive);
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
      name:       "cdparanoia",
      long_name:  "cdparanoia Options",
      type:       BG_PARAMETER_SECTION,
    },
    {
      name:       "cdparanoia_speed",
      long_name:  "Speed",
      type:       BG_PARAMETER_STRINGLIST,
      val_default: { val_str: "Auto" },
      multi_names: (char*[]){ "Auto",
                              "4",
                              "8",
                              "16",
                              "32",
                              (char*)0 },
    },
    {
      name:        "cdparanoia_max_retries",
      long_name:   "Maximum retries",
      type:        BG_PARAMETER_INT,
      val_min:     { val_i: 0 },
      val_max:     { val_i: 200 },
      val_default: { val_i: 20 },
      help_string: "Maximum number of retries, 0 = infinite"
    },
    {
      name:        "cdparanoia_disable_paranoia",
      long_name:   "Disable paranoia",
      type:        BG_PARAMETER_CHECKBUTTON,
      val_default: { val_i: 0 },
      help_string: "Disable all data verification and  correction  features."
    },
    {
      name:        "cdparanoia_disable_extra_paranoia",
      long_name:   "Disable extra paranoia",
      type:        BG_PARAMETER_CHECKBUTTON,
      val_default: { val_i: 0 },
      help_string: "Disables intra-read data verification; only overlap checking at\
read boundaries is performed. It can wedge if errors  occur  in \
the attempted overlap area. Not recommended."
    },
    { /* End of parameters */ }
  };

bg_parameter_info_t * bg_cdaudio_rip_get_parameters()
  {
  return parameters;
  }


int
bg_cdaudio_rip_set_parameter(void * data, char * name,
                             bg_parameter_value_t * val)
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
