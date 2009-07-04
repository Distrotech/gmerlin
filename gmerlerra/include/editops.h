/* Edit operations */

enum bg_nle_edit_op_e
  {
    BG_NLE_EDIT_ADD_TRACK,
    BG_NLE_EDIT_DELETE_TRACK,
    BG_NLE_EDIT_MOVE_TRACK,
    BG_NLE_EDIT_ADD_OUTSTREAM,
    BG_NLE_EDIT_DELETE_OUTSTREAM,
    BG_NLE_EDIT_MOVE_OUTSTREAM,
  } ;

// BG_NLE_EDIT_ADD_TRACK
typedef struct
  {
  bg_nle_track_t * track;
  
  } bg_nle_op_add_track_t;

// BG_NLE_EDIT_DELETE_TRACK
typedef struct
  {
  bg_nle_track_t * track;
  int index;
  } bg_nle_op_delete_track_t;

// BG_NLE_EDIT_MOVE_TRACK
typedef struct
  {
  bg_nle_track_t * track;
  int old_index;
  int new_index;
  } bg_nle_op_move_track_t;

// BG_NLE_EDIT_ADD_OUTSTREAM
typedef struct
  {
  bg_nle_outstream_t * outstream;
  
  } bg_nle_op_add_outstream_t;

// BG_NLE_EDIT_DELETE_OUTSTREAM
typedef struct
  {
  bg_nle_outstream_t * outstream;
  int index;
  
  } bg_nle_op_delete_outstream_t;

// BG_NLE_EDIT_MOVE_OUTSTREAM
typedef struct
  {
  bg_nle_outstream_t * outstream;
  int old_index;
  int new_index;
  } bg_nle_op_move_outstream_t;

