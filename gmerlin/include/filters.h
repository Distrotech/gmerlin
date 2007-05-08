#include <gavl/gavl.h>
#include <bggavl.h>
#include <plugin.h>



/** \defgroup filter_chain Filter chains
 * \brief Chains of A/V filters
 *
 * @{
 */

/** \brief Audio filter chain
 *
 *  Opaque handle for an audio filter chain. You don't want to know,
 *  what's inside.
 */

typedef struct bg_audio_filter_chain_s bg_audio_filter_chain_t;

/** \brief Video filter chain
 *
 *  Opaque handle for a video filter chain. You don't want to know,
 *  what's inside.
 */

typedef struct bg_video_filter_chain_s bg_video_filter_chain_t;

/* Audio */

/** \brief Create an audio filter chain
 *  \param opt Conversion options
 *  \param plugin_reg A plugin registry
 *
 *  The conversion options should be valid for the whole lifetime of the filter chain.
 */

bg_audio_filter_chain_t *
bg_audio_filter_chain_create(const bg_gavl_audio_options_t * opt,
                             bg_plugin_registry_t * plugin_reg);

/** \brief Return parameters
 *  \param ch An audio filter chain
 *  \returns A NULL terminated array of parameter descriptions
 *
 *  Usually, there will be a parameter of type BG_PARAMETER_MULTI_CHAIN,  which includes
 *  all installed filters and their respective parameters.
 */

bg_parameter_info_t *
bg_audio_filter_chain_get_parameters(bg_audio_filter_chain_t * ch);

/** \brief Set a parameter for an audio chain
 *  \param data An audio converter as void*
 *  \param name Name
 *  \param val Value
 *
 *  In some cases the filter chain must be rebuilt after setting a parameter.
 *  The application should therefore call \ref bg_audio_filter_chain_need_rebuild
 *  and call \ref bg_audio_filter_chain_init if necessary.
 */

void bg_audio_filter_chain_set_parameter(void * data,
                                         char * name,
                                         bg_parameter_value_t * val);

/** \brief Check if an audio filter chain needs to be reinitialized
 *  \param ch An audio filter chain
 *  \returns 1 if the chain must be reinitialized, 0 else
 */

int bg_audio_filter_chain_need_rebuild(bg_audio_filter_chain_t * ch);

/** \brief Initialize an audio filter chain
 *  \param ch An audio filter chain
 *  \param in_format Input format
 *  \param out_format Returns the output format
 */

int bg_audio_filter_chain_init(bg_audio_filter_chain_t * ch,
                               const gavl_audio_format_t * in_format,
                               gavl_audio_format_t * out_format);

/** \brief Set input callback of an audio filter chain
 *  \param ch An audio filter chain
 *  \param func The function to call
 *  \param priv The private handle to pass to func
 *  \param stream The stream argument to pass to func
 */

void bg_audio_filter_chain_connect_input(bg_audio_filter_chain_t * ch,
                                         bg_read_audio_func_t func,
                                         void * priv,
                                         int stream);

/** \brief Read a audio samples from an audio filter chain
 *  \param priv An audio filter chain
 *  \param frame An audio frame
 *  \param stream Stream number (must be 0)
 *  \param num_samples Number of samples to read
 *  \returns Number of samples read, 0 means EOF.
 */

int bg_audio_filter_chain_read(void * priv, gavl_audio_frame_t* frame,
                               int stream,
                               int num_samples);

/** \brief Destroy an audio filter chain
 *  \param ch An audio filter chain
 */

void bg_audio_filter_chain_destroy(bg_audio_filter_chain_t * ch);

/** \brief Lock an audio filter chain
 *  \param ch An audio filter chain
 *
 *  In multithreaded enviroments, you must lock the chain for calls to
 *  \ref bg_audio_filter_chain_set_parameter and \ref bg_audio_filter_chain_read.
 */

void bg_audio_filter_chain_lock(bg_audio_filter_chain_t * ch);

/** \brief Unlock an audio filter chain
 *  \param ch An audio filter chain
 *
 *  In multithreaded enviroments, you must lock the chain for calls to
 *  \ref bg_audio_filter_chain_set_parameter and \ref bg_audio_filter_chain_read.
 */

void bg_audio_filter_chain_unlock(bg_audio_filter_chain_t * ch);


/* Video */

/** \brief Create a video filter chain
 *  \param opt Conversion options
 *  \param plugin_reg A plugin registry
 *
 *  The conversion options should be valid for the whole lifetime of the filter chain.
 */

bg_video_filter_chain_t *
bg_video_filter_chain_create(const bg_gavl_video_options_t * opt,
                             bg_plugin_registry_t * plugin_reg);

/** \brief Return parameters
 *  \param ch A video filter chain
 *  \returns A NULL terminated array of parameter descriptions
 *
 *  Usually, there will be a parameter of type BG_PARAMETER_MULTI_CHAIN,  which includes
 *  all installed filters and their respective parameters.
 */

bg_parameter_info_t *
bg_video_filter_chain_get_parameters(bg_video_filter_chain_t * ch);

/** \brief Set a parameter for a video chain
 *  \param data A video converter as void*
 *  \param name Name
 *  \param val Value
 *
 *  In some cases the filter chain must be rebuilt after setting a parameter.
 *  The application should therefore call \ref bg_video_filter_chain_need_rebuild
 *  and call \ref bg_video_filter_chain_init if necessary.
 */

void bg_video_filter_chain_set_parameter(void * data, char * name,
                                         bg_parameter_value_t * val);

/** \brief Check if a video filter chain needs to be reinitialized
 *  \param ch A video filter chain
 *  \returns 1 if the chain must be reinitialized, 0 else
 */

int bg_video_filter_chain_need_rebuild(bg_video_filter_chain_t * ch);

/** \brief Initialize a video filter chain
 *  \param ch A video filter chain
 *  \param in_format Input format
 *  \param out_format Returns the output format
 */

int bg_video_filter_chain_init(bg_video_filter_chain_t * ch,
                               const gavl_video_format_t * in_format,
                               gavl_video_format_t * out_format);


/** \brief Set input callback of a video filter chain
 *  \param ch A video filter chain
 *  \param func The function to call
 *  \param priv The private handle to pass to func
 *  \param stream The stream argument to pass to func
 */

void bg_video_filter_chain_connect_input(bg_video_filter_chain_t * ch,
                                         bg_read_video_func_t func,
                                         void * priv, int stream);

/** \brief Read a video frame from a video filter chain
 *  \param priv A video filter chain
 *  \param frame A video frame
 *  \param stream Stream number (must be 0)
 *  \returns 1 if a frame was read, 0 means EOF.
 */

int bg_video_filter_chain_read(void * priv, gavl_video_frame_t* frame,
                               int stream);

/** \brief Destroy a video filter chain
 *  \param ch A video filter chain
 */

void bg_video_filter_chain_destroy(bg_video_filter_chain_t * ch);

/** \brief Lock a video filter chain
 *  \param ch A video filter chain
 *
 *  In multithreaded enviroments, you must lock the chain for calls to
 *  \ref bg_video_filter_chain_set_parameter and \ref bg_video_filter_chain_read.
 */

void bg_video_filter_chain_lock(bg_video_filter_chain_t * ch);

/** \brief Unlock a video filter chain
 *  \param ch A video filter chain
 *
 *  In multithreaded enviroments, you must lock the chain for calls to
 *  \ref bg_video_filter_chain_set_parameter and \ref bg_video_filter_chain_read.
 */

void bg_video_filter_chain_unlock(bg_video_filter_chain_t * ch);

/**
 * @}
 */
