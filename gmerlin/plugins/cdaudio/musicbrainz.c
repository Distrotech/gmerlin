#include <string.h>
#include <stdlib.h>
#include <musicbrainz/mb_c.h>

#include <utils.h>
#include "cdaudio.h"
#include "sha1.h"

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

#define DISCID_SIZE 33

/* Calculate the CDIndex DISC ID from a bg_cdaudio_index */

void get_cdindex_id(bg_cdaudio_index_t * idx, char disc_id[DISCID_SIZE])
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

#if 0
/*
 * Test CDindex generation (the example from http://musicbrainz.org/disc.html)
 *
 * Return value must be "MUtMmKN402WPj3_VFsgUelxpc8U-"
 */

static void test_cdindex()
  {
  bg_cdaudio_index_t * idx;
  char disc_id[DISCID_SIZE];

  idx = calloc(1, sizeof(*idx));
  idx->num_tracks = 15;
  idx->tracks = calloc(idx->num_tracks, sizeof(*(idx->tracks)));

  idx->tracks[0].first_sector = 0+150;
  idx->tracks[1].first_sector = 18641+150;
  idx->tracks[2].first_sector = 34667+150;
  idx->tracks[3].first_sector = 56350+150;
  idx->tracks[4].first_sector = 77006+150;
  idx->tracks[5].first_sector = 106094+150;
  idx->tracks[6].first_sector = 125729+150;
  idx->tracks[7].first_sector = 149785+150;
  idx->tracks[8].first_sector = 168885+150;
  idx->tracks[9].first_sector = 185910+150;
  idx->tracks[10].first_sector = 205829+150;
  idx->tracks[11].first_sector = 230142+150;
  idx->tracks[12].first_sector = 246659+150;
  idx->tracks[13].first_sector = 265614+150;
  idx->tracks[14].first_sector = 289479+150;
  idx->tracks[14].last_sector  = 325731+150;
  get_cdindex_id(idx, disc_id);

  fprintf(stderr, "ID: %s\n", disc_id);

  free(idx->tracks);
  free(idx);
  }
#endif

int bg_cdaudio_get_metadata_musicbrainz(bg_cdaudio_index_t * idx,
                                        bg_track_info_t * info,
                                        char * musicbrainz_host,
                                        int musicbrainz_port,
                                        char * musicbrainz_proxy_host,
                                        int musicbrainz_proxy_port)
  {
  int num_tracks, is_multiple_artist, i, j;
  
  int ret = 0;
  char * args[2];
  char data[256], temp[256], album_name[256], artist[256];
  
  int result;
  char disc_id[DISCID_SIZE];
  musicbrainz_t m;
  
  m = mb_New();
  mb_UseUTF8(m, 1);
  mb_SetDepth(m, 4);

  mb_SetServer(m, musicbrainz_host, musicbrainz_port);

  if(musicbrainz_proxy_host && *musicbrainz_proxy_host)
    mb_SetProxy(m, musicbrainz_proxy_host, musicbrainz_proxy_port);
  
  //  test_cdindex();

  get_cdindex_id(idx, disc_id);
  fprintf(stderr, "ID: %s\n", disc_id);

  args[0] = disc_id;
  args[1] = (char*)0;
  
  result =  mb_QueryWithArgs(m, MBQ_GetCDInfoFromCDIndexId, args);

  if(!result)
    goto fail;

  /* Now extract the metadata */

  // Select the first album
  if(!mb_Select1(m, MBS_SelectAlbum, 1))
    {
    fprintf(stderr, "Query failed\n");
    goto fail;
    }

  // Pull back the album id to see if we got the album
  if (!mb_GetResultData(m, MBE_AlbumGetAlbumId, data, 256))
    {
    fprintf(stderr, "Query failed\n");
    goto fail;
    }

  //  fprintf(stderr, "Query succeeded %s\n", data);

  mb_GetResultData(m, MBE_AlbumGetAlbumName, album_name, 256);

  //  fprintf(stderr, "Name: %s\n", album_name);

  num_tracks = mb_GetResultInt(m, MBE_AlbumGetNumTracks);
  is_multiple_artist = 0;
  
  // Check to see if there is more than one artist for this album
  for(i = 1; i <= num_tracks; i++)
    {
    if (!mb_GetResultData1(m, MBE_AlbumGetArtistId, data, 256, i))
      break;
    
    if (i == 1)
      strcpy(temp, data);
    
    if(strcmp(temp, data))
      {
      is_multiple_artist = 1;
      break;
      }
    }

  if(!is_multiple_artist)
    mb_GetResultData1(m, MBE_AlbumGetArtistName, artist, 256, 1);

  //  fprintf(stderr, "Multiple artists: %s\n", (is_multiple_artist ? "Yes" : "No"));

  for(i = 0; i < num_tracks; i++)
    {
    if(!idx->tracks[i].is_audio)
      continue;
    
    j = idx->tracks[i].index;
    
    /* Title */
    mb_GetResultData1(m, MBE_AlbumGetTrackName, data, 256, i+1);
    info[j].metadata.title = bg_strdup(info[j].metadata.title, data);

    /* Artist */
    if(is_multiple_artist)
      {
      mb_GetResultData1(m, MBE_AlbumGetArtistName, data, 256, i+1);
      info[j].metadata.artist = bg_strdup(info[j].metadata.artist, data);
      info[j].name = bg_sprintf("%s - %s", info[j].metadata.artist, info[j].metadata.title);
      }
    else
      {
      info[j].metadata.artist = bg_strdup(info[j].metadata.artist, artist);
      info[j].name = bg_sprintf("%s", info[j].metadata.title);
      }

    /* Album name */

    info[j].metadata.album = bg_strdup(info[j].metadata.album, album_name);
    }
  ret = 1;
  fail:
  mb_Delete(m);
  return ret;
  }
