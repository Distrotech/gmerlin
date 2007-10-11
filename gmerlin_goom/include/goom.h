#ifndef _GOOMCORE_H
#define _GOOMCORE_H

#include "goom_config.h"
#include "goom_plugin_info.h"
#include "goomsl.h"

#define NB_FX 10

/**
 * Initialize goom for a given screen resolution
 */
PluginInfo *goom_init (guint32 resx, guint32 resy);

/**
 * Change resolution goom is working at
 */
void goom_set_resolution (PluginInfo *goomInfo, guint32 resx, guint32 resy);

/**
 * Update goom (call this every frame)
 *
 * return a pointer to the RGBA buffer to display
 * 
 * gint16 data[2][512]: stereo sound sample
 * 
 * forceMode == 0  : do nothing
 * forceMode == -1 : lock the FX
 * forceMode == 1..NB_FX : force a switch to FX n# forceMode
 *
 * songTitle = pointer to the title of the song...
 *      - NULL if it is not the start of the song
 *      - only have a value at the start of the song
 */
guint32 *goom_update (PluginInfo *goomInfo, gint16 data[2][512], int forceMode, float fps,
                      char *songTitle, char *message);

/**
 * Free goom's internals
 */
void goom_close (PluginInfo *goomInfo);

/**
 * Undocumented for now: please do not use
 */
int goom_set_screenbuffer(PluginInfo *goomInfo, void *buffer);

#endif
