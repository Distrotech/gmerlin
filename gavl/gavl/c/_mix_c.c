
static void RENAME(mix_1_to_1)(gavl_mix_output_channel_t * channel,
                               gavl_audio_frame_t * input_frame,
                               gavl_audio_frame_t * output_frame)
  {
  int i;
  TMP_TYPE tmp;

  SAMPLE_TYPE factor1 = FACTOR(0);
  
  i = input_frame->valid_samples;
  
  while(i--)
    {
    tmp = SRC(0,i) * factor1;
    ADJUST_TMP(tmp);
    SETDST(i, tmp);
    }
  }


static void RENAME(mix_2_to_1)(gavl_mix_output_channel_t * channel,
                               gavl_audio_frame_t * input_frame,
                               gavl_audio_frame_t * output_frame)
  {
  int i;
  TMP_TYPE tmp;

  SAMPLE_TYPE factor1 = FACTOR(0);
  SAMPLE_TYPE factor2 = FACTOR(1);
  
  i = input_frame->valid_samples;
  
  while(i--)
    {
    tmp = SRC(0,i) * factor1 + SRC(1,i) * factor2;
    ADJUST_TMP(tmp);
    SETDST(i, tmp);
    }
  }

static void RENAME(mix_3_to_1)(gavl_mix_output_channel_t * channel,
                               gavl_audio_frame_t * input_frame,
                               gavl_audio_frame_t * output_frame)
  {
  int i;
  TMP_TYPE tmp;
    
  SAMPLE_TYPE factor1 = FACTOR(0);
  SAMPLE_TYPE factor2 = FACTOR(1);
  SAMPLE_TYPE factor3 = FACTOR(2);
  
  i = input_frame->valid_samples;
  
  while(i--)
    {
    tmp = SRC(0,i) * factor1 + SRC(1,i) * factor2 + SRC(2,i) * factor3;
    ADJUST_TMP(tmp);
    SETDST(i, tmp);
    }
  }

static void RENAME(mix_4_to_1)(gavl_mix_output_channel_t * channel,
                               gavl_audio_frame_t * input_frame,
                               gavl_audio_frame_t * output_frame)
  {
  int i;
  TMP_TYPE tmp;

  SAMPLE_TYPE factor1 = FACTOR(0);
  SAMPLE_TYPE factor2 = FACTOR(1);
  SAMPLE_TYPE factor3 = FACTOR(2);
  SAMPLE_TYPE factor4 = FACTOR(3);
  
  i = input_frame->valid_samples;
  
  while(i--)
    {
    tmp = SRC(0,i) * factor1 + SRC(1,i) * factor2 + SRC(2,i) * factor3 +
      SRC(3,i) * factor4;
    ADJUST_TMP(tmp);
    SETDST(i, tmp);
    }
  }


static void RENAME(mix_5_to_1)(gavl_mix_output_channel_t * channel,
                               gavl_audio_frame_t * input_frame,
                               gavl_audio_frame_t * output_frame)
  {
  int i;
  TMP_TYPE tmp;

  SAMPLE_TYPE factor1 = FACTOR(0);
  SAMPLE_TYPE factor2 = FACTOR(1);
  SAMPLE_TYPE factor3 = FACTOR(2);
  SAMPLE_TYPE factor4 = FACTOR(3);
  SAMPLE_TYPE factor5 = FACTOR(4);
  
  i = input_frame->valid_samples;
  
  while(i--)
    {
    tmp = SRC(0,i) * factor1 + SRC(1,i) * factor2 + SRC(1,i) * factor3 +
      SRC(3,i) * factor4 + SRC(4,i) * factor5;
    ADJUST_TMP(tmp);
    SETDST(i, tmp);
    }
  }

static void RENAME(mix_6_to_1)(gavl_mix_output_channel_t * channel,
                               gavl_audio_frame_t * input_frame,
                               gavl_audio_frame_t * output_frame)
  {
  int i;
  TMP_TYPE tmp;

  SAMPLE_TYPE factor1 = FACTOR(0);
  SAMPLE_TYPE factor2 = FACTOR(1);
  SAMPLE_TYPE factor3 = FACTOR(2);
  SAMPLE_TYPE factor4 = FACTOR(3);
  SAMPLE_TYPE factor5 = FACTOR(4);
  SAMPLE_TYPE factor6 = FACTOR(5);

  i = input_frame->valid_samples;
  
  while(i--)
    {
    tmp = SRC(0,i) * factor1 + SRC(1,i) * factor2 + SRC(2,i) * factor3 +
      SRC(3,i) * factor4 + SRC(4,i) * factor5 + SRC(5,i) * factor6;
    ADJUST_TMP(tmp);

    SETDST(i, tmp);
    
    }
  }

