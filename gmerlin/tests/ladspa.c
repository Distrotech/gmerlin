/*****************************************************************
 * gmerlin - a general purpose multimedia framework and applications
 *
 * Copyright (c) 2001 - 2011 Members of the Gmerlin project
 * gmerlin-general@lists.sourceforge.net
 * http://gmerlin.sourceforge.net
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 * *****************************************************************/

#include "ladspa.h"
#include <dlfcn.h>
#include <stdio.h>

static void dump_filter(const LADSPA_Descriptor * desc)
  {
  int i;
  fprintf(stderr, "Ladspa filter\n");
  fprintf(stderr, "  UniqueID:   %ld\n", desc->UniqueID);
  fprintf(stderr, "  Label:      %s\n", desc->Label);
  fprintf(stderr, "  Name:       %s\n", desc->Name);
  fprintf(stderr, "  Properties: ");
  if(desc->Properties & LADSPA_PROPERTY_REALTIME)
    fprintf(stderr, "Realtime ");
  if(desc->Properties & LADSPA_PROPERTY_INPLACE_BROKEN)
    fprintf(stderr, "InplaceBroken ");
  if(desc->Properties & LADSPA_PROPERTY_HARD_RT_CAPABLE)
    fprintf(stderr, "HardRTCapable ");
  fprintf(stderr, "\n");
  fprintf(stderr, "  Maker:      %s\n", desc->Maker);
  fprintf(stderr, "  Copyright:  %s\n", desc->Copyright);
  fprintf(stderr, "  PortCount:  %ld\n", desc->PortCount);

  for(i = 0; i < desc->PortCount; i++)
    {
    fprintf(stderr, "    Port %d: \"%s\"\n", i+1, desc->PortNames[i]);
    fprintf(stderr, "      Type: ");

    if(LADSPA_IS_PORT_INPUT(desc->PortDescriptors[i]))
      fprintf(stderr, "Input ");
    if(LADSPA_IS_PORT_OUTPUT(desc->PortDescriptors[i]))
      fprintf(stderr, "Output ");
    if(LADSPA_IS_PORT_CONTROL(desc->PortDescriptors[i]))
      fprintf(stderr, "Control ");
    if(LADSPA_IS_PORT_AUDIO(desc->PortDescriptors[i]))
      fprintf(stderr, "Audio ");
    fprintf(stderr, "\n");

    fprintf(stderr, "      Hints: ");
    if(desc->PortRangeHints[i].HintDescriptor & LADSPA_HINT_BOUNDED_BELOW)
      fprintf(stderr, "BoundedBelow ");
    if(desc->PortRangeHints[i].HintDescriptor & LADSPA_HINT_BOUNDED_ABOVE)
      fprintf(stderr, "BoundedAbove ");
    if(desc->PortRangeHints[i].HintDescriptor & LADSPA_HINT_TOGGLED)
      fprintf(stderr, "Toggled ");
    if(desc->PortRangeHints[i].HintDescriptor & LADSPA_HINT_SAMPLE_RATE)
      fprintf(stderr, "Samplerate ");
    if(desc->PortRangeHints[i].HintDescriptor & LADSPA_HINT_LOGARITHMIC)
      fprintf(stderr, "Logarithmic ");
    if(desc->PortRangeHints[i].HintDescriptor & LADSPA_HINT_INTEGER)
      fprintf(stderr, "Integer ");
    if((desc->PortRangeHints[i].HintDescriptor & LADSPA_HINT_DEFAULT_MASK) == LADSPA_HINT_DEFAULT_MINIMUM)
      fprintf(stderr, "DefaultMinimum ");
    if((desc->PortRangeHints[i].HintDescriptor & LADSPA_HINT_DEFAULT_MASK) == LADSPA_HINT_DEFAULT_MAXIMUM)
      fprintf(stderr, "DefaultMaximum ");
    if((desc->PortRangeHints[i].HintDescriptor & LADSPA_HINT_DEFAULT_MASK) == LADSPA_HINT_DEFAULT_LOW)
      fprintf(stderr, "DefaultLow ");
    if((desc->PortRangeHints[i].HintDescriptor & LADSPA_HINT_DEFAULT_MASK) == LADSPA_HINT_DEFAULT_MIDDLE)
      fprintf(stderr, "DefaultMiddle ");
    if((desc->PortRangeHints[i].HintDescriptor & LADSPA_HINT_DEFAULT_MASK) == LADSPA_HINT_DEFAULT_HIGH)
      fprintf(stderr, "DefaultHigh ");
    if((desc->PortRangeHints[i].HintDescriptor & LADSPA_HINT_DEFAULT_MASK) == LADSPA_HINT_DEFAULT_0)
      fprintf(stderr, "Default0 ");
    if((desc->PortRangeHints[i].HintDescriptor & LADSPA_HINT_DEFAULT_MASK) == LADSPA_HINT_DEFAULT_1)
      fprintf(stderr, "Default1 ");
    if((desc->PortRangeHints[i].HintDescriptor & LADSPA_HINT_DEFAULT_MASK) == LADSPA_HINT_DEFAULT_440)
      fprintf(stderr, "Default440 ");
    fprintf(stderr, "\n");    

    if(desc->PortRangeHints[i].HintDescriptor & LADSPA_HINT_BOUNDED_BELOW)
      fprintf(stderr, "      Minimum: %f\n", desc->PortRangeHints[i].LowerBound);
    if(desc->PortRangeHints[i].HintDescriptor & LADSPA_HINT_BOUNDED_ABOVE)
      fprintf(stderr, "      Maximum: %f\n", desc->PortRangeHints[i].UpperBound);
    
    }
  
  }

int main(int argc, char ** argv)
  {
  void * dll_handle;
  const LADSPA_Descriptor * desc;
  LADSPA_Descriptor_Function desc_func;
  int index;
  
  if(argc != 2)
    {
    fprintf(stderr, "Usage: %s <plugin_file>\n", argv[0]);
    return -1;
    }
  dll_handle = dlopen(argv[1], RTLD_NOW);
  if(!dll_handle)
    {
    fprintf(stderr, "Cannot open %s: %s\n", argv[0], dlerror());
    return -1;
    }
  desc_func = dlsym(dll_handle, "ladspa_descriptor");
  if(!desc_func)
    {
    fprintf(stderr, "No symbol \"ladspa_descriptor\" found: %s\n",
            dlerror());
    return -1;
    }
  index = 0;
  while((desc = desc_func(index)))
    {
    dump_filter(desc);
    index++;
    }
  return 0;
  }
