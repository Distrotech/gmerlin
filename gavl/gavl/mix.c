#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include <audio.h>
#include <mix.h>

/*
 *   Mixing coefficients (should be made variable)
 */

/* If we have more output- than input channels */

#define FRONT_TO_REAR     1.0
#define FRONT_TO_CENTER   1.0

/* If we have more input than output channels */

#define CENTER_TO_LR   1.0
#define REAR_TO_FRONT  1.0

/* Macros */

#define MINUS_3dB 0.707
#define MINUS_6dB 0.5

typedef struct
  {
  int front;
  int rear;
  int center;
  int lfe;
  gavl_channel_setup_t channel_setup;
  } channel_info;

static void get_channel_info(gavl_audio_format_t * format,
                             channel_info * ret)
  {
  ret->channel_setup = format->channel_setup;
  switch(ret->channel_setup)
    {
    case GAVL_CHANNEL_MONO: //  |Front  | (lfe) |      |      |      |      |
    case GAVL_CHANNEL_1:    //  |Front  | (lfe) |      |      |      |      |
    case GAVL_CHANNEL_2:    //  |Front  | (lfe) |      |      |      |      |
      ret->front  = 1;
      ret->center = 0;
      ret->rear   = 0;
      break;
    case GAVL_CHANNEL_2F:   //  |Front L|Front R| (lfe)|      |      |      |
      ret->front  = 2;
      ret->center = 0;
      ret->rear   = 0;
      break;

    case GAVL_CHANNEL_3F:   //  |Front L|Front R|Center| (lfe)|      |      |
      ret->front  = 2;
      ret->center = 1;
      ret->rear   = 0;
      break;

    case GAVL_CHANNEL_2F1R: //  |Front L|Front R|Rear  | (lfe)|      |      |
      ret->front  = 2;
      ret->center = 0;
      ret->rear   = 1;
      break;
    case GAVL_CHANNEL_3F1R: //  |Front L|Front R|Rear  |Center| (lfe)|      |
      ret->front  = 2;
      ret->center = 1;
      ret->rear   = 1;
      break;
    case GAVL_CHANNEL_2F2R: //  |Front L|Front R|Rear L|Rear R|Center|      |
      ret->front  = 2;
      ret->center = 0;
      ret->rear   = 2;
      break;
    case GAVL_CHANNEL_3F2R: //  |Front L|Front R|Rear L|Rear R|Center| (lfe)|
      ret->front  = 2;
      ret->center = 1;
      ret->rear   = 2;
      break;
    }
  ret->lfe = format->lfe;
  }

static void handle_front(gavl_mix_context_t * ctx,
                         gavl_audio_options_t * opt,
                         channel_info * input,
                         channel_info * output)
  {
  switch(input->front)
    {
    case 1:
      switch(output->front)
        {
        case 1:
          ctx->f_matrix[0][0] = 1.0;
          break;
        case 2:
          ctx->f_matrix[0][0] = MINUS_3dB;
          ctx->f_matrix[0][1] = MINUS_3dB;
          break;
        }
      break;
    case 2:
      switch(output->front)
        {
        case 1:
          if(output->channel_setup == GAVL_CHANNEL_1)
            {
            ctx->f_matrix[0][0] = 1.0;
            }
          else if(output->channel_setup == GAVL_CHANNEL_2)
            {
            ctx->f_matrix[1][0] = 1.0;
            }
          else
            {
            ctx->f_matrix[0][0] = 1.0;
            ctx->f_matrix[1][0] = 1.0;
            }
          break;
        case 2:
          ctx->f_matrix[0][0] = 1.0;
          ctx->f_matrix[1][1] = 1.0;
        }
    }

  }


static void handle_rear(gavl_mix_context_t * ctx,
                        gavl_audio_options_t * opt,
                        channel_info * input,
                        channel_info * output)
  {
  switch(input->rear)
    {
    case 0: /* No input rear */
      switch(output->rear)
        {
        case 0:
          break;
        case 1: /* Front -> 1 Rear channel */
          if(input->front == 1)
            {
            ctx->f_matrix[0][output->front] = FRONT_TO_REAR;
            }
          else if(input->front == 2)
            {
            ctx->f_matrix[0][output->front] = FRONT_TO_REAR;
            ctx->f_matrix[1][output->front] = FRONT_TO_REAR;
            }
          break;
        case 2: /* Front -> 2 Rear channels */
          if(input->front == 1)
            {
            ctx->f_matrix[0][output->front]   = FRONT_TO_REAR * MINUS_3dB;
            ctx->f_matrix[0][output->front+1] = FRONT_TO_REAR * MINUS_3dB;
            }
          else if(input->front == 2)
            {
            ctx->f_matrix[0][output->front]   = FRONT_TO_REAR;
            ctx->f_matrix[1][output->front+1] = FRONT_TO_REAR;
            }
          break;
        }
      break;
    case 1: /* One input rear */
      switch(output->rear)
        {
        case 0: /* Mix one surround channel to front */
          if((opt->conversion_flags & GAVL_AUDIO_DOWNMIX_DOLBY) &&
             (output->front == 2))
            {
            ctx->f_matrix[input->front][0] = -MINUS_3dB;
            ctx->f_matrix[input->front][1] =  MINUS_3dB;
            }
          else if(output->front == 2)
            {
            ctx->f_matrix[input->front][0] =  ctx->slev * MINUS_3dB;
            ctx->f_matrix[input->front][1] =  ctx->slev * MINUS_3dB;
            }
          else if(output->front == 1)
            {
            ctx->f_matrix[input->front][0] =  ctx->slev * MINUS_3dB * 2;
            }
          break;
        case 1: /* Route through surround channel */
          ctx->f_matrix[input->front][output->front] = 1.0;
          break;
        case 2: /* Make 2 surround channels from one */
          ctx->f_matrix[input->front][output->front]   = MINUS_3dB;
          ctx->f_matrix[input->front][output->front+1] = MINUS_3dB;
          break;
        }
      break;
    case 2: /* Two input rears */
      switch(output->rear)
        {
        case 0: /* 2 Rear channels -> Front */
          if((opt->conversion_flags & GAVL_AUDIO_DOWNMIX_DOLBY) &&
             (output->front == 2))
            {
            ctx->f_matrix[input->front][0]   = -MINUS_3dB;
            ctx->f_matrix[input->front][1]   =  MINUS_3dB;
            ctx->f_matrix[input->front+1][0] = -MINUS_3dB;
            ctx->f_matrix[input->front+1][1] =  MINUS_3dB;
            }
          else if(output->front == 2)
            {
            ctx->f_matrix[input->front][0]   =  ctx->slev;
            ctx->f_matrix[input->front+1][1] =  ctx->slev;
            }
          else if(output->front == 1)
            {
            ctx->f_matrix[input->front][0]   =  ctx->slev;
            ctx->f_matrix[input->front+1][0] =  ctx->slev;
            }
          break;
        case 1: /* 2 Rear channels -> one */
          ctx->f_matrix[input->front][0]   =  1.0;
          ctx->f_matrix[input->front+1][0] =  1.0;
          break;
        case 2: /* Route through rear channels */
          ctx->f_matrix[input->front][0]   =  1.0;
          ctx->f_matrix[input->front+1][1] =  1.0;
          break;
        }
      break;
    }
  }

static void handle_center(gavl_mix_context_t * ctx,
                          gavl_audio_options_t * opt,
                          channel_info * input,
                          channel_info * output)
  {
  if(!input->center && !output->center)
    return;
  
  if(input->center && !output->center)
    {
    if((opt->conversion_flags & GAVL_AUDIO_DOWNMIX_DOLBY) &&
       (output->front == 2))
      {
      ctx->f_matrix[input->front+input->rear][0] = MINUS_3dB;
      ctx->f_matrix[input->front+input->rear][1] = MINUS_3dB;
      }
    else if(output->front == 2)
      {
      ctx->f_matrix[input->front+input->rear][0] = ctx->clev;
      ctx->f_matrix[input->front+input->rear][1] = ctx->clev;
      }
    else if(output->front == 1)
      {
      ctx->f_matrix[input->front+input->rear][0] = 2.0 * ctx->clev;
      }
    }

  if(!input->center && output->center)
    {
    if(input->front == 2)
      {
      ctx->f_matrix[0][output->front+output->rear]=FRONT_TO_CENTER*MINUS_6dB;
      ctx->f_matrix[1][output->front+output->rear]=FRONT_TO_CENTER*MINUS_6dB;
      }
    else if(input->front == 1)
      {
      ctx->f_matrix[0][output->front+output->rear] = FRONT_TO_CENTER;
      }
    }
  if(input->center && output->center)
    {
    /* Route through center channel */
    ctx->f_matrix[input->front + input->rear][output->front+output->rear] = 1.0;
    }
  }

static void handle_lfe(gavl_mix_context_t * ctx,
                       gavl_audio_options_t * opt,
                       channel_info * input,
                       channel_info * output)
  {
  if(!input->lfe && !output->lfe)
    {
    return;
    }
  else if(input->lfe && !output->lfe)
    {
    return;
    }
  if(!input->lfe && output->lfe)
    {
    return;
    }
  if(input->lfe && output->lfe)
    {
    ctx->f_matrix[input->front +input->rear + input->center][input->front +input->rear + input->center] = 1.0;
    }

  }

#define CLAMP(x,a,b) if(x<a)x=a;if(x>b)x=b;


static void create_matrix(gavl_audio_options_t * opt,
                          gavl_audio_format_t * output_format,
                          gavl_mix_context_t * ctx)
  {
  int i, j;
  float tmp;
  int i_tmp;

  gavl_mixer_table_t * tab;
  tab = calloc(1, sizeof(*tab));

  if(opt->accel_flags & GAVL_ACCEL_C)
    gavl_setup_mix_funcs_c(tab, output_format);
    
  for(i = 0; i < output_format->num_channels; i++) // Output Channels 
    {
    for(j = 0; j < ctx->matrix[i].num_inputs; j++) // Input  Channels
      {
      tmp = ctx->f_matrix[ctx->matrix[i].inputs[j]][i];
      fprintf(stderr, "Mix %d -> %d: %f\n", j, i, tmp);
      switch(output_format->sample_format)
        {
        case GAVL_SAMPLE_S8:
          i_tmp = (int)(tmp * 127.0);
          CLAMP(i_tmp, -128, 127);
          ctx->matrix[i].factors[j].s_8 = (int8_t)(i_tmp);
          break;
        case GAVL_SAMPLE_S16NE:
          i_tmp = (int)(tmp * 32767.0);
          CLAMP(i_tmp, -32768, 32767);
          ctx->matrix[i].factors[j].s_16 = (int8_t)(i_tmp);
          break;
        case GAVL_SAMPLE_FLOAT:
          ctx->matrix[i].factors[j].f = tmp;
          break;
        }
      }
    switch(ctx->matrix[i].num_inputs)
      {
      case 1:
        ctx->matrix[i].func = tab->mix_1_to_1;
        break;
      case 2:
        ctx->matrix[i].func = tab->mix_2_to_1;
        break;
      case 3:
        ctx->matrix[i].func = tab->mix_3_to_1;
        break;
      case 4:
        ctx->matrix[i].func = tab->mix_4_to_1;
        break;
      case 5:
        ctx->matrix[i].func = tab->mix_5_to_1;
        break;
      case 6:
        ctx->matrix[i].func = tab->mix_6_to_1;
        break;
      }
    }
  free(tab);
  }

void gavl_init_mix_matrix(gavl_mix_context_t * ctx,
                          gavl_audio_options_t * opt,
                          gavl_audio_format_t * in,
                          gavl_audio_format_t * out)
  {
  float max_amplitude;
  float amplitude;
  int i, j;
  
  ctx->clev = MINUS_3dB;
  ctx->slev = MINUS_3dB;
  
  memset(&(ctx->f_matrix), 0, sizeof(ctx->f_matrix));
  
  channel_info input;
  channel_info output;
  
  /* Get the channel numbers */
  
  get_channel_info(in, &input);
  get_channel_info(out, &output);

  /* Check, what we have to mix */
  
  handle_front(ctx, opt, &input, &output);
  handle_rear(ctx, opt, &input, &output);
  handle_center(ctx, opt, &input, &output);
  handle_lfe(ctx, opt, &input, &output);

  /* Check, which coefficients are used */

  for(i = 0; i < out->num_channels; i++)
    {
    ctx->matrix[i].num_inputs = 0;
    
    for(j = 0; j < in->num_channels; j++)
      {
      if(ctx->f_matrix[j][i] != 0.0)
        {
        ctx->matrix[i].inputs[ctx->matrix[i].num_inputs] = j;
        ctx->matrix[i].num_inputs++;
        }
      }
    }

  /* Normalize the mix matrix */

  max_amplitude = 0.0;

  for(i = 0; i < out->num_channels; i++)
    {
    amplitude = 0.0;
    
    for(j = 0; j < in->num_channels; j++)
      amplitude += fabs(ctx->f_matrix[j][i]);

    if(amplitude > max_amplitude)
      max_amplitude = amplitude;
    
    }
  for(i = 0; i < out->num_channels; i++)
    {
    for(j = 0; j < in->num_channels; j++)
      ctx->f_matrix[j][i] /= max_amplitude;
    }

  /* Create coefficients in sample format */
  
  create_matrix(opt, out, ctx);
  }

void gavl_mix_audio(gavl_audio_convert_context_t * ctx)
  {
  int i;
  
  for(i = 0; i < ctx->output_format->num_channels; i++)
    {
    if(ctx->mix_context->matrix[i].func)
      ctx->mix_context->matrix[i].func(ctx, i);
    }
  }


gavl_mix_context_t *
gavl_create_mix_context(gavl_audio_options_t * opt,
                        gavl_audio_format_t * in,
                        gavl_audio_format_t * out)
  {
  gavl_mix_context_t * ret;
  ret = calloc(1, sizeof(*ret));

  gavl_init_mix_matrix(ret, opt, in, out);
  
  return ret;
  }

void gavl_destroy_mix_context(gavl_mix_context_t * ctx)
  {
  free(ctx);
  }
