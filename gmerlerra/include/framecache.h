typedef struct
  {
  void * frame;
  int64_t time;
  int64_t duration;
  int refcount;
  } bg_nle_frame_cache_entry_t;

typedef struct bg_nle_frame_cache_s bg_nle_frame_cache_t;

bg_nle_frame_cache_t *
bg_nle_frame_cache_create(int max_cache_size, int consumers,
                          void (*create_callback)(void*, bg_nle_frame_cache_entry_t*e),
                          void (*destroy_callback)(void*, bg_nle_frame_cache_entry_t*e),
                          void*);

void bg_nle_frame_cache_destroy(bg_nle_frame_cache_t *);

bg_nle_frame_cache_entry_t * bg_nle_frame_cache_get(bg_nle_frame_cache_t * c,
                                                    int64_t time);

void bg_nle_frame_cache_flush(bg_nle_frame_cache_t * c, int64_t time);
