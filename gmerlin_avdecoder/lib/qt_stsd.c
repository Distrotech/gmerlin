/*****************************************************************
 
  qt_stsd.c
 
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

#include <qt.h>
#include <utils.h>

extern bgav_palette_entry_t bgav_qt_default_palette_2[];
extern bgav_palette_entry_t bgav_qt_default_palette_4[];
extern bgav_palette_entry_t bgav_qt_default_palette_16[];
extern bgav_palette_entry_t bgav_qt_default_palette_256[];

/*
 *  Sample description
 */
static void stsd_dump_audio(qt_sample_description_t * d)
  {
  fprintf(stderr, "  fourcc: ");
  bgav_dump_fourcc(d->fourcc);
  fprintf(stderr, "\n");
    
  fprintf(stderr, "  data_reference_index: %d\n", d->data_reference_index);
  fprintf(stderr, "  version:              %d\n", d->version);
  fprintf(stderr, "  revision_level:       %d\n", d->revision_level);
  fprintf(stderr, "  vendor:               ");
  bgav_dump_fourcc(d->vendor);
  
  fprintf(stderr, "\n  num_channels          %d\n", d->format.audio.num_channels);
  fprintf(stderr, "  bits_per_sample:      %d\n", d->format.audio.bits_per_sample);
  fprintf(stderr, "  compression_id:       %d\n", d->format.audio.compression_id);
  fprintf(stderr, "  packet_size:          %d\n", d->format.audio.packet_size);
  fprintf(stderr, "  samplerate:           %d\n", d->format.audio.samplerate);

  if(d->version > 0)
    {
    fprintf(stderr, "  samples_per_packet:   %d\n", d->format.audio.samples_per_packet);
    fprintf(stderr, "  bytes_per_packet:     %d\n", d->format.audio.bytes_per_packet);
    fprintf(stderr, "  bytes_per_frame:      %d\n", d->format.audio.bytes_per_frame);
    fprintf(stderr, "  bytes_per_sample:     %d\n", d->format.audio.bytes_per_sample);
    }
  }
static void stsd_dump_video(qt_sample_description_t * d)
  {
  fprintf(stderr, "  fourcc: ");
  bgav_dump_fourcc(d->fourcc);
  fprintf(stderr, "\n");
    
  fprintf(stderr, "  data_reference_index:  %d\n", d->data_reference_index);
  fprintf(stderr, "  version:               %d\n", d->version);
  fprintf(stderr, "  revision_level:        %d\n", d->revision_level);
  fprintf(stderr, "  vendor:                ");
  bgav_dump_fourcc(d->vendor);
  fprintf(stderr, "\n");

  fprintf(stderr, "  temporal_quality:      %d\n", d->format.video.temporal_quality);
  fprintf(stderr, "  spatial_quality:       %d\n", d->format.video.spatial_quality);
  fprintf(stderr, "  width:                 %d\n", d->format.video.width);
  fprintf(stderr, "  height:                %d\n", d->format.video.height);
  fprintf(stderr, "  horizontal_resolution: %f\n", d->format.video.horizontal_resolution);
  fprintf(stderr, "  vertical_resolution:   %f\n", d->format.video.vertical_resolution);
  fprintf(stderr, "  data_size:             %d\n", d->format.video.data_size);
  fprintf(stderr, "  frame_count:           %d\n", d->format.video.frame_count); /* Frames / sample */
  fprintf(stderr, "  compressor_name:       %s\n", d->format.video.compressor_name);
  fprintf(stderr, "  depth:                 %d\n", d->format.video.depth);
  fprintf(stderr, "  ctab_id:               %d\n", d->format.video.ctab_id);
  fprintf(stderr, "  ctab_size:             %d\n", d->format.video.ctab_size);
  }

static int stsd_read_common(bgav_input_context_t * input,
                            qt_sample_description_t * ret)
  {
  return (bgav_input_read_fourcc(input, &(ret->fourcc)) &&
          (bgav_input_read_data(input, ret->reserved, 6) == 6) &&
          bgav_input_read_16_be(input, &(ret->data_reference_index)) &&
          bgav_input_read_16_be(input, &(ret->version)) &&
          bgav_input_read_16_be(input, &(ret->revision_level)) &&
          bgav_input_read_32_be(input, &(ret->vendor)));
  }

static int stsd_read_audio(bgav_input_context_t * input,
                           qt_sample_description_t * ret)
  {
  int result;
  bgav_input_context_t * input_mem;
  qt_atom_header_t h;
  qt_atom_header_t h1;
  uint32_t tmp_32;
  if(!stsd_read_common(input, ret))
    return 0;
  ret->type = BGAV_STREAM_AUDIO;
  result = bgav_input_read_16_be(input, &(ret->format.audio.num_channels)) &&
    bgav_input_read_16_be(input, &(ret->format.audio.bits_per_sample)) &&
    bgav_input_read_16_be(input, &(ret->format.audio.compression_id)) &&
    bgav_input_read_16_be(input, &(ret->format.audio.packet_size)) &&
    bgav_input_read_32_be(input, &tmp_32);
  if(!result)
    return 0;
  ret->format.audio.samplerate = (tmp_32 >> 16);
  if(ret->version == 1)
    {
    result = bgav_input_read_32_be(input, &(ret->format.audio.samples_per_packet)) &&
      bgav_input_read_32_be(input, &(ret->format.audio.bytes_per_packet)) &&
      bgav_input_read_32_be(input, &(ret->format.audio.bytes_per_frame)) &&
      bgav_input_read_32_be(input, &(ret->format.audio.bytes_per_sample));
    }

  /* Read remaining atoms */

  while(1)
    {
    if(!bgav_qt_atom_read_header(input, &h))
      break;
    // fprintf(stderr, "Blupp\n");
    switch(h.fourcc)
      {
      case BGAV_MK_FOURCC('w', 'a', 'v', 'e'):
        ret->format.audio.has_wave_atom = 1;
        ret->format.audio.wave_atom.size = h.size - (input->position - h.start_position);
        ret->format.audio.wave_atom.data = malloc(ret->format.audio.wave_atom.size);
        if(bgav_input_read_data(input,
                                ret->format.audio.wave_atom.data,
                                ret->format.audio.wave_atom.size) < ret->format.audio.wave_atom.size)
          return 0;
        // fprintf(stderr, "Found wave atom, %d bytes\n", ret->format.audio.wave_atom.size);
        // bgav_hexdump(ret->format.audio.wave_atom.data,
        //             ret->format.audio.wave_atom.size, 16); 
        
        /* Sometimes, the ess atom is INSIDE the wav atom, so let's catch this */

        input_mem = bgav_input_open_memory(ret->format.audio.wave_atom.data,
                                           ret->format.audio.wave_atom.size);

        while(1)
          {
          if(!bgav_qt_atom_read_header(input_mem, &h1))
            break;
          if(h1.fourcc == BGAV_MK_FOURCC('e', 's', 'd', 's'))
            {
            //            fprintf(stderr, "Found esds atom inside of wave atom, %lld bytes\n",
            //                    h1.size);
            if(!bgav_qt_esds_read(&h1, input_mem, &(ret->esds)))
              return 0;
            ret->has_esds = 1;
            }
          else
            {
            // fprintf(stderr, "Skipping atom ");
            // bgav_dump_fourcc(h1.fourcc);
            // fprintf(stderr, " Start: %lld, bytes: %lld\n", h1.start_position, h1.size);
            bgav_qt_atom_skip(input_mem, &h1);
            }
          }
        bgav_input_destroy(input_mem);
        break;
      case BGAV_MK_FOURCC('e', 's', 'd', 's'):
        //        fprintf(stderr, "Found esds atom, %lld bytes\n", h.size);
        if(!bgav_qt_esds_read(&h, input, &(ret->esds)))
          return 0;
        ret->has_esds = 1;
        
        break;
      default:
        fprintf(stderr, "Unknown atom ");
        bgav_dump_fourcc(h.fourcc);
        fprintf(stderr, "\n");
        bgav_qt_atom_skip(input, &h);
        break;
      }
    }
  //  stsd_dump_audio(ret);
  return result;
  }

static int stsd_read_video(bgav_input_context_t * input,
                           qt_sample_description_t * ret)
  {
  int i;
  uint8_t len;
  qt_atom_header_t h;
  int bits_per_pixel;
  if(!stsd_read_common(input, ret))
    return 0;

  //  fprintf(stderr, "Video stream: ");
  //  bgav_dump_fourcc(ret->fourcc);
  //  fprintf(stderr, "\n");
  
  
  ret->type = BGAV_STREAM_VIDEO;
  if(!bgav_input_read_32_be(input, &(ret->format.video.temporal_quality)) ||
     !bgav_input_read_32_be(input, &(ret->format.video.spatial_quality)) ||
     !bgav_input_read_16_be(input, &(ret->format.video.width)) ||
     !bgav_input_read_16_be(input, &(ret->format.video.height)) ||
     !bgav_qt_read_fixed32(input, &(ret->format.video.horizontal_resolution)) ||
     !bgav_qt_read_fixed32(input, &(ret->format.video.vertical_resolution)) ||
     !bgav_input_read_32_be(input, &(ret->format.video.data_size)) ||
     !bgav_input_read_16_be(input, &(ret->format.video.frame_count)) ||
     !bgav_input_read_8(input, &len) ||
     (bgav_input_read_data(input, ret->format.video.compressor_name, 31) < 31) ||
     !bgav_input_read_16_be(input, &(ret->format.video.depth)) ||
     !bgav_input_read_16_be(input, &(ret->format.video.ctab_id)))
    return 0;
  //  fprintf(stderr, "compressor name len: %d\n", len);
  if(len < 31)
    ret->format.video.compressor_name[len] = '\0';
  
  /* Create colortable */

  bits_per_pixel = ret->format.video.depth & 0x1f;
    
  if((bits_per_pixel == 1) ||  (bits_per_pixel == 2) ||
     (bits_per_pixel == 4) || (bits_per_pixel == 8))
    {
    if(!ret->format.video.ctab_id)
      {
      //      fprintf(stderr, "Reading palette...");
      ret->format.video.private_ctab = 1;
      bgav_input_skip(input, 4); /* Seed */
      bgav_input_skip(input, 2); /* Flags */
      if(!bgav_input_read_16_be(input, &(ret->format.video.ctab_size)))
        return 0;
      ret->format.video.ctab_size++;
      ret->format.video.ctab =
        malloc(ret->format.video.ctab_size * sizeof(*(ret->format.video.ctab)));
      for(i = 0; i < ret->format.video.ctab_size; i++)
        {
        if(!bgav_input_read_16_be(input, &(ret->format.video.ctab[i].a)) ||
           !bgav_input_read_16_be(input, &(ret->format.video.ctab[i].r)) ||
           !bgav_input_read_16_be(input, &(ret->format.video.ctab[i].g)) ||
           !bgav_input_read_16_be(input, &(ret->format.video.ctab[i].b)))
          return 0;
        }
      //      fprintf(stderr, "Done, %d colors\n", ret->format.video.ctab_size);
      }
    else /* Set the default quicktime palette for this depth */
      {
      switch(ret->format.video.depth)
        {
        case 1:
          ret->format.video.ctab = bgav_qt_default_palette_2;
          ret->format.video.ctab_size = 2;
          break;
        case 2:
          ret->format.video.ctab = bgav_qt_default_palette_4;
          ret->format.video.ctab_size = 4;
          break;
        case 4:
          ret->format.video.ctab = bgav_qt_default_palette_16;
          ret->format.video.ctab_size = 16;
          break;
        case 8:
          ret->format.video.ctab = bgav_qt_default_palette_256;
          ret->format.video.ctab_size = 256;
          break;
        }
      }
    }
  while(1)
    {
    //    fprintf(stderr, "Reading Atom...");
    if(!bgav_qt_atom_read_header(input, &h))
      {
      //      fprintf(stderr, "failed\n");
      break;
      }
    //    fprintf(stderr, "done\n");
    switch(h.fourcc)
      {
      case BGAV_MK_FOURCC('e', 's', 'd', 's'):
        if(!bgav_qt_esds_read(&h, input, &(ret->esds)))
          return 0;
        ret->has_esds = 1;
        break;
      default:
        //        fprintf(stderr, "Unknown atom ");
        //        bgav_dump_fourcc(h.fourcc);
        //        fprintf(stderr, "\n");
        bgav_qt_atom_skip(input, &h);
        break;
      }
    }
  //  stsd_dump_video(ret);
  return 1;
  }
  
int bgav_qt_stsd_read(qt_atom_header_t * h, bgav_input_context_t * input,
                      qt_stsd_t * ret)
  {
  uint32_t i;
  READ_VERSION_AND_FLAGS;
  memcpy(&(ret->h), h, sizeof(*h));
  
  if(!bgav_input_read_32_be(input, &(ret->num_entries)))
    return 0;

  ret->entries = calloc(ret->num_entries, sizeof(*(ret->entries)));
  
  for(i = 0; i < ret->num_entries; i++)
    {
    if(!bgav_input_read_32_be(input, &(ret->entries[i].data_size)))
      return 0;
    ret->entries[i].data_size -= 4;
    ret->entries[i].data = malloc(ret->entries[i].data_size);
    if(bgav_input_read_data(input,
                            ret->entries[i].data,
                            ret->entries[i].data_size) <
       ret->entries[i].data_size)
      return 0;
    
    //    fprintf(stderr, "stsd:\n");
    //    bgav_hexdump(ret->entries[i].data, ret->entries[i].data_size, 16);
    
    }
  return 1;
  }

int bgav_qt_stsd_finalize(qt_stsd_t * c, qt_trak_t * trak)
  {
  int i;
  int result;
  bgav_input_context_t * input_mem;
  for(i = 0; i < c->num_entries; i++)
    {
    if(trak->mdia.minf.has_vmhd) /* Video sample description */
      {
      //      fprintf(stderr, "Reading video stsd\n");
      //      bgav_hexdump(c->entries[i].data, c->entries[i].data_size, 16);
      
      input_mem = bgav_input_open_memory(c->entries[i].data,
                                         c->entries[i].data_size);
      
      result = stsd_read_video(input_mem, &(c->entries[i].desc));

      if(!c->entries[i].desc.format.video.width)
        {
        c->entries[i].desc.format.video.width = (int)(trak->tkhd.track_width);
        }
      if(!c->entries[i].desc.format.video.height)
        {
        c->entries[i].desc.format.video.height = (int)(trak->tkhd.track_height);
        }

      bgav_input_destroy(input_mem);
      if(!result)
        return 0;
      }
    else if(trak->mdia.minf.has_smhd) /* Audio sample description */
      {
      //      fprintf(stderr, "Reading audio stsd\n");
      //      bgav_hexdump(c->entries[i].data, c->entries[i].data_size, 16);
      
      input_mem = bgav_input_open_memory(c->entries[i].data,
                                         c->entries[i].data_size);
      
      result = stsd_read_audio(input_mem, &(c->entries[i].desc));
      bgav_input_destroy(input_mem);
      if(!result)
        return 0;
      }
    //    else
    //      {
    //      fprintf(stderr, "Skipping stsd\n");
    //      }
    }
  return 1;
  }

void bgav_qt_stsd_free(qt_stsd_t * c)
  {
  int i;
  for(i = 0; i < c->num_entries; i++)
    {
    if(c->entries[i].data)
      free(c->entries[i].data);
    if(c->entries[i].desc.type == BGAV_STREAM_AUDIO)
      {
      if(c->entries[i].desc.format.audio.has_wave_atom)
        free(c->entries[i].desc.format.audio.wave_atom.data);
      
      }
    if(c->entries[i].desc.type == BGAV_STREAM_VIDEO)
      {
      if(c->entries[i].desc.format.video.private_ctab)
        free(c->entries[i].desc.format.video.ctab);
      }
    if(c->entries[i].desc.has_esds)
      bgav_qt_esds_free(&(c->entries[i].desc.esds));
    }
  free(c->entries);
  }

void bgav_qt_stsd_dump(qt_stsd_t * s)
  {
  int i;

  fprintf(stderr, "stsd\n");

  fprintf(stderr, "  num_entries: %d\n", s->num_entries);
  
  for(i = 0; i < s->num_entries; i++)
    {
    fprintf(stderr, "  Sample description: %d\n", i);
    if(s->entries[i].desc.type == BGAV_STREAM_AUDIO)
      stsd_dump_audio(&s->entries[i].desc);
    else if(s->entries[i].desc.type == BGAV_STREAM_VIDEO)
      stsd_dump_video(&s->entries[i].desc);
    }
  }
