/*****************************************************************

  cdaudio.h

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
#include <plugin.h>

#define DISCID_SIZE 33

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

void bg_cdaudio_get_disc_id(bg_cdaudio_index_t * idx, char disc_id[DISCID_SIZE]);


/* Functions for getting the metadata */

#ifdef HAVE_MUSICBRAINZ
/* Try to get the metadata using musicbrainz */
int bg_cdaudio_get_metadata_musicbrainz(bg_cdaudio_index_t*,
                                        bg_track_info_t * info,
                                        char * disc_id,
                                        char * musicbrainz_host,
                                        int musicbrainz_port,
                                        char * musicbrainz_proxy_host,
                                        int musicbrainz_proxy_port);
#endif

/*
 *  Ripping support
 *  Several versions (cdparanoia, simple linux ripper) can go here
 */

void * bg_cdaudio_rip_create();

int bg_cdaudio_rip_init(void *, char * device, int start_sector, int start_sector_lba,
                        int * frames_per_read);

int bg_cdaudio_rip_rip(void * data, gavl_audio_frame_t * f);

/* Sector is absolute */

void bg_cdaudio_rip_seek(void * data, int sector, int start_sector_lba);

void bg_cdaudio_rip_close(void * data);

bg_parameter_info_t * bg_cdaudio_rip_get_parameters();

int
bg_cdaudio_rip_set_parameter(void * data, char * name,
                             bg_parameter_value_t * val);

void bg_cdaudio_rip_destroy(void * data);

/* Load and save cd metadata */

int bg_cdaudio_load(bg_track_info_t * tracks, const char * filename);

void bg_cdaudio_save(bg_track_info_t * tracks, int num_tracks,
                     const char * filename);

