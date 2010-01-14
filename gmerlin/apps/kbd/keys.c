/*****************************************************************
 * gmerlin - a general purpose multimedia framework and applications
 *
 * Copyright (c) 2001 - 2010 Members of the Gmerlin project
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

#include <X11/Xlib.h>

#include <inttypes.h>
#include <string.h>

#include <config.h>
#include <gmerlin/utils.h>

#include <kbd.h>

/* X11 Modifiers */

static const struct
  {
  uint32_t flag;
  const char * name;
  }
modifiers[] =
  {
    { ShiftMask,   "Shift" }, //   = 1 << 0,
    { LockMask,    "Lock" }, //	    = 1 << 1,
    { ControlMask, "Control" }, //  = 1 << 2,
    { Mod1Mask,    "Mod1" }, //	    = 1 << 3,
    { Mod2Mask,    "Mod2" }, //	    = 1 << 4,
    { Mod3Mask,    "Mod3" }, //	    = 1 << 5,
    { Mod4Mask,    "Mod4" }, //	    = 1 << 6,
    { Mod5Mask,    "Mod5" }, //	    = 1 << 7,
    { Button1Mask, "Button1" }, //  = 1 << 8,
    { Button2Mask, "Button2" }, //  = 1 << 9,
    { Button3Mask, "Button3" }, //  = 1 << 10,
    { Button4Mask, "Button4" }, //  = 1 << 11,
    { Button5Mask, "Button5" }, //  = 1 << 12,
    { /* End of array */ }, //  = 1 << 12,
  };

uint32_t kbd_modifiers_from_string(const char * str)
  {
  char ** mods;
  int i, j;
  uint32_t ret = 0;
  
  if(!str)
    return ret;

  mods = bg_strbreak(str, '+');

  i = 0;
  while(mods[i])
    {
    j = 0;
    while(modifiers[j].name)
      {
      if(!strcmp(modifiers[j].name, mods[i]))
        {
        ret |= modifiers[j].flag;
        break;
        }
      j++;
      }
    i++;
    }
  bg_strbreak_free(mods);
  return ret;
  }

char * kbd_modifiers_to_string(uint32_t state)
  {
  int i;
  char * ret = (char*)0;
  
  for(i = 0; i < sizeof(modifiers)/sizeof(modifiers[0]); i++)
    {
    if(state & modifiers[i].flag)
      {
      if(ret)
        {
        ret = bg_strcat(ret, "+");
        ret = bg_strcat(ret, modifiers[i].name);
        }
      else
        ret = bg_strdup(ret, modifiers[i].name);
      }
    }
  return ret;
  }
