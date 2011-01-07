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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <config.h>
#include <gmerlin/translation.h>


#include <gmerlin/utils.h>
#include <math.h> /* pow for volume calculation */


#include <cdio/cdio.h>
#include <cdio/audio.h>
#include <cdio/mmc.h>

#include "cdaudio.h"
#include "sha1.h"

#include <gmerlin/log.h>
#define LOG_DOMAIN "cdaudio"


void bg_cdaudio_index_dump(bg_cdaudio_index_t * idx)
  {
  int i;
  FILE * out = stderr;
  
  fprintf(out, "CD index, %d tracks (%d audio, %d data)\n",
          idx->num_tracks, idx->num_audio_tracks,
          idx->num_tracks - idx->num_audio_tracks);

  for(i = 0; i < idx->num_tracks; i++)
    {
    fprintf(out, "  Track %d: %s [%d %d]\n", i+1,
            ((idx->tracks[i].is_audio) ? "Audio" : "Data"),
            idx->tracks[i].first_sector,
            idx->tracks[i].last_sector);
    }
  }

void bg_cdaudio_index_destroy(bg_cdaudio_index_t * idx)
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

CdIo_t * bg_cdaudio_open(const char * device)
  {
  driver_return_code_t err;
  CdIo_t * ret;

  /* Close the tray */
  if((err = cdio_close_tray(device, NULL)))
#if LIBCDIO_VERSION_NUM >= 77
    bg_log(BG_LOG_ERROR, LOG_DOMAIN, "cdio_close_tray failed: %s",
            cdio_driver_errmsg(err));
#else
    bg_log(BG_LOG_ERROR, LOG_DOMAIN, "cdio_close_tray failed");
#endif
  
  ret = cdio_open (device, DRIVER_DEVICE);
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
  int i, num_tracks;

  bg_cdaudio_index_t * ret;
         
  /* Read the index of the CD */

  num_tracks = cdio_get_last_track_num(cdio);
  if(num_tracks == CDIO_INVALID_TRACK)
    return (bg_cdaudio_index_t*)0;
  
  ret = calloc(1, sizeof(*ret));
  ret->num_tracks = num_tracks;

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

  if(!ret->num_audio_tracks)
    {
    free(ret->tracks);
    free(ret);
    return (bg_cdaudio_index_t*)0;
    }

  return ret;
  }

void bg_cdaudio_close(CdIo_t * cdio)
  {
  cdio_destroy(cdio);
  }

static char * get_device_name(CdIo_t * cdio,
                              cdio_drive_read_cap_t  read_cap,
                              cdio_drive_write_cap_t write_cap,
                              const char * device)
  {
  cdio_hwinfo_t driveid;

  if(cdio_get_hwinfo(cdio, &driveid) &&
     (driveid.psz_model[0] != '\0'))
    {
    return bg_sprintf("%s %s", driveid.psz_vendor, driveid.psz_model);
    }
  
  if(write_cap & CDIO_DRIVE_CAP_WRITE_DVD_R)
    {
    return bg_sprintf(TR("DVD Writer (%s)"), device);
    }
  else if(write_cap & CDIO_DRIVE_CAP_WRITE_CD_R)
    {
    return bg_sprintf(TR("CD Writer (%s)"), device);
    }
  else if(read_cap & CDIO_DRIVE_CAP_READ_DVD_ROM)
    {
    return bg_sprintf(TR("DVD Drive (%s)"), device);
    }
  return bg_sprintf(TR("CD-ROM Drive (%s)"), device);
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

int bg_cdaudio_play(CdIo_t * cdio,
                    int start_sector,
                    int end_sector)
  {
  msf_t start_msf, end_msf;

  cdio_lsn_to_msf (start_sector, &start_msf);
  cdio_lsn_to_msf (end_sector, &end_msf);

  if(cdio_audio_play_msf(cdio,
                         &start_msf,
                         &end_msf) != DRIVER_OP_SUCCESS)
    return 0;
  return 1;
  }

void bg_cdaudio_stop(CdIo_t * cdio)
  {
  cdio_audio_stop(cdio);
  }
