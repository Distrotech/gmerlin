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

#include <linux/cdrom.h>
#include <unistd.h>
#include <sys/ioctl.h>                                                             
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>

#include <avdec_private.h>

extern bgav_demuxer_t bgav_demuxer_mpegps;

#define SECTOR_SIZE 2324

typedef struct
  {
  int fd;

  int current_track;
  int current_sector;

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

static int read_toc(vcd_priv * priv)
  {
  int i;
  struct cdrom_tochdr hdr;
  struct cdrom_tocentry entry;

  if(ioctl(priv->fd, CDROMREADTOCHDR, &hdr) == -1 )
    return 0;

  priv->num_tracks = hdr.cdth_trk1 - hdr.cdth_trk0+1;
  priv->tracks = calloc(priv->num_tracks, sizeof(*(priv->tracks)));
  
  for(i = 0; i < priv->num_tracks; i++)
    {
    entry.cdte_track = hdr.cdth_trk0 + i;
    entry.cdte_format = CDROM_LBA;
    
    if(ioctl(priv->fd, CDROMREADTOCENTRY, &entry) == -1 )
      return 0;

    if(i)
      priv->tracks[i-1].end_sector = entry.cdte_addr.lba - 1;
    priv->tracks[i].start_sector = entry.cdte_addr.lba;
    }

  entry.cdte_track = CDROM_LEADOUT;
  entry.cdte_format = CDROM_LBA;
  if(ioctl(priv->fd, CDROMREADTOCENTRY, &entry) == -1 )
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

    stream =  bgav_track_add_video_stream(track);
    stream->fourcc = BGAV_MK_FOURCC('m', 'p', 'g', 'v');
    stream->stream_id = 0xe0;
    track->name = bgav_sprintf("VCD Track %d", i);
    }
  }

static int open_vcd(bgav_input_context_t * ctx, const char * url)
  {
  vcd_priv * priv;
  //  fprintf(stderr, "OPEN VCD\n");
  
  priv = calloc(1, sizeof(*priv));
  
  ctx->priv = priv;

  priv->buffer = priv->sector + 24;
  priv->buffer_ptr = priv->buffer + SECTOR_SIZE;
  
  priv->fd = open(url, O_RDONLY|O_EXCL);
  if(priv->fd == -1)
    return 0;
  
  /* Read TOC */

  if(!read_toc(priv))
    return 0;
  toc_2_tt(ctx);

  /* Create demuxer */
  
  ctx->demuxer = bgav_demuxer_create(&bgav_demuxer_mpegps, ctx);
  
  return 1;
  }

static int read_sector(vcd_priv * priv)
  {
  struct cdrom_msf cdrom_msf;
  int secnum;

  do
    {
    //    fprintf(stderr, "read_sector %d %d %d\n",
    //            priv->current_sector,
    //            priv->current_track,
    //            priv->tracks[priv->current_track].end_sector);
    
    if(priv->current_sector > priv->tracks[priv->current_track].end_sector)
      return 0;

    secnum = priv->current_sector;
    
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
    priv->current_sector++;
    
    } while((priv->sector[18] & ~0x01) == 0x60);
  //  fprintf(stderr, "read sector done\n");
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
    priv->tracks[priv->current_track].start_sector + ctx->position / SECTOR_SIZE;

  sector_offset = ctx->position % SECTOR_SIZE;

  if(sector == priv->current_sector - 1)
    priv->buffer_ptr = priv->buffer + sector_offset;
  else
    {
    priv->current_sector = sector;
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

static void select_track_vcd(bgav_input_context_t * ctx, int track)
  {
  vcd_priv * priv;
  priv = (vcd_priv*)(ctx->priv);

  //  fprintf(stderr, "Select track VCD\n");

  priv->current_track = track+1;
  priv->current_sector = priv->tracks[priv->current_track].start_sector;
  ctx->position = 0;
  ctx->total_bytes = SECTOR_SIZE *
    (priv->tracks[priv->current_track].end_sector -
     priv->tracks[priv->current_track].start_sector + 1);
  }

bgav_input_t bgav_input_vcd =
  {
    open:          open_vcd,
    read:          read_vcd,
    seek_byte:     seek_byte_vcd,
    close:         close_vcd,
    select_track:  select_track_vcd
  };

