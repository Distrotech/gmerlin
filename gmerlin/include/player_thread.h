typedef struct bg_player_thread_common_s bg_player_thread_common_t;
typedef struct bg_player_thread_s        bg_player_thread_t;


bg_player_thread_common_t *
bg_player_thread_common_create();

void bg_player_thread_common_destroy(bg_player_thread_common_t * com);


bg_player_thread_t *
bg_player_thread_create(bg_player_thread_common_t * com);

void bg_player_thread_destroy(bg_player_thread_t * th);

void bg_player_thread_set_func(bg_player_thread_t * th,
                               void * (*func)(void*), void * arg);

void bg_player_threads_init(bg_player_thread_t ** th, int num);

void bg_player_threads_start(bg_player_thread_t ** th, int num);

/* Pause all threads, resume with bg_player_threads_start */
void bg_player_threads_pause(bg_player_thread_t ** th, int num);

/* Stop and join all threads, must call bg_player_threads_init() after */
void bg_player_threads_join(bg_player_thread_t ** th, int num);

/* Returns 1 if the thread should continue, 0 else.
   If the thread was paused, this function blocks until we restart */
   
int bg_player_thread_check(bg_player_thread_t * th);
void bg_player_thread_wait_for_start(bg_player_thread_t * th);


