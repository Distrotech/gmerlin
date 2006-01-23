/*****************************************************************
 
  qt_wave.c
 
  Copyright (c) 2005 by Burkhard Plaum - plaum@ipf.uni-stuttgart.de
 
  http://gmerlin.sourceforge.net
 
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
 
  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111, USA.
 
*****************************************************************/

#include <avdec_private.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <qt.h>

int bgav_qt_wave_read(qt_atom_header_t * h, bgav_input_context_t * ctx,
                      qt_wave_t * ret)
  {
  qt_atom_header_t ch; /* Child header */
  uint8_t * data_ptr;
  bgav_input_context_t * input_mem;
    
  ret->raw_size = h->size - (ctx->position - h->start_position);
  ret->raw = malloc(ret->raw_size);

  
  
  if(bgav_input_read_data(ctx, ret->raw, ret->raw_size) < ret->raw_size)
    return 0;
  
  input_mem = bgav_input_open_memory(ret->raw,
                                     ret->raw_size);
  
  while(input_mem->position < ret->raw_size)
    {
    data_ptr = ret->raw + input_mem->position;
    
    if(!bgav_qt_atom_read_header(input_mem, &ch))
      return 0;
    switch(ch.fourcc)
      {
      case BGAV_MK_FOURCC('e', 's', 'd', 's'):
        if(!bgav_qt_esds_read(&ch, input_mem, &(ret->esds)))
          return 0;
        ret->has_esds = 1;
        break;
      case BGAV_MK_FOURCC('f', 'r', 'm', 'a'):
        if(!bgav_qt_frma_read(&ch, input_mem, &(ret->frma)))
          return 0;
        ret->has_frma = 1;
        break;
      case BGAV_MK_FOURCC('e', 'n', 'd', 'a'):
        if(!bgav_qt_enda_read(&ch, input_mem, &(ret->enda)))
          return 0;
        ret->has_enda = 1;
        break;
      case 0: /* audio atom Terminator */
        break;
      default:
        /* Append user atom */
        //        fprintf(stderr, "Got user atom:\n");
        //        bgav_qt_atom_dump_header(&ch);
        ret->user_atoms = realloc(ret->user_atoms,
                                  sizeof(*(ret->user_atoms))*(ret->num_user_atoms+1));
        ret->user_atoms[ret->num_user_atoms] = malloc(ch.size);
        memcpy(ret->user_atoms[ret->num_user_atoms], data_ptr, ch.size);
        bgav_qt_atom_skip(input_mem, &ch);
        ret->num_user_atoms++;
        break;
      }
    }
  bgav_input_destroy(input_mem);
  return 1;
  }

void bgav_qt_wave_dump(qt_wave_t * f)
  {
  int i;
  int size;
  uint32_t fourcc;

  fprintf(stderr, "wave\n");
  if(f->has_frma)
    bgav_qt_frma_dump(&f->frma);
  if(f->has_enda)
    bgav_qt_enda_dump(&f->enda);
  if(f->has_esds)
    bgav_qt_esds_dump(&f->esds);

  for(i = 0; i < f->num_user_atoms; i++)
    {
    size = BGAV_PTR_2_32BE(f->user_atoms[i]);
    fourcc = BGAV_PTR_2_FOURCC(f->user_atoms[i]+4);
    fprintf(stderr, "User atom: ");
    bgav_dump_fourcc(fourcc);
    fprintf(stderr, " (size: %d)\n", size);
    bgav_hexdump(f->user_atoms[i], size, 16);
    }
  }

void bgav_qt_wave_free(qt_wave_t * w)
  {
  int i;
  if(w->raw)
    free(w->raw);
  
  if(w->has_esds)
    bgav_qt_esds_free(&w->esds);
  if(w->user_atoms)
    {
    for(i = 0; i < w->num_user_atoms; i++)
      free(w->user_atoms[i]);
    free(w->user_atoms);
    }
  
  }
