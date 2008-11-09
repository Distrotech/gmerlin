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

static NPNetscapeFuncs  browser_funcs;

NPError bg_NPN_GetURL(NPP instance, 
                      const char* url,
                      const char* target)
  {
  return browser_funcs.geturl(instance, url, target);
  }

NPError bg_NPN_DestroyStream(NPP     instance, 
                          NPStream* stream, 
                          NPError   reason)
  {
  return browser_funcs.destroystream(instance, stream, reason);
  }

void *bg_NPN_MemAlloc (uint32 size)
  {
  return browser_funcs.memalloc(size);
  }

void bg_NPN_MemFree(void* ptr)
  {
  return browser_funcs.memfree(ptr);
  }

NPError bg_NPN_GetValue(NPP instance, NPNVariable variable, void * value)
  {
  return browser_funcs.getvalue(instance, variable, value);
  }

NPError bg_NPN_SetValue(NPP instance, NPPVariable variable, void * value)
  {
  return browser_funcs.setvalue(instance, variable, value);
  }

void bg_NPN_InvalidateRect(NPP instance, NPRect * invalidRect)
  {
  return browser_funcs.invalidaterect(instance, invalidRect);
  }

void bg_NPN_InvalidateRegion(NPP instance, NPRegion invalidRegion)
  {
  browser_funcs.invalidateregion(instance, invalidRegion);
  }

void bg_NPN_ForceRedraw(NPP instance)
  {
  browser_funcs.forceredraw(instance);
  }

void bg_NPN_ReleaseVariantValue(NPVariant * variant)
  {
  browser_funcs.releasevariantvalue(variant);
  }

NPIdentifier bg_NPN_GetStringIdentifier(const NPUTF8 * name)
  {
  return browser_funcs.getstringidentifier(name);
  }

void bg_NPN_GetStringIdentifiers(const NPUTF8 ** names,
                              int32_t nameCount,
                              NPIdentifier * identifiers)
  {
  browser_funcs.getstringidentifiers(names, nameCount, identifiers);
  }

NPIdentifier bg_NPN_GetIntIdentifier(int32_t intid)
  {
  return browser_funcs.getintidentifier(intid);
  }

bool bg_NPN_IdentifierIsString(NPIdentifier * identifier)
  {
  return browser_funcs.identifierisstring(identifier);
  }

NPUTF8 * bg_NPN_UTF8FromIdentifier(NPIdentifier identifier)
  {
  return browser_funcs.utf8fromidentifier(identifier);
  }

int32_t bg_NPN_IntFromIdentifier(NPIdentifier identifier)
  {
  return browser_funcs.intfromidentifier(identifier);
  }

NPObject * bg_NPN_CreateObject(NPP npp, NPClass * aClass)
  {
  return browser_funcs.createobject(npp, aClass);
  }

NPObject * bg_NPN_RetainObject(NPObject * npobj)
  {
  return browser_funcs.retainobject(npobj);
  }

void bg_NPN_ReleaseObject(NPObject * npobj)
  {
  browser_funcs.releaseobject(npobj);
  }

bool bg_NPN_Invoke(NPP npp, NPObject * npobj, NPIdentifier methodName,
                const NPVariant * args, uint32_t argCount, NPVariant * result)
  {
  return browser_funcs.invoke(npp, npobj, methodName, args, argCount, result);
  }

bool bg_NPN_InvokeDefault(NPP npp, NPObject * npobj, const NPVariant * args,
                       uint32_t argCount, NPVariant * result)
  {
  return browser_funcs.invokeDefault(npp, npobj, args, argCount, result);
  }

bool bg_NPN_Evaluate(NPP npp, NPObject * npobj, NPString * script,
                  NPVariant * result)
  {
  return browser_funcs.evaluate(npp, npobj, script, result);
  }

bool bg_NPN_GetProperty(NPP npp, NPObject * npobj, NPIdentifier propertyName,
                     NPVariant * result)
  {
  return browser_funcs.getproperty(npp, npobj, propertyName, result);
  }

bool bg_NPN_SetProperty(NPP npp, NPObject * npobj, NPIdentifier propertyName,
                     const NPVariant * result)
  {
  return browser_funcs.setproperty(npp, npobj, propertyName, result);
  }

bool bg_NPN_RemoveProperty(NPP npp, NPObject * npobj, NPIdentifier propertyName)
  {
  return browser_funcs.removeproperty(npp, npobj, propertyName);
  }

bool bg_NPN_HasProperty(NPP npp, NPObject * npobj, NPIdentifier propertyName)
  {
  return browser_funcs.hasproperty(npp, npobj, propertyName);
  }

bool bg_NPN_HasMethod(NPP npp, NPObject * npobj, NPIdentifier methodName)
  {
  return browser_funcs.hasmethod(npp, npobj, methodName);
  }

void bg_mozilla_init_browser_funcs(NPNetscapeFuncs * funcs)
  {
  browser_funcs.version              = funcs->version;
  browser_funcs.size                 = funcs->size;
  browser_funcs.posturl              = funcs->posturl;
  browser_funcs.geturl               = funcs->geturl;
  browser_funcs.geturlnotify         = funcs->geturlnotify;
  browser_funcs.requestread          = funcs->requestread;
  browser_funcs.newstream            = funcs->newstream;
  browser_funcs.write                = funcs->write;
  browser_funcs.destroystream        = funcs->destroystream;
  browser_funcs.status               = funcs->status;
  browser_funcs.uagent               = funcs->uagent;
  browser_funcs.memalloc             = funcs->memalloc;
  browser_funcs.memfree              = funcs->memfree;
  browser_funcs.memflush             = funcs->memflush;
  browser_funcs.reloadplugins        = funcs->reloadplugins;
  browser_funcs.getJavaEnv           = funcs->getJavaEnv;
  browser_funcs.getJavaPeer          = funcs->getJavaPeer;
  browser_funcs.getvalue             = funcs->getvalue;
  browser_funcs.getstringidentifier  = funcs->getstringidentifier;
  browser_funcs.getstringidentifiers = funcs->getstringidentifiers;
  browser_funcs.getintidentifier     = funcs->getintidentifier;
  browser_funcs.identifierisstring   = funcs->identifierisstring;
  browser_funcs.utf8fromidentifier   = funcs->utf8fromidentifier;
  browser_funcs.intfromidentifier    = funcs->intfromidentifier;
  browser_funcs.createobject         = funcs->createobject;
  browser_funcs.retainobject         = funcs->retainobject;
  browser_funcs.releaseobject        = funcs->releaseobject;
  browser_funcs.invoke               = funcs->invoke;
  browser_funcs.invokeDefault        = funcs->invokeDefault;
  browser_funcs.evaluate             = funcs->evaluate;
  browser_funcs.getproperty          = funcs->getproperty;
  browser_funcs.setproperty          = funcs->setproperty;
  browser_funcs.removeproperty       = funcs->removeproperty;
  browser_funcs.hasproperty          = funcs->hasproperty;
  browser_funcs.hasmethod            = funcs->hasmethod;
  browser_funcs.releasevariantvalue  = funcs->releasevariantvalue;
  browser_funcs.setexception         = funcs->setexception;


  return;
  }
