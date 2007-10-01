/*****************************************************************

  bgflac.h

  Copyright (c) 2006 by Burkhard Plaum - plaum@ipf.uni-stuttgart.de

  http://gmerlin.sourceforge.net

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111, USA.

*****************************************************************/

#define MAKE_VERSION(maj,min,pat) ((maj<<16)|(min<<8)|pat)
#define BGAV_FLAC_VERSION_INT MAKE_VERSION(BGAV_FLAC_MAJOR,BGAV_FLAC_MINOR,BGAV_FLAC_PATCHLEVEL)

/* Dirty hack to support different FLAC API versions */

#if BGAV_FLAC_VERSION_INT <= MAKE_VERSION(1, 1, 2)
#include <FLAC/file_encoder.h>
#include <FLAC/file_decoder.h> /* For creating the seektable */
#else
#define FLAC__FileEncoder FLAC__StreamEncoder 
#define FLAC__FileDecoder FLAC__StreamDecoder
#define FLAC__file_encoder_new FLAC__stream_encoder_new
#define FLAC__file_encoder_set_metadata FLAC__stream_encoder_set_metadata
#define FLAC__file_encoder_process FLAC__stream_encoder_process 
#define FLAC__FileDecoderWriteStatus FLAC__StreamDecoderWriteStatus
#define FLAC__file_decoder_get_decode_position FLAC__stream_decoder_get_decode_position 
#define FLAC__file_decoder_new FLAC__stream_decoder_new
#define FLAC__file_decoder_set_md5_checking FLAC__stream_decoder_set_md5_checking
#define FLAC__file_decoder_set_metadata_ignore_all FLAC__stream_decoder_set_metadata_ignore_all
#define FLAC__file_decoder_process_until_end_of_metadata FLAC__stream_decoder_process_until_end_of_metadata
#define FLAC__file_decoder_process_until_end_of_file FLAC__stream_decoder_process_until_end_of_stream
#define FLAC__file_decoder_delete FLAC__stream_decoder_delete
#define FLAC__file_encoder_finish FLAC__stream_encoder_finish
#define FLAC__file_encoder_delete FLAC__stream_encoder_delete
#endif

#include <FLAC/stream_encoder.h> /* Flac in Ogg */
#include <FLAC/metadata.h>


typedef struct
  {
  int clevel; /* Compression level 0..8 */

  int bits_per_sample;
  int shift_bits;
  int divisor;
  int samples_per_block;
    
  void (*copy_frame)(int32_t * dst[], gavl_audio_frame_t * src,
                     int num_channels);

  /* Buffer */
    
  int32_t * buffer[GAVL_MAX_CHANNELS];
  int buffer_alloc; /* In samples */
  
  gavl_audio_format_t *format;

  FLAC__StreamMetadata * vorbis_comment;
  } bg_flac_t;

bg_parameter_info_t * bg_flac_get_parameters(void * data);
  
void bg_flac_set_parameter(void * data, const char * name, const bg_parameter_value_t * val);

void bg_flac_init_stream_encoder(bg_flac_t * flac, FLAC__StreamEncoder * enc);

#if BGAV_FLAC_VERSION_INT <= MAKE_VERSION(1, 1, 2)
void bg_flac_init_file_encoder(bg_flac_t * flac, FLAC__FileEncoder * enc);
#else
#define bg_flac_init_file_encoder(a,b) bg_flac_init_stream_encoder(a,b)
#endif

void bg_flac_init_metadata(bg_flac_t * flac, bg_metadata_t * metadata);

void bg_flac_prepare_audio_frame(bg_flac_t * flac, gavl_audio_frame_t * frame);
void bg_flac_free(bg_flac_t * flac);

