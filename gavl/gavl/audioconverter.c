/*****************************************************************

  audioconverter.c
 
  Copyright (c) 2003-2006 by Burkhard Plaum - plaum@ipf.uni-stuttgart.de
 
  http://gmerlin.sourceforge.net
 
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
 
  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111, USA.
 
*****************************************************************/

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <audio.h>
#include <mix.h>
#include <accel.h>

struct gavl_audio_converter_s
  {
  gavl_audio_format_t input_format;
  gavl_audio_format_t output_format;
  gavl_audio_options_t opt;
    
  int num_conversions;
  
  gavl_audio_convert_context_t * contexts;
  gavl_audio_convert_context_t * last_context;
  
  gavl_audio_format_t * current_format;
  };

static void destroy_context(gavl_audio_convert_context_t * ctx)
  {
  if(ctx->mix_matrix)
    gavl_destroy_mix_matrix(ctx->mix_matrix);

  if(ctx->samplerate_converter)
    gavl_samplerate_converter_destroy(ctx->samplerate_converter);

  if(ctx->dither_context)
    gavl_audio_dither_context_destroy(ctx->dither_context);

  free(ctx);
  }

static void audio_converter_cleanup(gavl_audio_converter_t* cnv)
  {
  gavl_audio_convert_context_t * ctx;
  
  ctx = cnv->contexts;

  while(ctx)
    {
    ctx = cnv->contexts->next;
    if(ctx)
      gavl_audio_frame_destroy(cnv->contexts->output_frame);
    destroy_context(cnv->contexts);
    cnv->contexts = ctx;
    }
  cnv->num_conversions = 0;
  cnv->contexts     = (gavl_audio_convert_context_t*)0;
  cnv->last_context = (gavl_audio_convert_context_t*)0;
  }

void gavl_audio_converter_destroy(gavl_audio_converter_t* cnv)
  {
  audio_converter_cleanup(cnv);
  free(cnv);
  }

gavl_audio_convert_context_t *
gavl_audio_convert_context_create(gavl_audio_format_t  * input_format,
                                  gavl_audio_format_t  * output_format)
  {
  gavl_audio_convert_context_t * ret;
  ret = calloc(1, sizeof(*ret));
  gavl_audio_format_copy(&(ret->input_format),  input_format);
  gavl_audio_format_copy(&(ret->output_format), input_format);

  return ret;
  }

static void adjust_format(gavl_audio_format_t * f)
  {
  if(f->num_channels == 1)
    f->interleave_mode = GAVL_INTERLEAVE_NONE;

  
  if((f->num_channels == 2) &&
     (f->interleave_mode == GAVL_INTERLEAVE_2))
    f->interleave_mode = GAVL_INTERLEAVE_ALL;
  }

static void add_context(gavl_audio_converter_t* cnv,
                        gavl_audio_convert_context_t * ctx)
  {
  if(cnv->last_context)
    {
    cnv->last_context->next = ctx;
    cnv->last_context = cnv->last_context->next;
    }
  else
    {
    cnv->last_context = ctx;
    cnv->contexts     = ctx;
    }
  cnv->current_format = &(ctx->output_format);
  cnv->num_conversions++;
  }
#if 0
static void dump_context(gavl_audio_convert_context_t * ctx)
  {
  fprintf(stderr, "==== Conversion context ====\n");
  fprintf(stderr, "Input format:\n");
  gavl_audio_format_dump(&(ctx->input_format));
  fprintf(stderr, "Output format:\n");
  gavl_audio_format_dump(&(ctx->output_format));
  fprintf(stderr, "Func: %p\n", ctx->func);
  
  }
#endif

int gavl_audio_converter_init(gavl_audio_converter_t* cnv,
                              const gavl_audio_format_t * input_format,
                              const gavl_audio_format_t * output_format)
  {
  int do_mix, do_resample;
  int i;
  gavl_audio_convert_context_t * ctx;

  gavl_audio_format_t tmp_format;
  
#if 1
  fprintf(stderr, "Initializing audio converter, quality: %d, Flags: 0x%08x\n",
          cnv->opt.quality, cnv->opt.accel_flags);
#endif
  
  /* Delete previous conversions */
  audio_converter_cleanup(cnv);

  /* Copy formats and options */

  gavl_audio_format_copy(&(cnv->input_format), input_format);
  gavl_audio_format_copy(&(cnv->output_format), output_format);

  adjust_format(&(cnv->input_format));
  adjust_format(&(cnv->output_format));

  if(cnv->input_format.samples_per_frame < cnv->output_format.samples_per_frame)
    cnv->input_format.samples_per_frame = cnv->output_format.samples_per_frame;
  else
    cnv->output_format.samples_per_frame = cnv->input_format.samples_per_frame;
  
  gavl_set_conversion_parameters(&(cnv->opt.accel_flags),
                                 &(cnv->opt.quality));
  
  memset(&tmp_format, 0, sizeof(tmp_format));
  cnv->current_format = &(cnv->input_format);
    
  /* Check if we must mix */

  do_mix = 0;
  
  if((input_format->num_channels != output_format->num_channels) ||
     (gavl_front_channels(input_format) != gavl_front_channels(output_format)) ||
     (gavl_rear_channels(input_format) != gavl_rear_channels(output_format)) ||
     (gavl_side_channels(input_format) != gavl_side_channels(output_format)))
    {
    do_mix = 1;
    }
  if(!do_mix)
    {
    i = (input_format->num_channels < output_format->num_channels) ?
      input_format->num_channels : output_format->num_channels;
    
    while(i--)
      {
      if(input_format->channel_locations[i] != output_format->channel_locations[i])
        {
        do_mix = 1;
        break;
        }
      }
    }

  /* Check is we must resample */

  do_resample = (input_format->samplerate != output_format->samplerate) ? 1 : 0;

  /* Check for resampling. We take care, that we do resampling for the least possible channels */

  if(do_resample && (!do_mix || (do_mix && (input_format->num_channels <= output_format->num_channels))))
    {
    /* Sampleformat conversion with GAVL_INTERLEAVE_2 is not supported */
    if(cnv->current_format->interleave_mode == GAVL_INTERLEAVE_2)
      {
      tmp_format.interleave_mode = GAVL_INTERLEAVE_NONE;
      ctx = gavl_interleave_context_create(&(cnv->opt),
                                           cnv->current_format,
                                           &tmp_format);
      add_context(cnv, ctx);
      }
   
    if(cnv->current_format->sample_format != GAVL_SAMPLE_FLOAT)
      {
      tmp_format.sample_format = GAVL_SAMPLE_FLOAT;
      ctx = gavl_sampleformat_context_create(&(cnv->opt),
                                             cnv->current_format,
                                             &tmp_format);
      add_context(cnv, ctx);
      }

    tmp_format.samplerate = output_format->samplerate;
    ctx = gavl_samplerate_context_create(&(cnv->opt),
                                         cnv->current_format,
                                         &tmp_format);
    add_context(cnv, ctx);
    }

  /* Check for mixing */
    
  if(do_mix)
    {
    if(cnv->current_format->interleave_mode != GAVL_INTERLEAVE_NONE)
      {
      tmp_format.interleave_mode = GAVL_INTERLEAVE_NONE;
      ctx = gavl_interleave_context_create(&(cnv->opt),
                                           cnv->current_format,
                                           &tmp_format);
      add_context(cnv, ctx);
      }
    
    if((cnv->current_format->sample_format != GAVL_SAMPLE_FLOAT) &&
       (cnv->opt.accel_flags & GAVL_ACCEL_C_HQ))
      {
      tmp_format.sample_format = GAVL_SAMPLE_FLOAT;
      ctx = gavl_sampleformat_context_create(&(cnv->opt),
                                             cnv->current_format,
                                             &tmp_format);
      add_context(cnv, ctx);
      }
    else if(gavl_bytes_per_sample(cnv->current_format->sample_format) <
            gavl_bytes_per_sample(cnv->output_format.sample_format))
      {
      tmp_format.sample_format = cnv->output_format.sample_format;
      ctx = gavl_sampleformat_context_create(&(cnv->opt),
                                             cnv->current_format,
                                             &tmp_format);
      add_context(cnv, ctx);
      }

    tmp_format.num_channels = cnv->output_format.num_channels;
    memcpy(tmp_format.channel_locations, cnv->output_format.channel_locations,
           GAVL_MAX_CHANNELS * sizeof(tmp_format.channel_locations[0]));

    ctx = gavl_mix_context_create(&(cnv->opt), cnv->current_format,
                                  &tmp_format);
    add_context(cnv, ctx);
    }

  if(do_resample && do_mix && (input_format->num_channels > output_format->num_channels))
    {
    /* Sampleformat conversion with GAVL_INTERLEAVE_2 is not supported */
    if(cnv->current_format->interleave_mode == GAVL_INTERLEAVE_2)
      {
      tmp_format.interleave_mode = GAVL_INTERLEAVE_NONE;
      ctx = gavl_interleave_context_create(&(cnv->opt),
                                           cnv->current_format,
                                           &tmp_format);
      add_context(cnv, ctx);
      }
    
    if(cnv->current_format->sample_format != GAVL_SAMPLE_FLOAT)
      {
      tmp_format.sample_format = GAVL_SAMPLE_FLOAT;
      ctx = gavl_sampleformat_context_create(&(cnv->opt),
                                             cnv->current_format,
                                             &tmp_format);
      add_context(cnv, ctx);
      }
    tmp_format.samplerate = output_format->samplerate;
    ctx = gavl_samplerate_context_create(&(cnv->opt),
                                         cnv->current_format,
                                         &tmp_format);
    add_context(cnv, ctx);
    }
    
  /* Check, if we must change the sample format */

  if(cnv->current_format->sample_format != cnv->output_format.sample_format)
    {
    if(cnv->current_format->interleave_mode == GAVL_INTERLEAVE_2)
      {
      tmp_format.interleave_mode = GAVL_INTERLEAVE_NONE;
      ctx = gavl_interleave_context_create(&(cnv->opt),
                                           cnv->current_format,
                                           &tmp_format);
      add_context(cnv, ctx);
      }

    tmp_format.sample_format = cnv->output_format.sample_format;
    ctx = gavl_sampleformat_context_create(&(cnv->opt),
                                           cnv->current_format,
                                           &tmp_format);
    add_context(cnv, ctx);
    
    }
     
  /* Final interleaving */

  if(cnv->current_format->interleave_mode != cnv->output_format.interleave_mode)
    {
    tmp_format.interleave_mode = cnv->output_format.interleave_mode;
    ctx = gavl_interleave_context_create(&(cnv->opt),
                                         cnv->current_format,
                                         &tmp_format);
    add_context(cnv, ctx);
    }

  //  fprintf(stderr, "Audio converter initialized, %d conversions\n", cnv->num_conversions);
  
  //  gavl_audio_format_dump(&(cnv->input_format));

  //  gavl_audio_format_dump(&(cnv->output_format));
  //  gavl_audio_format_dump(cnv->current_format);

  /* Create temporary buffers */
  ctx = cnv->contexts;
  
  while(ctx)
    {
    //    dump_context(ctx);
    if(ctx->next)
      {
      ctx->output_frame = gavl_audio_frame_create(&(ctx->output_format));
      ctx->next->input_frame = ctx->output_frame;
      }
    ctx = ctx->next;
    }
  
  return cnv->num_conversions;
  }

/* Convert audio */

void gavl_audio_convert(gavl_audio_converter_t * cnv,
                        gavl_audio_frame_t * input_frame,
                        gavl_audio_frame_t * output_frame)
  {
  int i;
  gavl_audio_convert_context_t * ctx;
    
  cnv->contexts->input_frame = input_frame;
  cnv->last_context->output_frame = output_frame;

  ctx = cnv->contexts;
  
  for(i = 0; i < cnv->num_conversions; i++)
    {
    ctx->output_frame->valid_samples = 0;
    if(ctx->func)
      {
      ctx->func(ctx);
      if(!ctx->output_frame->valid_samples)
        ctx->output_frame->valid_samples = ctx->input_frame->valid_samples;
      }
    ctx = ctx->next;
    }
  }

gavl_audio_converter_t * gavl_audio_converter_create()
  {
  gavl_audio_converter_t * ret = calloc(1, sizeof(*ret));
  gavl_audio_options_set_defaults(&(ret->opt));
  return ret;
  }

gavl_audio_options_t *
gavl_audio_converter_get_options(gavl_audio_converter_t * cnv)
  {
  return &(cnv->opt);
  }
