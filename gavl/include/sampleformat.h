typedef struct
  {

  gavl_audio_func_t swap_sign_8;
  gavl_audio_func_t swap_sign_16;
  gavl_audio_func_t swap_sign_16_oe;

  gavl_audio_func_t swap_endian_16;
  
  gavl_audio_func_t swap_sign_endian_16;
  gavl_audio_func_t swap_endian_sign_16;

  /* 8 -> 16 bits */

  gavl_audio_func_t s_8_to_s_16;
  gavl_audio_func_t s_8_to_s_16_oe;

  gavl_audio_func_t u_8_to_s_16;
  gavl_audio_func_t u_8_to_s_16_oe;

  gavl_audio_func_t s_8_to_u_16;
  gavl_audio_func_t u_8_to_u_16;

  /* 16 -> 8 bits */
  
  gavl_audio_func_t convert_16_to_8;
  gavl_audio_func_t convert_16_to_8_sign;

  gavl_audio_func_t convert_16_to_8_oe;
  gavl_audio_func_t convert_16_to_8_sign_oe;

  /* Int to Float  */

  gavl_audio_func_t convert_s8_to_float;
  gavl_audio_func_t convert_u8_to_float;

  gavl_audio_func_t convert_s16ne_to_float;
  gavl_audio_func_t convert_u16ne_to_float;

  gavl_audio_func_t convert_s16oe_to_float;
  gavl_audio_func_t convert_u16oe_to_float;

  /* Float to int */
  
  gavl_audio_func_t convert_float_to_s8;
  gavl_audio_func_t convert_float_to_u8;

  gavl_audio_func_t convert_float_to_s16ne;
  gavl_audio_func_t convert_float_to_u16ne;

  gavl_audio_func_t convert_float_to_s16oe;
  gavl_audio_func_t convert_float_to_u16oe;
  } gavl_sampleformat_table_t;

gavl_sampleformat_table_t *
gavl_create_sampleformat_table(gavl_audio_options_t * opt);

void gavl_init_sampleformat_funcs_c(gavl_sampleformat_table_t * t);

gavl_audio_func_t
gavl_find_sampleformat_converter(gavl_sampleformat_table_t * t,
                                 gavl_audio_format_t * in,
                                 gavl_audio_format_t * out);

void gavl_destroy_sampleformat_table(gavl_sampleformat_table_t * t);

