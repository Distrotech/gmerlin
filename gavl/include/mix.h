typedef void (*gavl_mix_func_t)(gavl_audio_convert_context_t * ctx,
                                int output_index);

typedef struct
  {
  gavl_mix_func_t mix_1_to_1;
  gavl_mix_func_t mix_2_to_1;
  gavl_mix_func_t mix_3_to_1;
  gavl_mix_func_t mix_4_to_1;
  gavl_mix_func_t mix_5_to_1;
  gavl_mix_func_t mix_6_to_1;
  } gavl_mixer_table_t;

struct gavl_mix_context_s
  {
  struct
    {
    int num_inputs;
    int inputs[GAVL_MAX_CHANNELS];

    union
      {
      float f;
      int8_t s_8;
      uint8_t u_8;
      int16_t s_16;
      uint16_t u_16;
      } factors[GAVL_MAX_CHANNELS];

    gavl_mix_func_t func;
    } matrix[GAVL_MAX_CHANNELS];

  //  int out_channels;

  float f_matrix[GAVL_MAX_CHANNELS][GAVL_MAX_CHANNELS];

  float clev;
  float slev;
  };

gavl_mix_context_t *
gavl_create_mix_context(gavl_audio_options_t * opt,
                        gavl_audio_format_t * in,
                        gavl_audio_format_t * out);

void gavl_destroy_mix_context(gavl_mix_context_t *);

void gavl_mix_audio(gavl_audio_convert_context_t * ctx);

void gavl_setup_mix_funcs_c(gavl_mixer_table_t * c,
                            gavl_audio_format_t * f);
