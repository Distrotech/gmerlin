
struct gavl_dsp_context_s
  {
  int quality;
  int accel_flags;
  gavl_dsp_funcs_t funcs;
  };

void gavl_dsp_init_c(gavl_dsp_funcs_t * funcs, 
                     int quality);

#ifdef HAVE_MMX
void gavl_dsp_init_mmx(gavl_dsp_funcs_t * funcs, 
                       int quality);
void gavl_dsp_init_mmxext(gavl_dsp_funcs_t * funcs, 
                          int quality);
#endif

#ifdef HAVE_SSE
void gavl_dsp_init_sse(gavl_dsp_funcs_t * funcs, 
                       int quality);
#endif
