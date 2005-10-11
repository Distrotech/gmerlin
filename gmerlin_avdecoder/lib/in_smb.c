#include <avdec_private.h>

#include <stdio.h>
#include <libsmbclient.h>

static int open_smb(bgav_input_context_t * ctx, const char * url)
  {
  return 0;
  }

static int     read_smb(bgav_input_context_t* ctx,
                         uint8_t * buffer, int len)
  {
  return 0;
  }

static int64_t seek_byte_smb(bgav_input_context_t * ctx,
                              int64_t pos, int whence)
  {
  return 0;
  }

static void    close_smb(bgav_input_context_t * ctx)
  {

  }

bgav_input_t bgav_input_smb =
  {
    name:      "samba",
    open:      open_smb,
    read:      read_smb,
    seek_byte: seek_byte_smb,
    close:     close_smb
  };

