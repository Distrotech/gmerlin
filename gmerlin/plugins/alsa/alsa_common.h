#include <gavl/gavl.h>
#include <alsa/asoundlib.h>

/* For reading, the sample format, num_channels and samplerate must be set */

snd_pcm_t * bg_alsa_open_read(const char * card, gavl_audio_format_t * format);

/* For writing, the complete format must be set, values will be changed if not compatible */

snd_pcm_t * bg_alsa_open_write(const char * card, gavl_audio_format_t * format);

/* Builds a parameter array for all available cards */

void bg_alsa_create_card_parameters(bg_parameter_info_t * ret,
                                    bg_parameter_info_t * per_card_params);

