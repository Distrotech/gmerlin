#include <stdlib.h>
#include <avdec_private.h>

/*
 *  Wichtige Funktionen:
 *
 *  int bgav_url_split(const char * url,
 *                   char ** protocol,
 *                   char ** hostname,
 *                   int * port,
 *                   char ** path);
 *
 *  int bgav_tcp_connect(const char * host, int port, int milliseconds, char ** error_msg);
 *  int bgav_tcp_send(int fd, uint8_t * data, int len, char ** error_msg);
 *  int bgav_read_line_fd(int fd, char ** ret, int * ret_alloc, int milliseconds);
 *  int bgav_read_data_fd(int fd, uint8_t * ret, int size, int milliseconds);
 *  char * bgav_sprintf(const char * format,...);
 *
 *  Fuer error_msg muss &ctx->error_msg uebergeben werden
 */ 

typedef struct
  {
  int dummy;
  } ftp_priv_t;

/* Open Funktion: Gibt 1 bei Erfolg zurueck */

static int open_ftp(bgav_input_context_t * ctx, const char * url)
  {
  ftp_priv_t * p;

  p = calloc(1, sizeof(*p));
  ctx->priv = p;

  return 0;
  }

/* Lese funktionen: Geben die Zahl der gelesenen BYTES zurueck */

static int read_ftp(bgav_input_context_t* ctx,
                     uint8_t * buffer, int len)
  {
  ftp_priv_t * p;
  p = (ftp_priv_t*)(ctx->priv);

  return 0;
  }

static int read_nonblock_ftp(bgav_input_context_t* ctx,
                              uint8_t * buffer, int len)
  {
  ftp_priv_t * p;
  p = (ftp_priv_t*)(ctx->priv);

  return 0;
  }

/* Close: Zumachen */

static void close_ftp(bgav_input_context_t * ctx)
  {
  ftp_priv_t * p;
  p = (ftp_priv_t*)(ctx->priv);
  free(p);
  }

bgav_input_t bgav_input_ftp =
  {
    name:          "ftp",
    open:          open_ftp,
    read:          read_ftp,
    read_nonblock: read_nonblock_ftp,
    close:         close_ftp
  };

