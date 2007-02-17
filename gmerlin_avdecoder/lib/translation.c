#include <pthread.h>
#include <avdec_private.h>
#include <libintl.h>

static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
static int translation_initialized = 0;

void bgav_translation_init()
  {
  pthread_mutex_lock(&mutex);

  if(!translation_initialized)
    {
    bindtextdomain (PACKAGE, LOCALE_DIR);
    translation_initialized = 1;
    }
  pthread_mutex_unlock(&mutex);
  }
