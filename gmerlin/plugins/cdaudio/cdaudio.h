#include <config.h>
#include <plugin.h>

/* Index structure */

typedef struct
  {
  int num_tracks;
  int num_audio_tracks;
  
  struct
    {
    uint32_t first_sector;
    uint32_t last_sector;

    /* We read in all available tracks. This flag signals if we can play audio */
    int is_audio;
    int index; /* Index into the track_info structre */
    } * tracks;

  } bg_cdaudio_index_t;

void bg_cdaudio_index_dump(bg_cdaudio_index_t*);
void bg_cdaudio_index_free(bg_cdaudio_index_t*);

/* CD status (obtained periodically during playback) */

typedef struct
  {
  int track;
  int sector;
  } bg_cdaudio_status_t;

/* Stuff, which varies from OS to OS.
   For now, linux is the only supported
   platform */

int bg_cdaudio_open(const char * device);

bg_cdaudio_index_t * bg_cdaudio_get_index(int fd);

void bg_cdaudio_close(int);
int bg_cdaudio_check_device(const char * device, char ** name);
bg_device_info_t * bg_cdaudio_find_devices();

void bg_cdaudio_play(int fd, int first_sector, int last_sector);
void bg_cdaudio_stop(int fd);

/*
 * Get the status (time and track) of the currently played CD
 * The st structure MUST be saved between calls
 */

int bg_cdaudio_get_status(int fd, bg_cdaudio_status_t *st);

void bg_cdaudio_set_volume(int fd, float volume);

void bg_cdaudio_set_pause(int fd, int pause);



/* Functions for getting the metadata */

#ifdef HAVE_MUSICBRAINZ
/* Try to get the metadata using musicbrainz */
int bg_cdaudio_get_metadata_musicbrainz(bg_cdaudio_index_t*,
                                        bg_track_info_t * info,
                                        char * musicbrainz_host,
                                        int musicbrainz_port,
                                        char * musicbrainz_proxy_host,
                                        int musicbrainz_proxy_port);
#endif

