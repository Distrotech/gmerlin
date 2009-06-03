#include <stdlib.h>

#include <audio.h>
#include <sampleformat.h>

gavl_sampleformat_table_t * gavl_create_sampleformat_table(gavl_audio_options_t * opt)
  {
  gavl_sampleformat_table_t * ret = calloc(1, sizeof(*ret));

  if(opt->accel_flags & GAVL_ACCEL_C)
    gavl_init_sampleformat_funcs_c(ret);
  return ret;
  }

gavl_audio_func_t gavl_find_sampleformat_converter(gavl_sampleformat_table_t * t,
                                                   gavl_audio_format_t * in,
                                                   gavl_audio_format_t * out)
  {
  switch(in->sample_format)
    {
    case GAVL_SAMPLE_U8:
      switch(out->sample_format)
        {
        case GAVL_SAMPLE_U8:
          // Nothing
          break;
        case GAVL_SAMPLE_S8:
          return t->swap_sign_8;
          break;
        case GAVL_SAMPLE_U16NE:
          return t->u_8_to_u_16;
          break;
        case GAVL_SAMPLE_S16NE:
          return t->u_8_to_s_16;
          break;
        case GAVL_SAMPLE_U16OE:
          return t->u_8_to_u_16;
          break;
        case GAVL_SAMPLE_S16OE:
          return t->u_8_to_s_16_oe;
          break;
        case GAVL_SAMPLE_FLOAT:
          return t->convert_u8_to_float;
          break;
        }
      break;
    case GAVL_SAMPLE_S8:
      switch(out->sample_format)
        {
        case GAVL_SAMPLE_U8:
          return t->swap_sign_8;
          break;
        case GAVL_SAMPLE_S8:
          // Nothing
          break;
        case GAVL_SAMPLE_U16NE:
          return t->s_8_to_u_16;
          break;
        case GAVL_SAMPLE_S16NE:
          return t->s_8_to_s_16;
          break;
        case GAVL_SAMPLE_U16OE:
          return t->s_8_to_u_16;
          break;
        case GAVL_SAMPLE_S16OE:
          return t->s_8_to_s_16_oe;
          break;
        case GAVL_SAMPLE_FLOAT:
          return t->convert_s8_to_float;
          break;
        }
      break;
    case GAVL_SAMPLE_U16NE:
      switch(out->sample_format)
        {
        case GAVL_SAMPLE_U8:
          return t->convert_16_to_8;
          break;
        case GAVL_SAMPLE_S8:
          return t->convert_16_to_8_sign;
          break;
        case GAVL_SAMPLE_U16NE:
          // Nothing
          break;
        case GAVL_SAMPLE_S16NE:
          return t->swap_sign_16;
          break;
        case GAVL_SAMPLE_U16OE:
          return t->swap_endian_16;
          break;
        case GAVL_SAMPLE_S16OE:
          return t->swap_sign_endian_16;
          break;
        case GAVL_SAMPLE_FLOAT:
          return t->convert_u16ne_to_float;
          break;
        }
      break;
    case GAVL_SAMPLE_S16NE:
      switch(out->sample_format)
        {
        case GAVL_SAMPLE_U8:
          return t->convert_16_to_8_sign;
          break;
        case GAVL_SAMPLE_S8:
          return t->convert_16_to_8;
          break;
        case GAVL_SAMPLE_U16NE:
          return t->swap_sign_16;
          break;
        case GAVL_SAMPLE_S16NE:
          // Nothing
          break;
        case GAVL_SAMPLE_U16OE:
          return t->swap_sign_endian_16;
          break;
        case GAVL_SAMPLE_S16OE:
          return t->swap_endian_16;
          break;
        case GAVL_SAMPLE_FLOAT:
          return t->convert_s16ne_to_float;
          break;
        }
      break;
    case GAVL_SAMPLE_U16OE:
      switch(out->sample_format)
        {
        case GAVL_SAMPLE_U8:
          return t->convert_16_to_8_oe;
          break;
        case GAVL_SAMPLE_S8:
          return t->convert_16_to_8_sign_oe;
          break;
        case GAVL_SAMPLE_U16NE:
          return t->swap_endian_16;
          break;
        case GAVL_SAMPLE_S16NE:
          return t->swap_endian_sign_16;
          break;
        case GAVL_SAMPLE_U16OE:
          // Nothing
          break;
        case GAVL_SAMPLE_S16OE:
          return t->swap_sign_16_oe;
          break;
        case GAVL_SAMPLE_FLOAT:
          return t->convert_u16oe_to_float;
          break;
        }
      break;
    case GAVL_SAMPLE_S16OE:
      switch(out->sample_format)
        {
        case GAVL_SAMPLE_U8:
          return t->convert_16_to_8_sign_oe;
          break;
        case GAVL_SAMPLE_S8:
          return t->convert_16_to_8_oe;
          break;
        case GAVL_SAMPLE_U16NE:
          return t->swap_endian_sign_16;
          break;
        case GAVL_SAMPLE_S16NE:
          return t->swap_endian_16;
          break;
        case GAVL_SAMPLE_U16OE:
          return t->swap_sign_16_oe;
          break;
        case GAVL_SAMPLE_S16OE:
          // Nothing
          break;
        case GAVL_SAMPLE_FLOAT:
          return t->convert_s16oe_to_float;
          break;
        }
      break;
    case GAVL_SAMPLE_FLOAT:
      switch(out->sample_format)
        {
        case GAVL_SAMPLE_U8:
          return t->convert_float_to_u8;
          break;
        case GAVL_SAMPLE_S8:
          return t->convert_float_to_s8;
          break;
        case GAVL_SAMPLE_U16NE:
          return t->convert_float_to_u16ne;
          break;
        case GAVL_SAMPLE_S16NE:
          return t->convert_float_to_s16ne;
          break;
        case GAVL_SAMPLE_U16OE:
          return t->convert_float_to_u16oe;
          break;
        case GAVL_SAMPLE_S16OE:
          return t->convert_float_to_s16oe;
          break;
        case GAVL_SAMPLE_FLOAT:
          // Nothing
          break;
        }
      break;

    }
  return (gavl_audio_func_t)0;
  }


void gavl_destroy_sampleformat_table(gavl_sampleformat_table_t * t)
  {
  free(t);
  }

