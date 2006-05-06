/*****************************************************************
 
  mix.c
 
  Copyright (c) 2003-2004 by Burkhard Plaum - plaum@ipf.uni-stuttgart.de
 
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

#define CLAMP(x,a,b) if(x<a)x=a;if(x>b)x=b;

#if 0

static void input_channel_dump(gavl_mix_input_channel_t * c)
  {
  fprintf(stderr, "Index: %d, Factor [ 0x%02x 0x%04x 0x%08x %f ]\n",
          c->index, c->factor.f_8, c->factor.f_16, c->factor.f_32, c->factor.f_float);
  }

static void output_channel_dump(gavl_mix_output_channel_t * c)
  {
  int i;
  fprintf(stderr, "Output channel %d, num_inputs: %d, func: %p\n",
          c->index, c->num_inputs, c->func);
  for(i = 0; i < c->num_inputs; i++)
    {
    input_channel_dump(&(c->inputs[i]));
    }
  }
#endif

void gavl_mix_audio(gavl_audio_convert_context_t * ctx)
  {
  int i;
  //  fprintf(stderr, "MIX...");
  for(i = 0; i < ctx->output_format.num_channels; i++)
    {
    if(ctx->mix_matrix->output_channels[i].func)
      ctx->mix_matrix->output_channels[i].func(&(ctx->mix_matrix->output_channels[i]),
                                                ctx->input_frame,
                                                ctx->output_frame);
    else
      gavl_audio_frame_mute(ctx->output_frame, &(ctx->output_format));
    //      fprintf(stderr, "Mixer function missing\n");
    
    }
  //  fprintf(stderr, "done\n");
  }

#if 0

static void dump_matrix(gavl_audio_format_t * in,
                        gavl_audio_format_t * out,
                        float matrix[GAVL_MAX_CHANNELS][GAVL_MAX_CHANNELS])
  {
  int i, j;
  fprintf(stderr, "Mix Matrix:\n");
  for(i = 0; i < out->num_channels; i++)
    {
    for(j = 0; j < in->num_channels; j++)
      {
      fprintf(stderr, "%5.3f ", matrix[i][j]);
      }
    fprintf(stderr, "\n");
    }
  //  fprintf(stderr, "End Mix Matrix\n");
  }

#endif

static void normalize_matrix(float ret[GAVL_MAX_CHANNELS][GAVL_MAX_CHANNELS],
                             int in_channels, int out_channels)
  {
  float max_ampl;
  float ampl;
  int i, j;

  /* Normalize matrix */

  max_ampl = 0.0;

  for(i = 0; i < out_channels; i++)
    {
    ampl = 0.0;

    for(j = 0; j < in_channels; j++)
      ampl += fabs(ret[i][j]);
    
    if(ampl > max_ampl)
      max_ampl = ampl;
    }

  //  dump_matrix(in, out, ret);
  
  for(i = 0; i < out_channels; i++)
    for(j = 0; j < in_channels; j++)
      ret[i][j] /= max_ampl;

  
  
  }

#define IN_INDEX(id)  in_index  = gavl_channel_index(in, id);  if(in_index < 0)  goto fail;
#define OUT_INDEX(id) out_index = gavl_channel_index(out, id); if(out_index < 0) goto fail;

static void init_matrix(float ret[GAVL_MAX_CHANNELS][GAVL_MAX_CHANNELS],
                        gavl_audio_options_t * opt,
                        gavl_audio_format_t * in,
                        gavl_audio_format_t * out)
  {
  int input_front;
  int output_front;
  int input_rear;
  int output_rear;
  int input_side;
  int output_side;
  int input_lfe;
  int output_lfe;

  int in_index, out_index;
  

  input_front = gavl_front_channels(in);
  output_front = gavl_front_channels(out);

  input_rear  = gavl_rear_channels(in);
  output_rear = gavl_rear_channels(out);

  input_side  = gavl_side_channels(in);
  output_side = gavl_side_channels(out);

  input_lfe  = gavl_lfe_channels(in);
  output_lfe = gavl_lfe_channels(out);

#if 0
  fprintf(stderr, "INIT MATRIX\n");
  fprintf(stderr, "In:\n");
  gavl_audio_format_dump(in);
  fprintf(stderr, "Out:\n");
  gavl_audio_format_dump(out);
#endif

  /* Handle front channels */
  
  switch(input_front)
    {
    case 1:
      switch(output_front)
        {
        case 1: /* 1 Front -> 1 Front */
          OUT_INDEX(GAVL_CHID_FRONT_CENTER);
          IN_INDEX(GAVL_CHID_FRONT_CENTER);
          ret[out_index][in_index] = 1.0;
          break;
        case 2: /* 1 Front -> 2 Front */
          OUT_INDEX(GAVL_CHID_FRONT_LEFT);
          IN_INDEX(GAVL_CHID_FRONT_CENTER);
          
          ret[out_index][in_index] = 1.0;

          OUT_INDEX(GAVL_CHID_FRONT_RIGHT);
          IN_INDEX(GAVL_CHID_FRONT_CENTER);
          
          ret[out_index][in_index] = 1.0;
          break;
        case 3: /* 1 Front -> 3 Front */
          OUT_INDEX(GAVL_CHID_FRONT_LEFT);
          IN_INDEX(GAVL_CHID_FRONT_CENTER);
          ret[out_index][in_index] = 1.0;

          OUT_INDEX(GAVL_CHID_FRONT_RIGHT);
          IN_INDEX(GAVL_CHID_FRONT_CENTER);
          ret[out_index][in_index] = 1.0;

          OUT_INDEX(GAVL_CHID_FRONT_CENTER);
          IN_INDEX(GAVL_CHID_FRONT_CENTER);
          
          ret[out_index][in_index] = 1.0;
          break;
        case 4: /* 1 Front -> 4 Front */
          OUT_INDEX(GAVL_CHID_FRONT_LEFT);
          IN_INDEX(GAVL_CHID_FRONT_CENTER);
          ret[out_index][in_index] = 1.0;

          OUT_INDEX(GAVL_CHID_FRONT_RIGHT);
          IN_INDEX(GAVL_CHID_FRONT_CENTER);
          ret[out_index][in_index] = 1.0;

          OUT_INDEX(GAVL_CHID_FRONT_CENTER_LEFT);
          IN_INDEX(GAVL_CHID_FRONT_CENTER);
          ret[out_index][in_index] = 1.0;

          OUT_INDEX(GAVL_CHID_FRONT_CENTER_RIGHT);
          IN_INDEX(GAVL_CHID_FRONT_CENTER);
          ret[out_index][in_index] = 1.0;
          break;
        case 5: /* 1 Front -> 5 Front */
          OUT_INDEX(GAVL_CHID_FRONT_LEFT);
          IN_INDEX(GAVL_CHID_FRONT_CENTER);
          ret[out_index][in_index] = 1.0;

          OUT_INDEX(GAVL_CHID_FRONT_RIGHT);
          IN_INDEX(GAVL_CHID_FRONT_CENTER);
          ret[out_index][in_index] = 1.0;

          OUT_INDEX(GAVL_CHID_FRONT_CENTER);
          IN_INDEX(GAVL_CHID_FRONT_CENTER);
          ret[out_index][in_index] = 1.0;
          
          OUT_INDEX(GAVL_CHID_FRONT_CENTER_LEFT);
          IN_INDEX(GAVL_CHID_FRONT_CENTER);
          ret[out_index][in_index] = 1.0;

          OUT_INDEX(GAVL_CHID_FRONT_CENTER_RIGHT);
          IN_INDEX(GAVL_CHID_FRONT_CENTER);
          ret[out_index][in_index] = 1.0;
          break;
        }
      break;
    case 2:
      switch(output_front)
        {
        case 1: /* 2 Front -> 1 Front */
          switch(opt->conversion_flags & GAVL_AUDIO_STEREO_TO_MONO_MASK)
            {
            case GAVL_AUDIO_STEREO_TO_MONO_LEFT:
              OUT_INDEX(GAVL_CHID_FRONT_CENTER);
              IN_INDEX(GAVL_CHID_FRONT_LEFT);
              ret[out_index][in_index]      = 1.0;
              break;
            case GAVL_AUDIO_STEREO_TO_MONO_RIGHT:
              OUT_INDEX(GAVL_CHID_FRONT_CENTER);
              IN_INDEX(GAVL_CHID_FRONT_RIGHT);
              ret[out_index][in_index]     = 1.0;
              break;
            case GAVL_AUDIO_STEREO_TO_MONO_MIX:
              OUT_INDEX(GAVL_CHID_FRONT_CENTER);
              IN_INDEX(GAVL_CHID_FRONT_LEFT);
              ret[out_index][in_index]      = 1.0;

              OUT_INDEX(GAVL_CHID_FRONT_CENTER);
              IN_INDEX(GAVL_CHID_FRONT_RIGHT);
              ret[out_index][in_index]     = 1.0;
              break;
            }
          break;
        case 2: /* 2 Front -> 2 Front */
          OUT_INDEX(GAVL_CHID_FRONT_LEFT);
          IN_INDEX(GAVL_CHID_FRONT_LEFT);
          ret[out_index][in_index]    = 1.0;

          OUT_INDEX(GAVL_CHID_FRONT_RIGHT);
          IN_INDEX(GAVL_CHID_FRONT_RIGHT);
          ret[out_index][in_index]  = 1.0;
          break;
        case 3: /* 2 Front -> 3 Front */
          OUT_INDEX(GAVL_CHID_FRONT_LEFT);
          IN_INDEX(GAVL_CHID_FRONT_LEFT);
          ret[out_index][in_index]  = 1.0;

          OUT_INDEX(GAVL_CHID_FRONT_RIGHT);
          IN_INDEX(GAVL_CHID_FRONT_RIGHT);
          ret[out_index][in_index] = 1.0;

          OUT_INDEX(GAVL_CHID_FRONT_CENTER);
          IN_INDEX(GAVL_CHID_FRONT_LEFT);
          ret[out_index][in_index] = 0.5;

          OUT_INDEX(GAVL_CHID_FRONT_CENTER);
          IN_INDEX(GAVL_CHID_FRONT_RIGHT);
          ret[out_index][in_index] = 0.5;
          break;
        case 4: /* 2 Front -> 4 Front */
          OUT_INDEX(GAVL_CHID_FRONT_LEFT);
          IN_INDEX(GAVL_CHID_FRONT_LEFT);
          ret[out_index][in_index]  = 1.0;

          OUT_INDEX(GAVL_CHID_FRONT_RIGHT);
          IN_INDEX(GAVL_CHID_FRONT_RIGHT);
          ret[out_index][in_index] = 1.0;

          OUT_INDEX(GAVL_CHID_FRONT_CENTER_LEFT);
          IN_INDEX(GAVL_CHID_FRONT_LEFT);
          ret[out_index][in_index] = 0.5;

          OUT_INDEX(GAVL_CHID_FRONT_CENTER_RIGHT);
          IN_INDEX(GAVL_CHID_FRONT_RIGHT);
          ret[out_index][in_index] = 0.5;
          break;
        case 5: /* 2 Front -> 5 Front */
          OUT_INDEX(GAVL_CHID_FRONT_LEFT);
          IN_INDEX(GAVL_CHID_FRONT_LEFT);
          ret[out_index][in_index]  = 1.0;

          OUT_INDEX(GAVL_CHID_FRONT_RIGHT);
          IN_INDEX(GAVL_CHID_FRONT_RIGHT);
          ret[out_index][in_index] = 1.0;

          OUT_INDEX(GAVL_CHID_FRONT_CENTER_LEFT);
          IN_INDEX(GAVL_CHID_FRONT_LEFT);
          ret[out_index][in_index] = 0.5;

          OUT_INDEX(GAVL_CHID_FRONT_CENTER_RIGHT);
          IN_INDEX(GAVL_CHID_FRONT_RIGHT);
          ret[out_index][in_index] = 0.5;

          OUT_INDEX(GAVL_CHID_FRONT_CENTER);
          IN_INDEX(GAVL_CHID_FRONT_LEFT);
          ret[out_index][in_index] = 0.5;

          OUT_INDEX(GAVL_CHID_FRONT_CENTER);
          IN_INDEX(GAVL_CHID_FRONT_RIGHT);
          ret[out_index][in_index] = 0.5;
          break;
        }
      break;
    case 3:
      switch(output_front)
        {
        case 1: /* 3 Front -> 1 Front */
          OUT_INDEX(GAVL_CHID_FRONT_CENTER);
          IN_INDEX(GAVL_CHID_FRONT_LEFT);
          ret[out_index][in_index]   = 1.0;

          OUT_INDEX(GAVL_CHID_FRONT_CENTER);
          IN_INDEX(GAVL_CHID_FRONT_RIGHT);
          ret[out_index][in_index]  = 1.0;

          OUT_INDEX(GAVL_CHID_FRONT_CENTER);
          IN_INDEX(GAVL_CHID_FRONT_CENTER);
          ret[out_index][in_index] = in->center_level;
          break;
        case 2: /* 3 Front -> 2 Front */
          OUT_INDEX(GAVL_CHID_FRONT_LEFT);
          IN_INDEX(GAVL_CHID_FRONT_LEFT);
          ret[out_index][in_index]   = 1.0;

          OUT_INDEX(GAVL_CHID_FRONT_RIGHT);
          IN_INDEX(GAVL_CHID_FRONT_RIGHT);
          ret[out_index][in_index]  = 1.0;

          OUT_INDEX(GAVL_CHID_FRONT_LEFT);
          IN_INDEX(GAVL_CHID_FRONT_CENTER);
          ret[out_index][in_index] = in->center_level;

          OUT_INDEX(GAVL_CHID_FRONT_RIGHT);
          IN_INDEX(GAVL_CHID_FRONT_CENTER);
          ret[out_index][in_index] = in->center_level;
          break;
        case 3: /* 3 Front -> 3 Front */
          OUT_INDEX(GAVL_CHID_FRONT_LEFT);
          IN_INDEX(GAVL_CHID_FRONT_LEFT);
          ret[out_index][in_index]   = 1.0;

          OUT_INDEX(GAVL_CHID_FRONT_RIGHT);
          IN_INDEX(GAVL_CHID_FRONT_RIGHT);
          ret[out_index][in_index]  = 1.0;

          OUT_INDEX(GAVL_CHID_FRONT_CENTER);
          IN_INDEX(GAVL_CHID_FRONT_CENTER);
          ret[out_index][in_index] = 1.0;
          break;
        case 4: /* 3 Front -> 4 Front */
          OUT_INDEX(GAVL_CHID_FRONT_LEFT);
          IN_INDEX(GAVL_CHID_FRONT_LEFT);
          ret[out_index][in_index]   = 1.0;
          
          OUT_INDEX(GAVL_CHID_FRONT_RIGHT);
          IN_INDEX(GAVL_CHID_FRONT_RIGHT);
          ret[out_index][in_index]  = 1.0;

          OUT_INDEX(GAVL_CHID_FRONT_CENTER_LEFT);
          OUT_INDEX(GAVL_CHID_FRONT_CENTER_LEFT);
          ret[out_index][in_index] = 0.5;

          OUT_INDEX(GAVL_CHID_FRONT_CENTER_RIGHT);
          IN_INDEX(GAVL_CHID_FRONT_CENTER);
          ret[out_index][in_index] = 0.5;
          break;
        case 5: /* 3 Front -> 5 Front */
          OUT_INDEX(GAVL_CHID_FRONT_LEFT);
          IN_INDEX(GAVL_CHID_FRONT_LEFT);
          ret[out_index][in_index]   = 1.0;

          OUT_INDEX(GAVL_CHID_FRONT_RIGHT);
          IN_INDEX(GAVL_CHID_FRONT_RIGHT);
          ret[out_index][in_index]  = 1.0;

          OUT_INDEX(GAVL_CHID_FRONT_CENTER_LEFT);
          IN_INDEX(GAVL_CHID_FRONT_CENTER);
          ret[out_index][in_index] = 0.5;

          OUT_INDEX(GAVL_CHID_FRONT_CENTER_RIGHT);
          IN_INDEX(GAVL_CHID_FRONT_CENTER);
          ret[out_index][in_index] = 0.5;

          OUT_INDEX(GAVL_CHID_FRONT_CENTER);
          IN_INDEX(GAVL_CHID_FRONT_CENTER);
          ret[out_index][in_index] = 0.5;
          break;
        }
      break;
    }

  /* Handle rear channels */
  
  switch(input_rear)
    {
    case 0:
      switch(output_rear)
        {
        case 0: /* 0 Rear -> 0 Rear */
          break;
        case 1: /* 0 Rear -> 1 Rear */
          switch(opt->conversion_flags & GAVL_AUDIO_FRONT_TO_REAR_MASK)
            {
            case GAVL_AUDIO_FRONT_TO_REAR_COPY:
              if(input_front > 1)
                {
                OUT_INDEX(GAVL_CHID_REAR_CENTER);
                IN_INDEX(GAVL_CHID_FRONT_LEFT);
                ret[out_index][in_index] = 0.5;

                OUT_INDEX(GAVL_CHID_REAR_CENTER);
                IN_INDEX(GAVL_CHID_FRONT_RIGHT);
                ret[out_index][in_index] = 0.5;
                }
              else
                {
                OUT_INDEX(GAVL_CHID_REAR_CENTER);
                IN_INDEX(GAVL_CHID_FRONT_CENTER);
                ret[out_index][in_index] = 1.0;
                }
              break;
            case GAVL_AUDIO_FRONT_TO_REAR_MUTE:
              break;
            case GAVL_AUDIO_FRONT_TO_REAR_DIFF:
              if(input_front > 1)
                {
                OUT_INDEX(GAVL_CHID_REAR_CENTER);
                IN_INDEX(GAVL_CHID_FRONT_LEFT);
                ret[out_index][in_index]  = -0.5;
                
                OUT_INDEX(GAVL_CHID_REAR_CENTER);
                IN_INDEX(GAVL_CHID_FRONT_RIGHT);
                ret[out_index][in_index] =  0.5;
                }
              else
                {
                OUT_INDEX(GAVL_CHID_REAR_CENTER);
                IN_INDEX(GAVL_CHID_FRONT_CENTER);
                ret[out_index][in_index] = 1.0;
                }
            }
          break;
        case 2: /* 0 Rear -> 2 Rear */

          switch(opt->conversion_flags & GAVL_AUDIO_FRONT_TO_REAR_MASK)
            {
            case GAVL_AUDIO_FRONT_TO_REAR_COPY:
              if(input_front > 1)
                {
                OUT_INDEX(GAVL_CHID_REAR_LEFT);
                IN_INDEX(GAVL_CHID_FRONT_LEFT);
                ret[out_index][in_index] = 1.0;

                OUT_INDEX(GAVL_CHID_REAR_RIGHT);
                IN_INDEX(GAVL_CHID_FRONT_RIGHT);
                ret[out_index][in_index] = 1.0;
                }
              else
                {
                OUT_INDEX(GAVL_CHID_REAR_LEFT);
                IN_INDEX(GAVL_CHID_FRONT_CENTER);
                ret[out_index][in_index] = 1.0;

                OUT_INDEX(GAVL_CHID_REAR_RIGHT);
                IN_INDEX(GAVL_CHID_FRONT_CENTER);
                ret[out_index][in_index] = 1.0;
                }
              break;
            case GAVL_AUDIO_FRONT_TO_REAR_MUTE:
              break;
            case GAVL_AUDIO_FRONT_TO_REAR_DIFF:
              if(input_front > 1)
                {
                OUT_INDEX(GAVL_CHID_REAR_LEFT);
                IN_INDEX(GAVL_CHID_FRONT_LEFT);
                ret[out_index][in_index]  = -0.5;

                OUT_INDEX(GAVL_CHID_REAR_LEFT);
                IN_INDEX(GAVL_CHID_FRONT_RIGHT);
                ret[out_index][in_index] =  0.5;

                OUT_INDEX(GAVL_CHID_REAR_RIGHT);
                IN_INDEX(GAVL_CHID_FRONT_LEFT);
                ret[out_index][in_index]  = -0.5;

                OUT_INDEX(GAVL_CHID_REAR_RIGHT);
                IN_INDEX(GAVL_CHID_FRONT_RIGHT);
                ret[out_index][in_index] =  0.5;
                }
              else
                {
                OUT_INDEX(GAVL_CHID_REAR_LEFT);
                IN_INDEX(GAVL_CHID_FRONT_CENTER);
                ret[out_index][in_index] = 1.0;

                OUT_INDEX(GAVL_CHID_REAR_RIGHT);
                IN_INDEX(GAVL_CHID_FRONT_CENTER);
                ret[out_index][in_index] = 1.0;
                }
            }
          break;
        }
      break;
    case 1:
      switch(output_rear)
        {
        case 0: /* 1 Rear -> 0 Rear */
          switch(output_front)
            {
            case 1:  /* 1 Rear -> 1 Front */
              OUT_INDEX(GAVL_CHID_FRONT_CENTER);
              IN_INDEX(GAVL_CHID_REAR_CENTER);
              ret[out_index][in_index] = in->rear_level;
              break;
            case 2:  /* 1 Rear -> 2 Front */
            case 3:
              OUT_INDEX(GAVL_CHID_FRONT_LEFT);
              IN_INDEX(GAVL_CHID_REAR_CENTER);
              ret[out_index][in_index] = in->rear_level;

              OUT_INDEX(GAVL_CHID_FRONT_RIGHT);
              IN_INDEX(GAVL_CHID_REAR_CENTER);
              ret[out_index][in_index] = in->rear_level;
              break;
            }
          break;
        case 1: /* 1 Rear -> 1 Rear */
          OUT_INDEX(GAVL_CHID_REAR_CENTER);
          IN_INDEX(GAVL_CHID_REAR_CENTER);
          ret[out_index][in_index] = 1.0;
          break;
        case 2: /* 1 Rear -> 2 Rear */
          OUT_INDEX(GAVL_CHID_REAR_LEFT);
          IN_INDEX(GAVL_CHID_REAR_CENTER);
          ret[out_index][in_index] = 1.0;
          
          OUT_INDEX(GAVL_CHID_REAR_RIGHT);
          IN_INDEX(GAVL_CHID_REAR_CENTER);
          ret[out_index][in_index] = 1.0;
          break;
        }
      break;
    case 2:
      switch(output_rear)
        {
        case 0: /* 2 Rear -> 0 Rear */
          if(output_front == 1)
            {
            /* 2 Rear -> 1 Front */
            OUT_INDEX(GAVL_CHID_FRONT_CENTER);
            IN_INDEX(GAVL_CHID_REAR_LEFT);
            ret[out_index][in_index] = in->rear_level;

            OUT_INDEX(GAVL_CHID_FRONT_CENTER);
            IN_INDEX(GAVL_CHID_REAR_RIGHT);
            ret[out_index][in_index] = in->rear_level;
            }
          else
            {
            OUT_INDEX(GAVL_CHID_FRONT_LEFT);
            IN_INDEX(GAVL_CHID_REAR_LEFT);
            ret[out_index][in_index] = in->rear_level;

            OUT_INDEX(GAVL_CHID_FRONT_RIGHT);
            IN_INDEX(GAVL_CHID_REAR_RIGHT);
            ret[out_index][in_index] = in->rear_level;
            break;
            }
          break;
        case 1: /* 2 Rear -> 1 Rear */
          OUT_INDEX(GAVL_CHID_REAR_CENTER);
          IN_INDEX(GAVL_CHID_REAR_LEFT);
          ret[out_index][in_index] = 1.0;

          OUT_INDEX(GAVL_CHID_REAR_CENTER);
          IN_INDEX(GAVL_CHID_REAR_LEFT);
          ret[out_index][in_index] = 1.0;
          break;
        case 2: /* 2 Rear -> 2 Rear */
          OUT_INDEX(GAVL_CHID_REAR_LEFT);
          IN_INDEX(GAVL_CHID_REAR_LEFT);
          ret[out_index][in_index] = 1.0;

          OUT_INDEX(GAVL_CHID_REAR_RIGHT);
          IN_INDEX(GAVL_CHID_REAR_RIGHT);
          ret[out_index][in_index] = 1.0;
          break;
        case 3: /* 2 Rear -> 3 Rear */
          OUT_INDEX(GAVL_CHID_REAR_LEFT);
          IN_INDEX(GAVL_CHID_REAR_LEFT);
          ret[out_index][in_index] = 1.0;

          OUT_INDEX(GAVL_CHID_REAR_RIGHT);
          IN_INDEX(GAVL_CHID_REAR_RIGHT);
          ret[out_index][in_index] = 1.0;

          OUT_INDEX(GAVL_CHID_REAR_CENTER);
          IN_INDEX(GAVL_CHID_REAR_LEFT);
          ret[out_index][in_index] = 0.5;

          OUT_INDEX(GAVL_CHID_REAR_CENTER);
          IN_INDEX(GAVL_CHID_REAR_RIGHT);
          ret[out_index][in_index] = 0.5;
          break;
        }
      break;
    }
  /* Handle LFE */
  
  if(input_lfe && output_lfe)
    {
    OUT_INDEX(GAVL_CHID_LFE);
    IN_INDEX(GAVL_CHID_LFE);
    ret[out_index][in_index] = 1.0;
    }

  normalize_matrix(ret, in->num_channels, out->num_channels);
  return;
  
  fail:
  fprintf(stderr, "Couldn't construct mix matrix, using (probably wrong) default\n");
  in_index = 0;
  out_index = 0;

  while(out_index < out->num_channels)
    {
    memset(ret[out_index], 0, sizeof(ret[out_index][0]) * in->num_channels);
    ret[out_index][in_index] = 1.0;
    in_index++;
    if(in_index == in->num_channels)
      in_index++;
    }
  
  //  dump_matrix(in, out, ret);
  }

static void set_factor(gavl_mix_input_channel_t * ret,
                       float fac, gavl_sample_format_t format)
  {
  switch(format)
    {
    case GAVL_SAMPLE_U8:
    case GAVL_SAMPLE_S8:
      ret->factor.f_8 =     (int8_t)(fac * INT8_MAX + 0.5);
      break;
    case GAVL_SAMPLE_U16:
    case GAVL_SAMPLE_S16:
      ret->factor.f_16 =    (int16_t)(fac * INT16_MAX + 0.5);
      break;
    case GAVL_SAMPLE_S32:
      ret->factor.f_32 =    (int32_t)(fac * (INT32_MAX>>1) + 0.5);
      break;
    case GAVL_SAMPLE_FLOAT:
      ret->factor.f_float = fac;
      break;
    case GAVL_SAMPLE_NONE:
      break;
    }
  }

static void init_context(gavl_mix_matrix_t * ctx,
                         float matrix[GAVL_MAX_CHANNELS][GAVL_MAX_CHANNELS],
                         gavl_audio_format_t * in_format,
                         gavl_audio_format_t * out_format)
  {
  int i, j;
  int num_inputs;
  gavl_mixer_table_t tab;
  //  fprintf(stderr, "init_context...");
  memset(&tab, 0, sizeof(tab));

  gavl_setup_mix_funcs_c(&tab, in_format);
  
  for(i = 0; i < out_format->num_channels; i++)
    {
    ctx->output_channels[i].index = i;
    num_inputs = 0;
    for(j = 0; j < in_format->num_channels; j++)
      {
      if(matrix[i][j] != 0.0)
        {
        //        fprintf(stderr, "Set factor %f %d %d\n",
        //                matrix[i][j], i, j);
        set_factor(&(ctx->output_channels[i].inputs[num_inputs]),
                   matrix[i][j], in_format->sample_format);
        ctx->output_channels[i].inputs[num_inputs].index = j;
        num_inputs++;
        }
      }
    ctx->output_channels[i].num_inputs = num_inputs;

    //    fprintf(stderr, "Fac: %f\n", matrix[i][ctx->output_channels[i].inputs[0].index]);
        
    /* Check, if we just need to copy */
    
    if((ctx->output_channels[i].num_inputs == 1) &&
       (fabs(matrix[i][ctx->output_channels[i].inputs[0].index]- 1.0) < 0.01))
      {
      ctx->output_channels[i].func = tab.copy_func;
#if 0
      fprintf(stderr, "Copying channel %s to %s\n",
              gavl_channel_id_to_string(in_format->channel_locations[ctx->output_channels[i].inputs[0].index]),
              gavl_channel_id_to_string(out_format->channel_locations[i]));
#endif
      }
    else
      {
      switch(ctx->output_channels[i].num_inputs)
        {
        case 0:
          break;
        case 1:
          ctx->output_channels[i].func = tab.mix_1_to_1;
          break;
        case 2:
          ctx->output_channels[i].func = tab.mix_2_to_1;
          break;
        case 3:
          ctx->output_channels[i].func = tab.mix_3_to_1;
          break;
        case 4:
          ctx->output_channels[i].func = tab.mix_4_to_1;
          break;
        case 5:
          ctx->output_channels[i].func = tab.mix_5_to_1;
          break;
        case 6:
          ctx->output_channels[i].func = tab.mix_6_to_1;
          break;
        }
      }

    //    output_channel_dump(&(ctx->output_channels[i]));
    }
  
  //  fprintf(stderr, "done\n");
  }

gavl_mix_matrix_t *
gavl_create_mix_matrix(gavl_audio_options_t * opt,
                        gavl_audio_format_t * in,
                        gavl_audio_format_t * out)
  {
  float mix_matrix[GAVL_MAX_CHANNELS][GAVL_MAX_CHANNELS];
  gavl_mix_matrix_t * ret;
  ret = calloc(1, sizeof(*ret));

  memset(&mix_matrix, 0, sizeof(mix_matrix));

  //  gavl_audio_format_dump(out);

  //  fprintf(stderr, "Init Matrix...");
  
  init_matrix(mix_matrix, opt, in, out);
  //  fprintf(stderr, "done\n");
  
  //  fprintf(stderr, "Init mix context\n");
  init_context(ret, mix_matrix, in, out);
  //  fprintf(stderr, "done\n");
                 
  return ret;
  }

void gavl_destroy_mix_matrix(gavl_mix_matrix_t * ctx)
  {
  free(ctx);
  }


gavl_audio_convert_context_t *
gavl_mix_context_create(gavl_audio_options_t * opt,
                        gavl_audio_format_t * in_format,
                        gavl_audio_format_t * out_format)
  {
  gavl_audio_convert_context_t * ret;
#if 0
  fprintf(stderr, "gavl_mix_context_create\n");
  gavl_audio_format_dump(in_format);
  gavl_audio_format_dump(out_format);
#endif
  ret = gavl_audio_convert_context_create(in_format,
                                          out_format);

  ret->output_format.num_channels =  out_format->num_channels;
  memcpy(ret->output_format.channel_locations,
         out_format->channel_locations,
         GAVL_MAX_CHANNELS * sizeof(out_format->channel_locations[0]));
  ret->mix_matrix = gavl_create_mix_matrix(opt,
                                            &(ret->input_format),
                                            &(ret->output_format));
  ret->func = gavl_mix_audio;
  
  return ret;
  }
