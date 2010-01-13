
#include <config.h>
#include <gmerlin/translation.h>

#include <gmerlin/plugin.h>
#include <gmerlin/utils.h>

#include <renderer.h>

static bg_plugin_registry_t * plugin_reg = NULL;

static void * create_plugin()
  {
  return bg_nle_plugin_create(NULL, NULL);
  }

const bg_input_plugin_t the_plugin =
  GMERLERRA_PLUGIN("i_gmerlerra",
                   TRS("Gmerlerra plugin"), create_plugin);

/* Include this into all plugin modules exactly once
   to let the plugin loader obtain the API version */
BG_GET_PLUGIN_API_VERSION;
