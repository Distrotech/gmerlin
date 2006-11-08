/*****************************************************************
 
  qt_stsd.c
 
  Copyright (c) 2003-2006 by Burkhard Plaum - plaum@ipf.uni-stuttgart.de
 
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
#include <utils.h>

extern bgav_palette_entry_t bgav_qt_default_palette_2[];
extern bgav_palette_entry_t bgav_qt_default_palette_4[];
extern bgav_palette_entry_t bgav_qt_default_palette_16[];
extern bgav_palette_entry_t bgav_qt_default_palette_256[];

extern bgav_palette_entry_t bgav_qt_default_palette_4_gray[];
extern bgav_palette_entry_t bgav_qt_default_palette_16_gray[];
extern bgav_palette_entry_t bgav_qt_default_palette_256_gray[];

/*
 *  Sample description
 */

static void stsd_dump_common(int indent, qt_sample_description_t * d)
  {
  bgav_diprintf(indent, "fourcc:                ");
  bgav_dump_fourcc(d->fourcc);
  bgav_dprintf( "\n");
  
  bgav_diprintf(indent, "data_reference_index:  %d\n", d->data_reference_index);
  bgav_diprintf(indent, "version:               %d\n", d->version);
  bgav_diprintf(indent, "revision_level:        %d\n", d->revision_level);
  bgav_diprintf(indent, "vendor:                ");
  bgav_dump_fourcc(d->vendor);
  bgav_dprintf( "\n");
  }


static void stsd_dump_audio(int indent, qt_sample_description_t * d)
  {
  
  bgav_diprintf(indent, "  num_channels          %d\n", d->format.audio.num_channels);
  bgav_diprintf(indent, "  bits_per_sample:      %d\n", d->format.audio.bits_per_sample);
  bgav_diprintf(indent, "  compression_id:       %d\n", d->format.audio.compression_id);
  bgav_diprintf(indent, "  packet_size:          %d\n", d->format.audio.packet_size);
  bgav_diprintf(indent, "  samplerate:           %d\n", d->format.audio.samplerate);

  if(d->version == 1)
    {
    bgav_diprintf(indent, "samples_per_packet:   %d\n",
            d->format.audio.samples_per_packet);
    bgav_diprintf(indent, "bytes_per_packet:     %d\n",
            d->format.audio.bytes_per_packet);
    bgav_diprintf(indent, "bytes_per_frame:      %d\n",
            d->format.audio.bytes_per_frame);
    bgav_diprintf(indent, "bytes_per_sample:     %d\n",
            d->format.audio.bytes_per_sample);
    }
  if(d->version == 2)
    {
    
    }
  if(d->format.audio.has_wave)
    bgav_qt_wave_dump(indent+2, &d->format.audio.wave);
  if(d->format.audio.has_chan)
    bgav_qt_chan_dump(indent+2, &d->format.audio.chan);
  }

static void stsd_dump_video(int indent, qt_sample_description_t * d)
  {

  bgav_dprintf( "  temporal_quality:      %d\n", d->format.video.temporal_quality);
  bgav_dprintf( "  spatial_quality:       %d\n", d->format.video.spatial_quality);
  bgav_dprintf( "  width:                 %d\n", d->format.video.width);
  bgav_dprintf( "  height:                %d\n", d->format.video.height);
  bgav_dprintf( "  horizontal_resolution: %f\n", d->format.video.horizontal_resolution);
  bgav_dprintf( "  vertical_resolution:   %f\n", d->format.video.vertical_resolution);
  bgav_dprintf( "  data_size:             %d\n", d->format.video.data_size);
  bgav_dprintf( "  frame_count:           %d\n", d->format.video.frame_count); /* Frames / sample */
  bgav_dprintf( "  compressor_name:       %s\n", d->format.video.compressor_name);
  bgav_dprintf( "  depth:                 %d\n", d->format.video.depth);
  bgav_dprintf( "  ctab_id:               %d\n", d->format.video.ctab_id);
  bgav_dprintf( "  ctab_size:             %d\n", d->format.video.ctab_size);
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
  qt_atom_header_t h;
  uint32_t tmp_32;
  double tmp_d;
  uint16_t tmp_16;
  
  if(!stsd_read_common(input, ret))
    return 0;
  ret->type = BGAV_STREAM_AUDIO;

  if(ret->version == 2)
    {
    /*
     * SInt16     always3;
     * SInt16     always16;
     * SInt16     alwaysMinus2;
     * SInt16     always0;
     * UInt32     always65536;
     * UInt32     sizeOfStructOnly;
     * Float64    audioSampleRate;
     */
    bgav_input_skip(input, 16);

    if(!bgav_input_read_double_64_be(input, &tmp_d))
      return 0;
    ret->format.audio.samplerate = (int)(tmp_d + 0.5);

    if(!bgav_input_read_32_be(input, &(ret->format.audio.num_channels)))
      return 0;

    /*
     *  SInt32     always7F000000;
     */
    bgav_input_skip(input, 4);

    /* 
     *  UInt32     constBitsPerChannel;
     */
    if(!bgav_input_read_32_be(input, &(ret->format.audio.bits_per_sample)))
      return 0;

    if(!bgav_input_read_32_be(input, &(ret->format.audio.formatSpecificFlags)))
      return 0;

    /*
     * UInt32     constBytesPerAudioPacket;
     * UInt32     constLPCMFramesPerAudioPacket;
     */
    bgav_input_skip(input, 8);
    }
  else
    {
    if(!bgav_input_read_16_be(input, &(tmp_16)))
      return 0;
    ret->format.audio.num_channels = tmp_16;

    if(!bgav_input_read_16_be(input, &tmp_16))
      return 0;

    ret->format.audio.bits_per_sample = tmp_16;
    
    if(!bgav_input_read_16_be(input, &(ret->format.audio.compression_id)) ||
       !bgav_input_read_16_be(input, &(ret->format.audio.packet_size)) ||
       !bgav_input_read_32_be(input, &tmp_32))
      return 0;
    
    ret->format.audio.samplerate = (tmp_32 >> 16);
    
    if(ret->version > 0)
      {
      if(!bgav_input_read_32_be(input, &(ret->format.audio.samples_per_packet)) ||
         !bgav_input_read_32_be(input, &(ret->format.audio.bytes_per_packet)) ||
         !bgav_input_read_32_be(input, &(ret->format.audio.bytes_per_frame)) ||
         !bgav_input_read_32_be(input, &(ret->format.audio.bytes_per_sample)))
        return 0;
      }
    }
  
  /* Read remaining atoms */

  while(1)
    {
    if(!bgav_qt_atom_read_header(input, &h))
      break;
    switch(h.fourcc)
      {
      case BGAV_MK_FOURCC('w', 'a', 'v', 'e'):
        if(!bgav_qt_wave_read(&h, input, &(ret->format.audio.wave)))
          {
          return 0;
          }
        ret->format.audio.has_wave = 1;
        break;
      case BGAV_MK_FOURCC('e', 's', 'd', 's'):
        if(!bgav_qt_esds_read(&h, input, &(ret->esds)))
          return 0;
        ret->has_esds = 1;
        
        break;
      case BGAV_MK_FOURCC('c', 'h', 'a', 'n'):
        if(!bgav_qt_chan_read(&h, input, &(ret->format.audio.chan)))
          return 0;
        ret->format.audio.has_chan = 1;
        break;
      case 0:
        break;
      default:
        bgav_qt_atom_skip_unknown(input, &h, BGAV_MK_FOURCC('s','t','s','d'));
        break;
      }
    }
  //  stsd_dump_audio(ret);
  return 1;
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
     (bgav_input_read_data(input,
                           (uint8_t*)(ret->format.video.compressor_name), 31) < 31) ||
     !bgav_input_read_16_be(input, &(ret->format.video.depth)) ||
     !bgav_input_read_16_be(input, &(ret->format.video.ctab_id)))
    return 0;
  if(len < 31)
    ret->format.video.compressor_name[len] = '\0';
  
  /* Create colortable */

  bits_per_pixel = ret->format.video.depth & 0x1f;
    
  if((bits_per_pixel == 1) ||  (bits_per_pixel == 2) ||
     (bits_per_pixel == 4) || (bits_per_pixel == 8))
    {
    if(!ret->format.video.ctab_id)
      {
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
        case 34:
          ret->format.video.ctab = bgav_qt_default_palette_4_gray;
          ret->format.video.ctab_size = 4;
          break;
        case 36:
          ret->format.video.ctab = bgav_qt_default_palette_16_gray;
          ret->format.video.ctab_size = 16;
          break;
        case 40:
          ret->format.video.ctab = bgav_qt_default_palette_256_gray;
          ret->format.video.ctab_size = 256;
          break;
        }
      }
    }
  while(1)
    {
    if(!bgav_qt_atom_read_header(input, &h))
      {
      break;
      }
    switch(h.fourcc)
      {
      case BGAV_MK_FOURCC('e', 's', 'd', 's'):
        if(!bgav_qt_esds_read(&h, input, &(ret->esds)))
          return 0;
        ret->has_esds = 1;
        break;
      case BGAV_MK_FOURCC('a', 'v', 'c', 'C'):
        ret->avcC_offset = input->position;
        ret->avcC_size   = h.size - 8;
        bgav_qt_atom_skip(input, &h);
        break;
      case BGAV_MK_FOURCC('S', 'M', 'I', ' '):
        ret->has_SMI = 1;
        bgav_qt_atom_skip(input, &h);
        break;
      case BGAV_MK_FOURCC('p', 'a', 's', 'p'):
        if(!bgav_qt_pasp_read(&h, input, &(ret->format.video.pasp)))
          return 0;
        else
          ret->format.video.has_pasp = 1;
        break;
      case BGAV_MK_FOURCC('f', 'i', 'e', 'l'):
        if(!bgav_qt_fiel_read(&h, input, &(ret->format.video.fiel)))
          return 0;
        else
          ret->format.video.has_fiel = 1;
        //        bgav_qt_fiel_dump(&(ret->format.video.fiel));
        break;
      default:
        bgav_qt_atom_skip_unknown(input, &h, BGAV_MK_FOURCC('s','t','s','d'));
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
    
    
    }
  return 1;
  }

int bgav_qt_stsd_finalize(qt_stsd_t * c, qt_trak_t * trak,
                          const bgav_options_t * opt)
  {
  int i;
  int result;
  bgav_input_context_t * input_mem;
  for(i = 0; i < c->num_entries; i++)
    {
    if(trak->mdia.minf.has_vmhd) /* Video sample description */
      {
      
      input_mem = bgav_input_open_memory(c->entries[i].data,
                                         c->entries[i].data_size,
                                         opt);
      
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
      input_mem = bgav_input_open_memory(c->entries[i].data,
                                         c->entries[i].data_size, opt);
      
      result = stsd_read_audio(input_mem, &(c->entries[i].desc));
      bgav_input_destroy(input_mem);
      if(!result)
        return 0;
      }
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
      if(c->entries[i].desc.format.audio.has_wave)
        bgav_qt_wave_free(&(c->entries[i].desc.format.audio.wave));
      if(c->entries[i].desc.format.audio.has_chan)
        bgav_qt_chan_free(&(c->entries[i].desc.format.audio.chan));
      
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

void bgav_qt_stsd_dump(int indent, qt_stsd_t * s)
  {
  int i;

  bgav_diprintf(indent, "stsd\n");

  bgav_diprintf(indent+2, "num_entries: %d\n", s->num_entries);
  
  for(i = 0; i < s->num_entries; i++)
    {
    bgav_diprintf(indent+2, "Sample description: %d\n", i);

    stsd_dump_common(indent+2, &s->entries[i].desc);

    if(s->entries[i].desc.type == BGAV_STREAM_AUDIO)
      stsd_dump_audio(indent, &s->entries[i].desc);
    else if(s->entries[i].desc.type == BGAV_STREAM_VIDEO)
      stsd_dump_video(indent, &s->entries[i].desc);

    if(s->entries[i].desc.has_esds)
      bgav_qt_esds_dump(indent+2, &s->entries[i].desc.esds);
    }
  }
