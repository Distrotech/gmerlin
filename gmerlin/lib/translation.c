#include <config.h>
#include <pthread.h>
#include <translation.h>

static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
static int translation_initialized = 0;

void bg_translation_init()
  {
  pthread_mutex_lock(&mutex);

  if(!translation_initialized)
    {
    bindtextdomain (PACKAGE, LOCALE_DIR);
    translation_initialized = 1;
    }
  pthread_mutex_unlock(&mutex);
  }
