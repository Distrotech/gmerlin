
/* Commands for the player */
/* Comments are arguments passed along with the message */

#define PLAYER_CMD_PLAY      0
#define PLAYER_CMD_NEXT      1
#define PLAYER_CMD_PREV      2
#define PLAYER_CMD_SEL_VIDEO 3 /* int */
#define PLAYER_CMD_SEL_AUDIO 4 /* int */
#define PLAYER_CMD_SEL_SUBP  5 /* int */

#define PLAYER_MSG_TIME_CHANGED  0 /* int (in seconds) */
#define PLAYER_MSG_TRACK_CHANGED 1 /* */
#define PLAYER_MSG_STATE_CHANGED 2 /* int, see defines below */

#define PLAYER_STATE_STOPPED      0
#define PLAYER_STATE_CHANGING     1
#define PLAYER_STATE_BUFFERING    2
#define PLAYER_STATE_PAUSED       3
#define PLAYER_STATE_PLAYING      4
#define PLAYER_STATE_SEEKING      5
