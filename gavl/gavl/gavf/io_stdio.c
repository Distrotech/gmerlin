#include <gavfprivate.h>

#ifdef _WIN32
#define GAVL_FSEEK(a,b,c) fseeko64(a,b,c)
#define GAVL_FTELL(a) ftello64(a)
#else
#ifdef HAVE_FSEEKO
#define GAVL_FSEEK fseeko
#else
#define GAVL_FSEEK fseek
#endif

#ifdef HAVE_FTELLO
#define GAVL_FTELL ftello
#else
#define GAVL_FTELL ftell
#endif
#endif

static int read_file(void * priv, uint8_t * data, int len)
  {
  return fread(data, 1, len, (FILE*)priv);
  }

static int write_file(void * priv, const uint8_t * data, int len)
  {
  return fwrite(data, 1, len, (FILE*)priv);
  }

static int64_t seek_file(void * priv, int64_t pos, int whence)
  {
  GAVL_FSEEK((FILE*)priv, pos, whence);
  return GAVL_FTELL((FILE*)priv);
  }

GAVL_PUBLIC
gavf_io_t * gavf_io_create_file(FILE * f, int wr, int can_seek)
  {
  gavf_read_func rf;
  gavf_write_func wf;
  gavf_seek_func sf;
    
  if(wr)
    {
    wf = write_file;
    rf = NULL;
    }
  else
    {
    wf = NULL;
    rf = read_file;
    }
  if(can_seek)
    sf = seek_file;
  else
    sf = NULL;

  return gavf_io_create(rf, wf, sf, NULL, f);
  }
