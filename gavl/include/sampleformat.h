typedef struct
  {

  gavl_audio_func_t swap_sign_8;
  gavl_audio_func_t swap_sign_16;
  
  /* 8 -> 16 bits */

  gavl_audio_func_t s_8_to_s_16;
  gavl_audio_func_t u_8_to_s_16;

  gavl_audio_func_t s_8_to_u_16;
  gavl_audio_func_t u_8_to_u_16;

  /* 8 -> 32 bits */

  gavl_audio_func_t s_8_to_s_32;
  gavl_audio_func_t u_8_to_s_32;
  
  /* 16 -> 8 bits */
  
  gavl_audio_func_t convert_16_to_8_swap;
  gavl_audio_func_t convert_16_to_8;

  /* 16 -> 32 bits */

  gavl_audio_func_t s_16_to_s_32;
  gavl_audio_func_t u_16_to_s_32;
  
  /* 32 -> 8 bits */
  
  gavl_audio_func_t convert_32_to_8_swap;
  gavl_audio_func_t convert_32_to_8;

  /* 32 -> 16 bits */
  
  gavl_audio_func_t convert_32_to_16_swap;
  gavl_audio_func_t convert_32_to_16;
  
  /* Int to Float  */

  gavl_audio_func_t convert_s8_to_float;
  gavl_audio_func_t convert_u8_to_float;

  gavl_audio_func_t convert_s16_to_float;
  gavl_audio_func_t convert_u16_to_float;

  gavl_audio_func_t convert_s32_to_float;

  /* Float to int */
  
  gavl_audio_func_t convert_float_to_s8;
  gavl_audio_func_t convert_float_to_u8;

  gavl_audio_func_t convert_float_to_s16;
  gavl_audio_func_t convert_float_to_u16;

  gavl_audio_func_t convert_float_to_s32;
  } gavl_sampleformat_table_t;

gavl_sampleformat_table_t *
gavl_create_sampleformat_table(gavl_audio_options_t * opt);

void gavl_init_sampleformat_funcs_c(gavl_sampleformat_table_t * t);

gavl_audio_func_t
gavl_find_sampleformat_converter(gavl_sampleformat_table_t * t,
                                 gavl_audio_format_t * in,
                                 gavl_audio_format_t * out);

void gavl_destroy_sampleformat_table(gavl_sampleformat_table_t * t);

