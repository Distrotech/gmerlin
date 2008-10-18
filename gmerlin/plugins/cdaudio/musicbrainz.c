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


#include <string.h>
#include <stdlib.h>
#include <musicbrainz/mb_c.h>

#include <gmerlin/utils.h>
#include "cdaudio.h"

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


  free(idx->tracks);
  free(idx);
  }
#endif

int bg_cdaudio_get_metadata_musicbrainz(bg_cdaudio_index_t * idx,
                                        bg_track_info_t * info,
                                        char * disc_id,
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
  musicbrainz_t m;
  
  m = mb_New();
  mb_UseUTF8(m, 1);
  mb_SetDepth(m, 4);

  mb_SetServer(m, musicbrainz_host, musicbrainz_port);

  if(musicbrainz_proxy_host && *musicbrainz_proxy_host)
    mb_SetProxy(m, musicbrainz_proxy_host, musicbrainz_proxy_port);
  
  //  test_cdindex();
    
  args[0] = disc_id;
  args[1] = (char*)0;
  
  result =  mb_QueryWithArgs(m, MBQ_GetCDInfoFromCDIndexId, args);

  if(!result)
    goto fail;

  /* Now extract the metadata */

  // Select the first album
  if(!mb_Select1(m, MBS_SelectAlbum, 1))
    {
    goto fail;
    }

  // Pull back the album id to see if we got the album
  if (!mb_GetResultData(m, MBE_AlbumGetAlbumId, data, 256))
    {
    goto fail;
    }


  mb_GetResultData(m, MBE_AlbumGetAlbumName, album_name, 256);


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
      }
    else
      {
      info[j].metadata.artist = bg_strdup(info[j].metadata.artist, artist);
      }

    /* Album name */

    info[j].metadata.album = bg_strdup(info[j].metadata.album, album_name);
    }
  ret = 1;
  fail:
  mb_Delete(m);
  return ret;
  }
