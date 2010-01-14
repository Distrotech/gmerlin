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


/** \defgroup visualize Visualizer
 *  \brief Visualization module
 *
 *  This is the recommended frontend for visualization plugins.
 *  You pass it audio frames in your format, and it will handle
 *  all format conversions, rendering and fps control. Visualizations
 *  are run in a separate process, so plugins can never make the calling
 *  application crash.
 *
 *  @{
 */

/** \brief Opaque visualizer structure
 *
 *  You don't want to know what's inside
 *
 */

typedef struct bg_visualizer_s bg_visualizer_t;

/** \brief Create a visualizer
 *  \param plugin_reg A plugin registry
 *  \returns A newly create visualizer
 */

bg_visualizer_t * bg_visualizer_create(bg_plugin_registry_t * plugin_reg);

/** \brief Destroy a visualizer
 *  \param v A Visualizer
 */


void bg_visualizer_destroy(bg_visualizer_t * v);

/** \brief Get the parameters of a visualizer
 *  \param v A Visualizer
 *  \returns NULL terminated array of parameter descriptions
 */

const bg_parameter_info_t * bg_visualizer_get_parameters(bg_visualizer_t* v);

/** \brief Set ov callbacks
 *  \param priv A visualizer 
 *  \param cb Callbacks
 */

void bg_visualizer_set_callbacks(bg_visualizer_t* v,
                                 bg_ov_callbacks_t * cb);


/** \brief Set a parameter of a visualizer
 *  \param priv A visualizer casted to void
 *  \param name Name
 *  \param val Value
 */

void bg_visualizer_set_parameter(void * priv,
                                 const char * name,
                                 const bg_parameter_value_t * val);

/** \brief Set the visualization plugin
 *  \param v A Visualizer
 *  \param info A plugin info
 */

void bg_visualizer_set_vis_plugin(bg_visualizer_t * v,
                                  const bg_plugin_info_t * info);

/** \brief Set a parameter of the visualization plugin
 *  \param priv A visualizer casted to void
 *  \param name Name
 *  \param val Value
 */

void bg_visualizer_set_vis_parameter(void * priv,
                                     const char * name,
                                     const bg_parameter_value_t * val);

/** \brief Open visualization with a video output plugin
 *  \param v A Visualizer
 *  \param format An audio format
 *  \param ov_handle Handle for the video output plugin
 */

void bg_visualizer_open_plugin(bg_visualizer_t * v,
                               const gavl_audio_format_t * format,
                               bg_plugin_handle_t * ov_handle);

/** \brief Open visualization with a plugin info and a window ID
 *  \param v A Visualizer
 *  \param format An audio format
 *  \param ov_info Which video output plugin to use
 *  \param display_id Window ID
 */

void bg_visualizer_open_id(bg_visualizer_t * v,
                           const gavl_audio_format_t * format,
                           const bg_plugin_info_t * ov_info,
                           const char * display_id);

/* Set new audio format without stopping the visualization thread */

/** \brief Set the audio format of a visualizer
 *  \param v A Visualizer
 *  \param format An audio format
 *
 *  This function can be called on a visualizer, which is already
 *  open, to change the audio format on the fly
 */

void bg_visualizer_set_audio_format(bg_visualizer_t * v,
                                    const gavl_audio_format_t * format);

/** \brief Send audio data to a visualizer
 *  \param v A Visualizer
 *  \param frame An audio frame
 */

void bg_visualizer_update(bg_visualizer_t * v,
                          const gavl_audio_frame_t * frame);

/** \brief Close a visualizer
 *  \param v A Visualizer
 *
 * This stops all the internal visualization thread as well as the
 * child process, in which the visualization is run
 */

void bg_visualizer_close(bg_visualizer_t * v);

/** \brief Check, whether a visualizer needs to be restarted
 *  \param v A Visualizer
 *  \returns 1 if the visualizer needs a restart
 *
 *  Call this function after \ref bg_visualizer_set_parameter to check,
 *  if you must close and reopen the visualizer.
 */

int bg_visualizer_need_restart(bg_visualizer_t * v);

/** \brief Get the frames per second
 *  \param v A Visualizer
 *  \returns The frames per second
 */

double bg_visualizer_get_fps(bg_visualizer_t * v);

/**
 *  @}
 */
