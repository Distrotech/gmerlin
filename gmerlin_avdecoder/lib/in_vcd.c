/*****************************************************************
 
  in_vcd.c
 
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
#include <config.h>

#ifdef HAVE_LINUX_CDROM_H
#include <linux/cdrom.h>
#endif

#ifdef HAVE_LINUX_HDREG_H
#include <linux/hdreg.h>
#endif

#include <unistd.h>
#include <sys/ioctl.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>


#include <avdec_private.h>

extern bgav_demuxer_t bgav_demuxer_mpegps;

#define SECTOR_SIZE 2324

typedef struct
  {
  int fd;

  int current_track;
  int next_sector; /* Next sector to be read */
  int last_sector; /* Sector currently in buffer */
  int num_tracks; 
  
  struct
    {
    uint32_t start_sector;
    uint32_t end_sector;
    } * tracks;

  uint8_t sector[2352];

  uint8_t * buffer;
  uint8_t * buffer_ptr;
    
  } vcd_priv;

static void select_track_vcd(bgav_input_context_t * ctx, int track)
  {
  vcd_priv * priv;
  priv = (vcd_priv*)(ctx->priv);

//  fprintf(stderr, "Select track VCD\n");

  priv->current_track = track+1;
  priv->next_sector = priv->tracks[priv->current_track].start_sector;
  ctx->position = 0;
  ctx->total_bytes = SECTOR_SIZE *
    (priv->tracks[priv->current_track].end_sector -
     priv->tracks[priv->current_track].start_sector + 1);
  priv->buffer_ptr = priv->buffer;
  }

static int read_toc(vcd_priv * priv)
  {
  int i;
  struct cdrom_tochdr hdr;
  struct cdrom_tocentry entry;

  if(ioctl(priv->fd, CDROMREADTOCHDR, &hdr) < 0 )
    return 0;

  priv->num_tracks = hdr.cdth_trk1 - hdr.cdth_trk0+1;

  /* VCD needs at least 2 tracks */
  if(priv->num_tracks < 2)
    return 0;
  
  priv->tracks = calloc(priv->num_tracks, sizeof(*(priv->tracks)));
  
  for(i = 0; i < priv->num_tracks; i++)
    {
    
    entry.cdte_track = hdr.cdth_trk0 + i;
    entry.cdte_format = CDROM_LBA;
    
    if(ioctl(priv->fd, CDROMREADTOCENTRY, &entry) < 0 )
      return 0;

    //    fprintf(stderr, "Track %d: Datamode: %d\n", i+1, entry.cdte_datamode);
    if(i)
      priv->tracks[i-1].end_sector = entry.cdte_addr.lba - 1;
    priv->tracks[i].start_sector = entry.cdte_addr.lba;
    }

  entry.cdte_track = CDROM_LEADOUT;
  entry.cdte_format = CDROM_LBA;
  if(ioctl(priv->fd, CDROMREADTOCENTRY, &entry) < 0 )
    return 0;

  priv->tracks[priv->num_tracks-1].end_sector =
    entry.cdte_addr.lba - 1;
  
  /* Dump this */
#if 0  
  for(i = priv->num_tracks-1; i>=0; i--)
    {
    fprintf(stderr, "Track %d, S: %d, E: %d\n",
            i+1, priv->tracks[i].start_sector, priv->tracks[i].end_sector);
    }
#endif
  return 1;
  }

void toc_2_tt(bgav_input_context_t * ctx)
  {
  bgav_stream_t * stream;
  bgav_track_t * track;
  int i;
  
  vcd_priv * priv;
  priv = (vcd_priv*)(ctx->priv);

  ctx->tt = bgav_track_table_create(priv->num_tracks-1);

  for(i = 1; i < priv->num_tracks; i++)
    {
    track = &(ctx->tt->tracks[i-1]);

    stream =  bgav_track_add_audio_stream(track);
    stream->fourcc = BGAV_MK_FOURCC('.', 'm', 'p', '2');
    stream->stream_id = 0xc0;
    stream->timescale = 90000;
        
    stream =  bgav_track_add_video_stream(track);
    stream->fourcc = BGAV_MK_FOURCC('m', 'p', 'g', 'v');
    stream->stream_id = 0xe0;
    stream->timescale = 90000;
    
    track->name = bgav_sprintf("VCD Track %d", i);
    track->duration = GAVL_TIME_UNDEFINED;
    }
  }

static int open_vcd(bgav_input_context_t * ctx, const char * url)
  {
  int i;
  vcd_priv * priv;
  //  fprintf(stderr, "OPEN VCD\n");

  //  bgav_find_devices_vcd();
    
  priv = calloc(1, sizeof(*priv));
  
  ctx->priv = priv;

  priv->buffer = priv->sector + 24;
  priv->buffer_ptr = priv->buffer + SECTOR_SIZE;
  
  priv->fd = open(url, O_RDONLY|O_NONBLOCK);
  if(priv->fd < 0)
    {
    fprintf(stderr, "VCD: Open failed\n");
    return 0;
    }
  /* Get some infos */

  //  dump_cdrom(priv->fd);
  
  /* Read TOC */
  
  if(!read_toc(priv))
    {
    fprintf(stderr, "VCD: Read toc failed\n");
    return 0;
    }
  toc_2_tt(ctx);

  /* Create demuxer */
  
  ctx->demuxer = bgav_demuxer_create(&bgav_demuxer_mpegps, ctx);
  ctx->demuxer->tt = ctx->tt;

  /* Now, loop through all tracks and let the demuxer find the durations */

  for(i = 0; i < ctx->tt->num_tracks; i++)
    {
    select_track_vcd(ctx, i);
    bgav_track_table_select_track(ctx->tt, i);
    bgav_demuxer_start(ctx->demuxer, NULL);
    bgav_demuxer_stop(ctx->demuxer);
    }
  return 1;
  }

static int read_sector(vcd_priv * priv)
  {
  struct cdrom_msf cdrom_msf;
  int secnum;

  fprintf(stderr, "Read sector %d ", priv->next_sector);

  do
    {
#if 0
    fprintf(stderr, "read_sector %d %d %d\n",
            priv->next_sector,
            priv->current_track,
            priv->tracks[priv->current_track].end_sector);
#endif    
    if(priv->next_sector > priv->tracks[priv->current_track].end_sector)
      return 0;

    secnum = priv->next_sector;
    
    cdrom_msf.cdmsf_frame0 = secnum % 75;
    secnum /= 75;
    cdrom_msf.cdmsf_sec0 = secnum % 60;
    secnum /= 60;
    cdrom_msf.cdmsf_min0 = secnum;

    memcpy(&(priv->sector), &cdrom_msf, sizeof(cdrom_msf));
        
    if(ioctl(priv->fd, CDROMREADRAW, &(priv->sector)) == -1)
      {
      fprintf(stderr, "CDROMREADRAW ioctl failed\n");
      return 0;
      }
    priv->next_sector++;
    } while((priv->sector[18] & ~0x01) == 0x60);
  priv->last_sector = priv->next_sector - 1;
  fprintf(stderr, " %d\n", priv->last_sector);
  priv->buffer_ptr = priv->buffer;
  //  bgav_hexdump(priv->sector, 2352, 16);

  return 1;
  }

static int read_vcd(bgav_input_context_t* ctx,
                    uint8_t * buffer, int len)
  {
  int bytes_read = 0;
  int bytes_to_copy;
    
  vcd_priv * priv;
  priv = (vcd_priv*)(ctx->priv);

//  fprintf(stderr, "Read VCD %d\n", len);
  
  while(bytes_read < len)
    {
    if(priv->buffer_ptr - priv->buffer >= SECTOR_SIZE)
      {
      if(!read_sector(priv))
        return bytes_read;
      }

    if(len - bytes_read < SECTOR_SIZE - (priv->buffer_ptr - priv->buffer))
      bytes_to_copy = len - bytes_read;
    else
      bytes_to_copy = SECTOR_SIZE - (priv->buffer_ptr - priv->buffer);

    memcpy(buffer + bytes_read, priv->buffer_ptr, bytes_to_copy);
    priv->buffer_ptr += bytes_to_copy;
    bytes_read += bytes_to_copy;
    }
  return bytes_read;
  }

static int64_t seek_byte_vcd(bgav_input_context_t * ctx,
                             int64_t pos, int whence)
  {
  vcd_priv * priv;
  int sector;
  int sector_offset;
  priv = (vcd_priv*)(ctx->priv);
  sector =
    priv->tracks[priv->current_track].start_sector + 
    ctx->position / SECTOR_SIZE;

  sector_offset = ctx->position % SECTOR_SIZE;

  fprintf(stderr, "Seek: Pos: %lld, whence: %d, S: %d, O: %d\n", 
          pos, whence, sector, sector_offset);

  if(sector == priv->last_sector)
    priv->buffer_ptr = priv->buffer + sector_offset;
  else
    {
    priv->next_sector = sector;
    read_sector(priv);
    priv->buffer_ptr = priv->buffer + sector_offset;
    }
  return ctx->position;
  }

static void    close_vcd(bgav_input_context_t * ctx)
  {
  vcd_priv * priv;
  //  fprintf(stderr, "CLOSE VCD\n");
  priv = (vcd_priv*)(ctx->priv);
  if(priv->fd > -1)
    close(priv->fd);
  if(priv->tracks)
    free(priv->tracks);
  free(priv);
  return;
  }

bgav_input_t bgav_input_vcd =
  {
    name:          "vcd",
    open:          open_vcd,
    read:          read_vcd,
    seek_byte:     seek_byte_vcd,
    close:         close_vcd,
    select_track:  select_track_vcd,
  };

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
    return bgav_strndup(driveid.model, NULL);
    }
  
#endif
  
  if(cap & CDC_DVD_R)
    {
    return bgav_sprintf("DVD Writer (%s)", device);
    }
  else if(cap & CDC_CD_R)
    {
    return bgav_sprintf("CD Writer (%s)", device);
    }
  else if(cap & CDC_DVD)
    {
    return bgav_sprintf("DVD Drive (%s)", device);
    }
  return bgav_sprintf("CDrom Drive (%s)", device);
  }

int bgav_check_device_vcd(const char * device, char ** name)
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
  
  /* Ok, seems the drive is ok */

  if(name)
    *name = get_device_name(fd, cap, device);
  close(fd);
  return 1;
  }

bgav_device_info_t * bgav_find_devices_vcd()
  {
  int have_scsi_new = 0;
  
  int i = -1;
  char * device_name;
  
  bgav_device_info_t * ret = (bgav_device_info_t *)0;

  i = -1;
  while(device_names_scsi_new[i+1])
    {
    i++;
    
    device_name = (char*)0;
    if(bgav_check_device_vcd(device_names_scsi_new[i], &device_name))
      {
      have_scsi_new = 1;
      ret = bgav_device_info_append(ret,
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
      if(bgav_check_device_vcd(device_names_scsi_old[i], &device_name))
        {
        ret = bgav_device_info_append(ret,
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
    if(bgav_check_device_vcd(device_names_ide[i], &device_name))
      {
      ret = bgav_device_info_append(ret,
                                    device_names_ide[i],
                                    device_name);
      if(device_name)
        free(device_name);
      }
    }
  
  //  bgav_device_info_dump(ret);
  return ret;
  }
