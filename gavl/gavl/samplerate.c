/*****************************************************************

  samplerate.c

  Copyright (c) 2001-2002 by Burkhard Plaum - plaum@ipf.uni-stuttgart.de

  http://gmerlin.sourceforge.net

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111, USA.

*****************************************************************/


/* Interface file for the Sectret Rabbit samplerate converter */

// http://www.mega-nerd.com/SRC/ 
// http://freshmeat.net/projects/libsamplerate

#include <stdlib.h>
#include <stdio.h>

#include <audio.h>

#include <samplerate.h>

struct gavl_samplerate_converter_s
  {
  int num_resamplers;
  SRC_STATE ** resamplers;
  SRC_DATA data;
  double ratio;
  };

static int get_filter_type(int gavl_quality)
  {
  switch(gavl_quality)
    {
    case 1:
      return SRC_LINEAR;
      break;
    case 2:
      return SRC_ZERO_ORDER_HOLD;
      break;
    case 3:
      return SRC_SINC_FASTEST;
      break;
    case 4:
      return SRC_SINC_MEDIUM_QUALITY;
      break;
    case 5:
      return SRC_SINC_BEST_QUALITY;
      break;
    }
  return SRC_LINEAR;
  }

#define GET_OUTPUT_SAMPLES(ni, r) (int)((double)(ni)*(r)+10.5)

static void resample_interleave_none(gavl_audio_convert_context_t * ctx)
  {
  int i, result;
  for(i = 0; i < ctx->samplerate_converter->num_resamplers; i++)
    {
    ctx->samplerate_converter->data.input_frames  = ctx->input_frame->valid_samples;
    ctx->samplerate_converter->data.output_frames =
      GET_OUTPUT_SAMPLES(ctx->input_frame->valid_samples,
                         ctx->samplerate_converter->ratio);
#if 0    
    fprintf(stderr, "resample_interleave_none_1 %d %d\n",
            ctx->samplerate_converter->data.input_frames,
            ctx->samplerate_converter->data.output_frames);
#endif
    ctx->samplerate_converter->data.data_in  = ctx->input_frame->channels.f[i];
    ctx->samplerate_converter->data.data_out = ctx->output_frame->channels.f[i];
    result = src_process(ctx->samplerate_converter->resamplers[i],
                         &(ctx->samplerate_converter->data));
    if(result)
      {
      fprintf(stderr, "src_process returned %s (%p)\n", src_strerror(result), ctx->output_frame->samples.f);
      break;
      }
#if 0
    fprintf(stderr, "resample_interleave_none_2 %d %d %d %d\n",
            ctx->input_frame->valid_samples,
            ctx->samplerate_converter->data.output_frames,
            ctx->samplerate_converter->data.input_frames_used,
            ctx->samplerate_converter->data.output_frames_gen);
#endif
    }
  ctx->output_frame->valid_samples =
    ctx->samplerate_converter->data.output_frames_gen;

  }

static void resample_interleave_2(gavl_audio_convert_context_t * ctx)
  {
  int i;
  ctx->samplerate_converter->data.input_frames  =
    ctx->input_frame->valid_samples; 
  ctx->samplerate_converter->data.output_frames =
    GET_OUTPUT_SAMPLES(ctx->input_frame->valid_samples,
                       ctx->samplerate_converter->ratio);
  for(i = 0; i < ctx->samplerate_converter->num_resamplers; i++)
    {
    ctx->samplerate_converter->data.data_in  =
      ctx->input_frame->channels.f[2*i];
    ctx->samplerate_converter->data.data_out =
      ctx->output_frame->channels.f[2*i];
    src_process(ctx->samplerate_converter->resamplers[i],
                &(ctx->samplerate_converter->data));
    }
  ctx->output_frame->valid_samples =
    ctx->samplerate_converter->data.output_frames_gen;
#if 0
  fprintf(stderr, "resample_interleave_2 %d %d %d\n",
          ctx->input_frame->valid_samples,
          ctx->samplerate_converter->data.input_frames_used,
          ctx->samplerate_converter->data.output_frames_gen);
#endif
  }
  
static void resample_interleave_all(gavl_audio_convert_context_t * ctx)
  {
  ctx->samplerate_converter->data.input_frames  =
    ctx->input_frame->valid_samples; 
  ctx->samplerate_converter->data.output_frames =
    GET_OUTPUT_SAMPLES(ctx->input_frame->valid_samples,
                       ctx->samplerate_converter->ratio);

  ctx->samplerate_converter->data.data_in  = ctx->input_frame->samples.f;
  ctx->samplerate_converter->data.data_out = ctx->output_frame->samples.f;
  src_process(ctx->samplerate_converter->resamplers[0],
              &(ctx->samplerate_converter->data));
  ctx->output_frame->valid_samples =
    ctx->samplerate_converter->data.output_frames_gen;
#if 0
  fprintf(stderr, "resample_interleave_all %d %d %d %d\n",
          ctx->input_frame->valid_samples,
          ctx->samplerate_converter->data.output_frames,
          ctx->samplerate_converter->data.input_frames_used,
          ctx->samplerate_converter->data.output_frames_gen);
#endif
  }

static void init_interleave_none(gavl_audio_convert_context_t * ctx,
                                 gavl_audio_options_t * opt,
                                 gavl_audio_format_t  * input_format,
                                 gavl_audio_format_t  * output_format)
  {
  int i, error = 0, filter_type;

  filter_type = get_filter_type(opt->quality);
    
  /* No interleaving: num_channels interleave converters */

  ctx->samplerate_converter->num_resamplers = input_format->num_channels;

  ctx->samplerate_converter->resamplers =
    calloc(ctx->samplerate_converter->num_resamplers,
           sizeof(*(ctx->samplerate_converter->resamplers)));
  
  for(i = 0; i < ctx->samplerate_converter->num_resamplers; i++)
    {
    ctx->samplerate_converter->resamplers[i] =
      src_new(filter_type, 1, &error);
    }

  ctx->func = resample_interleave_none;
  
  }

static void init_interleave_2(gavl_audio_convert_context_t * ctx,
                              gavl_audio_options_t * opt,
                              gavl_audio_format_t  * input_format,
                              gavl_audio_format_t  * output_format)
  {
  int i, error = 0, filter_type;
  int num_channels;
  
  filter_type = get_filter_type(opt->quality);
  ctx->samplerate_converter->num_resamplers = (input_format->num_channels+1)/2;

  ctx->samplerate_converter->resamplers =
    calloc(ctx->samplerate_converter->num_resamplers,
           sizeof(*(ctx->samplerate_converter->resamplers)));

  for(i = 0; i < ctx->samplerate_converter->num_resamplers; i++)
    {
    num_channels = ((input_format->num_channels % 2) &&
                    (i == ctx->samplerate_converter->num_resamplers - 1)) ? 1 : 2;
    
    ctx->samplerate_converter->resamplers[i] =
      src_new(filter_type, num_channels, &error);
    }
  
  ctx->func = resample_interleave_2;
  }

static void init_interleave_all(gavl_audio_convert_context_t * ctx,
                                gavl_audio_options_t * opt,
                                gavl_audio_format_t  * input_format,
                                gavl_audio_format_t  * output_format)
  {
  int error = 0;
  ctx->samplerate_converter->num_resamplers = 1;

  ctx->samplerate_converter->resamplers =
    calloc(ctx->samplerate_converter->num_resamplers,
           sizeof(*(ctx->samplerate_converter->resamplers)));
  
  ctx->samplerate_converter->resamplers[0] =
    src_new(get_filter_type(opt->quality),
            input_format->num_channels, &error);

  ctx->func = resample_interleave_all;
  }



gavl_audio_convert_context_t *
gavl_samplerate_context_create(gavl_audio_options_t * opt,
                               gavl_audio_format_t  * input_format,
                               gavl_audio_format_t  * output_format)
  {
  gavl_audio_convert_context_t * ret;

  ret = gavl_audio_convert_context_create(opt, input_format, output_format);

  ret->samplerate_converter = calloc(1, sizeof(*(ret->samplerate_converter)));
  
  
  if(input_format->num_channels > 1)
    {
    switch(input_format->interleave_mode)
      {
      case GAVL_INTERLEAVE_NONE:
        init_interleave_none(ret, opt, input_format, output_format);
        break;
      case GAVL_INTERLEAVE_2:
        init_interleave_2(ret, opt, input_format, output_format);
        break;
      case GAVL_INTERLEAVE_ALL:
        init_interleave_all(ret, opt, input_format, output_format);
        break;
      }
    }
  else
    init_interleave_none(ret, opt, input_format, output_format);

  ret->samplerate_converter->ratio =
    (double)(output_format->samplerate)/(double)(input_format->samplerate);

  fprintf(stderr, "Doing samplerate conversion, %d -> %d (Ratio: %f)\n",
          input_format->samplerate, output_format->samplerate,
          ret->samplerate_converter->ratio);
#if 0
  
  for(i = 0; i < ret->samplerate_converter->num_resamplers; i++)
    src_set_ratio(ret->samplerate_converter->resamplers[i],
                  ret->samplerate_converter->ratio);
#endif
  
  ret->samplerate_converter->data.src_ratio = ret->samplerate_converter->ratio;
  return ret;
  }

void gavl_samplerate_converter_destroy(gavl_samplerate_converter_t * s)
  {
  int i;
  for(i = 0; i < s->num_resamplers; i++)
    {
    src_delete(s->resamplers[i]);
    }
  free(s->resamplers);
  free(s);
  }
