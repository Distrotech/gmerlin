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

  sprintf(temp, "%08X", idx->tracks[idx->num_tracks-1].last_sector + 1);
  bg_cdaudio_sha_update(&sha, (unsigned char *)temp, strlen(temp));

  for(i = 0; i < idx->num_tracks; i++)
    {
    sprintf(temp, "%08X", idx->tracks[i].first_sector);
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
