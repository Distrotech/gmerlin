#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <audio.h>
#include <interleave.h>
#include <sampleformat.h>
#include <mix.h>

struct gavl_audio_converter_s
  {
  gavl_audio_format_t input_format;
  gavl_audio_format_t output_format;
  gavl_audio_options_t opt;
  
  int num_conversions;
  
  gavl_audio_convert_context_t * contexts;
  gavl_audio_buffer_t * buffer;
  gavl_audio_frame_t * buffer_frame;
  };

void gavl_audio_format_copy(gavl_audio_format_t * dst,
                            const gavl_audio_format_t * src)
  {
  memcpy(dst, src, sizeof(*dst));
  }


static gavl_audio_format_t * copy_format(const gavl_audio_format_t * format)
  {
  gavl_audio_format_t * ret;
  ret = calloc(1, sizeof(*ret));
  memcpy(ret, format, sizeof(*ret));
  return ret;
  }

static gavl_audio_convert_context_t *
create_mix_context(gavl_audio_options_t * opt,
                   gavl_audio_format_t * in_format,
                   gavl_audio_format_t * out_format)
  {
  gavl_audio_convert_context_t * ret;
  ret = calloc(1, sizeof(*ret));

  //  fprintf(stderr, "Gavl: initializing mixer\n");
  
  ret->input_format  = in_format;
  ret->output_format = copy_format(in_format);
  ret->options = opt;
    
  ret->output_format->channel_setup = out_format->channel_setup;
  ret->output_format->lfe =           out_format->lfe;
  ret->output_format->num_channels =  out_format->num_channels;
  ret->mix_context = gavl_create_mix_context(opt,
                                             in_format,
                                             out_format);
  ret->func = gavl_mix_audio;
  
  return ret;
  }

static gavl_audio_convert_context_t *
create_sampleformat_context(gavl_sampleformat_table_t ** table,
                            gavl_audio_options_t * opt,
                            gavl_audio_format_t * in_format,
                            gavl_audio_format_t * out_format)
  {
  gavl_audio_convert_context_t * ret;
  ret = calloc(1, sizeof(*ret));

  //  fprintf(stderr, "Gavl: initializing sampleformat converter\n");
  
  if(!(*table))
    *table = gavl_create_sampleformat_table(opt);
  
  ret->input_format  = in_format;
  ret->output_format = copy_format(in_format);
  ret->options = opt;

  ret->output_format->sample_format = out_format->sample_format;

  ret->func =
    gavl_find_sampleformat_converter(*table, ret->input_format,
                                     ret->output_format);
/*   if(!ret->func) */
/*     fprintf(stderr, "No function found\n"); */
  return ret;
  }

static gavl_audio_convert_context_t *
create_interleave_context(gavl_interleave_table_t ** table,
                          gavl_audio_options_t * opt,
                          gavl_audio_format_t * in_format,
                          gavl_audio_format_t * out_format)
  {
  gavl_audio_convert_context_t * ret;
  ret = calloc(1, sizeof(*ret));

  //  fprintf(stderr, "Gavl: initializing interleaving converter\n");

  if(!(*table))
    *table = gavl_create_interleave_table(opt);
  
  ret->input_format  = in_format;
  ret->output_format = copy_format(in_format);
  ret->options = opt;

  ret->output_format->interleave_mode = out_format->interleave_mode;
  
  ret->func =
    gavl_find_interleave_converter(*table, ret->input_format,
                                   ret->output_format);
  
  return ret;
  }

static gavl_audio_convert_context_t *
create_resample_context(gavl_audio_options_t * opt,
                        const gavl_audio_format_t * in_format,
                        const gavl_audio_format_t * out_format)
  {
  gavl_audio_convert_context_t * ret;
  ret = calloc(1, sizeof(*ret));
  fprintf(stderr, "Resampling required but not supported\n");
  return ret;
  }

static void destroy_context(gavl_audio_convert_context_t * ctx)
  {
  if(ctx->mix_context)
    gavl_destroy_mix_context(ctx->mix_context);
  free(ctx);
  }

void gavl_audio_converter_destroy(gavl_audio_converter_t* cnv)
  {
  gavl_audio_convert_context_t * ctx;
  if(cnv->buffer)
    {
    gavl_destroy_audio_buffer(cnv->buffer);
    }
  
  ctx = cnv->contexts;

  while(ctx)
    {
    ctx = cnv->contexts->next;
    if(ctx)
      {
      gavl_audio_frame_destroy(cnv->contexts->output_frame);
      }
    free(cnv->contexts->output_format);
    destroy_context(cnv->contexts);
    cnv->contexts = ctx;
    }
  if(cnv->buffer_frame)
    gavl_audio_frame_destroy(cnv->buffer_frame);
  free(cnv);
  }

int gavl_bytes_per_sample(gavl_sample_format_t format)
  {
  switch(format)
    {
    case     GAVL_SAMPLE_U8:
    case     GAVL_SAMPLE_S8:
      return 1;
      break;
    case     GAVL_SAMPLE_U16LE:
    case     GAVL_SAMPLE_S16LE:
    case     GAVL_SAMPLE_U16BE:
    case     GAVL_SAMPLE_S16BE:
      return 2;
      break;
    case     GAVL_SAMPLE_FLOAT:
      return sizeof(float);
      break;
    case     GAVL_SAMPLE_NONE:
      return 0;
    }
  return 0;
  }

gavl_audio_converter_t * gavl_audio_converter_create()
  {
  gavl_audio_converter_t * ret = calloc(1, sizeof(*ret));
  return ret;
  }

void gavl_audio_default_options(gavl_audio_options_t * opt)
  {
  /* Switch buffering on by default */
  
  opt->conversion_flags = GAVL_AUDIO_DO_BUFFER;
  opt->accel_flags = GAVL_ACCEL_C;
  }

static void adjust_format(gavl_audio_format_t * f)
  {
  if(f->num_channels == 1)
    f->interleave_mode = GAVL_INTERLEAVE_NONE;

  if(f->channel_setup == GAVL_CHANNEL_1)
    f->channel_setup = GAVL_CHANNEL_MONO;
  else if(f->channel_setup == GAVL_CHANNEL_2)
    f->channel_setup = GAVL_CHANNEL_MONO;
  
  if((!f->lfe) && (f->num_channels == 2) &&
     (f->interleave_mode == GAVL_INTERLEAVE_2))
    f->interleave_mode = GAVL_INTERLEAVE_ALL;
  }

#define ADD_CONTEXT() \
if(!cnv->contexts) \
  { \
  cnv->contexts = new_context; \
  current_context = new_context; \
  } \
else \
  { \
  current_context->next=new_context;\
  current_context=current_context->next;\
  } \
format_1 = current_context->output_format;\
cnv->num_conversions++;

#define PUSH_MIX_CONTEXT(n, s, l) \
memcpy(&tmp_format, format_1, sizeof(gavl_audio_format_t)); \
tmp_format.num_channels = n; \
tmp_format.channel_setup = s; \
tmp_format.lfe = l; \
new_context = create_mix_context(&(cnv->opt), format_1, &tmp_format); \
ADD_CONTEXT()

#define PUSH_INTERLEAVE_CONTEXT(m) \
memcpy(&tmp_format, format_1, sizeof(gavl_audio_format_t));\
tmp_format.interleave_mode = m;\
new_context = create_interleave_context(&interleave_table,\
                                        &(cnv->opt), format_1, &tmp_format);\
ADD_CONTEXT()

#define PUSH_SAMPLEFORMAT_CONTEXT(f) \
memcpy(&tmp_format, format_1, sizeof(gavl_audio_format_t));\
tmp_format.sample_format = f;\
new_context = create_sampleformat_context(&sampleformat_table,\
                                          &(cnv->opt), format_1, &tmp_format);\
ADD_CONTEXT()

#define PUSH_RESAMPLE_CONTEXT(s) \
memcpy(&tmp_format, format_1, sizeof(gavl_audio_format_t));\
tmp_format.samplerate = s;\
new_context = create_resample_context(&(cnv->opt), \
format_1, &tmp_format);\
ADD_CONTEXT()

int gavl_audio_init(gavl_audio_converter_t* cnv,
                    const gavl_audio_options_t * opt,
                    const gavl_audio_format_t * input_format,
                    const gavl_audio_format_t * output_format)
  {
  gavl_sampleformat_table_t * sampleformat_table;
  gavl_interleave_table_t * interleave_table;
  gavl_audio_convert_context_t * new_context;
  gavl_audio_convert_context_t * current_context;
  gavl_audio_format_t * format_1;
  gavl_audio_format_t * format_2;
  int i;
  int do_mix;
  int do_resample;
  int do_reformat;
  int in_bytes;
  int out_bytes;
  gavl_audio_format_t tmp_format;
  
  interleave_table = (gavl_interleave_table_t *)0;
  sampleformat_table = (gavl_sampleformat_table_t *)0;

  cnv->num_conversions = 0;
  
  /* Copy formats */

  memcpy(&(cnv->opt), opt, sizeof(gavl_audio_options_t));
  memcpy(&(cnv->input_format), input_format, sizeof(gavl_audio_format_t));
  memcpy(&(cnv->output_format), output_format, sizeof(gavl_audio_format_t));

  if(cnv->opt.conversion_flags & GAVL_AUDIO_DO_BUFFER)
    {
    cnv->input_format.samples_per_frame =
      cnv->output_format.samples_per_frame;
    }
  
  /* Adjust formats */
  
  adjust_format(&(cnv->input_format));
  adjust_format(&(cnv->output_format));

  in_bytes = gavl_bytes_per_sample(cnv->input_format.sample_format);
  out_bytes = gavl_bytes_per_sample(cnv->output_format.sample_format);
  
  /* Free previously allocated memory */

  /* Set up conversions */

  format_1 = &(cnv->input_format);
  format_2 = &(cnv->output_format);

  cnv->contexts = (gavl_audio_convert_context_t *)0;
  current_context = (gavl_audio_convert_context_t *)0;
  
  do_mix = 0;
  do_resample = 0;
  do_reformat = 0;

  if((format_1->channel_setup != format_2->channel_setup) ||
     (format_1->lfe != format_2->lfe))
    do_mix = 1;

  if(format_1->sample_format != output_format->sample_format)
    do_reformat = 1;
    
  /* Check for deinterleaving */

  if(format_1->samplerate != output_format->samplerate)
    do_resample = 1;
  
  if((do_mix || do_reformat || do_resample) && 
     (format_1->interleave_mode != GAVL_INTERLEAVE_NONE))
    {
    PUSH_INTERLEAVE_CONTEXT(GAVL_INTERLEAVE_NONE);
    }

  /* Change sampleformat for Resampling/Mixing */
  
  if(do_resample || do_mix)
    {
    /*
     *  In HQ Mode, mixing and resampling are done in floating point
     *  sample size
     */
    if((opt->accel_flags & GAVL_ACCEL_C_HQ) &&
       (format_1->sample_format != GAVL_SAMPLE_FLOAT))
      {
      PUSH_SAMPLEFORMAT_CONTEXT(GAVL_SAMPLE_FLOAT);
      }
    /*
     *  To increase quality, do mixing and resampling with the larger
     *  sample size
     */
    else if(in_bytes < out_bytes)
      {
      PUSH_SAMPLEFORMAT_CONTEXT(cnv->output_format.sample_format);
      }
    }

  /* Mixing and resampling */

  if(do_resample && do_mix)
    {
    if(format_1->num_channels > format_2->num_channels)
      {
      PUSH_MIX_CONTEXT(cnv->output_format.num_channels,
                       cnv->output_format.channel_setup,
                       cnv->output_format.lfe);
      PUSH_RESAMPLE_CONTEXT(cnv->output_format.samplerate);
      }
    else
      {
      PUSH_RESAMPLE_CONTEXT(cnv->output_format.samplerate);
      PUSH_MIX_CONTEXT(cnv->output_format.num_channels,
                       cnv->output_format.channel_setup,
                       cnv->output_format.lfe);
      }
    }
  else if(do_resample)
    {
    PUSH_RESAMPLE_CONTEXT(cnv->output_format.samplerate);
    }
  else if(do_mix)
    {
    PUSH_MIX_CONTEXT(cnv->output_format.num_channels,
                     cnv->output_format.channel_setup,
                     cnv->output_format.lfe);
    }

  /* Do the remaining stuff */

  if(format_1->sample_format != cnv->output_format.sample_format)
    {
    PUSH_SAMPLEFORMAT_CONTEXT(cnv->output_format.sample_format);
    }

  if(format_1->interleave_mode != cnv->output_format.interleave_mode)
    {
    PUSH_INTERLEAVE_CONTEXT(cnv->output_format.interleave_mode);
    }

  if(sampleformat_table)
    gavl_destroy_sampleformat_table(sampleformat_table);
  if(interleave_table)
    gavl_destroy_interleave_table(interleave_table);

  /* Create temporary audio frames */

  current_context = cnv->contexts;
    
  for(i = 0; i < cnv->num_conversions-1; i++)
    {
    current_context->output_frame =
      gavl_audio_frame_create(current_context->output_format);
    current_context->next->input_frame = current_context->output_frame;
    current_context = current_context->next;
    }

  if(cnv->opt.conversion_flags & GAVL_AUDIO_DO_BUFFER)
    {
    memcpy(&tmp_format, &(cnv->input_format),
           sizeof(gavl_audio_format_t));
    
    cnv->buffer = gavl_audio_buffer_create(&(cnv->input_format));
    if(cnv->num_conversions)
      cnv->buffer_frame =
        gavl_audio_frame_create(&(cnv->input_format));
    cnv->num_conversions++;
    }
  
  return cnv->num_conversions;
  }

/* Convert audio and do buffering */



int gavl_audio_convert(gavl_audio_converter_t * cnv,
                       gavl_audio_frame_t * input_frame,
                       gavl_audio_frame_t * output_frame)
  {
  int ret;

  gavl_audio_frame_t * start_frame;
  gavl_audio_convert_context_t * ctx;

  ctx = cnv->contexts;
    
  if(cnv->opt.conversion_flags & GAVL_AUDIO_DO_BUFFER)
    {
    if(cnv->contexts)
      {
      ret = gavl_buffer_audio(cnv->buffer, input_frame,
                              cnv->buffer_frame);
      if(cnv->buffer_frame->valid_samples <
         cnv->output_format.samples_per_frame)
        return ret;
      }
    else
      {
      ret = gavl_buffer_audio(cnv->buffer, input_frame,
                              output_frame);
      if(output_frame->valid_samples <
         cnv->output_format.samples_per_frame)
        return ret;
      }
    start_frame = cnv->buffer_frame;
    }
  else
    {
    start_frame = input_frame;
    ret = 1;
    }
  
  /* Do the conversion */

  if(ctx)
    {
    ctx->input_frame = start_frame;
    
    while(ctx)
      {
      if(!ctx->next)
        ctx->output_frame = output_frame;

      //      fprintf(stderr, "Convert %p %p\n", ctx->input_frame, ctx->output_frame);

      ctx->output_frame->valid_samples = 0;
      ctx->func(ctx);
      if(!ctx->output_frame->valid_samples)
        ctx->output_frame->valid_samples =
          ctx->input_frame->valid_samples;
      ctx = ctx->next;
      }
    }
  if(cnv->buffer_frame)
    cnv->buffer_frame->valid_samples = 0;
  return ret;
  }
 

