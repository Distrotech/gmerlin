
static void RENAME(mix_1_to_1)(gavl_audio_convert_context_t * ctx,
                               int output_index)
  {
  int i;
  TMP_TYPE tmp;

  SAMPLE_TYPE factor1 = FACTOR(0);
  SAMPLE_TYPE * s1    = SRC(0);
  SAMPLE_TYPE * d     = DST();
  
  i = ctx->input_frame->valid_samples;
  
  while(i--)
    {
    tmp = *s1 * factor1;
    ADJUST_TMP(tmp);
    *d = tmp;
    s1++;
    d++;
    }
  }


static void RENAME(mix_2_to_1)(gavl_audio_convert_context_t * ctx,
                               int output_index)
  {
  int i;
  TMP_TYPE tmp;

  SAMPLE_TYPE factor1 = FACTOR(0);
  SAMPLE_TYPE factor2 = FACTOR(1);
  SAMPLE_TYPE * s1    = SRC(0);
  SAMPLE_TYPE * s2    = SRC(1);
  SAMPLE_TYPE * d     = DST();
  
  i = ctx->input_frame->valid_samples;
  
  while(i--)
    {
    tmp = *s1 * factor1 + *s2 * factor2;
    ADJUST_TMP(tmp);
    *d = tmp;
    s1++;
    s2++;
    d++;
    }
  }

static void RENAME(mix_3_to_1)(gavl_audio_convert_context_t * ctx,
                               int output_index)
  {
  int i;
  TMP_TYPE tmp;
    
  SAMPLE_TYPE factor1 = FACTOR(0);
  SAMPLE_TYPE factor2 = FACTOR(1);
  SAMPLE_TYPE factor3 = FACTOR(2);
  SAMPLE_TYPE * s1    = SRC(0);
  SAMPLE_TYPE * s2    = SRC(1);
  SAMPLE_TYPE * s3    = SRC(2);
  SAMPLE_TYPE * d     = DST();
  
  i = ctx->input_frame->valid_samples;
  
  while(i--)
    {
    tmp = *s1 * factor1 + *s2 * factor2 + *s3 * factor3;
    ADJUST_TMP(tmp);
    *d = tmp;
    s1++;
    s2++;
    s3++;
    d++;
    }
  }

static void RENAME(mix_4_to_1)(gavl_audio_convert_context_t * ctx,
                               int output_index)
  {
  int i;
  TMP_TYPE tmp;

  SAMPLE_TYPE factor1 = FACTOR(0);
  SAMPLE_TYPE factor2 = FACTOR(1);
  SAMPLE_TYPE factor3 = FACTOR(2);
  SAMPLE_TYPE factor4 = FACTOR(3);
  SAMPLE_TYPE * s1    = SRC(0);
  SAMPLE_TYPE * s2    = SRC(1);
  SAMPLE_TYPE * s3    = SRC(2);
  SAMPLE_TYPE * s4    = SRC(3);
  SAMPLE_TYPE * d     = DST();
  
  i = ctx->input_frame->valid_samples;
  
  while(i--)
    {
    tmp = *s1 * factor1 + *s2 * factor2 + *s3 * factor3 + *s4 * factor4;
    ADJUST_TMP(tmp);
    *d = tmp;
    s1++;
    s2++;
    s3++;
    s4++;
    d++;
    }
  }


static void RENAME(mix_5_to_1)(gavl_audio_convert_context_t * ctx,
                               int output_index)
  {
  int i;
  TMP_TYPE tmp;

  SAMPLE_TYPE factor1 = FACTOR(0);
  SAMPLE_TYPE factor2 = FACTOR(1);
  SAMPLE_TYPE factor3 = FACTOR(2);
  SAMPLE_TYPE factor4 = FACTOR(3);
  SAMPLE_TYPE factor5 = FACTOR(4);
  SAMPLE_TYPE * s1    = SRC(0);
  SAMPLE_TYPE * s2    = SRC(1);
  SAMPLE_TYPE * s3    = SRC(2);
  SAMPLE_TYPE * s4    = SRC(3);
  SAMPLE_TYPE * s5    = SRC(4);
  SAMPLE_TYPE * d     = DST();
  
  i = ctx->input_frame->valid_samples;
  
  while(i--)
    {
    tmp = *s1 * factor1 + *s2 * factor2 + *s3 * factor3 +
      *s4 * factor4 + *s5 * factor5;
    ADJUST_TMP(tmp);
    *d = tmp;
    s1++;
    s2++;
    s3++;
    s4++;
    s5++;
    d++;
    }
  }

static void RENAME(mix_6_to_1)(gavl_audio_convert_context_t * ctx,
                               int output_index)
  {
  int i;
  TMP_TYPE tmp;

  SAMPLE_TYPE factor1 = FACTOR(0);
  SAMPLE_TYPE factor2 = FACTOR(1);
  SAMPLE_TYPE factor3 = FACTOR(2);
  SAMPLE_TYPE factor4 = FACTOR(3);
  SAMPLE_TYPE factor5 = FACTOR(4);
  SAMPLE_TYPE factor6 = FACTOR(5);
  SAMPLE_TYPE * s1    = SRC(0);
  SAMPLE_TYPE * s2    = SRC(1);
  SAMPLE_TYPE * s3    = SRC(2);
  SAMPLE_TYPE * s4    = SRC(3);
  SAMPLE_TYPE * s5    = SRC(4);
  SAMPLE_TYPE * s6    = SRC(5);
  SAMPLE_TYPE * d     = DST();

  i = ctx->input_frame->valid_samples;
  
  while(i--)
    {
    tmp = *s1 * factor1 + *s2 * factor2 + *s3 * factor3 +
      *s4 * factor4 + *s5 * factor5 + *s6 * factor6;
    ADJUST_TMP(tmp);

    *d = tmp;

    s1++;
    s2++;
    s3++;
    s4++;
    s5++;
    s6++;
    
    d++;
    }
  }

