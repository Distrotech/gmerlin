/* DSP Context */

typedef struct gavl_dsp_context_s gavl_dsp_context_t;

typedef struct
  {
  /* Sum of absolute differences */
  int (*sad_rgb15)(uint8_t * src_1, uint8_t * src_2, 
                   int stride_1, int stride_2, 
                   int w, int h);
  int (*sad_rgb16)(uint8_t * src_1, uint8_t * src_2, 
                   int stride_1, int stride_2, 
                   int w, int h);
  int (*sad_8)(uint8_t * src_1, uint8_t * src_2, 
               int stride_1, int stride_2, 
               int w, int h);
  int (*sad_16)(uint8_t * src_1, uint8_t * src_2, 
               int stride_1, int stride_2, 
               int w, int h);
  float (*sad_f)(uint8_t * src_1, uint8_t * src_2, 
                 int stride_1, int stride_2, 
                 int w, int h);

  /* Averaging */
  void (*average_rgb15)(uint8_t * src_1, uint8_t * src_2, 
                        uint8_t * dst, int num);

  void (*average_rgb16)(uint8_t * src_1, uint8_t * src_2, 
                        uint8_t * dst, int num);

  void (*average_8)(uint8_t * src_1, uint8_t * src_2, 
                    uint8_t * dst, int num);

  void (*average_16)(uint8_t * src_1, uint8_t * src_2, 
                     uint8_t * dst, int num);

  void (*average_f)(uint8_t * src_1, uint8_t * src_2, 
                    uint8_t * dst, int num);
  
  } gavl_dsp_funcs_t;


gavl_dsp_context_t * gavl_dsp_context_create();

void gavl_dsp_context_set_quality(gavl_dsp_context_t * ctx,
                                  int q);

gavl_dsp_funcs_t * 
gavl_dsp_context_get_funcs(gavl_dsp_context_t * ctx);

void gavl_dsp_context_destroy(gavl_dsp_context_t * ctx);
