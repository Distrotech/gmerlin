/*****************************************************************
 * gmerlin-mozilla - A gmerlin based mozilla plugin
 *
 * Copyright (c) 2001 - 2008 Members of the Gmerlin project
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

#include <gmerlin_mozilla.h>
#include <string.h>

#include <gmerlin/log.h>
#define LOG_DOMAIN "scriptable"

#define SCRIPT_PLAY           0
#define SCRIPT_STOP           1
#define SCRIPT_PAUSE          2
#define SCRIPT_FULLSCREEN     3
#define SCRIPT_SETDISPLAYSIZE 4

static struct
  {
  const char * name;
  int func;
  NPIdentifier id;
  }
funcs[] =
  {
    { "play",       SCRIPT_PLAY },
    { "stop",       SCRIPT_STOP },
    { "pause",      SCRIPT_PAUSE },
    { "fullscreen", SCRIPT_FULLSCREEN },
    { "SetFullScreen", SCRIPT_FULLSCREEN }, // Real player
    /* WMP */
    { "Play",       SCRIPT_PLAY }, // WMP
    { "SetDisplaySize", SCRIPT_SETDISPLAYSIZE }, // WMP
    { "Stop",       SCRIPT_STOP }, // WMP
    { "Pause",      SCRIPT_PAUSE }, // WMP
    { /* End */ },
  };

void gmerlin_mozilla_init_scriptable()
  {
  int i = 0;
  while(funcs[i].name)
    {
    funcs[i].id = bg_NPN_GetStringIdentifier(funcs[i].name);
    i++;
    }
  }

static int find_func(NPIdentifier id)
  {
  int i = 0;
  while(funcs[i].name)
    {
    if(funcs[i].id == id)
      return funcs[i].func;
    i++;
    }
  return -1;
  }

typedef struct
  {
  NPObject npobj;
  NPP npp;
  } Scriptable;

static NPObject * scriptable_allocate(NPP npp, NPClass * class)
  {
  Scriptable * npobj = bg_NPN_MemAlloc(sizeof(*npobj));
  npobj->npp = npp;
  return (NPObject*)(npobj);
  }

static void scriptable_deallocate(NPObject * npobj)
  {
  bg_NPN_MemFree(npobj);
  }

static void scriptable_invalidate(NPObject * npobj)
  {
  }

static bool scriptable_hasMethod(NPObject * npobj, const NPIdentifier name)
  {
  int func;
  func = find_func(name);
  if(func < 0)
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Has method %s",
           bg_NPN_UTF8FromIdentifier(name));
    return false;
    }
  return true;
  }

static bool scriptable_invoke(NPObject * npobj,
                              const NPIdentifier name,
                              const NPVariant * const args,
                              const uint32_t argCount,
                              NPVariant * const result)
  {
  int func;
  Scriptable * s;
  bg_mozilla_t * m;

  s = (Scriptable *)npobj;
  m = s->npp->pdata;
  
  func = find_func(name);
  if(func < 0)
    return false;
  
  switch(func)
    {
    case SCRIPT_PLAY:
      bg_mozilla_play(m);
      break;
    case SCRIPT_STOP:
      bg_mozilla_stop(m);
      break;
    case SCRIPT_PAUSE:
      bg_mozilla_pause(m);
      break;
    case SCRIPT_FULLSCREEN:
      bg_mozilla_widget_toggle_fullscreen(m->widget);
      break;
    case SCRIPT_SETDISPLAYSIZE:
      if((argCount == 1) && (args[0].type == NPVariantType_Int32) &&
         (args[0].value.intValue == 3))
        {
        bg_mozilla_widget_toggle_fullscreen(m->widget);
        }
      break;
    }
  
  return false;
  }

static bool scriptable_invokeDefault(NPObject * npobj,
                                     const NPVariant * args,
                                     uint32_t argCount,
                                     NPVariant * result)
  {
  return false;
  }

static bool scriptable_hasProperty(NPObject * npobj,
                            NPIdentifier name)
  {
  Scriptable * s;
  bg_mozilla_t * m;

  s = (Scriptable *)npobj;
  m = s->npp->pdata;
  
  if(!strcmp(bg_NPN_UTF8FromIdentifier(name), "id"))
    return m->ei.id ? true : false;

  bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Has property %s",
         bg_NPN_UTF8FromIdentifier(name));
  return false;
  }

static void set_variant_string(NPVariant * result,
                               const char * str)
  {
  char * string;
  result->value.stringValue.utf8length = strlen(str);
  string = bg_NPN_MemAlloc(result->value.stringValue.utf8length+1);
  strcpy(string, str);
  result->value.stringValue.utf8characters = string;
  result->type = NPVariantType_String;
  }

static bool scriptable_getProperty(NPObject * npobj,
                                   NPIdentifier name,
                                   NPVariant * result)
  {
  Scriptable * s;
  bg_mozilla_t * m;

  s = (Scriptable *)npobj;
  m = s->npp->pdata;

  if(!strcmp(bg_NPN_UTF8FromIdentifier(name), "id"))
    {
    set_variant_string(result, m->ei.id);
    return true;
    }
  return false;
  }

static bool scriptable_setProperty(NPObject * npobj,
                                   NPIdentifier name,
                                   const NPVariant * value)
  {
  return false;
  }

static bool scriptable_removeProperty(NPObject * npobj,
                               NPIdentifier name)
  {
  return false;
  }

NPClass npclass =
  {
    NP_CLASS_STRUCT_VERSION,
    scriptable_allocate,
    scriptable_deallocate,
    scriptable_invalidate,
    scriptable_hasMethod,
    scriptable_invoke,
    scriptable_invokeDefault,
    scriptable_hasProperty,
    scriptable_getProperty,
    scriptable_setProperty,
    scriptable_removeProperty
  };


void gmerlin_mozilla_create_scriptable(bg_mozilla_t * g)
  {
  g->scriptable = bg_NPN_CreateObject(g->instance, &npclass);
  }
