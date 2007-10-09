
typedef struct gavl_mix_output_channel_s gavl_mix_output_channel_t;

typedef void (*gavl_mix_func_t)(gavl_mix_output_channel_t * channel,
                                const gavl_audio_frame_t * input_frame,
                                gavl_audio_frame_t * output_frame);
                                
typedef struct
  {
  gavl_mix_func_t copy_func;
  gavl_mix_func_t mix_1_to_1;
  gavl_mix_func_t mix_2_to_1;
  gavl_mix_func_t mix_3_to_1;
  gavl_mix_func_t mix_4_to_1;
  gavl_mix_func_t mix_5_to_1;
  gavl_mix_func_t mix_6_to_1;
  gavl_mix_func_t mix_all_to_1;
  } gavl_mixer_table_t;

typedef struct gavl_mix_input_channel_s
  {
  int index; /* Which input channel */
  union      /* Weighing factor     */
    {
    float    f_float;
    int8_t   f_8;
    int16_t  f_16;
    int32_t  f_32;
    } factor;
  } gavl_mix_input_channel_t;

struct gavl_mix_output_channel_s
  {
  int num_inputs;
  int index;
  gavl_mix_input_channel_t inputs[GAVL_MAX_CHANNELS];
  gavl_mix_func_t func;
  };

struct gavl_mix_matrix_s
  {
  gavl_mix_output_channel_t output_channels[GAVL_MAX_CHANNELS];
  gavl_mixer_table_t mixer_table;
  };

gavl_mix_matrix_t *
gavl_create_mix_matrix(gavl_audio_options_t * opt,
                       gavl_audio_format_t * in,
                       gavl_audio_format_t * out);

void gavl_destroy_mix_matrix(gavl_mix_matrix_t *);

void gavl_mix_audio(gavl_audio_convert_context_t * ctx);

void gavl_setup_mix_funcs_c(gavl_mixer_table_t * c,
                            gavl_audio_format_t * f);
