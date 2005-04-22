/*****************************************************************

  cdaudio.c

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
#include <stdio.h>
#include <string.h>

#include <utils.h>
#include <math.h> /* pow for volume calculation */


#include <cdio/cdio.h>
#include <cdio/audio.h>

#include "cdaudio.h"
#include "sha1.h"

void bg_cdaudio_index_dump(bg_cdaudio_index_t * idx)
  {
  int i;

  fprintf(stderr, "CD index, %d tracks (%d audio, %d data)\n",
          idx->num_tracks, idx->num_audio_tracks,
          idx->num_tracks - idx->num_audio_tracks);

  for(i = 0; i < idx->num_tracks; i++)
    {
    fprintf(stderr, "  Track %d: %s [%d %d]\n", i+1,
            ((idx->tracks[i].is_audio) ? "Audio" : "Data"),
            idx->tracks[i].first_sector,
            idx->tracks[i].last_sector);
    }
  }

void bg_cdaudio_index_free(bg_cdaudio_index_t * idx)
  {
  if(idx->tracks)
    free(idx->tracks);
  free(idx);
  }


/* base64 encoding routine taken from the source code of mucisbrainz
 * http://www.musicbrainz.org
 *
 * Author:      Mark Crispin
 *              Networks and Distributed Computing
 *              Computing & Communications
 *              University of Washington
 *              Administration Building, AG-44
 *              Seattle, WA  98195
 *              Internet: MRC@CAC.Washington.EDU
 */

/* NOTE: This is not true RFC822 anymore. The use of the characters
   '/', '+', and '=' is no bueno when the ID will be used as part of a URL.
   '_', '.', and '-' have been used instead
*/

static unsigned char *rfc822_binary (void *src,unsigned long srcl,unsigned long *len)
{
  unsigned char *ret,*d;
  unsigned char *s = (unsigned char *) src;
  char *v = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789._";
  unsigned long i = ((srcl + 2) / 3) * 4;
  *len = i += 2 * ((i / 60) + 1);
  d = ret = (unsigned char *) malloc ((size_t) ++i);
  for (i = 0; srcl; s += 3) {   /* process tuplets */
    *d++ = v[s[0] >> 2];        /* byte 1: high 6 bits (1) */
                                /* byte 2: low 2 bits (1), high 4 bits (2) */
    *d++ = v[((s[0] << 4) + (--srcl ? (s[1] >> 4) : 0)) & 0x3f];
                                /* byte 3: low 4 bits (2), high 2 bits (3) */
    *d++ = srcl ? v[((s[1] << 2) + (--srcl ? (s[2] >> 6) : 0)) & 0x3f] : '-';
                                /* byte 4: low 6 bits (3) */
    *d++ = srcl ? v[s[2] & 0x3f] : '-';
    if (srcl) srcl--;           /* count third character if processed */
    if ((++i) == 15) {          /* output 60 characters? */
      i = 0;                    /* restart line break count, insert CRLF */
      *d++ = '\015'; *d++ = '\012';
    }
  }
  *d = '\0';                    /* tie off string */

  return ret;                   /* return the resulting string */
}

/* Calculate the CDIndex DISC ID from a bg_cdaudio_index */

void bg_cdaudio_get_disc_id(bg_cdaudio_index_t * idx, char disc_id[DISCID_SIZE])
  {
  int i;
  unsigned long size;
  SHA_INFO sha;
  char temp[9];
  unsigned char digest[20];
  unsigned char * base64;
    
  bg_cdaudio_sha_init(&sha);
  sprintf(temp, "%02X", 1);
  bg_cdaudio_sha_update(&sha, (unsigned char *)temp, strlen(temp));
  sprintf(temp, "%02X", idx->num_tracks);
  bg_cdaudio_sha_update(&sha, (unsigned char *)temp, strlen(temp));

  /* First 4 byte value is the start sector of the leadout track */

  sprintf(temp, "%08X", idx->tracks[idx->num_tracks-1].last_sector + 151);
  bg_cdaudio_sha_update(&sha, (unsigned char *)temp, strlen(temp));

  for(i = 0; i < idx->num_tracks; i++)
    {
    sprintf(temp, "%08X", idx->tracks[i].first_sector+150);
    bg_cdaudio_sha_update(&sha, (unsigned char *)temp, strlen(temp));
    }
  sprintf(temp, "%08X", 0);

  for(i = idx->num_tracks; i < 99; i++)
    bg_cdaudio_sha_update(&sha, (unsigned char *)temp, strlen(temp));

  bg_cdaudio_sha_final(digest, &sha);
  
  base64 = rfc822_binary(digest, 20, &size);
  memcpy(disc_id, base64, size);
  disc_id[size] = 0;
  free(base64);

  }



/* cdio related stuff */
#if 0
#include "cdaudio.h"
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#include <utils.h>

#ifdef HAVE_LINUX_CDROM_H
#include <linux/cdrom.h>
#endif

#ifdef HAVE_LINUX_HDREG_H
#include <linux/hdreg.h>
#endif

#include <math.h>
#endif

CdIo_t * bg_cdaudio_open(const char * device)
  {
  CdIo_t * ret;
  fprintf(stderr, "Opening CD device %s...", device);
  ret = cdio_open (device, DRIVER_DEVICE);
  
  fprintf(stderr, "done, using %s driver\n", cdio_get_driver_name(ret));
  return ret;
  }
#if 0
static unsigned int get_sector(struct cdrom_msf0* msf)
  {
  unsigned int ret;
  ret = msf->minute;
  ret *= 60;
  ret += msf->second;
  ret *= 75;
  ret += msf->frame;
  return ret;
  }
#endif
bg_cdaudio_index_t * bg_cdaudio_get_index(CdIo_t * cdio)
  {
  int i;

  bg_cdaudio_index_t * ret;
         
  /* Read the index of the CD */

  ret = calloc(1, sizeof(*ret));

  ret->num_tracks = cdio_get_last_track_num(cdio);

  ret->tracks = calloc(ret->num_tracks, sizeof(*(ret->tracks)));
  
  for(i = cdio_get_first_track_num(cdio) - 1; i < ret->num_tracks; i++)
    {
    if(cdio_get_track_format(cdio, i+1) != TRACK_FORMAT_AUDIO)
      ret->tracks[i].is_audio = 0;
    else
      {
      ret->tracks[i].is_audio = 1;
      ret->tracks[i].index = ret->num_audio_tracks++;
      }
    ret->tracks[i].first_sector = cdio_get_track_lsn(cdio, i+1);
    ret->tracks[i].last_sector = cdio_get_track_last_lsn(cdio, i+1);
    }
  //  bg_cdaudio_index_dump(ret);
  return ret;
  }

void bg_cdaudio_close(CdIo_t * cdio)
  {
  fprintf(stderr, "Close CD device\n");
  cdio_destroy(cdio);
  }

#if 0
static char * device_names_scsi_old[] =
  {
    "/dev/sr0",
    "/dev/sr1",
    "/dev/sr2",
    "/dev/sr3",
    "/dev/sr4",
    "/dev/sr5",
    "/dev/sr6",
    "/dev/sr7",
    (char*)0
  };

static char * device_names_scsi_new[] =
  {
    "/dev/scd0",
    "/dev/scd1",
    "/dev/scd2",
    "/dev/scd3",
    "/dev/scd4",
    "/dev/scd5",
    "/dev/scd6",
    "/dev/scd7",
    (char*)0
  };


static char * device_names_ide[] =
  {
    "/dev/hda",
    "/dev/hdb",
    "/dev/hdc",
    "/dev/hdd",
    "/dev/hde",
    "/dev/hdf",
    "/dev/hdg",
    "/dev/hdh",
    (char*)0
  };

#endif

static char * get_device_name(CdIo_t * cdio,   cdio_drive_read_cap_t  read_cap,
                              cdio_drive_write_cap_t write_cap, const char * device)
  {
  cdio_hwinfo_t driveid;

  if(cdio_get_hwinfo(cdio, &driveid) &&
     (driveid.psz_model[0] != '\0'))
    {
    return bg_sprintf("%s %s", driveid.psz_vendor, driveid.psz_model);
    }
  
  if(write_cap & CDIO_DRIVE_CAP_WRITE_DVD_R)
    {
    return bg_sprintf("DVD Writer (%s)", device);
    }
  else if(write_cap & CDIO_DRIVE_CAP_WRITE_CD_R)
    {
    return bg_sprintf("CD Writer (%s)", device);
    }
  else if(read_cap & CDIO_DRIVE_CAP_READ_DVD_ROM)
    {
    return bg_sprintf("DVD Drive (%s)", device);
    }
  return bg_sprintf("CDrom Drive (%s)", device);
  }

int bg_cdaudio_check_device(const char * device, char ** name)
  {
  CdIo_t * cdio;
  cdio_drive_read_cap_t  read_cap;
  cdio_drive_write_cap_t write_cap;
  cdio_drive_misc_cap_t  misc_cap;

  cdio = bg_cdaudio_open(device);
  if(!cdio)
    return 0;
  
  cdio_get_drive_cap(cdio, &read_cap, &write_cap, &misc_cap);

  if(!(read_cap & CDIO_DRIVE_CAP_READ_AUDIO))
    {
    bg_cdaudio_close(cdio);
    return 0;
    }
  
  /* Seems the drive is ok */

  if(name)
    *name = get_device_name(cdio, read_cap, write_cap, device);
  bg_cdaudio_close(cdio);
  return 1;
  }

bg_device_info_t * bg_cdaudio_find_devices()
  {
  int i;
  char * device_name;
  char ** devices;
  bg_device_info_t * ret = (bg_device_info_t *)0;

  devices = cdio_get_devices(DRIVER_DEVICE);

  if(!devices)
    return 0;
    
  i = 0;
  while(devices[i])
    {
    fprintf(stderr, "Checking %s\n", devices[i]);
    device_name = (char*)0;
    if(bg_cdaudio_check_device(devices[i], &device_name))
      {
      ret = bg_device_info_append(ret,
                                  devices[i],
                                  device_name);
      if(device_name)
        free(device_name);
      }
    i++;
    }
  cdio_free_device_list(devices);
  return ret;
  }

void bg_cdaudio_play(CdIo_t * cdio,
                     int start_sector,
                     int end_sector)
  {
  msf_t start_msf, end_msf;

  cdio_lsn_to_msf (start_sector, &start_msf);
  cdio_lsn_to_msf (end_sector, &end_msf);

  cdio_audio_play_msf(cdio,
                      &start_msf,
                      &end_msf);
  }

void bg_cdaudio_stop(CdIo_t * cdio)
  {
  cdio_audio_stop(cdio);
  }

int bg_cdaudio_get_status(CdIo_t * cdio, bg_cdaudio_status_t *st)
  {
  cdio_subchannel_t subchannel;

  if(cdio_audio_read_subchannel(cdio, &subchannel) !=  DRIVER_OP_SUCCESS)
    return 0;
  
  //  fprintf(stderr, "Track: %d\n", subchnl.cdsc_trk);  
  st->track = subchannel.track - 1;
  
  st->sector = cdio_msf_to_lsn(&(subchannel.abs_addr));
  
  return 1;
  }

void bg_cdaudio_set_volume(CdIo_t * cdio, float volume)
  {
  int volume_i;

  cdio_audio_volume_t volctrl;
  
  volume_i = (int)(255.0 * pow(10, volume/20.0) + 0.5);
  //  fprintf(stderr, "bg_cdaudio_set_volume %f\n", volume);

  if(volume_i > 255)
    volume_i = 255;
  if(volume_i < 0)
    volume_i = 0;
  
  volctrl.level[0] = volume_i;
  volctrl.level[1] = volume_i;

  /* The following 2 arent really necessary */
  volctrl.level[2] = volume_i;
  volctrl.level[3] = volume_i;

  cdio_audio_set_volume(cdio, &volctrl);
  }

void bg_cdaudio_set_pause(CdIo_t * cdio, int pause)
  {
  //  fprintf(stderr, "bg_cdaudio_set_pause %d\n", pause);

  if(pause)
    cdio_audio_pause(cdio);
  else
    cdio_audio_resume(cdio);
  }
