#include <avdec_private.h>
#include <string.h>
#include <stdlib.h>

#include <qt.h>


int bgav_qt_glbl_read(qt_atom_header_t * h, bgav_input_context_t * ctx,
                      qt_glbl_t * ret)
  {
  ret->size = h->size - 8;
  ret->data = malloc(ret->size);
  if(bgav_input_read_data(ctx, ret->data, ret->size) < ret->size)
    return 0;
  return 1;
  }

void bgav_qt_glbl_dump(int indent, qt_glbl_t * f)
  {
  bgav_diprintf(indent, "glbl:\n");
  bgav_diprintf(indent+2, "Size: %d\n", f->size);
  bgav_hexdump(f->data, f->size, 16);
  }

void bgav_qt_glbl_free(qt_glbl_t * r)
  {
  if(r->data) free(r->data);
  }
