/*****************************************************************
 
  qt_hdlr.c
 
  Copyright (c) 2003-2004 by Burkhard Plaum - plaum@ipf.uni-stuttgart.de
 
  http://gmerlin.sourceforge.net
 
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
 
  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111, USA.
 
*****************************************************************/

#include <string.h>
#include <stdlib.h>

#include <avdec_private.h>
#include <stdio.h>

#include <qt.h>

/*

typedef struct
  {
  uint32_t component_type;
  uint32_t component_subtype;
  uint32_t component_manufacturer;
  uint32_t component_flags;
  uint32_t component_flag_mask;
  char * component_name;
  } qt_hdlr_t;
*/


void bgav_qt_hdlr_dump(qt_hdlr_t * ret)
  {
  fprintf(stderr,"hdlr:\n");
  
  fprintf(stderr,"  component_type: ");
  bgav_dump_fourcc(ret->component_type);
  
  fprintf(stderr,"\n  component_subtype: ");
  bgav_dump_fourcc(ret->component_subtype);

  fprintf(stderr,"\n  component_manufacturer: ");
  bgav_dump_fourcc(ret->component_manufacturer);

  fprintf(stderr,"\n  component_flags:     0x%08x\n", ret->component_flags);
  fprintf(stderr,"  component_flag_mask: 0x%08x\n", ret->component_flag_mask);
  fprintf(stderr,"  component_name: %s\n", ret->component_name);
  }


int bgav_qt_hdlr_read(qt_atom_header_t * h,
                      bgav_input_context_t * input,
                      qt_hdlr_t * ret)
  {
  int name_len;
  uint8_t tmp_8;
  READ_VERSION_AND_FLAGS;
  memcpy(&(ret->h), h, sizeof(*h));
  if (!bgav_input_read_fourcc(input, &(ret->component_type)) ||
      !bgav_input_read_fourcc(input, &(ret->component_subtype)) ||
      !bgav_input_read_32_le(input, &(ret->component_manufacturer)) ||
      !bgav_input_read_32_le(input, &(ret->component_flags)) ||
      !bgav_input_read_32_le(input, &(ret->component_flag_mask)))
    return 0;
#if 0
  fprintf(stderr, "Component type: ");
  bgav_dump_fourcc(ret->component_type);
  fprintf(stderr, "\n");
#endif
  if(!ret->component_type) /* mp4 case:
                               Read everything until the end of the atom */
    {
    name_len = h->start_position + h->size - input->position;
    //    fprintf(stderr, "MP4 Name len: %d\n", name_len);
    }
  else /* Quicktime case */
    {
    if(h->start_position + h->size == input->position)
      name_len = 0;
    else
      {
      if(!bgav_input_read_8(input, &tmp_8))
        return 0;
      name_len = tmp_8;
      }
    }
  //  fprintf(stderr, "Name len: %d\n", name_len);

  if(name_len)
    {
    ret->component_name = malloc(name_len + 1);
    if(bgav_input_read_data(input, (uint8_t*)(ret->component_name), name_len) <
       name_len)
      return 0;
    ret->component_name[name_len] = '\0';
    }
  //  fprintf(stderr, "Component name: %s\n", ret->component_name);

  //  fprintf(stderr, "Read hdlr: %lld %lld\n", h->start_position + h->size, input->position);
  
  bgav_qt_atom_skip(input, h);
  
  return 1;
  }

void bgav_qt_hdlr_free(qt_hdlr_t * c)
  {
  if(c->component_name)
    free(c->component_name);
  }
