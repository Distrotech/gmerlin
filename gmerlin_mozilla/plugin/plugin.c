#include <gmerlin/utils.h>

#include <gmerlin_mozilla.h>

#define WMP_MIME_DESCRIPTION \
"application/x-mplayer2:avi,wma,wmv,asx:AVI-Video;" \
"video/x-ms-asf-plugin:asf,wmv:ASF-Video;" \
"video/x-msvideo:avi:AVI-Video;" \
"video/x-ms-asf:asf:ASF-Video;" \
"video/x-ms-wmv:wmv:Windows Media video;" \
"video/x-wmv:wmv:Windows Media video;" \
"video/x-ms-wvx:wmv:Microsoft ASX playlist;" \
"video/x-ms-wm:wmv:ASF-Video;" \
"application/x-ms-wms:wms:Windows Media video;" \
"application/asx:asx:Microsoft ASX playlist;" \
"audio/x-ms-wma:wma:Windows Media audio;"

#define REAL_MIME_DESCRIPTION \
  "audio/x-pn-realaudio:ram,rm:RealAudio;" \
  "application/vnd.rn-realmedia:rm:RealMedia;" \
  "application/vnd.rn-realaudio:ra,ram:RealAudio;" \
  "video/vnd.rn-realvideo:rv:RealVideo;" \
  "audio/x-realaudio:ra:RealAudio;" \
  "audio/x-pn-realaudio-plugin:rpm:RealAudio;" \
  "application/smil:smil:SMIL;"


#define GMERLIN

/* Browser entry types */

#ifdef GMERLIN
#define MIME_TYPES_DESCRIPTION \
"application/gmerlin:gmr:Gmerlin test;"\
WMP_MIME_DESCRIPTION \
REAL_MIME_DESCRIPTION
#define PLUGIN_NAME "Gmerlin web plugin"

#endif

#include <gmerlin/log.h>

char* NP_GetMIMEDescription(void)
  {
  fprintf(stderr, "GET MIME DESCRIPTION\n");
  return(MIME_TYPES_DESCRIPTION);
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
      *((const char **) aValue) =
        "<a href=\"http://gmerlin.sourceforge.net/\">Gmerlin</a> browser plugin "
        VERSION;
      break;
    case NPPVpluginNeedsXEmbed:
      //      *((PRBool *) aValue) = PR_FALSE;
      *((PRBool *) aValue) = PR_TRUE;
      break;
    default:
      fprintf(stderr, "NP_GetValue %d\n", variable);
      return NPERR_GENERIC_ERROR;
    }
  return NPERR_NO_ERROR;
  }

NPError NPP_New(NPMIMEType pluginType,
                NPP instance, uint16 mode,
                int16 argc, char *argn[],
                char *argv[], NPSavedData *saved)
  {
  bg_mozilla_t * priv;
  priv = gmerlin_mozilla_create();
  instance->pdata = priv;
  return NPERR_NO_ERROR;
  }

NPError NPP_Destroy(NPP instance,
                    NPSavedData **saved)
  {
  bg_mozilla_t * priv;
  fprintf(stderr, "DESTROY\n");
  priv = (bg_mozilla_t *)instance->pdata;
  gmerlin_mozilla_destroy(priv);
  return NPERR_NO_ERROR;
  }


NPError NPP_GetValue(NPP instance, 
                     NPPVariable variable, 
                     void *value)
  {
  bg_mozilla_t * priv;
  fprintf(stderr, "NPP_GetValue %d\n", variable);
  priv = (bg_mozilla_t *)instance->pdata;
  switch(variable)
    {
    case NPPVpluginWindowBool:
      break;
    case NPPVpluginNeedsXEmbed:
      *((PRBool *) value) = PR_TRUE;
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
  fprintf(stderr, "NPP_SetValue\n");
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

NPError NPP_NewStream(NPP        instance, 
                      NPMIMEType type,
                      NPStream*  stream,
                      NPBool     seekable,
                      uint16*    stype)
  {
  char * new_url;
  bg_mozilla_t * priv;
  priv = (bg_mozilla_t *)instance->pdata;
  
  new_url = bg_strdup((char*)0, stream->url);
  fprintf(stderr, "NewStream %s\n", new_url);
  
  gmerlin_mozilla_set_url(priv, new_url);
  free(new_url);
  
  return NPERR_NO_ERROR;
  }

NPError NPP_SetWindow(NPP instance, NPWindow *window)
  {
  bg_mozilla_t * priv;
  priv = (bg_mozilla_t *)instance->pdata;

  //  fprintf(stderr, "SetWindow %ld\n", (int64_t)window->window);

  gmerlin_mozilla_set_window(priv,
                             (GdkNativeWindow)window->window);


  return NPERR_NO_ERROR;
  }

NPError NP_Initialize(NPNetscapeFuncs *aNPNFuncs,
                      NPPluginFuncs *aNPPFuncs)
  {
  fprintf(stderr, "INITIALIZE %d %d\n", aNPPFuncs->version, aNPPFuncs->size);
  aNPPFuncs->newp = NPP_New;
  aNPPFuncs->destroy = NPP_Destroy;
  aNPPFuncs->setwindow = NPP_SetWindow;
  aNPPFuncs->newstream = NPP_NewStream;
  aNPPFuncs->getvalue  = NPP_GetValue;
  aNPPFuncs->setvalue  = NPP_SetValue;
  
  bg_log_set_verbose(BG_LOG_DEBUG |
                     BG_LOG_WARNING |
                     BG_LOG_ERROR |
                     BG_LOG_INFO);
  
  return NPERR_NO_ERROR;
  }

NPError NP_Shutdown(void)
  {
  fprintf(stderr, "SHUTDOWN\n");
  return NPERR_NO_ERROR;
  }
