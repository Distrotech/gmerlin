
#include <stdlib.h>
#include <stdio.h>
#include "cdaudio.h"

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

