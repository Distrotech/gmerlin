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

int bg_cdaudio_open(const char * device)
  {
  int ret;
  ret = open(device, O_RDONLY | O_NONBLOCK);
  return ret;
  }

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

bg_cdaudio_index_t * bg_cdaudio_get_index(int fd)
  {
  int i;

  bg_cdaudio_index_t * ret;
  struct cdrom_tochdr hdr;
  struct cdrom_tocentry entry;

  memset(&hdr, 0, sizeof(hdr));
  memset(&entry, 0, sizeof(entry));
         
  
  if(ioctl(fd, CDROMREADTOCHDR, &hdr) < 0 )
    return (bg_cdaudio_index_t *)0;
  
  /* Read the index of the CD */

  ret = calloc(1, sizeof(*ret));
  ret->num_tracks = hdr.cdth_trk1 - hdr.cdth_trk0+1;

  ret->tracks = calloc(ret->num_tracks, sizeof(*(ret->tracks)));

  for(i = 0; i < ret->num_tracks; i++)
    {
    entry.cdte_track = hdr.cdth_trk0 + i;
    entry.cdte_format = CDROM_MSF;

    if(ioctl(fd, CDROMREADTOCENTRY, &entry) < 0 )
      goto fail;
    
    //    fprintf(stderr, "Track %d: Datamode: %d\n", i+1, entry.cdte_datamode);

    if(entry.cdte_ctrl & CDROM_DATA_TRACK)
      ret->tracks[i].is_audio = 0;
    else
      {
      ret->tracks[i].is_audio = 1;
      ret->tracks[i].index = ret->num_audio_tracks++;
      }

    ret->tracks[i].first_sector = get_sector(&(entry.cdte_addr.msf));
    
    if(i)
      ret->tracks[i-1].last_sector = ret->tracks[i].first_sector - 1;
    //    ret->tracks[i].first_sector = entry.cdte_addr.lba;
    }

  entry.cdte_track = CDROM_LEADOUT;
  entry.cdte_format = CDROM_MSF;
  if(ioctl(fd, CDROMREADTOCENTRY, &entry) < 0 )
    return 0;

  ret->tracks[ret->num_tracks-1].last_sector = get_sector(&(entry.cdte_addr.msf)) - 1;
  return ret;
  
  fail:
  free(ret->tracks);
  free(ret);
  return (bg_cdaudio_index_t *)0;
  
  }

void bg_cdaudio_close(int fd)
  {
  close(fd);
  }


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

static char * get_device_name(int fd, int cap, const char * device)
  {
#ifdef HAVE_LINUX_HDREG_H
  struct hd_driveid driveid;

  if((ioctl(fd, HDIO_GET_IDENTITY, &driveid) >= 0) &&
     (driveid.model[0] != '\0'))
    {
    return bg_strdup(NULL, driveid.model);
    }

#endif

  if(cap & CDC_DVD_R)
    {
    return bg_sprintf("DVD Writer (%s)", device);
    }
  else if(cap & CDC_CD_R)
    {
    return bg_sprintf("CD Writer (%s)", device);
    }
  else if(cap & CDC_DVD)
    {
    return bg_sprintf("DVD Drive (%s)", device);
    }
  return bg_sprintf("CDrom Drive (%s)", device);
  }

int bg_cdaudio_check_device(const char * device, char ** name)
  {
  int fd, cap;

  /* First step: Try to open the device */

  fd = open(device, O_RDONLY | O_NONBLOCK);

  if(fd < 0)
    {
    //    fprintf(stderr, "Couldn't open device %s: %s\n",
    //            device, strerror(errno));
    return 0;
    }
  /* Try a cdrom ioctl and see what happens */

  //  fprintf(stderr, "Opened %s\n", device);

  if((cap = ioctl(fd, CDROM_GET_CAPABILITY, 0)) < 0)
    {
    fprintf(stderr,
            "CDROM_GET_CAPABILITY ioctl failed for device %s\n",
            device);
    close(fd);
    return 0;
    }

  if(!(cap & CDC_PLAY_AUDIO))
    {
    fprintf(stderr,
            "Device %s doesn't support audio functions\n",
            device);
    close(fd);
    return 0;
    }
  
  /* Ok, seems the drive is ok */

  if(name)
    *name = get_device_name(fd, cap, device);
  close(fd);
  return 1;
  }

bg_device_info_t * bg_cdaudio_find_devices()
  {
  int have_scsi_new = 0;

  int i = -1;
  char * device_name;

  bg_device_info_t * ret = (bg_device_info_t *)0;

  i = -1;
  while(device_names_scsi_new[i+1])
    {
    i++;

    device_name = (char*)0;
    if(bg_cdaudio_check_device(device_names_scsi_new[i], &device_name))
      {
      have_scsi_new = 1;
      ret = bg_device_info_append(ret,
                                    device_names_scsi_new[i],
                                    device_name);
      if(device_name)
        free(device_name);
      }
    }
  if(!have_scsi_new)
    {
    i = -1;
    while(device_names_scsi_old[i+1])
      {
      i++;

      device_name = (char*)0;
      if(bg_cdaudio_check_device(device_names_scsi_old[i], &device_name))
        {
        ret = bg_device_info_append(ret,
                                      device_names_scsi_old[i],
                                      device_name);
        if(device_name)
          free(device_name);
        }
      }
    }
  i = -1;
  while(device_names_ide[i+1])
    {
    i++;

    device_name = (char*)0;
    if(bg_cdaudio_check_device(device_names_ide[i], &device_name))
      {
      ret = bg_device_info_append(ret,
                                    device_names_ide[i],
                                    device_name);
      if(device_name)
        free(device_name);
      }
    }
  return ret;
  }

void bg_cdaudio_play(int fd,
                     int start_sector,
                     int end_sector)
  {
  struct cdrom_msf msf;

  fprintf(stderr, "bg_cdaudio_play %d %d\n", start_sector, end_sector);
  
  msf.cdmsf_frame0 = start_sector % 75;
  start_sector /= 75;
  msf.cdmsf_sec0 = start_sector % 60;
  start_sector /= 60;
  msf.cdmsf_min0 = start_sector;

  fprintf(stderr, "Start %02d:%02d.%d\n", msf.cdmsf_min0, msf.cdmsf_sec0, msf.cdmsf_frame0);
    
  msf.cdmsf_frame1 = end_sector % 75;
  end_sector /= 75;
  msf.cdmsf_sec1 = end_sector % 60;
  end_sector /= 60;
  msf.cdmsf_min1 = end_sector;
  fprintf(stderr, "End %02d:%02d.%d\n", msf.cdmsf_min1, msf.cdmsf_sec1, msf.cdmsf_frame1);
  
  ioctl(fd, CDROMPLAYMSF, &msf);
  fprintf(stderr, "done\n");
  }

void bg_cdaudio_stop(int fd)
  {
  fprintf(stderr, "bg_cdaudio_stop\n");
  ioctl(fd, CDROMSTOP, 0);
  }

int bg_cdaudio_get_status(int fd, bg_cdaudio_status_t *st)
  {
  struct cdrom_subchnl subchnl;
  memset(&subchnl, 0, sizeof(subchnl));
  subchnl.cdsc_format = CDROM_MSF;
  
  if(ioctl(fd, CDROMSUBCHNL, &subchnl) < 0)
    {
    fprintf(stderr, "CDROMSUBCHNL ioctl failed\n");
    return 0;
    }
  if(subchnl.cdsc_audiostatus != CDROM_AUDIO_PLAY)
    {
    fprintf(stderr, "subchnl.cdsc_audiostatus != CDROM_AUDIO_PLAY\n");
    return 0;
    }
  //  fprintf(stderr, "Track: %d\n", subchnl.cdsc_trk);  
  st->track = subchnl.cdsc_trk - 1;
  
  st->sector = get_sector(&(subchnl.cdsc_absaddr.msf));
  
  return 1;
  }

void bg_cdaudio_set_volume(int fd, float volume)
  {
  int volume_i;
  struct cdrom_volctrl volctrl;
  volume_i = (int)(255.0 * pow(10, volume/20.0) + 0.5);
  fprintf(stderr, "bg_cdaudio_set_volume %f\n", volume);

  if(volume_i > 255)
    volume_i = 255;
  if(volume_i < 0)
    volume_i = 0;
  
  volctrl.channel0 = volume_i;
  volctrl.channel1 = volume_i;

  /* The following 2 arent really necessary */
  volctrl.channel2 = volume_i;
  volctrl.channel3 = volume_i;

  ioctl(fd, CDROMVOLCTRL, &volctrl);
  }

void bg_cdaudio_set_pause(int fd, int pause)
  {
  fprintf(stderr, "bg_cdaudio_set_pause %d\n", pause);

  if(pause)
    ioctl(fd, CDROMPAUSE, 0);
  else
    ioctl(fd, CDROMRESUME, 0);
  }
