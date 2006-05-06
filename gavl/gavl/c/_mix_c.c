
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
    tmp = (TMP_TYPE)SRC(0,i) * (TMP_TYPE)factor1;
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
    tmp =
      (TMP_TYPE)SRC(0,i) * (TMP_TYPE)factor1 +
      (TMP_TYPE)SRC(1,i) * (TMP_TYPE)factor2;
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
    tmp =
      (TMP_TYPE)SRC(0,i) * (TMP_TYPE)factor1 +
      (TMP_TYPE)SRC(1,i) * (TMP_TYPE)factor2 +
      (TMP_TYPE)SRC(2,i) * (TMP_TYPE)factor3;
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
    tmp =
      (TMP_TYPE)SRC(0,i) * (TMP_TYPE)factor1 +
      (TMP_TYPE)SRC(1,i) * (TMP_TYPE)factor2 +
      (TMP_TYPE)SRC(2,i) * (TMP_TYPE)factor3 +
      (TMP_TYPE)SRC(3,i) * (TMP_TYPE)factor4;
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
    tmp =
      (TMP_TYPE)SRC(0,i) * (TMP_TYPE)factor1 +
      (TMP_TYPE)SRC(1,i) * (TMP_TYPE)factor2 +
      (TMP_TYPE)SRC(1,i) * (TMP_TYPE)factor3 +
      (TMP_TYPE)SRC(3,i) * (TMP_TYPE)factor4 +
      (TMP_TYPE)SRC(4,i) * (TMP_TYPE)factor5;
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
    tmp =
      (TMP_TYPE)SRC(0,i) * (TMP_TYPE)factor1 +
      (TMP_TYPE)SRC(1,i) * (TMP_TYPE)factor2 +
      (TMP_TYPE)SRC(2,i) * (TMP_TYPE)factor3 +
      (TMP_TYPE)SRC(3,i) * (TMP_TYPE)factor4 +
      (TMP_TYPE)SRC(4,i) * (TMP_TYPE)factor5 +
      (TMP_TYPE)SRC(5,i) * (TMP_TYPE)factor6;
    ADJUST_TMP(tmp);

    SETDST(i, tmp);
    
    }
  }

static void RENAME(mix_all_to_1)(gavl_mix_output_channel_t * channel,
                                 gavl_audio_frame_t * input_frame,
                                 gavl_audio_frame_t * output_frame)
  {
  int i, j;
  TMP_TYPE tmp;
    
  i = input_frame->valid_samples;
  
  while(i--)
    {
    tmp = 0;
    j = channel->num_inputs;
    
    while(j--)
      tmp += (TMP_TYPE)SRC(j,i) * (TMP_TYPE)FACTOR(j);
    
    ADJUST_TMP(tmp);

    SETDST(i, tmp);
    
    }
  }

