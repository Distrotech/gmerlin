/* Communication between the visualizer and the slave */

#define BG_VIS_MSG_AUDIO_FORMAT  0
#define BG_VIS_MSG_AUDIO_DATA    1
#define BG_VIS_MSG_IMAGE_SIZE    2
#define BG_VIS_MSG_FPS           3
#define BG_VIS_MSG_QUIT          4
#define BG_VIS_MSG_VIS_PARAM     5
#define BG_VIS_MSG_OV_PARAM      6
#define BG_VIS_MSG_GAIN          7
#define BG_VIS_MSG_START         8

/* Let the slave tell his messages,
 * will be termimated with BG_VIS_SLAVE_MSG_END */
#define BG_VIS_MSG_TELL          9

/* Messages from the visualizer to the application */

#define BG_VIS_SLAVE_MSG_FPS     (BG_LOG_LEVEL_MAX+1)
#define BG_VIS_SLAVE_MSG_END     (BG_LOG_LEVEL_MAX+2)

/*
 * gmerlin_visualize_slave
 *   [-w "window_id"|-o "output_module"]
 *   -p "plugin_module"
 */
