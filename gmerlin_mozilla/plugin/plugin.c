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

#include <string.h>

#include <gmerlin/utils.h>
#include <gmerlin/log.h>
#define LOG_DOMAIN "plugin"

#include <gmerlin_mozilla.h>


static const char * general_mime_description =
"application/x-ogg:ogg:Ogg-Multimedia;" \
"application/ogg:ogg:Ogg-Multimedia;" \
"audio/ogg:ogg,oga:Ogg Audio;" \
"audio/x-ogg:ogg:Ogg Audio;" \
"video/ogg:ogv,ogg:Ogg Video;" \
"video/x-ogg:ogg:Ogg Video;" \
/* "application/annodex:anx:-" */ \
/* "audio/annodex:axa:-" */ \
/* "video/annodex:axv:-" */ \
"video/mpeg:mpg,mpeg,mpe:MPEG-Video;" \
"audio/wav:wav:WAV-Audio;" \
"audio/x-wav:wav:WAV-Audio;" \
"audio/mpeg:mp3:MP3-Audio;" \
"application/x-nsv-vp3-mp3:nsv:NullSoft video;" \
"video/flv:flv:Flash-Video;" \
"video/mp4:mp4,m4v:MPEG-4 Video;" \
"video/divx:divx:DivX Video;" \
;

static const char * vlc_mime_description =
"application/x-vlc-plugin:vlc:Videolan;";

static const char * quicktime_mime_description =
"video/quicktime:mov:Quicktime Video;" \
"application/x-quicktime-media-link:qtl:Quicktime Link;";

#if 1
static const char * wmp_mime_description =
"application/x-mplayer2:avi,wma,wmv:AVI-Video;" \
"video/x-ms-asf-plugin:asf,wmv:ASF-Video;" \
"video/x-msvideo:avi:AVI-Video;" \
"video/x-ms-asf:asf:ASF-Video;" \
"video/x-ms-wmv:wmv:Windows Media video;" \
"video/x-wmv:wmv:Windows Media video;" \
"video/x-ms-wvx:wmv:Microsoft ASX playlist;" \
"video/x-ms-wm:wmv:ASF-Video;" \
"application/x-ms-wms:wms:Windows Media video;" \
"application/asx:asx:Microsoft ASX playlist;" \
"audio/x-ms-wma:wma:Windows Media audio;";
#else
static const char * wmp_mime_description =
"application/asx:*:Media Files;" \
"video/x-ms-asf-plugin:*:Media Files;" \
"video/x-msvideo:avi,*:AVI;" \
"video/msvideo:avi,*:AVI;" \
"application/x-mplayer2:*:Media Files;" \
"application/x-ms-wmv:wmv,*:Microsoft WMV video;" \
"video/x-ms-asf:asf,asx,*:Media Files;" \
"video/x-ms-wm:wm,*:Media Files;" \
"video/x-ms-wmv:wmv,*:Microsoft WMV video;" \
"audio/x-ms-wmv:wmv,*:Windows Media;" \
"video/x-ms-wmp:wmp,*:Windows Media;" \
"application/x-ms-wmp:wmp,*:Windows Media;" \
"video/x-ms-wvx:wvx,*:Windows Media;" \
"audio/x-ms-wax:wax,*:Windows Media;" \
"audio/x-ms-wma:wma,*:Windows Media;" \
"application/x-drm-v2:asx,*:Windows Media;" \
"audio/wav:wav,*:Microsoft wave file;" \
"audio/x-wav:wav,*:Microsoft wave file;";
#endif

static const char * real_mime_description =
  "audio/x-pn-realaudio:ram,rm:RealAudio;" \
  "application/vnd.rn-realmedia:rm:RealMedia;" \
  "application/vnd.rn-realaudio:ra,ram:RealAudio;" \
  "video/vnd.rn-realvideo:rv:RealVideo;" \
  "audio/x-realaudio:ra:RealAudio;" \
  "audio/x-pn-realaudio-plugin:rpm:RealAudio;" \
"application/smil:smil:SMIL;";


static int check_mime_type(const char * types, const char * type)
  {
  int type_len = strlen(type);
  const char * pos;
  pos = types;

  while(1)
    {
    if(!strncmp(pos, type, type_len) &&
       (pos[type_len] == ':'))
      return 1;

    pos = strchr(pos, ';');
    if(!pos)
      break;
    pos++;
    if(*pos == '\0')
      break;
    }
  return 0;
  }

/* Browser entry types */

#ifdef GMERLIN
#define PLUGIN_NAME "Gmerlin web plugin"
#define PLUGIN_VERSION "<a href=\"http://gmerlin.sourceforge.net/\">Gmerlin</a> browser plugin "VERSION
#endif

#ifdef DIVX
#define PLUGIN_NAME "DivX\xc2\xae Web Player"
#define PLUGIN_VERSION "DivX Web Player version 1.4.0.233"
#endif

#include <gmerlin/log.h>


static char * mime_info = (char *)0;

char* NP_GetMIMEDescription(void)
  {
  if(!mime_info)
    mime_info = bg_sprintf("%s%s%s%s%s",
                           wmp_mime_description,
                           real_mime_description,
                           general_mime_description,
                           quicktime_mime_description,
                           vlc_mime_description);
  return(mime_info);
  }

NPError NP_GetValue(void *instance, 
                    NPPVariable variable, 
                    void *aValue)
  {
  switch(variable)
    {
    case NPPVpluginNameString:
      *((const char **) aValue) = PLUGIN_NAME;
      break;
    case NPPVpluginDescriptionString:
      *((const char **) aValue) = PLUGIN_VERSION;
      break;
    case NPPVpluginNeedsXEmbed:
      //      *((PRBool *) aValue) = PR_FALSE;
      *((PRBool *) aValue) = PR_TRUE;
      break;
    default:
      return NPERR_GENERIC_ERROR;
    }
  return NPERR_NO_ERROR;
  }

static void reload_url(bg_mozilla_t * m)
  {
  bg_NPN_GetURL(m->instance, m->uri, (const char*)0);
  }

static void open_url(bg_mozilla_t * m, const char * url)
  {
  //  bg_NPN_GetURL(m->instance, url, "_new");
  bg_NPN_GetURL(m->instance, url, (char*)0);
  }

NPError NPP_New(NPMIMEType pluginType,
                NPP instance, uint16 mode,
                int16 argc, char *argn[],
                char *argv[], NPSavedData *saved)
  {
  int i;
  int val_i;
  bg_mozilla_t * priv;

  /* Get toolkit */
  if((bg_NPN_GetValue(instance, NPNVToolkit, &val_i) != NPERR_NO_ERROR) ||
     (val_i != 2))
    {
    return NPERR_INVALID_PLUGIN_ERROR;
    }
  
  priv = gmerlin_mozilla_create();
  instance->pdata = priv;
  
  priv->instance = instance;

  gmerlin_mozilla_create_scriptable(priv);
  
  priv->reload_url = reload_url;
  priv->open_url = open_url;
  
  for(i = 0; i < argc; i++)
    {
    if(!strcmp(argn[i], "type"))
      {
      if(check_mime_type(general_mime_description, argv[i]))
        priv->ei.mode = MODE_GENERIC;
      if(check_mime_type(real_mime_description, argv[i]))
        priv->ei.mode = MODE_REAL;
      if(check_mime_type(wmp_mime_description, argv[i]))
        priv->ei.mode = MODE_WMP;
      if(check_mime_type(quicktime_mime_description, argv[i]))
        priv->ei.mode = MODE_QUICKTIME;
      if(check_mime_type(vlc_mime_description, argv[i]))
        priv->ei.mode = MODE_VLC;
      }
    bg_mozilla_embed_info_set_parameter(&priv->ei, argn[i], argv[i]);

    bg_log(BG_LOG_INFO, LOG_DOMAIN,
           "Set parameter: %s %s", argn[i], argv[i]);
    }
  if(!bg_mozilla_embed_info_check(&priv->ei))
    {
    gmerlin_mozilla_destroy(priv);
    instance->pdata = NULL;
    return NPERR_INVALID_PLUGIN_ERROR;
    }
  if(((priv->ei.mode == MODE_VLC) && (priv->ei.target)) ||
     (priv->ei.src && (!strncmp(priv->ei.src, "rtsp", 4) ||
                       !strncmp(priv->ei.src, "mms", 3))))
    {
    if(priv->ei.target)
      priv->url      = bg_strdup(priv->url,      priv->ei.target); 
    else
      priv->url      = bg_strdup(priv->url,      priv->ei.src); 
    
    priv->mimetype = bg_strdup(priv->mimetype, priv->ei.type);
    priv->url_mode = URL_MODE_LOCAL;
    }
  
  return NPERR_NO_ERROR;
  }

NPError NPP_Destroy(NPP instance,
                    NPSavedData **saved)
  {
  bg_mozilla_t * priv;
  priv = (bg_mozilla_t *)instance->pdata;
  gmerlin_mozilla_destroy(priv);
  return NPERR_NO_ERROR;
  }


NPError NPP_GetValue(NPP instance, 
                     NPPVariable variable, 
                     void *value)
  {
  bg_mozilla_t * priv;
  priv = (bg_mozilla_t *)instance->pdata;
  switch(variable)
    {
    case NPPVpluginWindowBool:
      *((PRBool *) value) = PR_TRUE;
      break;
    case NPPVpluginNeedsXEmbed:
      *((PRBool *) value) = PR_TRUE;
      return NPERR_GENERIC_ERROR;
      break;
    case NPPVpluginScriptableNPObject:
      bg_NPN_RetainObject(priv->scriptable);
      *((NPObject**) value) = priv->scriptable;
      break;
    default:
      return NPERR_GENERIC_ERROR;
    }
  return NPERR_NO_ERROR;
  }

NPError NPP_SetValue(NPP instance, 
                     NPNVariable variable, 
                     void *value)
  {
  bg_mozilla_t * priv;
  priv = (bg_mozilla_t *)instance->pdata;
  switch(variable)
    {
    case NPPVpluginWindowBool:
      break;
    default:
      return NPERR_GENERIC_ERROR;
    }
  return NPERR_NO_ERROR;
  }

int32 NPP_WriteReady(NPP instance, NPStream* stream)
  {
  return BUFFER_SIZE;
  }

int32 NPP_Write(NPP instance, NPStream* stream, int32 offset, 
                int32 len, void* buf)
  {
  bg_mozilla_t * priv;
  int ret;
  priv = (bg_mozilla_t *)instance->pdata;
  if(priv->state == STATE_ERROR)
    {
    bg_mozilla_buffer_destroy(priv->buffer);
    priv->buffer = NULL;
    }
  
  if(!priv->buffer) /* Player was stopped, close the stream */
    {
    return -1;
    }
  
  ret = bg_mozilla_buffer_write(priv->buffer, buf, len);
  
  if(priv->state == STATE_IDLE)
    gmerlin_mozilla_start(priv);
  return ret;
  }

NPError NPP_DestroyStream(NPP instance, 
                          NPStream *stream, NPError reason)
  {
  bg_mozilla_t * priv;
  priv = (bg_mozilla_t *)instance->pdata;
  /* Signal EOF */
  if(priv->buffer)
    bg_mozilla_buffer_write(priv->buffer, (void*)0, 0);
  return NPERR_NO_ERROR;
  }

NPError NPP_NewStream(NPP        instance, 
                      NPMIMEType type,
                      NPStream*  stream,
                      NPBool     seekable,
                      uint16*    stype)
  {
  char * new_url;
  bg_mozilla_t * priv;
  priv = (bg_mozilla_t *)instance->pdata;

  priv->uri = bg_strdup(priv->uri, stream->url);
    
  new_url = bg_uri_to_string(priv->uri, -1);
  
  gmerlin_mozilla_set_stream(priv, new_url, type, stream->end);
  
  if(priv->url_mode == URL_MODE_LOCAL)
    {
    bg_NPN_DestroyStream(instance, stream, NPRES_DONE);
    gmerlin_mozilla_start(priv);
    }
  else
    {
    
    }
  
  free(new_url);

  return NPERR_NO_ERROR;
  }



NPError NPP_SetWindow(NPP instance, NPWindow *window)
  {
  bg_mozilla_t * priv;
  priv = (bg_mozilla_t *)instance->pdata;
  gmerlin_mozilla_set_window(priv,
                             (GdkNativeWindow)(((int64_t)window->window)));
  return NPERR_NO_ERROR;
  }

static void set_plugin_funcs(NPPluginFuncs *aNPPFuncs)
  {
  aNPPFuncs->newp          = NPP_New;
  aNPPFuncs->destroy       = NPP_Destroy;
  aNPPFuncs->setwindow     = NPP_SetWindow;
  aNPPFuncs->newstream     = NPP_NewStream;
  aNPPFuncs->getvalue      = NPP_GetValue;
  aNPPFuncs->setvalue      = NPP_SetValue;
  aNPPFuncs->writeready    = NPP_WriteReady;
  aNPPFuncs->write         = NPP_Write;
  aNPPFuncs->destroystream = NPP_DestroyStream;
  }



NPError NP_Initialize(NPNetscapeFuncs *aNPNFuncs,
                      NPPluginFuncs *aNPPFuncs)
  {
#if 0
  bg_log_set_verbose(BG_LOG_DEBUG |
                     BG_LOG_WARNING |
                     BG_LOG_ERROR |
                     BG_LOG_INFO);
#endif
  set_plugin_funcs(aNPPFuncs);
  bg_mozilla_init_browser_funcs(aNPNFuncs);
  gmerlin_mozilla_init_scriptable();
  return NPERR_NO_ERROR;
  }

NPError NP_Shutdown(void)
  {
  return NPERR_NO_ERROR;
  }


#if defined(__GNUC__)

static void cleanup_mimeinfo() __attribute__ ((destructor));

static void cleanup_mimeinfo()
  {
  if(mime_info)
    free(mime_info);
  }

#endif


